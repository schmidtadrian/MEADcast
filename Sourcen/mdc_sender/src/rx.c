#include "rx.h"
#include "checksum.h"
#include "core.h"
#include "group.h"
#include "list.h"
#include "tree.h"
#include "util.h"
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/in6.h>
#include <linux/if_tun.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void init_rx(size_t n)
{
    rxlen = n;
    rxbuf = malloc(rxlen);
}

int update_topo(struct sockaddr_in6 *rsa, struct in6_addr *paddr, uint8_t hops)
{
    Pvoid_t *ht;
    Word_t *pv;
    struct node *node;
    struct child *c, *cr;
    struct leaf *l;
    struct rcvr *rcvr;
    struct router *r, *gp;

    ht = get_ht();
    JHSG(pv, *ht, paddr, (sizeof(*paddr)));

    if (!pv) {
        printf("Peer not in group\n");
        return -1;
    }

    node = (struct node *) *pv;
    if (node->type != LEAF_NODE) {
        printf("Peer is no receiver\n");
        return -1;
    }

    l = (struct leaf *) node;
    rcvr = &l->val;
    // printf("Dst node type: %d\n", l->node.type);
    // printf("Leaf port: %d\n", rcvr->addr.port);

    JHSI(pv, *ht, &rsa->sin6_addr, sizeof(rsa->sin6_addr));

    if (pv == PJERR) {
        printf("No mem left\n");
        return -1;
    }

    /* Insert router */
    if (*pv == 0) {
        printf("Inserting router\n");
        c = get_path_pos(l, hops);
        r = insert(rsa, c);
        r->hops = hops;
        *pv = (Word_t) r;
    }
    
    /* Update router */
    else {
        printf("Updating router\n");
        r = (struct router *) *pv;
        if (r->node.type != ROUTER_NODE) {
            printf("Address is already a peer\n");
            return -1;
        }

        r->hops = hops;
        c = get_path_pos(l, hops);

        if (!r->node.parent && r->nleaf == 0 && r->nchild == 0) {
            node = c->v;
            cr =  malloc(sizeof(struct router));
            cr->v = r;
            add_router(get_router(node->parent), cr);
        }
        adopt(r, c);
    }

    while (r->node.parent)
        r = get_router(r->node.parent);
    print_tree(r);

    return 0;
}

int rx_disc(int fd)
{
    int n;
    struct sockaddr_in6 rt;
    struct ip6_mdc_hdr *hdr;
    socklen_t rtlen = sizeof(rt);


    char paddr[INET6_ADDRSTRLEN];
    char raddr[INET6_ADDRSTRLEN];
    
    n = recvfrom(fd, rxbuf, rxlen, 0, (struct sockaddr *) &rt, &rtlen);
    if (n < 1) {
        perror("recvfrom (discover)");
        return n;
    }

    /* Sanity checks */
    if (n != get_mdc_pkt_size(1))
        return -1;

    hdr = (struct ip6_mdc_hdr *) rxbuf;

    if (hdr->rthdr.ip6r_type != IPV6_MEADCAST ||
        hdr->rthdr.ip6r_nxt != IPPROTO_NONE ||
        !hdr->dcv || !hdr->rsp || hdr->hops < 1 || hdr->dsts != 1)
        return -1;

    if (inet_ntop(AF_INET6, &rt.sin6_addr, raddr, sizeof(raddr)) <= 0)
        return -1;

    if (inet_ntop(AF_INET6, &hdr->addr, paddr, sizeof(paddr)) <= 0)
        return -1;

    printf("%s (%d hops) is router for %s\n", raddr, hdr->hops, paddr);
    update_topo(&rt, hdr->addr, hdr->hops);

    return 0;
}

void rx_loop(struct rx_targs *args)
{
}

pthread_t start_rx(int tun_fd, int ip6_fd, int udp_fd, int tcp_fd,
                   struct addr *ta, size_t buflen)
{
    int ret;
    pthread_t tid;
    struct rx_targs *args;

    init_rx(buflen);

    args = malloc(sizeof(*args));
    args->tun_fd = tun_fd;
    args->ip6_fd = ip6_fd;
    args->udp_fd = udp_fd;
    args->tcp_fd = tcp_fd;
    args->ta = ta;

    ret = pthread_create(&tid, NULL, (void *) rx_loop, args);

    if (ret) {
        perror("pthread_create (rx)");
        return -1;
    }

    printf("Started rx thread\n");
    return tid;

}
