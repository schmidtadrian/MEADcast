#include "tree.h"
#include "group.h"
#include "list.h"
#include "tx.h"
#include "util.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct child *get_child(void *n, struct child *l)
{
    if (!n || !l)
        return NULL;

    for (; l; l = l->n)
        if (l->v == n)
            return l;

    return NULL;
}

struct child *rm_childl(void *v, struct child **l, size_t *n, size_t *f)
{
    struct child *i, *j;

    if (!v || !*l)
        return NULL;

    printf("Searching ");
    print_ia(&get_router(v)->sa.sin6_addr);
    printf("\n");

    for (i = *l; i->n; i = i->n) {
        if (i->n->v == v) {
            (*n)--;
            (*f)--;
            j = i->n;
            i->n = i->n->n;
            j->n = NULL;
            printf("Removing ");
            print_ia(&get_router(j->v)->sa.sin6_addr);
            printf("\n");
            return j;
        }
    }

    if ((*l)->v == v) {
        (*n)--;
        (*f)--;
        j = *l;
        *l = (*l)->n;
        j->n = NULL;
        printf("Removing ");
        print_ia(&get_router(j->v)->sa.sin6_addr);
        printf("\n");
        return j;
    }

    return NULL;
}

struct leaf *create_leaf(struct sockaddr_in6 *addr, uint16_t port, struct router *p)
{
    struct leaf *l;
    struct child *c;

    if (!p)
        return NULL;

    l = malloc(sizeof(struct leaf));
    l->node.parent = &p->node;
    l->node.type = LEAF_NODE;
    l->val.addr.sa = *addr;
    l->val.addr.port = port;
    l->val.hops = -1;
    l->val.next = NULL;

    c = malloc(sizeof(struct child));
    c->v = &l->node;

    if (add_leaf(p, c)) {
        free(l);
        free(c);
        return NULL;
    }

    return l;
}

struct router *create_router(struct sockaddr_in6 *addr, struct router *p)
{
    struct router *r;
    struct child *c;

    r = calloc(1, sizeof(*r));
    r->node.type = ROUTER_NODE;
    r->sa = *addr;

    if (!p)
        return r;

    r->node.parent = &p->node;

    c = malloc(sizeof(struct child));
    c->v = &r->node;

    if (add_router(p, c)) {
        free(r);
        free(c);
        return NULL;
    }

    return r;
}

int free_router(struct router *r, struct child **childs, size_t *nchild,
                struct child **leafs, size_t *nleaf)
{
    struct router *p;
    struct child *c;

    if (!r)
        return -1;

    p = get_router(r->node.parent);
    c = get_child(&r->node, p->child);
    c = rm_router(p, c);

    *childs = r->child;
    *nchild = r->nchild;
    *leafs = r->leaf;
    *nleaf = r->nleaf;

    free(c);
    free(r);

    return 0;
}

int cut_router(struct router *r)
{
    struct router *p;
    struct child *childs, *leafs;
    size_t nchild, nleaf;

    if (!r)
        return -1;

    p = get_router(r->node.parent);
    free_router(r, &childs, &nchild, &leafs, &nleaf);
    add_childs(p, &p->child, childs, &p->nchild, &p->fchild);
    add_childs(p, &p->leaf, leafs, &p->nleaf, &p->fleaf);

    return 0;
}

int adopt(struct router *p, struct child *c)
{
    struct router *r;
    struct node *n;

    if (!p || !c || !c->v)
        return -1;

    n = get_node(c->v);
    r = get_router(n->parent);

    if (!r)
        return -1;

    switch (n->type) {
        case ROUTER_NODE:
            rm_router(r, c);
            return add_router(p, c);

        case LEAF_NODE:
            rm_leaf(r, c);
            return add_leaf(p, c);

        default:
            printf("Invalid node type\n");
            return -1;
    }
}

struct router *insert(struct sockaddr_in6 *sa, struct child *c)
{
    struct router *p, *gp;
    struct node *n;

    if (!c || !c->v)
        return NULL;

    n = get_node(c->v);
    gp = get_router(n->parent);
    p = create_router(sa, gp);

    return adopt(p, c) ? NULL : p;
}

struct child *get_path_pos(struct leaf *l, uint8_t h)
{
    struct router *p;
    struct child *c;
    struct node *n;

    if (!l)
        return NULL;

    p = get_router(l->node.parent);
    n = &l->node;

    while (p) {
        if (p->hops <= h)
            break;
        n = &p->node;
        p = get_router(n->parent);
    }

    c = n->type == ROUTER_NODE ? p->child : p->leaf;
    c = get_child(n, c);
    return c;
}


/*
 * Grouping
 */

void reduce_tree(struct router *r)
{
    struct child *i, *j;
    struct router *n;

    if (!r)
        return;

    for (j = r->child; j;) {
        /* save j->next pointer at the beginning, cutting router
         * in the next recursion invalidates the next pointer. */
        i = j;
        j = j->n;

        n = get_router(i->v);
        reduce_tree(n);
    }

    if (!r->node.parent)
        return;

    if (r->nchild < MIN_NUM_ROUTER && r->nleaf < MIN_NUM_LEAF) {
        printf("Removing node ");
        print_ia(&r->sa.sin6_addr);
        printf("\n");
        cut_router(r);
    }
}

struct router *get_start(struct router *r)
{
    struct child *c;
    struct router *s = NULL;

    if (!r)
        return s;

    c = get_free_router(r);

    if (r->node.parent && r->fleaf > 0)
        s = r;

    while (c) {
        r = get_router(c->v);
        c = get_free_router(r);

        if (r->fleaf > 0)
            s = r;
    }

