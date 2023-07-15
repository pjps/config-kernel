
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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
    else if (curr_node->type == CENTRY)
        ++((sEntry *)curr_root->data)->o_count;

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
    if (cur->type == CENTRY)
    {
        cEntry *c = cur->data;
        for (int i = 0; i < sp; i++)
            putchar(' ');
        if (c->opt_status > 0)
            printf("\033[32m%s\033[0m\n", c->opt_name);
        else if (c->opt_status < 0)
            printf("\033[33m%s\033[0m\n", c->opt_name);
        else
            printf("%s\n", c->opt_name);
    }

    sp += 2;
    tree_display(cur->down);
    sp -= 2;

    tree_display(cur->next);
    return;
}

uint16_t
tree_reset(cNode *root)
{
    static uint16_t count = 0;

    if (!root)
        return count;

    cNode *cur = root;
    tree_reset(cur->down);
    tree_reset(cur->next);

    if (cur->type == SENTRY)
    {
        sEntry *s = cur->data;
        free(s->fname);
    }
    if (cur->type == CENTRY)
    {
        cEntry *c = cur->data;
        free(c->opt_name);
        free(c->opt_value);
        free(c->opt_prompt);
        free(c->opt_depends);
        free(c->opt_select);
        free(c->opt_help);
        free(c);
    }

    count++;
    free(cur);
    return count;
}
