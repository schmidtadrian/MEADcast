#ifndef LIST_H
#define LIST_H

#include "group.h"
#include "tree.h"
#include <stddef.h>


static inline void insert_head(struct rcvr **head, struct rcvr *v, size_t *n)
{
    v->next = *head ? *head : NULL;
    *head = v;

    if (n)
        (*n)++;
}

/* Prepend `c` to `l` & increment `n` & `f`.
 * If `p` is provided, set `p` as parent of `c`. */
int add_child(struct router *p, struct child **l, struct child *c,
              size_t *n, size_t *f);

/* Prepend `c` and all its successors to `l` & set `p` as their parent.
 * Add the number of prepended childs to `n` & `f`. */
int add_childs(struct router *p, struct child **l, struct child *c,
               size_t *n, size_t *f);

/* Prepend `c` to router list of `p` & set `p` as parent of `c`. */
static inline int add_router(struct router *p, struct child *c)
{
    return add_child(p, &p->child, c, &p->nchild, &p->fchild);
}

/* Prepend `c` to leaf list of `p` & set `p` as parent of `c`. */
static inline int add_leaf(struct router *p, struct child *c)
{
    return add_child(p, &p->leaf, c, &p->nleaf, &p->fleaf);
}

/* Remove `v` from `l`, replace it with its successor
 * and decrement `n` and `f`.
 * Returns oprhan child. */
struct child *rm_child(void *v, struct child **l, size_t *n, size_t *f);

/* Remove `c` from router list of `p`.
 * Returns oprhan child. */
static inline struct child *rm_router(struct router *p, struct router *c)
{
    c->node.parent = NULL;
    return rm_child(c, &p->child, &p->nchild, &p->fchild);
}

/* Remove `c` from leaf list of `p`.
 * Returns oprhan child. */
static inline struct child *rm_leaf(struct router *p, struct leaf *c)
{
    c->node.parent = NULL;
    return rm_child(c, &p->leaf, &p->nleaf, &p->fleaf);
}

/* Returns the first free element from list `l` of len `n`.
 * `f` is the number of free elements in `l`. 
 * Assumes ordered usage of elements. */
struct child *get_free(struct child *l, size_t n, size_t f);

/* Returns the first free leaf of `r`. */
static inline struct child *get_free_leaf(struct router *r)
{
    return get_free(r->leaf, r->nleaf, r->fleaf);
}

/* Returns the first free router of `r`. */
static inline struct child *get_free_router(struct router *r)
{
    return get_free(r->child, r->nchild, r->fchild);
}

size_t get_len(struct rcvr *h);

#endif // !LIST_H
