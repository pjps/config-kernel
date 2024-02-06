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

%{
#include <stdlib.h>
#include "configk.h"
#include "parser.tab.h"

extern int errno;
extern char *gstr[];
extern struct hsearch_data chash;
inline static void set_yylloc(YYLTYPE *);
static void source_kconfigs(const char *);

#define YY_USER_ACTION set_yylloc(yylloc);
%}

/* %option debug */
%option prefix="yy"
%option noinput nounput
%option 8bit fast nodefault
%option yylineno bison-bridge bison-locations

%x s_config s_source s_help s_prompt s_text
%%

^[ \t]*(menu)?config { BEGIN(s_config); return T_CONFIG; }
<s_config>{
    [A-Za-z0-9_]+ {
        yylval->txt = strdup(yytext);
        BEGIN(0);
        return T_CONFID;
    }
}

^[ \t]*(source)[ \t] {
    yylval->txt = calloc(256, sizeof(char));
    BEGIN(s_source);
}
<s_source>{

    ([[:alnum:]]|[-/_.])+ {
        strcat(yylval->txt, yytext);
    }

    \$\(?SRCARCH\)? {
        strcat(yylval->txt, gstr[IARCH] ? gstr[IARCH] : yytext);
    }

    \n {
        source_kconfigs(yylval->txt);
        BEGIN(0);
        free(yylval->txt);
    }
}

(---)?help(---)?\n {
    yylval->txt = calloc(4 * 1024, sizeof(char));
    BEGIN(s_help);
    return T_HELP;
}
<s_help>{

    \n{1,2}[[:blank:]]+ |
    ([[:alnum:]]|[[:punct:]]|[[:blank:]])+ {
        strcat(yylval->txt, yytext);
    }

    \n{0,2}[[:^blank:]] |
    \n{1,2}(\ {1,7}|\t)[[:^blank:]] {
        yyless(0);
        BEGIN(0);
        return T_HELPTEXT;
    }

    <<EOF>> {
        BEGIN(0);
        return T_HELPTEXT;
    }
}

^choice         { yylval->txt = strdup(yytext); return T_CHOICE; }
^endchoice      { yylval->txt = strdup(yytext); return T_ENDCHOICE; }

int[ ]?         { BEGIN(s_text); yylval->num = CINT; return T_TYPE; }
hex[ ]?         { BEGIN(s_text); yylval->num = CHEX; return T_TYPE; }
bool[ ]?        { BEGIN(s_text); yylval->num = CBOOL; return T_TYPE; }
string[ ]?      { BEGIN(s_text); yylval->num = CSTRING; return T_TYPE; }
tristate[ ]?    { BEGIN(s_text); yylval->num = CTRISTATE; return T_TYPE; }
def_bool[ ]     { BEGIN(s_text); yylval->num = CBOOL; return T_DEFTYPE; }
def_tristate[ ] { BEGIN(s_text); yylval->num = CTRISTATE; return T_DEFTYPE; }
prompt[ ]       { BEGIN(s_text); return T_PROMPT; }
default[ ]      { BEGIN(s_text); return T_DEFAULT; }
"depends on "   { BEGIN(s_text); return T_DEPENDS; }
select[ ]       { BEGIN(s_text); return T_SELECT; }
imply[ ]        { BEGIN(s_text); return T_IMPLY; }
range[ ]        { BEGIN(s_text); return T_RANGE; }
<s_text>{

    \$.+ |
    ([[:alnum:]]|[[:punct:]]|[[:blank:]]|(\\\n))+ {
        yylval->txt = strdup(yytext);
        BEGIN(0);
        return T_TEXT;
    }
}


(\t|\ {7,8})+   { return T_TAB; }
<*>\n           { BEGIN(0); return T_EOL; }
<*>#.+          { /* ignore comments */ }
<*>.|[ ]+       {}

<<EOF>> {
    if (!YY_CURRENT_BUFFER) {
        yyterminate();
    }
    fclose(yyin);
}

%%


int
yywrap(void) {
    yypop_buffer_state();
    if (!YY_CURRENT_BUFFER) {
        if (opts & OUT_VERBOSE)
            warnx("no more buffers to scan: %p", yyin);
        return 1;
    }
    tree_curr_root_up();
    yylineno = YY_CURRENT_BUFFER->yy_bs_lineno;
    BEGIN(INITIAL);
    return 0;
}

static void
source_kconfigs(const char *fname)
{
    ENTRY e, *r;

    e.key = strdup(fname);
    if (hsearch_r(e, FIND, &r, &chash))
    {
        warnx("'%s' read again, use earlier object", r->key);
        free(e.key);
        return;
    }

    FILE *newfile = fopen(e.key, "r");
    if (!newfile)
    {
        if (opts & OUT_VERBOSE)
            warn("could not source file: %s", e.key);
        free(e.key);
        return;
    }

    yyin = newfile;
    YY_CURRENT_BUFFER->yy_bs_lineno = yylineno;
    yypush_buffer_state(yy_create_buffer(yyin, YY_BUF_SIZE));
    yylineno = 1;
    if (opts & OUT_VERBOSE)
        warnx("sourcing file %s", e.key);

    sEntry *s = calloc(1, sizeof(sEntry));
    s->fname = e.key;
    e.data = tree_add(tree_cnode(s, SENTRY));
    if (!hsearch_r(e, ENTER, &r, &chash))
        err(-1, "could not hash file '%s' %p", e.key, r);

    return;
}

inline static void
set_yylloc(YYLTYPE *yylloc)
{
    yylloc->first_line = yylloc->last_line;
    yylloc->last_line = yylineno;

    yylloc->first_column = 1;
    yylloc->last_column = yyleng;

    return;
}
