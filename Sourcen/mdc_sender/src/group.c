#include "group.h"
#include "list.h"
#include "tree.h"
#include "util.h"
#include <arpa/inet.h>
#include <Judy.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>


Word_t *_join(struct sockaddr_in6 *sa)
{
    Word_t *pv;

    JHSI(pv, ht, &sa->sin6_addr, sizeof(sa->sin6_addr));

    if (pv == PJERR) {
        printf("No mem left\n");
        return NULL;
    }

    if (*pv != 0) {
        print_ia(&sa->sin6_addr);
        printf(" already in group member\n");
        return NULL;
    }

    return pv;
}

/* Create MEADcast group. */
int init_group(struct in6_addr *addr, uint16_t port)
{
    int ret;
    Word_t *pv;
    struct sockaddr_in6 sa;

    if (root || ht)
        return -1;

    sa.sin6_family = AF_INET6;
    sa.sin6_addr = *addr;
    sa.sin6_port = port;

    pv = _join(&sa);
    if (!pv) {
        printf("ERROR\n");
        return -1;
    }

    root = create_router(&sa, NULL); 
    *pv = (Word_t) root;

    return 0;
}

/* Add receiver to MEADcast group. */
int join(struct in6_addr *addr, uint16_t port)
{
    Word_t *pv;
    struct sockaddr_in6 sa;
    struct leaf *l;

    if (!root || !ht)
        return -1;

    sa.sin6_family = AF_INET6;
    sa.sin6_addr = *addr;
    sa.sin6_port = 0;

    pv = _join(&sa);
    if (!pv)
        return -1;

    l = create_leaf(&sa, port, root);
    insert_head(&rcvr, &l->val, &nrcvr);
    *pv = (Word_t) l;

    return 0;
}

struct router *get_root(void)
{
    return root;
}

struct tx_group *get_txg(void)
{
    return atomic_load(&txg);
}

void set_txg(struct tx_group **v)
{
    atomic_store(&txg, *v);
}

int init_txg(void)
{
    size_t i;
    struct rcvr *r;
    struct tx_group *grp;

    if (!rcvr)
        return -1;

    r = rcvr;
    grp = malloc(sizeof(*grp) + nrcvr * sizeof(struct addr));
    grp->nuni = nrcvr;

    for (i = 0; r; r = r->next, i++)
        grp->uni[i] = r->addr;

    set_txg(&grp);
    return 0;
}

struct rcvr *get_rcvr(void)
{
    return rcvr;
}

Pvoid_t *get_ht(void)
{
    return &ht;
}