    if (!s) {
        r = get_router(r->node.parent);
        if (r)
            return get_start(r);
        printf("No start left!\n");
    } else {
        printf("Start Router: ");
        print_ia(&r->sa.sin6_addr);
        printf("\n");
    }

    return s;
}

struct router *back_propagate(struct router *r)
{
    struct router *p;

    if (!r || !r->node.parent || r->fleaf > 0 || r->fchild > 0)
        return NULL;

    p = get_router(r->node.parent);
    if (p->fchild > 0)
        p->fchild--;

    return p;
}

void rec_back_propagate(struct router *r)
{
    while (r)
        r = back_propagate(r);
}

/* Sets router bit and copies router addr to address list.
 * If `MERGE_NODES` is enabled routers will be merged under router with
 * shortest distance to root.
 * Returns start index offset for copying leafs to address list.
 * The offset is 0 if nodes should be merged and otherwise 1, to skip router
 * address. */
size_t set_txg_router(struct in6_addr *l, uint32_t *bm, struct router *r,
                      size_t n)
{
    size_t i, off;
    struct child *c;

    i = 0;
    off = 0;
    if (!MERGE_NODES) {
        off = n;
        i = 1;
        goto bitmap;
    }

    if (!closest) {
        closest = r;
        i = 1;
        goto bitmap;
    }

    if (closest->hops > r->hops)
        closest = r;

    goto copy;

bitmap:
    /* set bitmap starting from msb. */
    (*bm) |= 1 << (sizeof(*bm) * 8 - 1 - off);
copy:
    memcpy(&l[off], &r->sa.sin6_addr, sizeof(r->sa.sin6_addr));

    return i;
}

void add2txg(struct in6_addr *l, uint32_t *bm, struct router *r,
             size_t *n, size_t m)
{
    size_t i, s;
    struct child *c;
    struct node *v;
    struct rcvr *rcvr;

    s = set_txg_router(l, bm, r, *n);
    c = get_free_leaf(r);
    for (i = s; i < m + s; i++) {
        v = c->v;
        c = c->n;
        rcvr = &get_leaf(v)->val;
        memcpy(&l[*n + i], &rcvr->addr.sa.sin6_addr, sizeof(struct in6_addr));
    }

    (*n) += m + s;
    r->fleaf -= m;
}

void print_grp(struct in6_addr *l, size_t n, uint32_t bm)
{
    size_t i;

    printf("[");
    for (i = 0; i < n; i++) {
        print_ia(&l[i]);
        printf(",");
    }

    printf("]\t%u\n", bm);
}

void finish_txg(struct child **grp, struct in6_addr *addrs,
                size_t *len, uint32_t *bm)
{
    struct child *c;

    c = malloc(sizeof(struct child));
    set_data_hdr(&c->v, addrs, *len, *bm);
    c->n = *grp ? *grp : NULL;
    *grp = c;

    print_grp(&addrs[0], *len, *bm);
    *len = 0;
    *bm = 0;
    closest = NULL;
}

struct tx_group *greedy_grouping(struct router *s)
{
    size_t i, n, m, p, max;
    struct router *r;
    struct child *c, *mdc;
    struct tx_group *grp;
    struct rcvr *rcvr;
    struct addr *addr;
    uint32_t bm;

    max = MAX_NUM_ADDR < BITMAP_LIMIT ? MAX_NUM_ADDR : BITMAP_LIMIT;
    struct in6_addr addrs[max];

    if (!s)
        return NULL;
    
    mdc = NULL;
    bm = 0;
    n  = 0;
    m  = 0;

    r = get_start(s);
    while (r) {
start:
        while (r->fleaf > 0) {
            /* if nodes can be merged, don't consider router address. */
            p = closest ? 0 : 1;
            m = n + r->fleaf + p;

            if (m > max) {
                if (SPLIT_NODES) {
                    m = max - n - p;
                    add2txg(&addrs[0], &bm, r, &n, m);
                }
                goto next;
            }

            if (m >= max - 1 || m >= OK_NUM_ADDR) {
                add2txg(&addrs[0], &bm, r, &n, r->fleaf);
                rec_back_propagate(r);
                goto next;
            }

            if (m < max) {
                add2txg(&addrs[0], &bm, r, &n, r->fleaf);
                break;
            }
        }

        while (r->fchild > 0) {
            c = get_free_router(r);
            if (c) {
                r = get_router(c->v);
                goto start;
            }
        }

        if (r->node.parent) {
            back_propagate(r);
            r = get_router(r->node.parent);

            /* if parent is root finish group */
            if (!r->node.parent)
                goto next;

            goto start;
        }
next:
        finish_txg(&mdc, addrs, &n, &bm);
        r = get_start(r);
    }

    /* Add remaining leafs. */
    grp = malloc(sizeof(struct tx_group) + s->fleaf * sizeof(struct addr));
    grp->mdc = mdc;
    grp->nuni = s->fleaf;
    addr = grp->uni;

    for (i = 0; i < grp->nuni; i++, addr++) {
        c = get_free_leaf(s);
        rcvr = &get_leaf(c->v)->val;
        memcpy(addr, &rcvr->addr, sizeof(*addr));
    }

    /* TODO, if a group has less than X entries, break up to unicast. */

    return grp;
}

void rec_reset_tree(struct router *s, struct router *r)
{
    struct child *i, *j;
    struct router *p;

    for (i = r->child; i; i = j) {
        j = i->n;
        p = get_router(i->v);
        rec_reset_tree(s, p);
    }

    p = get_router(r->node.parent);
    if (!p)
        return;

    for (i = r->leaf; i; i = j) {
        j = i->n;
        adopt(s, i);
    }
    r->nleaf  = 0;
    r->fleaf  = 0;
    r->nchild = 0;
    r->fchild = 0;
    r->hops   = 0;

    i = rm_nrouter(p, r);
    free(i);

}
