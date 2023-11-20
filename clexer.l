
%{
#include "configk.h"
#include "cparse.tab.h"

#define YYSTYPE CCSTYPE
#define YYLTYPE CCLTYPE
%}

/* %option debug */
%option prefix="cc"
%option 8bit fast nodefault
%option bison-bridge bison-locations

%x s_config
%%

^CONFIG_ |
^#\ CONFIG_     { BEGIN(s_config); return CC_CONFIG; }

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
<*>.               {}

%%


int
yywrap(void)
{
   return 1;
}
