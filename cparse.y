
%locations
%define api.pure full
%define api.prefix {cc}
%define parse.error verbose
%parse-param {char *ret}

%union {
    int num;
    char *txt;
};

%token <txt> CC_CONFIG CC_CONFID CC_CONFVAL
%token <txt> CC_EOL

%start cline
%type <num> centry

%{
#include <stdio.h>
#include "configk.h"

void yyerror(YYLTYPE *, int8_t *, char const *);

extern char *gstr;
extern char *types[];
extern int yylex(YYSTYPE *, YYLTYPE *);
%}


%%

cline:
    %empty
    | cline centry
    ;

centry:
    CC_CONFIG CC_CONFID CC_CONFVAL CC_EOL {
        int8_t r = set_option($2, $3);
        if (!r)
            warnx("option '%s' not found in the source tree", $2);

        *ret = (r > -CVALNOSET && r <= 0) ? r : *ret;
    }
    | CC_EOL {}
    | error CC_EOL  { yyerrok; }
    ;

%%

/*
 */

void
yyerror(YYLTYPE *loc, int8_t *ret, char const *serr)
{
    warnx("%s => %s:%d", __func__, serr, *ret);
}
