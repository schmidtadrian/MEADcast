#ifndef UTIL_H
#define UTIL_H

#include "core.h"
#include "group.h"
#include "tree.h"
#include <netinet/in.h>
#include <stdint.h>

void print_ia(struct in6_addr *addr);
int init_sa(struct sockaddr_in6 *sa, char *addr, uint16_t port);
void print_mdc_hdr(struct ip6_mdc_hdr *hdr);

/* Print tx grouping of `txg` */
void print_txg(struct tx_group *txg);

/* Recursively print node & childs (depth first). */
void df_print(struct router *n, size_t lvl);

/* Print tree, starting at `r`. */
static inline void print_tree(struct router *r)
{
    df_print(r, 0);
}

#endif // !UTIL_H
