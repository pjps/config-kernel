/*
 * configk: an easy way to edit kernel configuration files and templates
 * Copyright (C) 2023-2024 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See COPYING file or <http://www.gnu.org/licenses/> for more details.
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "configk.h"
#include "parser.tab.h"
#include "eparse.tab.h"
#include "cparse.tab.h"

extern FILE *yyin, *ccin;
extern int yyparse(void);
extern int ccparse(char *);
extern int eescans(uint8_t, const char *, char **);

#define VERSION "0.2"

uint16_t opts = 0;
char *gstr[GSTRSZ]; /* global string pointers */
const char *types[] = { "", "int", "hex", "bool", "string", "tristate" };
struct hsearch_data chash;
static char *gets_range(const char *);

static void
usage(void)
{
    printf("Usage: %s [OPTIONS] <source-dir>\n", gstr[IPROG]);
}

static void
printh(void)
{
#define fmt " %-27s %s\n"
    usage();
    printf("\nOptions:\n");
    printf(fmt, " -a --srcarch <arch>", "set $SRCARCH variable");
    printf(fmt, " -c --check <file>", "check configs against the source tree");
    printf(fmt, " -C --config", "show output as a config file");
    printf(fmt, " -d --disable <option>", "disable config option");
    printf(fmt, " -e --enable <option>[=val]", "enable config option");
    printf(fmt, " -E --edit <file>", "edit config file with an $EDITOR");
    printf(fmt, " -h --help", "show help");
    printf(fmt, " -s --show <option>", "show a config option entry");
    printf(fmt, " -t --toggle <option>", "toggle an option between y & m");
    printf(fmt, " -v --version", "show version");
    printf(fmt, " -V --verbose", "show verbose output");
    printf("\nReport bugs to: <pjp@redhat.com>\n");
}

static uint8_t
check_options(int argc, char *argv[])
{
    int n;
    char optstr[] = "+a:c:Cd:e:E:hs:t:vV";
    extern int opterr, optind;

    struct option lopt[] = \
    {
        { "srcarch", required_argument, NULL, 'a' },
        { "check", required_argument, NULL, 'c' },
        { "config", no_argument, NULL, 'C' },
        { "disable", required_argument, NULL, 'd' },
        { "enable", required_argument, NULL, 'e' },
        { "edit", required_argument, NULL, 'E' },
        { "help", no_argument, NULL, 'h' },
        { "show", required_argument, NULL, 's' },
        { "toggle", required_argument, NULL, 't' },
        { "version", no_argument, NULL, 'v' },
        { "verbose", no_argument, NULL, 'V' },
        { 0, 0, 0, 0 }
    };

    opts = opterr = optind = 0;
    while ((n = getopt_long(argc, argv, optstr, lopt, &optind)) != -1)
    {
        switch (n)
        {
        case 'a':
            free(gstr[IARCH]);
            gstr[IARCH] = strdup(optarg);
            break;

        case 'c':
            opts |= CHECK_CONFIG | (opts & ~EDIT_CONFIG);
            free(gstr[IFOPT]);
            gstr[IFOPT] = strdup(optarg);
            break;

        case 'C':
            opts |= OUT_CONFIG;
            break;

        case 'd':
            opts |= DISABLE_CONFIG | (opts & ~EDIT_CONFIG);
            free(gstr[IDOPT]);
            gstr[IDOPT] = strdup(optarg);
            break;

        case 'e':
            opts |= ENABLE_CONFIG | (opts & ~EDIT_CONFIG);
            free(gstr[IEOPT]);
            gstr[IEOPT] = strdup(optarg);
            break;

        case 'E':
            opts = EDIT_CONFIG | (opts & OUTMASK);
            free(gstr[IFOPT]);
            gstr[IFOPT] = strdup(optarg);
            break;

        case 'h':
            printh();
            exit(0);

        case 's':
            opts |= SHOW_CONFIG | (opts & ~EDIT_CONFIG);
            free(gstr[ISOPT]);
            gstr[ISOPT] = strdup(optarg);
            break;

        case 't':
            opts |= TOGGLE_CONFIG | (opts & ~EDIT_CONFIG);
            free(gstr[ITOPT]);
            gstr[ITOPT] = strdup(optarg);
            break;

        case 'v':
            printf("%s version %s\n", gstr[IPROG], VERSION);
            exit(0);

        case 'V':
            opts |= OUT_VERBOSE;
            break;

        default:
            errx(-1, "invalid option -%c", optopt);
        }
    }

    return optind;
}

