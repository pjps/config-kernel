
%locations
%define api.pure full
%define api.prefix {yy}
%define parse.error verbose

%union {
    int num;
    char *txt;
};

%token <txt> T_CONFIG T_CONFID
%token <num> T_TYPE T_DEFTYPE
%token <txt> T_DEFAULT
%token <txt> T_PROMPT
%token <txt> T_DEPENDS
%token <txt> T_SELECT T_IMPLY
%token <txt> T_RANGE
%token <txt> T_HELP T_HELPTEXT
%token <txt> T_EOL T_TAB
%token <txt> T_TEXT

%start clist
%type <txt> centry cname  /* ctype */ cattrs attr

%{
#include <stdio.h>
#include "configk.h"

extern char *gstr;
extern char *types[];
extern int yylex(YYSTYPE *, YYLTYPE *);
void yyerror(YYLTYPE *, char const *);

cEntry *t;
%}

%%

clist:
    %empty
    | clist centry
    ;

centry:
    cname             { t = add_new_config($$); }
    | centry cattrs
    | T_EOL
    | error           { yyerrok; }
    ;

cname:
    T_CONFIG T_CONFID T_EOL { $$ = $2; }
    ;
/*
//  | centry ctype
ctype:
    T_TAB T_TYPE T_EOL {
        t->opt_type = t->opt_type ? t->opt_type : $2;
        if (CBOOL == t->opt_type || CTRISTATE == t->opt_type)
            t->opt_value = strdup("n");
        }
    | T_TAB T_TYPE T_TEXT T_EOL {
        t->opt_type = t->opt_type ? t->opt_type : $2;
        t->opt_prompt = strdup($3); free($3);
        if (CBOOL == t->opt_type || CTRISTATE == t->opt_type)
            t->opt_value = strdup("n");
        }
    | T_TAB T_DEFTYPE T_TEXT T_EOL {
        t->opt_type = t->opt_type ? t->opt_type : $2;
        t->opt_value = strdup($3); free($3);
        }
    ;
*/
cattrs:
    T_TAB attr T_EOL
    | T_TYPE T_TEXT T_EOL    { free($2); }
    | T_DEFTYPE T_TEXT T_EOL { free($2); }
    | T_DEFAULT T_TEXT T_EOL { free($2); }
    | T_DEPENDS T_TEXT T_EOL { free($2); }
    | T_PROMPT T_TEXT T_EOL  { free($2); }
    | T_SELECT T_TEXT T_EOL  { free($2); }
    | T_IMPLY T_TEXT T_EOL   { free($2); }
    | T_RANGE T_TEXT T_EOL   { free($2); }
    | T_HELP T_HELPTEXT T_EOL{ free($2); }
    | T_HELP T_EOL           { free($1); }
    | T_TAB T_EOL
    ;

attr:
    T_TYPE {
        t->opt_type = t->opt_type ? t->opt_type : $1;
        if (!t->opt_value
            && (CBOOL == t->opt_type || CTRISTATE == t->opt_type))
            t->opt_value = strdup("n");
        else if (!t->opt_value
            && (CINT == t->opt_type || CHEX == t->opt_type))
            t->opt_value = strdup("0");
        }
    | T_TYPE T_TEXT {
        t->opt_type = t->opt_type ? t->opt_type : $1;
        if (!t->opt_prompt)
            t->opt_prompt = strdup($2);
        free($2);
        if (!t->opt_value
            && (CBOOL == t->opt_type || CTRISTATE == t->opt_type))
            t->opt_value = strdup("n");
        else if (!t->opt_value
            && (CINT == t->opt_type || CHEX == t->opt_type))
            t->opt_value = strdup("0");
        }
    | T_DEFTYPE T_TEXT {
        t->opt_type = t->opt_type ? t->opt_type : $1;
        free(t->opt_value);
        t->opt_value = $2;
        }
    | T_DEFAULT T_TEXT {
        if (t->opt_value
            && (!strcmp(t->opt_value, "n") || !strcmp(t->opt_value, "0")))
        {
            free(t->opt_value);
            t->opt_value = NULL;
        }
        t->opt_value = append(t->opt_value, $2); free($2);
        }
    | T_PROMPT T_TEXT {
        if(!t->opt_prompt)
            t->opt_prompt = strdup($2);
        free($2);
        }
    | T_DEPENDS T_TEXT {
        t->opt_depends = append(t->opt_depends, $2); free($2);
        }
    | T_SELECT T_TEXT {
        t->opt_select = append(t->opt_select, $2); free($2);
        }
    | T_IMPLY T_TEXT {
        t->opt_imply = append(t->opt_imply, $2); free($2);
        }
    | T_RANGE T_TEXT {
        t->opt_range = append(t->opt_range, $2); free($2);
    }
    | T_HELP T_HELPTEXT {
        if (!t->opt_help)
            t->opt_help = strdup($2);
        free($2);
        }
    ;

%%


void
yyerror(YYLTYPE *loc, char const *serr)
{
    if (opts & OUT_VERBOSE)
    {
        sEntry *s = (tree_root()->data);
        warnx("%s: %d: %s", s->fname, loc->last_line, serr);
    }
}
