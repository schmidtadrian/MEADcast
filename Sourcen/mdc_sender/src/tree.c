#include "tree.h"
#include "argp.h"
#include "group.h"
#include "list.h"
#include "tx.h"
#include "util.h"
#include <Judy.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/*
 * Required for node merging
 */

/* Points to closest router in current group. */
static struct router *closest = NULL;
/* Stores the index of the closest router in the address list. */
static size_t ic = -1;
/* Stores distance of first router in current group. */
static int init_hops = 0;


struct child *get_child(void *n, struct child *l)
{
    if (!n || !l)
        return NULL;

    for (; l; l = l->n)
        if (l->v == n)
            return l;

    return NULL;
}

struct leaf *create_leaf(struct sockaddr_in6 *addr, uint16_t port,
                         struct router *p)
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
    Pvoid_t *ht;
    int ret;
    struct router *p;
    struct child *c;

    if (!r)
        return -1;

    p = get_router(r->node.parent);
    c = get_child(&r->node, p->child);
    c = rm_router(p, r);

    *childs = r->child;
    *nchild = r->nchild;
    *leafs = r->leaf;
    *nleaf = r->nleaf;

    ht = get_ht();
    JHSD(ret, *ht, &r->sa.sin6_addr, sizeof(r->sa.sin6_addr));

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
            rm_router(r, get_router(n));
            return add_router(p, c);

        case LEAF_NODE:
            rm_leaf(r, get_leaf(n));
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
 *
 * `set_txg_router, `add2txg` and `finish_txg` are tightly coupled to
 * `greedy_grouping` thus not designed for separate usage.
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

    if (r->nchild < args.min_routers && r->nleaf < args.min_leafs) {
        // printf("Removing router ");
        // print_ia(&r->sa.sin6_addr);
        // printf("\n");
        cut_router(r);
    }
}

/* Returns router to start grouping at with at least one free child.
 * Searches in the following sequence, stopping at the first match:
 *   1) Free child routers of `r` (depth first)
 *   2) `r` - the input router
 *   3) Sets `r` to its parent and repeats the process */
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
        // printf("No start left!\n");
    }
    /* else {
        printf("Start Router: ");
        print_ia(&r->sa.sin6_addr);
        printf("\n");
    } */

    return s;
}

/* Updates free leafs counter of `r's` parent. */
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

static inline void rec_back_propagate(struct router *r)
{
    while (r)
        r = back_propagate(r);
}

static inline void set_merging_stats(void *p, size_t i, int v)
{
    closest = p;
    ic = i;
    init_hops = v;
}

static inline bool is_mergeable(struct router *r)
{
    return closest &&
           abs(init_hops - (int) r->hops) <= args.merge_range &&
           abs((int) closest->hops - (int) r->hops) <= args.merge_range;
}

/* If we traverse the tree towards the root in the next iteration, it might be
 * beneficial to set `r`'s parent as the closest router.
 *
 * In other words: if the current router has no free leaf nodes or free child
 * nodes left and the parent router has at least one free child node (besides
 * `r`) or a free leaf node left, the leaf nodes should be merged under the
 * parent router. */
static inline struct router *should_use_parent_as_closest(struct router *r,
                                                          size_t m)
{
    struct router *pr;

    if (r && r->fleaf <= m && r->fchild < 1 &&
        (pr = get_router(r->node.parent)) &&
        (pr->fleaf > 0 || pr->fchild > 1) &&
        is_mergeable(pr)
    )
        return pr;

    return NULL;
}

/* Updates the router bitmap `bm` and the address list `l`.
 * Decides whether the leafs of `r` will be merged with those already present
 * in `l`. Staying within `MERGE_RANGE` must be ensured by the caller.
 *
 * Merge:
 * Returns an offset of 0, indicating that leafs will be merged.
 * If `r` is closer to the sender than `closest`, `closest` will be set to `r`
 * and the address of its predecessor at index `ic` in `l` will be overwritten.
 * If after adding `m` leafs of `r`, `r` has no free leafs or free childs left
 * the parent will be set as closest if it is within `MERGE_RANGE` (see:
 * `should_use_parent_as_closest`).
 *
 * No merge:
 * Returns an offset of 1, indicating that leafs will not be merged.
 * Sets the router bit in `bm` at index `n` and copies the address of `r` to
 * `l` at index `n`. */
