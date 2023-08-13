
#include <err.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "configk.h"
#include "parser.tab.h"

#define __USE_GNU
#include <search.h>
struct hsearch_data chash;

extern FILE *yyin;
extern int yylex(void);
extern int yyparse(void);
extern void yyrestart(FILE *);

#define VERSION "0.1"

uint16_t opts = 0;
char *gstr[GSTRSZ]; /* global string pointers */
const char *types[] = { "", "int", "hex", "bool", "string", "tristate" };

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
            opts = CHECK_CONFIG | (opts & OUTMASK);
            free(gstr[ISOPT]);
            gstr[ISOPT] = strdup(optarg);
            break;

        case 'C':
            opts |= OUT_CONFIG;
            break;

        case 'd':
            opts = DISABLE_CONFIG | (opts & OUTMASK);
            free(gstr[IDOPT]);
            gstr[IDOPT] = strdup(optarg);
            break;

        case 'e':
            opts = ENABLE_CONFIG | (opts & OUTMASK);
            free(gstr[IEOPT]);
            gstr[IEOPT] = strdup(optarg);
            break;

        case 'E':
            opts = EDIT_CONFIG | (opts & OUTMASK);
            free(gstr[ISOPT]);
            gstr[ISOPT] = strdup(optarg);
            break;

        case 'h':
            printh();
            exit(0);

        case 's':
            opts = SHOW_CONFIG | (opts & OUTMASK);
            free(gstr[ISOPT]);
            gstr[ISOPT] = strdup(optarg);
            break;

        case 't':
            opts = TOGGLE_CONFIG | (opts & OUTMASK);
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
    uint16_t n = 0;

    n = tree_reset(tree_root());
    if (opts & OUT_VERBOSE)
        printf("Config nodes released: %d\n", n);
    hdestroy_r(&chash);
    for (n = 0; n < GSTRSZ; n++)
        free(gstr[n]);

    return;
}

static void
show_configs(const char *sopt)
{
    ENTRY e, *r;
    cEntry *t = NULL;
    sEntry *s = NULL;

    e.data = t;
    e.key = (char *)sopt;
    if (!hsearch_r(e, FIND, &r, &chash))
    {
        warnx("'%s' not found in the options' list: %s", e.key, r);
        return;
    }

    t = ((cNode *)r->data)->data;
    s = ((cNode *)((cNode *)r->data)->up)->data;
    printf("%-7s: %s\n", "File", s->fname);
    printf("%-7s: %s\n", "Config", t->opt_name);
    printf("%-7s: %s(%d)\n", "Type", types[t->opt_type], t->opt_type);
    if (t->opt_value)
        printf("%-7s: %s\n", "Default", t->opt_value);
    if (t->opt_prompt)
        printf("%-7s: %s\n", "Prompt", t->opt_prompt);
    if (t->opt_depends)
        printf("%-7s: %s\n", "Depends", t->opt_depends);
    if (t->opt_select)
        printf("%-7s: %s\n", "Select", t->opt_select);
    if (t->opt_help)
        printf("%-7s:\n%s\n", "Help", t->opt_help);
    printf("\n");

    return;
}

static char *
append(char *dst, char *src)
{
#define CDLM    "\n\t" /* delimiter for select/depends list */
    char *tmp = NULL;
    uint16_t slen = strlen(src);
    uint16_t dlen = dst ? strlen(dst) + 3 : 1;

    tmp = calloc(slen + dlen, sizeof(char));
    if (tmp && dst)
    {
        strncpy(tmp, dst, strlen(dst));
        strncat(tmp, CDLM, 3);
        free(dst);
    }
    else if (!tmp)
        err(-1, "could not allocate memory for: '%s'", src);

    return strncat(tmp, src, slen);
}

