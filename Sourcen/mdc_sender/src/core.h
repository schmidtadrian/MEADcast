#ifndef CORE_H
#define CORE_H

#include <bits/types/struct_itimerspec.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <stddef.h>
#include <stdint.h>

#define IPV6_MEADCAST 253

struct ip6_mdc_hdr {
    struct ip6_rthdr rthdr;
    union {
        struct {
            uint32_t
                dsts: 8,
                dcv: 1,
                rsp: 1,
                hops: 6,
                res: 16;
        };
        uint32_t flags;
    };
    uint32_t dlvm;
    uint32_t rtm;
    struct in6_addr addr[];
} __attribute__((packed));

size_t get_mdc_pkt_size(size_t num_dst);
int start(char *tif, char *bif, struct in6_addr *taddr, struct in6_addr *baddr,
          uint16_t bport, size_t mtu, struct itimerspec *dcvr_int,
          struct itimerspec *dcvr_tout);

#endif // !CORE_H
