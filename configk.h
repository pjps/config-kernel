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

#include <err.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define __USE_GNU
#include <search.h>

typedef struct
{
    char *fname;
    uint16_t s_count;
    uint16_t o_count;
    uint16_t u_count;
} sEntry; /* source entry */


typedef enum
{
    CINT=0x1,
    CHEX=0x2,
    CBOOL=0x3,
    CSTRING=0x4,
    CTRISTATE=0x5,
    CVALNOSET=0x6
} cType; /* config value type */

typedef struct
{
    char *opt_name;
    char *opt_value;
    char *opt_prompt;
    char *opt_depends;
    char *opt_select;
    char *opt_imply;
    char *opt_range;
    char *opt_help;
    cType opt_type;
    int32_t opt_status;
} cEntry; /* config entry */


typedef enum
{
    SENTRY=0x1,
    CENTRY=0x2,
   CHENTRY=0x3
} nType; /* Node types */


/* configuration tree node */
typedef struct cNode cNode;
struct cNode
{
    cNode *up;
    cNode *down;
    cNode *next;
     void *data;
    nType type;
};


enum OPTS
{
     OUT_VERBOSE = 0x1,
      OUT_CONFIG = 0x2,
         OUTMASK = 0x3,
    CHECK_CONFIG = 0x4,
  DISABLE_CONFIG = 0x8,
   ENABLE_CONFIG = 0x10,
   TOGGLE_CONFIG = 0x20,
     EDIT_CONFIG = 0x40,
     SHOW_CONFIG = 0x80
};

enum INDX
{
    IPROG = 0x0,
    ISOPT = 0x1,
    IDOPT = 0x2,
    IEOPT = 0x3,
    IFOPT = 0x4,
    ITOPT = 0x5,
    IARCH = 0x6,
    IEDTR = 0x7,
    ITMPD = 0x8,
    IGREP = 0x9,
   GSTRSZ = 0xA
};

enum EXPRTYPE
{
    EXPR_DEPENDS = 0x1,
    EXPR_DEFAULT = 0x2,
    EXPR_RANGE = 0x3
};

extern uint16_t opts;
#define HASHSZ 20000
extern struct hsearch_data chash;

extern cNode *tree_root(void);
extern cNode *tree_curr_root_up(void);
extern cNode *tree_cnode(void *, nType);
extern cNode *tree_add(cNode *);
extern cNode *tree_init(char *);
extern void tree_display(cNode *);
extern uint32_t tree_reset(cNode *);
extern void tree_display_config(cNode *);

extern cNode *filenode(cNode *);
extern char *append(char *, char *);
extern cEntry *add_new_config(char *, nType);
extern int8_t check_depends(const char *);
extern int8_t set_option(const char *, char *);
extern int8_t validate_option(const char *);
extern cNode *hsearch_kconfigs(const char *);
extern int8_t toggle_configs(const char *, uint8_t, char *, bool);