int
read_kconfigs(void)
{
    FILE *fin;
    uint16_t n = 0;

    ENTRY e, *r;
    cEntry *t = NULL;

    fin = fopen("Kconfig", "r");
    if (!fin)
        err(-1, "could not open file: %s/%s", getcwd(NULL, 0), "Kconfig");

    yyin = fin;
    tree_init("Kconfig");
    /* yyparse(); */
    while(n = yylex())
    {
        switch (n)
        {
        case T_CONFIG:
            e.key = yylval.txt;
            if (hsearch_r(e, FIND, &r, &chash))
            {
                if (opts & OUT_VERBOSE)
                    printf("'%s' read again, use earlier object\n", r->key);
                t = ((cNode *)r->data)->data;
                if (!t)
                    err(-1, "'%s' data object is %p", r->key, t);
                break;
            }

            t = calloc(1, sizeof(cEntry));
            t->opt_name = yylval.txt;

            e.data = tree_add(tree_cnode(t, CENTRY));
            if (!hsearch_r(e, ENTER, &r, &chash))
                err(-1, "could not hash option '%s' %p", e.key, r);

            break;

        case T_DEFAULT:
            free(t->opt_value);
            t->opt_value = yylval.txt;
            break;

        case T_PROMPT:
            t->opt_prompt = yylval.txt;
            break;

        case T_DEPENDS:
            t->opt_depends = append(t->opt_depends, yylval.txt);
            break;

        case T_SELECT:
            t->opt_select = append(t->opt_select, yylval.txt);
            break;

        case T_TYPE:
            t->opt_type = t->opt_type ? t->opt_type : yylval.num;
            if ((CBOOL == t->opt_type || CTRISTATE == t->opt_type)
                && !t->opt_value)
                t->opt_value = strdup("n");
            break;

        case T_HELP:
            t->opt_help = yylval.txt;
            break;
        }
    }

    fclose(fin);
    return 0;
}

static void
list_kconfigs(void)
{
    FILE *out = stdout;
    cNode *r = tree_root();

    if (opts & OUT_CONFIG && opts & CHECK_CONFIG)
    {
        printf("# This file is generated by %s\n", gstr[IPROG]);
        tree_display_config(r);
        out = stderr;
    }
    else if (opts & OUT_CONFIG)
        tree_display_config(r);
    else
        tree_display(r);

    fprintf(out, "Config files: %d\n", ((sEntry *)r->data)->s_count);
    fprintf(out, "Config options: %d\n", ((sEntry *)r->data)->o_count);
    return;
}

static cNode *
hsearch_kconfigs(const char *copt)
{
    ENTRY e, *r;

    e.data = NULL;
    e.key = (char *)copt;
    if (!hsearch_r(e, FIND, &r, &chash))
        return NULL;

    return (cNode *)r->data;
}

static int8_t
validate_option(char *opt, char *val)
{
    uint8_t l;
    cNode *c = hsearch_kconfigs(opt);
    if (!c)
        return 0;

    cEntry *t = (cEntry *)c->data;
    if (-DISABLE_CONFIG == t->opt_status)
    {
        t->opt_status = 0;
        return 1;
    }
    ((sEntry *)c->up->data)->u_count++;
    if (ENABLE_CONFIG == t->opt_status)
    {
        free(val);
        val = t->opt_value;
    }
    else
    {
        free(t->opt_value);
        t->opt_value = val;
    }
    if (TOGGLE_CONFIG == t->opt_status
        && CTRISTATE == t->opt_type)
    {
        if ('y' == tolower(*t->opt_value))
            *t->opt_value = 'm';
        else if ('m' == tolower(*t->opt_value))
            *t->opt_value = 'y';
    }
    if (!strcmp(t->opt_value, "is not set"))
    {
        t->opt_status = -CVALNOSET;
        return 1;
    }

    switch (t->opt_type)
    {
    case CINT:
        if (*val != '0' && !atoi(val))
            return t->opt_status = -t->opt_type;
        break;

    case CBOOL:
        l = strlen(val);
        if (l > 1 || !strstr("yYnN", val))
            return t->opt_status = -t->opt_type;
        break;

    case CTRISTATE:
        l = strlen(val);
        if (l > 1 || !strstr("yYnNmM", val))
            return t->opt_status = -t->opt_type;
        break;

    case CHEX:
        if (val[0] != '0' || (val[1] != 'x' && val[1] != 'X'))
            return t->opt_status = -t->opt_type;
        char *c = val+2;
        if (strlen(c) > 16)
            return t->opt_status = -t->opt_type;
        while (*c && isxdigit(*c)) c++;
        if (*c)
            return t->opt_status = -t->opt_type;
    }

    return t->opt_status = t->opt_type;
}

