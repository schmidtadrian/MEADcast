#ifndef TREE_H
#define TREE_H

#include "group.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>


#ifndef LEAF_NODE
#define LEAF_NODE 0
#endif /* !LEAF_NODE */

#ifndef ROUTER_NODE
#define ROUTER_NODE 1
#endif /* !ROUTER_NODE */

#ifndef BITMAP_LIMIT
#define BITMAP_LIMIT 32
#endif /* !BITMAP_LIMIT */

#ifndef MAX_NUM_ADDR
#define MAX_NUM_ADDR 10
#endif /* ifndef MAX_NUM_ADDR */

#ifndef OK_NUM_ADDR
#define OK_NUM_ADDR 7
#endif /* ifndef OK_NUM_ADDR */


#ifndef MIN_NUM_LEAF
#define MIN_NUM_LEAF 2
#endif /* !MIN_NUM_LEAF */

#ifndef MIN_NUM_ROUTER
#define MIN_NUM_ROUTER 1
#endif /* !MIN_NUM_ROUTER */



struct child {
    void *v;
    struct child *n;
};

struct node {
    struct node *parent;
    uint8_t type;
};

struct leaf {
    struct node node;
    struct rcvr val;
};

struct router {
    struct node node;
    struct sockaddr_in6 sa;
    size_t hops;
    struct child *child;
    struct child *leaf;
    size_t nchild;
    size_t fchild;
    size_t nleaf;
    size_t fleaf;
};


/*
 *  Convenience functions
 */

/* Cast `n` to leaf. */
static inline struct leaf *get_leaf(struct node *n)
{
    return (struct leaf *) n;
}

/* Cast `n` to router. */
static inline struct router *get_router(struct node *n)
{
    return (struct router *) n;
}

static inline struct node *get_node(void *p)
{
    return (struct node *) p;
}

/* Set `p` as parent of `c`. */
static inline int set_parent(struct child *c, struct router *p)
{
    if (!c || !c->v)
        return EXIT_FAILURE;

    get_node(c->v)->parent = &p->node;
    return EXIT_SUCCESS;
}

struct child *rm_childl(void *v, struct child **l, size_t *n, size_t *f);
/*
 * Tree modification
 */

/* Insert a new leaf for `addr` with `port` below `p`.
 * Nothing happens if `p` is `NULL`. */
struct leaf *create_leaf(struct sockaddr_in6 *addr, uint16_t port,
                         struct router *p);

/* Insert a new router for `addr` below `p`.
 * If `p` is NULL, the new root is the root. */
struct router *create_router(struct sockaddr_in6 *addr, struct router *p);

/* Removes `c` from its current parent add adds it to `p`.
 * This moves `c` with all its childs & leafs. */
int adopt(struct router *p, struct child *c);

/* Insert a router node vor `sa` above `c` and adopt `c`. */
struct router *insert(struct sockaddr_in6 *sa, struct child *c);

/* Returns the child to insert a router with `h` hops above.
 * The child should be used as input for `adopt` or `insert`. */
struct child *get_path_pos(struct leaf *l, uint8_t h);


/*
 * Grouping
 */

/* Remove unnecessary routers from tree */
void reduce_tree(struct router *r);

/* Greedily groups leafs into MEADcast packets. */
struct tx_group *greedy_grouping(struct router *s);
// void greedy_grouping(struct router *s);

void reset_tree(struct router *s);
void rec_reset_tree(struct router *s, struct router *r);
#endif // !TREE_H