static void
_init(int argc, char *argv[])
{
    uint8_t n;
    struct rlimit rs;

    gstr[IPROG] = strdup(argv[0]);
    if (argc <= check_options(argc, argv))
    {
        usage();
        exit(0);
    }

    if (chdir(argv[optind]))
        err(-1, "could not change cwd: %s", argv[optind]);

    getrlimit(RLIMIT_NOFILE, &rs);
    rs.rlim_cur = 2048;
    if (setrlimit(RLIMIT_NOFILE, &rs))
        err(-1, "could not set open files limit to: %d", rs.rlim_cur);

    memset(&chash, '\0', sizeof(chash));
    if (!hcreate_r(HASHSZ, &chash))
        err(-1, "could not create chash table");

    gstr[IEDTR] = getenv("EDITOR");
    gstr[IEDTR] = gstr[IEDTR] ? strdup(gstr[IEDTR]) : strdup("vi");

    gstr[ITMPD] = getenv("TMPDIR");
    gstr[ITMPD] = gstr[ITMPD] ? strdup(gstr[ITMPD]) : strdup("/tmp");

    if (!gstr[IARCH])
    {
        gstr[IARCH] = getenv("SRCARCH");
        gstr[IARCH] = gstr[IARCH] ? strdup(gstr[IARCH]) : strdup("x86");
    }

    return;
}

static void
_reset(void)
{
    uint32_t tmem = 0;

    tmem = tree_reset(tree_root());
    tmem += (HASHSZ * sizeof(chash));
    hdestroy_r(&chash);
    for (uint8_t n = 0; n < GSTRSZ; n++)
    {
        tmem += gstr[n] ? strlen(gstr[n]) : 0;
        free(gstr[n]);
    }

    if (!(opts & SHOW_CONFIG) && !(opts & EDIT_CONFIG) && opts & OUT_CONFIG)
        fprintf(stderr, "Config memory: %.2f MB\n", (float)tmem / 1024 / 1024);
    else if (!(opts & SHOW_CONFIG) && !(opts & EDIT_CONFIG))
        printf("Config memory: %.2f MB\n", (float)tmem / 1024 / 1024);
    return;
}

static void
show_configs(const char *sopt)
{
    cNode *r = NULL;
    cEntry *t = NULL;
    sEntry *s = NULL;

    if (!(r = hsearch_kconfigs(sopt)))
    {
        warnx("option '%s' not found in the source tree", sopt);
        return;
    }
    t = (cEntry *)r->data;
    s = (sEntry *)r->up->data;

    printf("%-7s: %s\n", "File", s->fname);
    printf("%-7s: %s\n", "Config", t->opt_name);
    printf("%-7s: %s\n", "Type", types[t->opt_type]);
    if (t->opt_range)
    {
        char *range = gets_range(t->opt_range);
        printf("%-7s: %s => [%s]\n", "Range", t->opt_range, range);
        free(range);
    }
    if (t->opt_value)
        printf("%-7s: %s\n", "Default", t->opt_value);
    if (t->opt_prompt)
        printf("%-7s: %s\n", "Prompt", t->opt_prompt);
    if (t->opt_depends)
    {
        int8_t r = check_depends(t->opt_name);
        printf("%-7s: %s => %d\n", "Depends", t->opt_depends, r);
    }
    if (t->opt_select)
        printf("%-7s: %s\n", "Select", t->opt_select);
    if (t->opt_imply)
        printf("%-7s: %s\n", "Imply", t->opt_imply);
    if (t->opt_help)
        printf("%-7s:\n%s\n", "Help", t->opt_help);
    printf("\n");

    return;
}

char *
append(char *dst, char *src)
{
#define CDLM    ";\n\t" /* delimiter for select/depends list */
    char *tmp = NULL;
    uint16_t slen = strlen(src);
    uint16_t dlen = dst ? strlen(dst) + 4 : 1;

    tmp = calloc(slen + dlen, sizeof(char));
    if (tmp && dst)
    {
        strncpy(tmp, dst, strlen(dst));
        strncat(tmp, CDLM, 4);
        free(dst);
    }
    else if (!tmp)
        err(-1, "could not allocate memory for: '%s'", src);

    return strncat(tmp, src, slen);
}

cEntry *
add_new_config(char *cid, nType ctype)
{
    ENTRY e, *r;
    cEntry *t = NULL;

    e.key = cid;
    if (hsearch_r(e, FIND, &r, &chash))
    {
        if (opts & OUT_VERBOSE)
            warnx("'%s' read again, use earlier object", r->key);
        t = ((cNode *)r->data)->data;
        if (!t)
            err(-1, "'%s' data object is %p", r->key, t);
        free(cid);
        return t;
    }

    t = calloc(1, sizeof(cEntry));
    t->opt_name = cid;

    e.data = tree_add(tree_cnode(t, ctype));
    if (!hsearch_r(e, ENTER, &r, &chash))
        err(-1, "could not hash option '%s' %p", e.key, r);

    return t;
}