size_t set_txg_router(struct in6_addr *l, uint32_t *bm, struct router *r,
                      size_t n, size_t m)
{
    size_t i;
    struct router *pr, *tmp;

    i = 0;

    if (!args.merge_range)
        goto bitmap;

    if (!closest) {
        set_merging_stats(r, n, r->hops);
        goto bitmap;
    }

    else if (closest->hops > r->hops) {
        closest = r;
        goto copy;
    }

    return i;

bitmap:
    i = 1;
    /* set bitmap starting from msb. */
    (*bm) |= 1 << (sizeof(*bm) * 8 - 1 - ic);
copy:
    pr  = should_use_parent_as_closest(r, m);
    tmp = pr ? pr : r;
    memcpy(&l[ic], &tmp->sa.sin6_addr, sizeof(tmp->sa.sin6_addr));

    return i;
}

/* Appends `m` free leafs of `r` to `l` at index `n`, and updates bitmap `bm`
 * accordingly. */
void add2txg(struct in6_addr *l, uint32_t *bm, struct router *r,
             size_t *n, size_t m)
{
    size_t i, s;
    struct child *c;
    struct node *v;
    struct rcvr *rcvr;

    s = set_txg_router(l, bm, r, *n, m);
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

/* Creates a MEADcast header for `addrs` of length `len`
 * and the router bitmap `bm`.
 * The newly created header gets prepended to `grp`.*/
void finish_txg(struct child **grp, struct in6_addr *addrs,
                size_t *len, uint32_t *bm)
{
    struct child *c;

    c = malloc(sizeof(struct child));
    set_data_hdr(&c->v, addrs, *len, *bm);
    c->n = *grp ? *grp : NULL;
    *grp = c;

    // print_grp(&addrs[0], *len, *bm);
    *len = 0;
    *bm  = 0;
    set_merging_stats(NULL, 0, 0);
}

struct tx_group *greedy_grouping(struct router *s)
{
    size_t i, n, m, p, max;
    struct router *r, *pr;
    struct child *c, *mdc;
    struct tx_group *grp;
    struct rcvr *rcvr;
    struct addr *addr;
    uint32_t bm;

    max = args.max_addrs < BITMAP_LIMIT ? args.max_addrs : BITMAP_LIMIT;
    struct in6_addr addrs[max];

    if (!s)
        return NULL;
    
    mdc = NULL;
    bm  = 0;
    n   = 0;
    m   = 0;
    r   = get_start(s);

    while (r) {

start:
        /* If nodes are mergeable, don't consider the router for the address
         * list space calculation (p = 0).
         * Otherwise, include the router address for the space calculation
         * (p = 1), and reset leaf merging parameters. */
        if ((p = !is_mergeable(r)))
            set_merging_stats(NULL, 0, 0);

        while (r->fleaf > 0) {

            m = n + r->fleaf + p;

            if (m > max) {
                if (args.split_nodes) {
                    m = max - n - p;
                    add2txg(&addrs[0], &bm, r, &n, m);
                }
                goto next;
            }

            if (m >= max - 1 || m >= args.ok_addrs) {
                add2txg(&addrs[0], &bm, r, &n, r->fleaf);
                rec_back_propagate(r);
                goto next;
            }

            if (m < max) {
                add2txg(&addrs[0], &bm, r, &n, r->fleaf);
                break;
            }
        }

        if (r->fchild > 0 && (c = get_free_router(r))) {
            r = get_router(c->v);
            goto start;
        }

        if ((pr = get_router(r->node.parent))) {

            /* Set parent as closest while going up, to avoid wrong mergage. */
            if (closest == r && should_use_parent_as_closest(r, n)) {
                    memcpy(&addrs[ic], &pr->sa.sin6_addr,
                           sizeof(pr->sa.sin6_addr));
                    closest = pr;
            }

            back_propagate(r);
            r = pr;

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

    i = rm_router(p, r);
    free(i);

}
