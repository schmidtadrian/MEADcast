#include "core.h"
#include "group.h"
#include "tree.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

void print_ia(struct in6_addr *addr)
{
    static char str[INET6_ADDRSTRLEN];

    if (!addr)
        printf("Invalid address");

    if (!inet_ntop(AF_INET6, addr, str, INET6_ADDRSTRLEN))
        perror("print_addr");

    printf("%s", str);
}

int init_sa(struct sockaddr_in6 *sa, char *addr, uint16_t port)
{
    int ret;

    sa->sin6_family = AF_INET6;
    sa->sin6_port = port;

    ret = inet_pton(AF_INET6, addr, &sa->sin6_addr);
    if (!ret) {
        printf("Invalid address: %s\n", addr);
        return -1;
    }
    if (ret < 0) {
        perror("inet_pton");
        return -1;
    }

    return 0;
}

void print_mdc_hdr(struct ip6_mdc_hdr *hdr)
{
    if (!hdr)
        return;

    printf("addrs: %u\n", hdr->dsts);
    printf("map: %u\n", hdr->rtm);
    printf("[");
    for (int i = 0; i < hdr->dsts; i++) {
        print_ia(&hdr->addr[i]);
        printf(",");
    }
    printf("]\n");
}

void print_txg(struct tx_group *txg)
{
    struct in6_addr *ia;
    struct child *c;
    struct ip6_mdc_hdr *h;
    struct addr *a;
    size_t i;

    if (!txg)
        return;

    printf("MEADcast groups:\n");
    for (c = txg->mdc; c; c = c->n) {
        printf("[");
        h = c->v;
        for (ia = h->addr, i = 0; i < h->dsts; ia++, i++) {
            print_ia(ia);
            printf(",");
        }
        printf("]\tbm: %u\n", h->rtm);
    }

    printf("Unicast:\n");
    for (i = 0, a = txg->uni; i < txg->nuni; i++, a++) {
        print_ia(&a->sa.sin6_addr);
        printf("/%d\n", a->port);
    }
}

static inline void print_lvl(size_t lvl)
{
    for (int i = 0; i < lvl; i++)
        printf("    ");
}

void df_print(struct router *n, size_t lvl)
{
    struct child *c;
    struct leaf *l;

    print_lvl(lvl);
    printf("└─ ");
    print_ia(&n->sa.sin6_addr);
    printf(" (%zu/%zu,%zu/%zu)\n", n->nleaf, n->nchild, n->fleaf, n->fchild);

    lvl++;
    for (c = n->leaf; c; c = c->n) {
        l = get_leaf(c->v);
        print_lvl(lvl);
        printf("└─ ");
        print_ia(&l->val.addr.sa.sin6_addr);
        printf("\n");
    }

    for (c = n->child; c; c = c->n)
        df_print(get_router(c->v), lvl);
}

