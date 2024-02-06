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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "configk.h"

static cNode *root_node = NULL;
static cNode *curr_node = NULL;
static cNode *curr_root = NULL;

cNode *
tree_root(void)
{
    return curr_root;
}

cNode *
tree_curr_root_up(void)
{
    curr_node = curr_root;
    curr_root = curr_root->up;
    return curr_node;
}

cNode *
tree_cnode(void *data, nType type)
{
    cNode *c = calloc(1, sizeof(cNode));
    if (!c)
        return c;

    c->data = data;
    c->type = type;

    c->up = NULL;
    c->down = NULL;
    c->next = NULL;

    return c;
}

cNode *
tree_add(cNode *new)
{
    new->up = curr_root;
    if (curr_node == curr_root)
        curr_node->down = new;
    else if (curr_node->up == new->up)
        curr_node->next = new;

    curr_node = new;
    if (curr_node->type == SENTRY)
    {
        ++((sEntry *)curr_root->data)->s_count;
        curr_root = curr_node;
    }
    else if (curr_node->type == CHENTRY)
        curr_root = curr_node;
    else if (curr_node->type == CENTRY)
    {
        if (curr_root->type == CHENTRY)
            ++((sEntry *)curr_root->up->data)->o_count;
        else
            ++((sEntry *)curr_root->data)->o_count;
    }

    return curr_node;
}

cNode *
tree_init(char *fname)
{
    sEntry *s = calloc(1, sizeof(sEntry));

    s->fname = strdup(fname);
    root_node = tree_cnode(s, SENTRY);
    if (!root_node)
        return root_node;

    curr_root = root_node;
    curr_node = root_node;

    return curr_node;
}

void
tree_display(cNode *root)
{
    static uint16_t sp = 0;

    if (!root)
        return;

    cNode *cur = root;
    if (cur->type == SENTRY)
    {
        sEntry *s = cur->data;
        for (int i = 0; i < sp; i++)
            putchar(' ');
        printf("%s: %d, %d\n", s->fname, s->s_count, s->o_count);
        if (cur != curr_root)
        {
            ((sEntry *)curr_root->data)->o_count += s->o_count;
            ((sEntry *)curr_root->data)->s_count += s->s_count;
        }
    }
    if (cur->type == CHENTRY)
    {
        cEntry *c = cur->data;
        for (int i = 0; i < sp; i++)
            putchar(' ');
        printf("%s:%s\n", c->opt_name, c->opt_prompt);
    }
    if (cur->type == CENTRY)
    {
        cEntry *c = cur->data;

/*
 *      if (ENABLE_CONFIG == c->opt_status)
 *          validate_option(c->opt_name);
 *      if (TOGGLE_CONFIG == c->opt_status)
 *      {
 *          warnx("option '%s' is disabled, skip toggle", c->opt_name);
 *          c->opt_status = 0;
 *      }
 */
        if (c->opt_status && c->opt_status != -CVALNOSET
            && !check_depends(c->opt_name))
            warnx("option dependency not met for '%s'", c->opt_name);

        for (int i = 0; i < sp; i++)
            putchar(' ');
        if (c->opt_status > 0)
            printf("\033[32m%s: %s\033[0m\n", c->opt_name, c->opt_value);
        else if (c->opt_status < 0)
            printf("\033[33m%s: %s\033[0m\n", c->opt_name, c->opt_value);
        else
            printf("%s\n", c->opt_name);
    }

    sp += 2;
    tree_display(cur->down);
    sp -= 2;

    tree_display(cur->next);
    return;
}

static void
tree_display_centry(cNode *cur)
{
    if (!cur)
        return;

    if (cur->type == CENTRY)
    {
        cEntry *c = cur->data;

        if ((-CVALNOSET == c->opt_status)
            || (!c->opt_status && !(opts & CHECK_CONFIG)))
            printf("# CONFIG_%s is not set\n", c->opt_name);
        else if (c->opt_status)
            /* check_depends(c->opt_name); */
            printf("CONFIG_%s=%s\n", c->opt_name, c->opt_value);
    }
    else if (cur->type == CHENTRY)
        tree_display_centry(cur->down);
    tree_display_centry(cur->next);

    return;
}

void
tree_display_config(cNode *root)
{
    if (!root)
        return;

    cNode *cur = root;
    if (cur->type == SENTRY)
    {
        sEntry *s = cur->data;
        if (!(opts & CHECK_CONFIG))
            printf("\n# %s: %d\n#\n", s->fname, s->o_count);
        else if (s->u_count)
            printf("\n# %s\n#\n", s->fname);

        tree_display_centry(cur->down);
        if (cur != curr_root)
        {
            ((sEntry *)curr_root->data)->o_count += s->o_count;
            ((sEntry *)curr_root->data)->s_count += s->s_count;
        }
    }
    tree_display_config(cur->down);
    tree_display_config(cur->next);

    return;
}

uint32_t
tree_reset(cNode *root)
{
    static uint32_t tmem = 0;

    if (!root)
        return tmem;

    cNode *cur = root;
    tree_reset(cur->down);
    tree_reset(cur->next);

    if (cur->type == SENTRY)
    {
        sEntry *s = cur->data;
        tmem += strlen(s->fname);
        free(s->fname);
        tmem += sizeof(*s);
        free(s);
    }
    if (cur->type == CENTRY || cur->type == CHENTRY)
    {
        cEntry *c = cur->data;
        tmem += strlen(c->opt_name);
        free(c->opt_name);
        tmem += c->opt_value ? strlen(c->opt_value) : 0;
        free(c->opt_value);
        tmem += c->opt_prompt ? strlen(c->opt_prompt) : 0;
        free(c->opt_prompt);
        tmem += c->opt_depends ? strlen(c->opt_depends) : 0;
        free(c->opt_depends);
        tmem += c->opt_select ? strlen(c->opt_select) : 0;
        free(c->opt_select);
        tmem += c->opt_imply ? strlen(c->opt_imply) : 0;
        free(c->opt_imply);
        tmem += c->opt_range ? strlen(c->opt_range) : 0;
        free(c->opt_range);
        tmem += c->opt_help ? strlen(c->opt_help) : 0;
        free(c->opt_help);
        tmem += sizeof(*c);
        free(c);
    }

    tmem += sizeof(*cur);
    free(cur);

    return tmem;
}
