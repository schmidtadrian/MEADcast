#ifndef ARGP_H
#define ARGP_H

#include <argp.h>
#include <bits/types/struct_itimerspec.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

struct paddr {
    struct in6_addr v;
    struct paddr *n;
};

struct arguments {

    // network
    char *tifname;
    char *bifname;
    struct in6_addr taddr;
    struct in6_addr baddr;
    uint16_t bport;
    uint16_t pport;
    struct paddr *paddr;

    // discovery
    struct itimerspec dcvr_int;
    struct itimerspec dcvr_tout;

    // grouping
    size_t max_addrs;
    size_t ok_addrs;
    size_t min_leafs;
    size_t min_routers;
    bool split_nodes;
    size_t merge_range;

    // verbosity
    bool print_tree;
    bool print_txg;
};

void init_args(struct arguments *args, int argc, char *argv[]);
void print_grouping_args(struct arguments *args);

enum opt_grp {
    OPT_GRP_NET     = 1,
    OPT_GRP_DCVR    = 2,
    OPT_GRP_GRP     = 3,
    OPT_GRP_VERBOSE = 4
};

enum sopts {

    // network
    OPT_TIFNAME     = 't',
    OPT_BIFNAME     = 'i',
    /* Use non-ascii chars (x > 0x80) for options without short form */
    OPT_TADDR       = 0x100,
    OPT_BADDR       = 'a',
    OPT_BPORT       = 'p',

    // discovery
    OPT_DCVR_INT    = 'I',
    OPT_DCVR_TOUT   = 'T',
    OPT_DCVR_DELAY  = 'd',

    // grouping
    OPT_MAX_ADDRS   = 0x200,
    OPT_OK_ADDRS    = 0x300,
    OPT_MIN_LEAFS   = 0x400,
    OPT_MIN_ROUTERS = 0x500,
    OPT_SPLIT_NODES = 's',
    OPT_MERGE_RANGE = 'm',

    // verbosity
    OPT_PRINT_TREE  = 0x600,  // print topology tree
    OPT_PRINT_TXG   = 0x700   // print final MEADcast groups
};


extern struct arguments args;

#endif /* ARGP_H */
