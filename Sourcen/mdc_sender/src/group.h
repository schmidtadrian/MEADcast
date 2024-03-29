#ifndef GROUP_H
#define GROUP_H

#include <Judy.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdint.h>


/* Helper struct, to store port and sockaddr.
 * L3 sockets require port of sockaddr to be zero. */
struct addr {
    struct sockaddr_in6 sa;
    uint16_t port;
};

struct rcvr {
    struct addr addr;
    size_t hops;
    struct rcvr *next;
};

/* Only struct the sender needs to know.
 *
 * With each discovery the receiver creates a new instance.
 * TX & RX thread share an atomic pointer to the current group.
 * `mdc` is a list of ready to ship MEADcast packets.
 * `uni` is a list of addresses that can't be delivered via MEADcast */
struct tx_group {
    struct child  *mdc;
    size_t nuni;
    struct addr uni[];
};


int init_group(struct in6_addr *addr, uint16_t port);
int join(struct in6_addr *addr, uint16_t port);
struct router *get_root(void);
struct tx_group *get_txg(void);
void set_txg(struct tx_group **v);
struct rcvr *get_rcvr(void);
Pvoid_t *get_ht(void);
int init_txg(void);

#endif // !GROUP_H
