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

struct child *rm_child(struct child *c, struct child **l,
                       size_t *n, size_t *f)
{
    struct child *i;

    if (!c)
        return NULL;

    if (!*l)
        return c;

    for (i = *l; i->n; i = i->n) {
        if (i->n == c) {
            (*n)--;
            (*f)--;
            i->n = c->n;
            c->n = NULL;
            return c;
        }
    }

    if (*l == c) {
        (*n)--;
        (*f)--;
        *l = c->n;
        c->n = NULL;
    }

    return c;
}

struct child *rm_nchild(void *v, struct child **l, size_t *n, size_t *f)
{
    struct child *i, *j;

    if (!v || !*l)
        return NULL;

    for (i = *l; i->n; i = i->n) {
        if (i->n->v == v) {
            if (*n > 0) (*n)--;
            if (*f > 0) (*f)--;
            j = i->n;
            i->n = i->n->n;
            j->n = NULL;
            return j;
        }
    }

    if ((*l)->v == v) {
        if (*n > 0) (*n)--;
        if (*f > 0) (*f)--;
        j = *l;
        *l = (*l)->n;
        j->n = NULL;
        return j;
    }

    return NULL;
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