void
check_depends(const char *sopt)
{
    char *tok, *dps;
    cNode *c = hsearch_kconfigs(sopt);
    cEntry *t = (cEntry *)c->data;
    if (!t->opt_depends)
        return;

    dps = strdup(t->opt_depends);
    tok = strtok(dps, CDLM);
    while (tok)
    {
        c = hsearch_kconfigs(tok);
        if (!c || !((cEntry *)c->data)->opt_status)
            warnx("option '%s' is disabled. '%s' depends on it", tok, sopt);

        tok = strtok(NULL, CDLM);
    }

    free(dps);
    return;
}

static void
toggle_configs(const char *sopt, int8_t status, const char *val)
{
    static uint8_t sp = 1;
    char *tok, *slt, *svp;
    FILE *out = (opts & OUT_CONFIG) ? stderr : stdout;

    cNode *c = hsearch_kconfigs(sopt);
    if (!c)
    {
        warnx("'%s' not found in the options' list", sopt);
        return;
    }

    cEntry *t = (cEntry *)c->data;
    if (val)
    {
        free(t->opt_value);
        t->opt_value = strdup(val);
    }
    t->opt_status = status;
    for (int i = 0; i < sp; i++)
        fprintf(out, " ");
    fprintf(out, "%s\n", t->opt_name);
    if (!t->opt_select)
        return;

    slt = strdup(t->opt_select);
    tok = strtok_r(slt, CDLM, &svp);
    while (tok)
    {
        sp += 2;
        toggle_configs(tok, status, val);
        sp -= 2;

        tok = strtok_r(NULL, CDLM, &svp);
    }

    free(slt);
    return;
}

static int
check_kconfigs(const char *cfile)
{
    FILE *fin;
    char *opt, *val;
    uint16_t n, ret = 1;

    fin = fopen(cfile, "r");
    if (!fin)
        err(-1, "could not open file: %s", cfile);

    yyrestart(fin);
    while (n = yylex())
    {
        switch (n)
        {
        case T_CONFIG:
            opt = yylval.txt;
            break;

        case T_CONFVAL:
            val = yylval.txt;
            int8_t r = validate_option(opt, val);
            if (!r)
                warnx("option '%s' not found in the source tree", opt);
            else if(r < 0)
                warnx("option '%s' has invalid %s value: '%s'",
                                                  opt, types[-r], val);
            ret = (r <= 0) ? r : ret;
            break;
        }
    }

    fclose(fin);
    if (opts & CHECK_CONFIG)
        list_kconfigs();

    return ret;
}

static void
setforground(void)
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

        setforground();
        execl(cmd, basename(cmd), tmp, (char *)NULL);
        exit(-1);
    }

    waitpid(pid, &st, 0);
    setforground();
    if (0 >= (fd = check_kconfigs(tmp)))
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
    switch (opts & 0xFC)
    {
    case DISABLE_CONFIG:
    case ENABLE_CONFIG:
    case CHECK_CONFIG:
    case TOGGLE_CONFIG:
        FILE *out = (opts & OUT_CONFIG) ? stderr : stdout;
        if (gstr[IDOPT])
        {
            fprintf(out, "Disable option:\n");
            toggle_configs(gstr[IDOPT], -DISABLE_CONFIG, NULL);
        }
        if (gstr[IEOPT])
        {
            char *val = NULL;
            if (strchr(gstr[IEOPT], '='))
            {
                gstr[IEOPT] = strtok(gstr[IEOPT], "=");
                val = strtok(NULL, "=");
            }
            fprintf(out, "Enable option:\n");
            toggle_configs(gstr[IEOPT], ENABLE_CONFIG, val);
        }
        if (gstr[ITOPT])
        {
            fprintf(out, "Toggle option:\n");
            toggle_configs(gstr[ITOPT], TOGGLE_CONFIG, NULL);
        }
        if (gstr[ISOPT])
            check_kconfigs(gstr[ISOPT]);
        break;

    case EDIT_CONFIG:
        edit_kconfigs(gstr[ISOPT]);
        break;

    case SHOW_CONFIG:
        show_configs(gstr[ISOPT]);
        break;

    default:
        list_kconfigs();
    }

    _reset();
    return 0;
}
