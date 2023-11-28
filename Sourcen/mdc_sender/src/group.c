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

int init_group(char *addr, uint16_t port)
{
    int ret;
    Word_t *pv;
    struct sockaddr_in6 sa;

    if (root || ht)
        return -1;

    if (init_sa(&sa, addr, port) < 0)
        return -1;

    pv = _join(&sa);
    if (!pv) {
        printf("ERROR\n");
        return -1;
    }

    root = create_router(&sa, NULL); 
    *pv = (Word_t) root;

    return 0;
}

int join(char *addr, uint16_t port)
{
    Word_t *pv;
    struct sockaddr_in6 sa;
    struct leaf *l;

    if (!root || !ht)
        return -1;

    if (init_sa(&sa, addr, 0) < 0)
        return -1;

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
    // TODO make one liner
    struct  tx_group *ret;
    ret = atomic_load(&txg);
    return ret;
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