int
read_kconfigs(void)
{
    FILE *fin = fopen("Kconfig", "r");
    if (!fin)
        err(-1, "could not open file: %s/%s", getcwd(NULL, 0), "Kconfig");

    yyin = fin;
    tree_init("Kconfig");

    yyparse();
    fclose(fin);

    return 0;
}

static void
list_kconfigs(void)
{
    FILE *out = stderr;
    cNode *r = tree_root();

    if (opts & OUT_CONFIG && opts & CHECK_CONFIG)
    {
        printf("# This file is generated by %s\n", gstr[IPROG]);
        tree_display_config(r);
    }
    else if (opts & OUT_CONFIG)
        tree_display_config(r);
    else
    {
        tree_display(r);
        out = stdout;
    }

    fprintf(out, "Config files: %d\n", ((sEntry *)r->data)->s_count);
    fprintf(out, "Config options: %d\n", ((sEntry *)r->data)->o_count);
    return;
}

cNode *
hsearch_kconfigs(const char *copt)
{
    ENTRY e, *r;

    e.data = NULL;
    e.key = (char *)copt;
    if (!hsearch_r(e, FIND, &r, &chash))
        return NULL;

    return (cNode *)r->data;
}

static char *
gets_range(const char *exp)
{
    char *rxp, *txp, *tok, *svp;

    rxp = NULL;
    txp = strdup(exp);
    tok = strtok_r(txp, CDLM, &svp);
    while (tok)
    {
        uint8_t l = (rxp ? strlen(rxp) : 0) + strlen(tok) + 8;
        char *t = calloc(l, sizeof(uint8_t));

        if (rxp)
        {
            snprintf(t, l, "%s;range %s", rxp, tok);
            free(rxp);
        }
        else
            snprintf(t, l, "range %s", tok);

        rxp = t;
        tok = strtok_r(NULL, CDLM, &svp);
    }
    free(txp);

    char *rng = calloc(64, sizeof(uint8_t));
    int8_t r = eescans(EXPR_RANGE, rxp, &rng);
    //warnx("%s: %s(%d): %d=>%s", __func__, rxp, strlen(rxp), r, rng);

    free(rxp);
    return rng;
}

static uint8_t
validate_range(long val, const char *rexp)
{
    long r1, r2;
    char *range = gets_range(rexp);

    sscanf(range, "%li %li", &r1, &r2);
    free(range);

    return (r1 <= val && val <= r2);
}

int8_t
validate_option(const char *opt)
{
    uint8_t l, *val;
    int8_t rangerr = 17;

    cNode *c = hsearch_kconfigs(opt);
    cEntry *t = (cEntry *)c->data;

    val = t->opt_value;
    t->opt_status = t->opt_type;
    switch (t->opt_type)
    {
    case CINT:
        long v = atoi(val);
        if (*val != '0' && !v)
            t->opt_status = -t->opt_type;
        else if (t->opt_range && !validate_range(v, t->opt_range))
            t->opt_status = -rangerr;
        break;

    case CBOOL:
        l = strlen(val);
        if (l > 1 || !strstr("yYnN", val))
            t->opt_status = -t->opt_type;
        else if ('n' == *val || 'N' == *val)
            t->opt_status = -CVALNOSET;
        break;

    case CTRISTATE:
        l = strlen(val);
        if (l > 1 || !strstr("yYnNmM", val))
            t->opt_status = -t->opt_type;
        else if ('n' == *val || 'N' == *val)
            t->opt_status = -CVALNOSET;
        break;

    case CHEX:
        l = strlen(val);
        if (l > 18 || val[0] != '0' || (val[1] != 'x' && val[1] != 'X'))
            t->opt_status = -t->opt_type;
        if (t->opt_status > 0)
        {
            char *c = val + 2;
            while (*c && isxdigit(*c)) c++;
            if (*c)
                t->opt_status = -t->opt_type;
        }
        if (t->opt_status > 0 && t->opt_range)
        {
            long v = strtol(val, NULL, 0);
            if (!validate_range(v, t->opt_range))
                t->opt_status = -rangerr;
        }
    }
    if (-t->opt_type == t->opt_status)
        warnx("option '%s' has invalid %s value: '%s'",
                                        opt, types[t->opt_type], val);
    if (-rangerr == t->opt_status)
    {
        warnx("option '%s' has out of range value: '%s'", opt, val);
        t->opt_status = -t->opt_type;
    }

    return t->opt_status;
}

