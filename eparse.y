
%locations
%define lr.type ielr
%define api.pure full
%define api.prefix {ee}
%define parse.error verbose
%parse-param {unsigned char cmd}
%parse-param {char **val}
%initial-action { ifctx = 0; }

%union {
    int num;
    char *txt;
};

%token <txt> EE_CONFID EE_VALUE EE_NUM
%token <txt> EE_IF EE_OR EE_AND
%token <txt> EE_NE EE_LT EE_LE EE_GT EE_GE
%token <txt> EE_BM EE_EM EE_MARG
%token <txt> EE_TAB EE_EOL

%start expression
%type <num> expr
%type <txt> macroexpr

%left '=' EE_NE EE_LT EE_LE EE_GT EE_GE
%left EE_OR EE_AND
%right EE_IF
/* %precedence EE_IF */
%precedence NEG

%{
#include <stdio.h>
#include "configk.h"

static int8_t is_enabled(const char *);
static cEntry *get_centry(const char *);
int8_t eval_expression(uint8_t, const char *, char **);
void yyerror(YYLTYPE *, uint8_t, char **, char const *);

uint8_t ifctx = 0;
extern char *gstr;
extern char *types[];
extern int yylex(YYSTYPE *, YYLTYPE *);
%}

%%

expression:
    %empty
    | expression expr { return $2; }
    ;

expr:
    EE_CONFID {
        $$ = eval_expression(ifctx ? ifctx : cmd, $1, val);
        if (opts & OUT_VERBOSE)
            fprintf(stderr, "%s(%d) ", $1, $$);
    }
    | EE_CONFID EE_IF { ifctx = 1; } expr {
        ifctx = 0;
        if ($$ = $4) {
            $$ = eval_expression(cmd, $1, val);
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "%s(%d) ", $1, $$);
        }
    }
    | EE_VALUE {
        $$ = 0;
        if (val) {
            free(*val); *val = $1;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "v(%s) ", $1);
            $$ = 1;
        }
    }
    | EE_VALUE EE_IF { ifctx = 1; } expr {
        ifctx = 0;
        $$ = $4;
        if ($$ && val) {
            free(*val); *val = $1;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "v(%s) ", $1);
            $$ = 1;
        }
    }
    | EE_NUM EE_NUM {
        $$ = 0;
        if (val) {
            snprintf(*val, 64, "%s %s", $1, $2);
            $$ = 1;
        }
    }
    | EE_NUM EE_NUM EE_IF { ifctx = 1; } expr {
        ifctx = 0;
        $$ = $5;
        if ($$ && val) {
            snprintf(*val, 64, "%s %s", $1, $2);
        }
    }
    | EE_CONFID '=' EE_CONFID {
        int8_t e1 = eval_expression(1, $1, val);
        int8_t e3 = eval_expression(1, $3, val);
        $$ = (e1 == e3);
        if (opts & OUT_VERBOSE)
            fprintf(stderr, "(%s(%d) = %s(%d)) ", $1, e1, $3, e3);
    }
    | EE_CONFID '=' EE_VALUE {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        if (t1) {
            char *v1 = t1->opt_value;
            $$ = !strcmp(v1, $3);
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) = v(%s)) ", $1, v1, $3);
        }
    }
    | EE_CONFID EE_NE EE_CONFID {
        int8_t e1 = eval_expression(1, $1, val);
        int8_t e3 = eval_expression(1, $3, val);
        $$ = (e1 != e3);
        if (opts & OUT_VERBOSE)
            fprintf(stderr, "(%s(%d) != %s(%d)) ", $1, e1, $3, e3);
    }
    | EE_CONFID EE_NE EE_VALUE {
        $$ = 0;
        cEntry *t = get_centry($1);
        if (t) {
            $$ = strcmp(t->opt_value, $3);
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) != v(%s)) ", $1, t->opt_value, $3);
        }
    }
    | EE_CONFID EE_GE EE_CONFID {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        cEntry *t3 = get_centry($3);
        if (t1 && t3) {
            char *v1 = t1->opt_value;
            char *v3 = t3->opt_value;
            $$ = strcmp(v1, v3) >= 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) >= %s(%s)) ", $1, v1, $3, v3);
        }
    }
    | EE_CONFID EE_GE EE_VALUE {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        if (t1) {
            char *v1 = t1->opt_value;
            $$ = strcmp(v1, $3) >= 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) >= v(%s)) ", $1, v1, $3);
        }
    }
    | EE_CONFID EE_LE EE_CONFID { $$ = 0;
        $$ = 0;
        cEntry *t1 = get_centry($1);
        cEntry *t3 = get_centry($3);
        if (t1 && t3) {
            char *v1 = t1->opt_value;
            char *v3 = t3->opt_value;
            $$ = strcmp(v1, v3) <= 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) <= %s(%s)) ", $1, v1, $3, v3);
        }
    }
    | EE_CONFID EE_LE EE_VALUE {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        if (t1) {
            char *v1 = t1->opt_value;
            $$ = strcmp(v1, $3) <= 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) <= v(%s)) ", $1, v1, $3);
        }
    }
    | EE_CONFID EE_GT EE_CONFID {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        cEntry *t3 = get_centry($3);
        if (t1 && t3) {
            char *v1 = t1->opt_value;
            char *v3 = t3->opt_value;
            $$ = strcmp(v1, v3) > 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) > %s(%s)) ", $1, v1, $3, v3);
        }
    }
    | EE_CONFID EE_GT EE_VALUE {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        if (t1) {
            char *v1 = t1->opt_value;
            $$ = strcmp(v1, $3) > 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) > v(%s)) ", $1, v1, $3);
        }
    }
    | EE_CONFID EE_LT EE_CONFID {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        cEntry *t3 = get_centry($3);
        if (t1 && t3) {
            char *v1 = t1->opt_value;
            char *v3 = t3->opt_value;
            $$ = strcmp(v1, v3) < 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) < %s(%s)) ", $1, v1, $3, v3);
        }
    }
    | EE_CONFID EE_LT EE_VALUE {
        $$ = 0;
        cEntry *t1 = get_centry($1);
        if (t1) {
            char *v1 = t1->opt_value;
            $$ = strcmp(v1, $3) < 0 ? 1 : 0;
            if (opts & OUT_VERBOSE)
                fprintf(stderr, "(%s(%s) < v(%s)) ", $1, v1, $3);
        }
    }
    | '!' expr %prec NEG { $$ = !$2; }
    | expr EE_OR expr { $$ = ($1 || $3); }
    | expr EE_AND expr { $$ = ($1 && $3); }
    | EE_BM expr EE_EM { $$ = $2; }
    | EE_BM macroexpr EE_EM { $$=0; fprintf(stderr, "$(%s)\n", $2); }
    | EE_BM macroexpr EE_EM EE_IF expr {
        $$=0; fprintf(stderr, "$(%s) if %s\n", $2, $5);
    }
    ;

