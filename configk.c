
#include <err.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
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

char *prog;
uint16_t oindex = 0;
uint8_t opts = 0, *sopt;
cEntry oList[OPTLSTSZ];
const char *types[] = { "", "int", "hex", "bool", "string", "tristate" };

static void
usage(void)
{
    printf("Usage: %s [OPTIONS] <source-dir>\n", prog);
}

static void
printh(void)
{
#define fmt " %-22s %s\n"
    usage();
    printf("\nOptions:\n");
    printf(fmt, " -c --config <file>", "check configs against the source tree");
    printf(fmt, " -d --depends <option>", "list option dependencies");
    printf(fmt, " -h --help", "show help");
    printf(fmt, " -s --selects <option>", "list selected options");
    printf(fmt, " -v --version", "show version");
    printf(fmt, " -V --verbose", "show verbose output");
    printf("\nReport bugs to: <pjp@redhat.com>\n");
}

static uint8_t
check_options(int argc, char *argv[])
{
    int n;
    char optstr[] = "+c:d:hs:vV";
    extern int opterr, optind;

    struct option lopt[] = \
    {
        { "config", required_argument, NULL, 'c' },
        { "depends", required_argument, NULL, 'd' },
        { "help", no_argument, NULL, 'h' },
        { "select", required_argument, NULL, 's' },
        { "version", no_argument, NULL, 'v' },
        { "verbose", no_argument, NULL, 'V' },
        { 0, 0, 0, 0 }
    };

    opts = opterr = optind = 0;
    while ((n = getopt_long(argc, argv, optstr, lopt, &optind)) != -1)
    {
        switch (n)
        {
        case 'c':
            opts = CHECK_CONFIG | (opts & BE_VERBOSE);
            if (sopt) free(sopt);
            sopt = strdup(optarg);
            break;

        case 'd':
            opts = LIST_DEPENDS | (opts & BE_VERBOSE);
            if (sopt) free(sopt);
            sopt = strdup(optarg);
            break;

        case 'h':
            printh();
            exit(0);

        case 's':
            opts = LIST_SELECTS | (opts & BE_VERBOSE);
            if (sopt) free(sopt);
            sopt = strdup(optarg);
            break;

        case 'v':
            printf("%s version %s\n", prog, VERSION);
            exit(0);

        case 'V':
            opts |= BE_VERBOSE;
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

    prog = strdup(argv[0]);
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
    if (!hcreate_r(OPTLSTSZ, &chash))
        err(-1, "could not create chash table");

    return;
}

static void
print_centry (int16_t index)
{
    ENTRY e, *r;
    cEntry *t = NULL;

    e.data = NULL;
    e.key = oList[index].opt_name;
    if (!hsearch_r(e, FIND, &r, &chash))
        errx(-1, "could not find key '%s' in the hash: %s", e.key, r);

    t = r->data;
    printf("config %s\n", t->opt_name);
    printf("\t%s(%d)\n", types[t->opt_type], t->opt_type);
    if (t->opt_value)
        printf("\t%s\n", t->opt_value);
    if (t->opt_prompt)
        printf("\t%s\n", t->opt_prompt);
    if (t->opt_depends)
        printf("\t%s\n", t->opt_depends);
    if (t->opt_select)
        printf("\t%s\n", t->opt_select);
    if (t->opt_help)
        printf("\t%s\n", t->opt_help);
    printf("\n");

    return;
}

static void
_reset(void)
{
    cEntry *t;
    ENTRY e, *r;
    uint16_t n = 0;

    for (n = 0; n < oindex; n++)
    {
        e.data = NULL;
        e.key = oList[n].opt_name;
        hsearch_r(e, FIND, &r, &chash);
        if (r)
        {
            t = r->data;
            free(t->opt_name);
            free(t->opt_value);
            free(t->opt_prompt);
            free(t->opt_depends);
            free(t->opt_select);
            free(t->opt_help);
            free(t);
        }
    }
    hdestroy_r(&chash);

    free(sopt);
    free(prog);
    return;
}

static char *
append(char *dst, char *src)
{
    char *tmp = NULL;
    uint16_t slen = strlen(src);
    uint16_t dlen = dst ? strlen(dst) + 3 : 1;

    tmp = calloc(slen + dlen, sizeof(char));
    if (tmp && dst)
    {
        strncpy(tmp, dst, strlen(dst));
        strncat(tmp, "\n\t", 3);
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
    /* yyparse(); */
    while(n = yylex())
    {
        switch (n)
        {
        case T_CONFIG:
            e.key = yylval.txt;
            if (hsearch_r(e, FIND, &r, &chash))
            {
                if (opts & BE_VERBOSE)
                    printf("'%s' read again, use earlier object\n", r->key);
                t = r->data;
                if (!t)
                    err(-1, "'%s' data object is %p", r->key, t);
                break;
            }

            oList[oindex++].opt_name = yylval.txt;

            t = calloc(1, sizeof(cEntry));
            t->opt_name = yylval.txt;
            e.data = t;
            if (!hsearch_r(e, ENTER, &r, &chash))
                err(-1, "could not hash option '%s' %p", e.key, r);
            break;

        case T_DEFAULT:
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
            break;

        case T_HELP:
            t->opt_help = yylval.txt;
            break;
        }
    }

    fclose(fin);
    return 0;
}

static int8_t
validate_option(char *opt, char *val)
{
    ENTRY e, *r;
    cEntry *t = NULL;

    e.data = NULL;
    e.key = opt;
    if (!hsearch_r(e, FIND, &r, &chash))
        return 0;

    t = r->data;
    switch (t->opt_type)
    {
    case CINT:
        if (*val != '0' && !atoi(val))
            return -t->opt_type;
        break;

    case CBOOL:
    case CTRISTATE:
        uint8_t l = strlen(val);
        if (l > 1 || (*val != 'y' && *val != 'Y'
            && *val != 'n' && *val != 'N'
            && *val != 'm' && *val != 'M' && *val != ' '))
            return -t->opt_type;
        break;

    case CHEX:
        if (val[0] != '0' || (val[1] != 'x' && val[1] != 'X'))
            return -t->opt_type;
        char *c = val+2;
        while (isxdigit(*c++));
        if (*c)
            return -t->opt_type;
    }

    return t->opt_type;
}

static int
check_kconfig(const char *cfile)
{
    FILE *fin;
    char *opt, *val;
    uint16_t n, c = 0;

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

        case T_DEFAULT:
            val = yylval.txt;
            int8_t r = validate_option(opt, val);
            if (!r)
                warnx("option '%s' not found in the source tree", opt);
            else if(r < 0)
                warnx("option '%s' has invalid %s value: '%s'",
                                                  opt, types[-r], val);
            break;
        }
    }

    fclose(fin);
    return 0;
}

static void
list_depends(const char *sopt)
{
    ENTRY e, *r;
    cEntry *t = NULL;

    e.data = t;
    e.key = (char *)sopt;
    hsearch_r(e, FIND, &r, &chash);
    if (r)
    {
        t = r->data;
        printf("%s depends on:\n  => %s\n", t->opt_name, t->opt_depends);
    }
    else
        warnx("'%s' not found in the options' list: %s", e.key, r);

    return;
}

static void
list_selects(const char *sopt)
{
    ENTRY e, *r;
    cEntry *t = NULL;

    e.data = t;
    e.key = (char *)sopt;
    hsearch_r(e, FIND, &r, &chash);
    if (r)
    {
        t = r->data;
        printf("%s selects:\n  => %s\n", t->opt_name, t->opt_select);
    }
    else
        warnx("'%s' not found in the options' list: %s", e.key, r);

    return;
}

int
main(int argc, char *argv[])
{
    _init(argc, argv);

    read_kconfigs();
    switch (opts & 0xE)
    {
    case CHECK_CONFIG:
        check_kconfig(sopt);
        break;

    case LIST_DEPENDS:
        list_depends(sopt);
        break;

    case LIST_SELECTS:
        list_selects(sopt);
        break;

    default:
        for (int n = 0; n < oindex; n++)
            print_centry(n);
        printf("Config options read: %d\n", oindex);
    }

    _reset();
    return 0;
}
