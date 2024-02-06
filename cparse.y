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

%locations
%define api.pure full
%define api.prefix {cc}
%define parse.error verbose
%parse-param {char *ret}
%initial-action { reindex = 0; }

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

/* cache recently edited entries */
#define REDITSZ 256
static uint16_t reindex;
static cEntry *redits[REDITSZ];

uint8_t cache_redits(cEntry *);
static uint8_t is_redits(cEntry *);
void yyerror(YYLTYPE *, char *, char const *);

extern char *gstr;
extern char *types[];
extern uint8_t cstatus, postedit;
extern int yylex(YYSTYPE *, YYLTYPE *);
%}


%%

cline:
    %empty
    | cline centry
    ;

centry:
    CC_CONFIG CC_CONFID CC_CONFVAL CC_EOL {
        cNode *c = hsearch_kconfigs($2);
        cEntry *t = c ? (cEntry *)c->data : NULL;
        uint8_t ceditflag = 0;
        if (EDIT_CONFIG == postedit)
        {
            if (!t || is_redits(t))
                break;
            else if (cstatus == ENABLE_CONFIG)
            {
                if (t->opt_status == -CVALNOSET)
                {
                    ceditflag = 1;
                    warnx("Enable option:");
                }
                else if (strcmp(t->opt_value, $3))
                {
                    ceditflag = 1;
                    warnx("Edit option %s: %s => %s", $2, t->opt_value, $3);
                }
            }
            else if (cstatus == DISABLE_CONFIG && t->opt_status != -CVALNOSET)
            {
                ceditflag = 1;
                warnx("Disable option:");
            }
        }
        if (!postedit || ceditflag)
            toggle_configs($2, cstatus, $3, postedit);

        if (SHOW_CONFIG == postedit)
        {
            if (t)
            {
                if (t->opt_status == -CVALNOSET)
                    printf("# CONFIG_%s is not set\n", t->opt_name);
                else
                    printf("CONFIG_%s=%s\n", t->opt_name, t->opt_value);
            }
            else
            {
                warnx("option '%s' not found in the source tree", $2);
                if (cstatus == ENABLE_CONFIG)
                    printf("CONFIG_%s=%s\n", $2, $3);
                else if (cstatus == DISABLE_CONFIG)
                    printf("# CONFIG_%s is not set\n", $2);
            }
        }
        free($2); free($3);
    }
    | CC_EOL { if (SHOW_CONFIG == postedit) printf("\n"); }
    | error CC_EOL  { yyerrok; }
    ;

%%


void
yyerror(YYLTYPE *loc, char *ret, char const *serr)
{
    warnx("%s => %d: %d: %s", __func__, loc->last_line, *ret, serr);
}

uint8_t
is_redits(cEntry *t)
{
    for (int i = 0; i < reindex; i++)
        if (redits[i] == t)
            return 1;

    return 0;
}

uint8_t
cache_redits(cEntry *t)
{
    if (is_redits(t))
        return 0;

    redits[reindex++] = t;
    if (!(reindex %= REDITSZ))
        warnx("recent edits cache full, reset");

    return reindex;
}
