
%{
#include <string.h>
#include <stdlib.h>
#include "configk.h"
#include "parser.tab.h"

extern int errno;
extern char *prog;
%}

/* %option debug */
%option yylineno 8bit fast nodefault
%x s_config s_prompt s_default s_depends s_select s_help s_source

 /* to read .config file */
%x S_CONFIG

%%

^(menu)?config { BEGIN(s_config); }
<s_config>[A-Z0-9_]+ {
    yylval.txt = strdup(yytext);
    BEGIN(0);
    return T_CONFIG;
}

\tdefault      { BEGIN(s_default); }
<s_default>[[:alnum:]]{1}   |
<s_default>([[:alnum:]]|[[:punct:]]).+ {
    yylval.txt = strdup(yytext);
    BEGIN(0);
    return T_DEFAULT;
}

\tprompt { BEGIN(s_prompt); }
<s_prompt>([[:alnum:]]|[[:punct:]]).+ {
    yylval.txt = strdup(yytext);
    BEGIN(0);
    return T_PROMPT;
}

\t(depends\ on\ ) { BEGIN(s_depends); }
<s_depends>{
    ([[:alnum:]]|[[:punct:]]|[[:blank:]]|(\\\n))+ {
        yylval.txt = strdup(yytext);
        BEGIN(0);
        return T_DEPENDS;
    }
}

\t(select\ ) { BEGIN(s_select); }
<s_select>{
    [A-Z0-9_]+ {
        yylval.txt = strdup(yytext);
        BEGIN(0);
        return T_SELECT;
    }
}

\thelp { BEGIN(s_help); }
<s_help>{

    \n{1,2}\t |
    ([[:alnum:]]|[[:punct:]]|[[:blank:]]).* {
        yylval.txt = strdup(yytext);
        yymore();
        return T_HELP;
    }

    \n{1,2}[^\t] {
        yyless(1);
        BEGIN(0);
    }
}

[ \t]*source\ \" { BEGIN(s_source); }
<s_source>([[:alnum:]]|[-/_$(.)])+ {
    yylval.txt = strdup(yytext);

    FILE *newfile = fopen(yylval.txt, "r");
    if (newfile) {
        yyin = newfile;
        yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));
        if (opts & BE_VERBOSE)
            printf("%s: sourcing file %s\n", prog, yylval.txt);

        sEntry *s = calloc(1, sizeof(sEntry));
        s->fname = strdup(yylval.txt);

        tree_add(tree_cnode(s, SENTRY));
    }
    else if (opts & BE_VERBOSE)
        fprintf(stderr, "%s: could not source file: %s: %s\n",
                            prog, yylval.txt, strerror(errno));

    BEGIN(0);
    free (yylval.txt);
}


[ \t]+(def_)?int       { yylval.num = CINT; return T_TYPE; }
[ \t]+(def_)?hex       { yylval.num = CHEX; return T_TYPE; }
[ \t]+(def_)?bool      { yylval.num = CBOOL; return T_TYPE; }
[ \t]+(def_)?string    { yylval.num = CSTRING; return T_TYPE; }
[ \t]+(def_)?tristate  { yylval.num = CTRISTATE; return T_TYPE; }

<*>#.*              { ; /* ignore comments */ }
<*>[ \t]+           { ; /* ignore empty line */ }
<*>.|\n             { ; /* ignore other text */ }


^CONFIG_    { BEGIN(S_CONFIG); }
<S_CONFIG>{

    [A-Z0-9_]+ {
        yylval.txt = strdup(yytext);
        return T_CONFIG;
    }

    [=].+ {
        yylval.txt = strdup(yytext + 1);
        BEGIN(0);
        return T_DEFAULT;
    }
}

<<EOF>> {
    if (!YY_CURRENT_BUFFER) {
        /* fprintf(stderr, "%s: no input to scan: %p\n", prog, yyin); */
        yyterminate();
    }
    fclose(yyin);
}

%%


int
yywrap(void) {
    yypop_buffer_state();
    if (!YY_CURRENT_BUFFER) {
        if (opts & BE_VERBOSE)
            fprintf(stderr, "%s: no more buffers to scan: %p\n", prog, yyin);
        return 1;
    }
    tree_curr_root_up();
    BEGIN(INITIAL);
    return 0;
}


/*
    yypush_buffer_state(YY_CURRENT_BUFFER);
    yyrestart(yyin);
    yyterminate();

*/