macroexpr:
    EE_MARG { $$ = $1; }
    | EE_MARG ',' EE_MARG { $$ = "$1,$3"; }
    | EE_MARG ',' EE_MARG ',' EE_MARG { $$ = "$1,$3,$5"; }
    | EE_MARG ',' EE_MARG ',' EE_MARG ',' EE_MARG { $$ = "$1,$3,$5,$7"; }
    ;

%%

/*
 *
    | '"' expr '"' { $$ = $2; }
    | EE_CONFID EE_CONFID {
        cEntry *t1 = get_centry($1);
        cEntry *t2 = get_centry($2);
        if (val) {
            snprintf(*val, 64, "%s %s", t1->opt_value, t2->opt_value);
            $$ = 1;
        }
    }
 *
 */


void
yyerror(YYLTYPE *loc, uint8_t etype, char **val, char const *serr)
{
    warnx("%s => %d: %s", __func__, etype, serr);
}

static cEntry *
get_centry(const char *sopt)
{
    cNode *c = hsearch_kconfigs(sopt);
    if (!c)
        return NULL;

    return (cEntry *)c->data;
}

static int8_t
is_enabled(const char *sopt)
{
    cEntry *t = get_centry(sopt);
    if (!t)
    {
        warnx("%s: option %s not found", __func__, sopt);
        return -1;
    }
    if (!t->opt_status)
        return t->opt_status;

    if (t->opt_value && (CBOOL == t->opt_type || CTRISTATE == t->opt_type))
    {
        if ('n' == *t->opt_value || 'N' == *t->opt_value)
            return 0;
        else if ('m' == *t->opt_value || 'M' == *t->opt_value)
            return 1;
        else
            return 2;
    }

    return t->opt_status;
}

int8_t
get_default_value(const char *sopt, char **val)
{
    cNode *c = hsearch_kconfigs(sopt);
    if (!c)
        return 0;

    cEntry *t = (cEntry *)c->data;
    if (t->opt_status && val)
    {
        free(*val);
        *val = strdup(t->opt_value);
    }

    return t->opt_status;
}

int8_t
select_option(const char *sopt, char **val)
{
    cNode *c = hsearch_kconfigs(sopt);
    if (!c)
        return 0;

    return 1;
}


int8_t
eval_expression(uint8_t cmd, const char *opt, char **val)
{
    int8_t r = 0;

    switch (cmd)
    {
    case EXPR_DEFAULT:
        r = get_default_value(opt, val);
        break;

    case EXPR_SELECT:
    case EXPR_IMPLY:
        r = select_option(opt, val);
        break;

    default:
        r = is_enabled(opt);
    }

    return r;
}
