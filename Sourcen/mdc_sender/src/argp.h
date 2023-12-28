#ifndef ARGP_H
#define ARGP_H

#include <argp.h>
#include <bits/types/struct_itimerspec.h>
#include <netinet/in.h>
#include <stdint.h>

struct paddr {
    struct in6_addr v;
    struct paddr *n;
};

struct arguments {
    char *tifname;
    char *bifname;
    struct in6_addr taddr;
    struct in6_addr baddr;
    uint16_t bport;
    uint16_t pport;
    struct paddr *paddr;
    struct itimerspec dcvr_int;
    struct itimerspec dcvr_tout;
};

void get_args(struct arguments *args, int argc, char *argv[]);

#endif /* ARGP_H */