int8_t
set_option(const char *opt, char *val)
{
    cNode *c = hsearch_kconfigs(opt);
    if (!c)
        return 0;

    cEntry *t = (cEntry *)c->data;
    if (val)
    {
        free(t->opt_value);
        t->opt_value = strdup(val);
    }
    else if (t->opt_value)
    {
        val = strdup("n");
        int r = eescans(EXPR_DEFAULT, t->opt_value, &val);
        if (val)
        {
            free(t->opt_value);
            t->opt_value = val;
        }
    }
    if (!strcmp(t->opt_value, "is not set"))
        return t->opt_status = -CVALNOSET;

    ((sEntry *)c->up->data)->u_count++;
    return validate_option(opt);
}

int8_t
check_depends(const char *sopt)
{
    int8_t r = 0;
    cNode *c = hsearch_kconfigs(sopt);
    cEntry *t = (cEntry *)c->data;
    if (!t->opt_depends)
        return r;

    if (opts & OUT_VERBOSE)
        fprintf(stderr, "%s depends on %s: ", t->opt_name, t->opt_depends);
    r = eescans(EXPR_DEPENDS, t->opt_depends, NULL);
    if (opts & OUT_VERBOSE)
        fprintf(stderr, ":=> %d\n", r);

    return r;
}

int8_t
toggle_configs(const char *sopt, int8_t status, char *val)
{
    static uint8_t sp = 0;
    char *tok, *slt, *svp;

    cNode *c = hsearch_kconfigs(sopt);
    if (!c)
    {
        warnx("'%s' not found in the options' list", sopt);
        return 0;
    }
    cEntry *t = (cEntry *)c->data;
    if (ENABLE_CONFIG == status)
        set_option(sopt, val);
    if (TOGGLE_CONFIG == status)
    {
        if (t->opt_status && CTRISTATE == t->opt_type)
        {
            if ('y' == tolower(*t->opt_value))
                *t->opt_value = 'm';
            else if ('m' == tolower(*t->opt_value))
                *t->opt_value = 'y';
        }
        else
        {
            warnx("option '%s' is disabled or is not tristate, skip toggle",
                    t->opt_name);
            return 0;
        }
    }
    if (DISABLE_CONFIG == status)
        t->opt_status = -CVALNOSET;

    sp += 2;
    for (int i = 0; i < sp; i++)
        fprintf(stderr, " ");
    fprintf(stderr, "%s\n", t->opt_name);
    if (t->opt_select)
        eescans(status, t->opt_select, &val);
    if (t->opt_imply)
        eescans(status, t->opt_imply, &val);

    /* boolean choice: enable one and disable others */
    if (c->up->type == CHENTRY && t->opt_type == CBOOL)
    {
        if (ENABLE_CONFIG == status && t->opt_status > 0)
        {
            for (int i = 0; i < sp; i++)
                fprintf(stderr, " ");
            fprintf(stderr, "Disable option:\n");
            c = c->up->down;
            while (c)
            {
                cEntry *ch = (cEntry *)c->data;
                if (ch->opt_status > 0 && ch != t)
                    toggle_configs(ch->opt_name, DISABLE_CONFIG, NULL);
                c = c->next;
            }
        }
        if (DISABLE_CONFIG == status && t->opt_status < 0)
        {
            uint8_t ech = 0;
            c = c->up->down;
            while (c)
            {
                cEntry *ch = (cEntry *)c->data;
                if (ch->opt_status > 0)
                    ech += 1;
                c = c->next;
            }

            if (!ech)
            {
             /*
              * char *val = strdup("n");
              * t = (cEntry *)c->up->data;
              * int r = eescans(EXPR_DEFAULT, t->opt_value, &val);
              * if (r)
              *     toggle_configs(val, ENABLE_CONFIG, "y");
              */
                warnx("last choice '%s' disabled, none enabled now",
                        t->opt_name);
            }
        }
    }
    sp -= 2;

/*
    slt = strdup(t->opt_select);
    tok = strtok_r(slt, CDLM, &svp);
    while (tok)
    {
        sp += 2;
        int r = eescans(EXPR_SELECT, tok, &val);
        if (r)
            toggle_configs(tok, status, val);
        sp -= 2;

        tok = strtok_r(NULL, CDLM, &svp);
    }
    free(slt);
*/

    return 1;
}

