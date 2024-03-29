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
#include "configk.h"
#include "eparse.tab.h"

#define YYSTYPE EESTYPE
#define YYLTYPE EELTYPE
#define YY_USER_ACTION (yylloc->last_line=yylineno);
%}

/* %option debug
 reentrant */
%option prefix="ee"
%option 8bit fast nodefault noinput
%option yylineno bison-bridge bison-locations
%x s_kmacro
%%

if              { return EE_IF; }
"||"            { return EE_OR; }
"&&"            { return EE_AND; }
">"             { return EE_GT; }
">="            { return EE_GE; }
"<"             { return EE_LT; }
"<="            { return EE_LE; }
"!="            { return EE_NE; }
"$("            { BEGIN(s_kmacro); return EE_BM; }
"("             { return EE_BM; }
")"             { BEGIN(0); return EE_EM; }
[=!,;]          { return *yytext; }
"range"         { return EE_RANGE; }


(0x)[0-9a-fA-F]+ |
[a-z0-9_"-]+ {
    yylval->txt = strdup(yytext);
    return EE_VALUE;
}

[A-Z0-9]([[:alnum:]]|[-_])* {
    yylval->txt = strdup(yytext);
    return EE_CONFID;
}

<s_kmacro>{
    [,] {
        return *yytext;
    }

    [^\n]+[,] {
        unput(',');
        yylval->txt = strndup(yytext, yyleng-1);
        return EE_MARG;
    }

    [^\n]+[)] {
        uint8_t i = -1, n = 1;
        while (n && ++i < yyleng)
        {
            if ('(' == yytext[i])
                ++n;
            else if (')' == yytext[i])
                --n;
        }
        yyless(i);
        yylval->txt = strndup(yytext, i);
        BEGIN(0);
        return EE_MARG;
    }
}

(\t|\ {7,8})    { /* return EE_TAB; */ }
<*>\n           { /* return EE_EOL; */ }
<*>#.+          { /* ignore comments */ }
<*>.|[ ]+       {}

%%


int
yywrap(void)
{
    return 1;
}

int8_t
eescans(uint8_t etype, const char *expr, char **retval)
{
    int8_t r = 0;
    YY_BUFFER_STATE b = NULL;
    if (YY_CURRENT_BUFFER)
        b = YY_CURRENT_BUFFER;

    //warnx("%s: %s", __func__, expr);
    yy_scan_string(expr);
    r = eeparse(etype, retval);
    yy_delete_buffer(YY_CURRENT_BUFFER);

    if (b)
        yy_switch_to_buffer(b);

    return r;
}
