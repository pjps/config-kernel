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
#include "cparse.tab.h"

#define YYSTYPE CCSTYPE
#define YYLTYPE CCLTYPE

uint8_t cstatus;
extern uint8_t postedit;
%}

/* %option debug */
%option prefix="cc"
%option 8bit fast nodefault
%option bison-bridge bison-locations

%x s_config
%%

^CONFIG_      { cstatus = ENABLE_CONFIG; BEGIN(s_config); return CC_CONFIG; }
^#[ \t]*CONFIG_ { cstatus = DISABLE_CONFIG; BEGIN(s_config); return CC_CONFIG; }

<s_config>{
    [A-Za-z0-9_]+   {
        yylval->txt = strdup(yytext);
        return CC_CONFID;
    }

    [=].+ |
    " is not set" 	{
        yylval->txt = strdup(yytext + 1);
        return CC_CONFVAL;
    }
}

<*>\n              { BEGIN(0); return CC_EOL; }
<*>.               { if (SHOW_CONFIG == postedit) printf(yytext); }

%%


int
yywrap(void)
{
   return 1;
}