static int
check_kconfigs(const char *cfile)
{
    int8_t r = 11;
    FILE *fin = fopen(cfile, "r");
    if (!fin)
        err(-1, "could not open file: %s", cfile);

    ccin = fin;
    ccparse(&r);

    fclose(fin);
    return r;
}

static void
setforeground(void)
{
    pid_t p = getpid();

    if (tcsetpgrp(STDIN_FILENO, p) < 0)
        err(-1, "could not assign stdin to process %d", p);
    if (tcsetpgrp(STDOUT_FILENO, p) < 0)
        err(-1, "could not assign stdout to process %d", p);
    if (tcsetpgrp(STDERR_FILENO, p) < 0)
        err(-1, "could not assign stderr to process %d", p);

    return;
}

static uint32_t
copy_file(const char *dst, const char *src)
{
    char *c;
    uint16_t fd;
    struct stat s;

    if (stat(src, &s) < 0)
        err(-1, "could not access file: %s", src);
    if ((fd = open(src, O_RDONLY)) < 0)
        err(-1, "could not open file: %s", src);

    c = calloc(s.st_size + 1, sizeof(char));
    if (read(fd, c, s.st_size) < s.st_size)
        err(-1, "could not read file: %s", src);
    close(fd);

    if ((fd = open(dst, O_WRONLY|O_TRUNC|O_CREAT|O_DSYNC)) < 0)
        err(-1, "could not open file: %s", dst);
    if (write(fd, c, s.st_size) < s.st_size)
        err(-1, "could not write file: %s", dst);
    close(fd);

    free(c);
    return s.st_size;
}

static void
edit_kconfigs(const char *sopt)
{
    int8_t fd;
    struct stat s;
    char cmd[20], tmp[20];

    snprintf(tmp, sizeof(tmp), "%s/%s", gstr[ITMPD], "cXXXXXXX");
    if ((fd = mkstemp(tmp)) < 0)
        err(-1, "could not create a temporary file: %s", tmp);
    close(fd);

    copy_file(tmp, sopt);

    snprintf(cmd, sizeof(cmd), "%s/%s", "/usr/bin", gstr[IEDTR]);
    if (stat(cmd, &s) < 0)
        err(-1, "editor %s not found", cmd);
    if (S_IFREG != (s.st_mode & S_IFMT) || !(s.st_mode & S_IXOTH))
        errx(-1, "editor %s has wrong type or permissions", cmd);

    setsid();
    if (setpgid(0, 0) < 0)
        err(-1, "could not set parent pgid");
    signal(SIGTTOU, SIG_IGN);

editc:
    uint32_t st;
    pid_t pid = fork();

    if (pid < 0)
        err(-1, "could not start a process");
    if (!pid)
    {
        if (setpgid(0, 0) < 0)
            err(-1, "could not set child pgid");

        setforeground();
        execl(cmd, basename(cmd), tmp, (char *)NULL);
        exit(-1);
    }

    waitpid(pid, &st, 0);
    setforeground();
    if ((fd = check_kconfigs(tmp)) <= 0)
    {
        uint8_t r;
        printf("Do you wish to exit?[y/N]: ");
        fflush(stdout);
        scanf("%c%*C", &r);
        if (r == 'n' || r == '\n')
            goto editc;
    }

    copy_file(sopt, tmp);
    unlink(tmp);
    return;
}

int
main(int argc, char *argv[])
{
    _init(argc, argv);

    read_kconfigs();
    if (opts & CHECK_CONFIG)
        check_kconfigs(gstr[IFOPT]);

    if (opts & DISABLE_CONFIG)
    {
        fprintf(stderr, "Disable option:\n");
        toggle_configs(gstr[IDOPT], DISABLE_CONFIG, NULL);
    }
    if (opts & ENABLE_CONFIG)
    {
        char *val = NULL;
        if (strchr(gstr[IEOPT], '='))
        {
            gstr[IEOPT] = strtok(gstr[IEOPT], "=");
            val = strtok(NULL, "=");
        }
        fprintf(stderr, "Enable option:\n");
        toggle_configs(gstr[IEOPT], ENABLE_CONFIG, val);
    }
    if (opts & TOGGLE_CONFIG)
    {
        fprintf(stderr, "Toggle option:\n");
        toggle_configs(gstr[ITOPT], TOGGLE_CONFIG, NULL);
    }

    if (opts & EDIT_CONFIG)
        edit_kconfigs(gstr[IFOPT]);
    else if (opts & SHOW_CONFIG)
        show_configs(gstr[ISOPT]);
    else
        list_kconfigs();

    _reset();
    return 0;
}
