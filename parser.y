
%union {
    int num;
    char *txt;
};

%token <txt> T_CONFIG
%token <num> T_TYPE T_INT T_HEX T_STRING T_BOOL T_TRISTATE
%token <txt> T_PROMPT
%token <txt> T_DEFAULT
%token <txt> T_DEPENDS
%token <txt> T_SELECT
%token <txt> T_HELP
%token <txt> T_SOURCE

%type <txt> clist centry

%define parse.error verbose

%{
#include <stdio.h>
#include <stdint.h>
#include "configk.h"

extern char *prog;
extern int yylex(void);
void yyerror(char const *);
%}

%%

clist:
      centry
    | clist centry { ; }
    ;

centry:
      T_CONFIG { printf("ct: %s\n", $1); }
    | error "\n"    { yyerrok; }
    ;
%%

/*

%type <num> ctype

ctype:
      T_INT
    | T_HEX
    | T_BOOL
    | T_STRING
    | T_TRISTATE    { $$ = $1; }
    ;

    |T_BOOL         { optList[optindex].opt_type = CBOOL; }
    |T_TRISTATE     { optList[optindex].opt_type = CTRISTATE; }
    |T_STRING       { optList[optindex].opt_type = CSTRING; }
    |T_HEX          { optList[optindex].opt_type = CHEX; }
    |T_INT          { optList[optindex].opt_type = CINT; }
    |T_PROMPT       { optList[optindex].opt_prompt = $1; }
    |T_DEPENDS      { optList[optindex].opt_depends[0] = $1; }
*/

void
yyerror (char const *serr)
{
    fprintf (stderr, "%s: %s => %s\n", prog, serr, yylval.txt);
}
