#include "list.h"
#include "group.h"
#include <stdlib.h>

size_t get_len(struct rcvr *h)
{
    size_t len;

    for (len = 0; h; len++)
        h = h->next;

    return len;
}

int add_child(struct router *p, struct child **l, struct child *c,
              size_t *n, size_t *f)
{
    if (set_parent(c, p))
        return EXIT_FAILURE;

    c->n = *n ? *l : NULL;
    *l = c;
    (*n)++;
    (*f)++;

    return EXIT_SUCCESS;
}

int add_childs(struct router *p, struct child **l, struct child *c,
               size_t *n, size_t *f)
{
    struct child *i;
    size_t m;

    if (!c)
        return -1;

    for (i = c, m = 1; i->n; i = i->n, m++)
        set_parent(i, p);

    set_parent(i, p);
    i->n = *n ? *l : NULL;

    *l = c;
    *n += m;
    *f += m;

    return 0;
}

struct child *rm_child(void *v, struct child **l, size_t *n, size_t *f)
{
    struct child *i, *j, **k;

    if (!v || !*l)
        return NULL;

    for (i = *l; i->n; i = i->n) {
        if (i->n->v == v) {
            k = &i->n;
            goto update_ptrs;
        }
    }

    if ((*l)->v == v) {
        k = l;
        goto update_ptrs;
    }

    return NULL;

update_ptrs:
    if (*n > 0) (*n)--;
    if (*f > 0) (*f)--;
    j = *k;
    *k = (*k)->n;
    j->n = NULL;
    return j;
}

struct child *get_free(struct child *l, size_t n, size_t f)
{
    size_t i;

    if (!l || f == 0 || f > n)
        return NULL;

    for (i = n - f; l && i > 0; i--)
        l = l->n;

    return l;
}
