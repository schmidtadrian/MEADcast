#include "list.h"
#include "test.h"
#include "tree.h"
#include "util.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void test()
{
    struct router *root, *r;
    struct child *c;
    create_topo(&root);

    reduce_tree(root);
    print_tree(root);
    rec_reset_tree(root, root);
    print_tree(root);
    exit(EXIT_SUCCESS);
}

void create_topo(struct router **root)
{
    int i, nr, ne;
    uint16_t *t;

    char n[INET6_ADDRSTRLEN];
    char *rt_pref = "aa14::1";
    char *ep_pref = "bb15::1";

    nr = 12;
    ne = 28;
    
    struct sockaddr_in6 ssa;
    struct sockaddr_in6 rsa[nr + 1];
    struct sockaddr_in6 lsa[ne + 1];

    struct router *r[nr + 1];
    struct leaf *e[ne + 1];

    ssa.sin6_family = AF_INET6;
    ssa.sin6_port = 0;
    if (inet_pton(AF_INET6, "::", &ssa.sin6_addr) != 1) {
        perror("inet_pton (root)");
        exit(EXIT_FAILURE);
    }

    for (i = 1; i <= nr; i++) {
        rsa[i].sin6_family = AF_INET6;
        rsa[i].sin6_port = 0;
        snprintf(n, INET6_ADDRSTRLEN, "%s%d", ep_pref, i);
        if (inet_pton(AF_INET6, n, &rsa[i].sin6_addr) != 1) {
            perror("inet_pton (router)");
            exit(EXIT_FAILURE);
        }

        print_ia(&rsa[i].sin6_addr);
        printf("\n");
    }

    for (i = 1; i <= ne; i++) {
        lsa[i].sin6_family = AF_INET6;
        lsa[i].sin6_port = 0;
        snprintf(n, INET6_ADDRSTRLEN, "%s%d", rt_pref, i);
        if (inet_pton(AF_INET6, n, &lsa[i].sin6_addr) != 1) {
            perror("inet_pton (ep)");
            exit(EXIT_FAILURE);
        }

        print_ia(&lsa[i].sin6_addr);
        printf("\n");
    }

    // root
    *root = create_router(&ssa, NULL);
    r[1] = create_router(&lsa[15], *root);
    r[2] = create_router(&rsa[2], *root);

    // r1
    r[3] = create_router(&rsa[3], r[1]);
    r[4] = create_router(&rsa[4], r[1]);

    e[1] = create_leaf(&lsa[1], 0, r[1]);
    e[2] = create_leaf(&lsa[2], 0, r[1]);
    e[3] = create_leaf(&lsa[3], 0, r[1]);

    
    // r2
    r[5] = create_router(&rsa[5], r[2]);
    r[6] = create_router(&rsa[6], r[2]);

    e[4] = create_leaf(&lsa[4], 0, r[2]);
    e[5] = create_leaf(&lsa[5], 0, r[2]);


    // r3
    r[7] = create_router(&rsa[7], r[3]);

    e[6] = create_leaf(&lsa[6], 0, r[3]);
    e[7] = create_leaf(&lsa[7], 0, r[3]);
    

    // r4
    r[8] = create_router(&rsa[8], r[4]);
    r[9] = create_router(&rsa[9], r[4]);

    e[8] = create_leaf(&lsa[8], 0, r[4]);
    e[9] = create_leaf(&lsa[9], 0, r[4]);
    e[10] = create_leaf(&lsa[10], 0, r[4]);
    e[11] = create_leaf(&lsa[11], 0, r[4]);


    // r5
    e[12] = create_leaf(&lsa[12], 0, r[5]);
    e[13] = create_leaf(&lsa[13], 0, r[5]);


    // r6
    r[10] = create_router(&rsa[10], r[6]);

    e[14] = create_leaf(&lsa[14], 0, r[6]);


    // r7
    e[15] = create_leaf(&lsa[15], 0, r[7]);


    // r8
    r[11] = create_router(&rsa[11], r[8]);

    e[16] = create_leaf(&lsa[16], 0, r[8]);
    e[17] = create_leaf(&lsa[17], 0, r[8]);


    // r9
    r[12] = create_router(&rsa[12], r[9]);

    e[18] = create_leaf(&lsa[18], 0, r[9]);
    e[19] = create_leaf(&lsa[19], 0, r[9]);


    // r10
    e[20] = create_leaf(&lsa[20], 0, r[10]);
    e[21] = create_leaf(&lsa[21], 0, r[10]);
    e[22] = create_leaf(&lsa[22], 0, r[10]);


    // r11
    e[23] = create_leaf(&lsa[23], 0, r[11]);
    e[24] = create_leaf(&lsa[24], 0, r[11]);
    e[25] = create_leaf(&lsa[25], 0, r[11]);
    e[26] = create_leaf(&lsa[26], 0, r[11]);


    // r12
    e[27] = create_leaf(&lsa[27], 0, r[12]);
    e[28] = create_leaf(&lsa[28], 0, r[12]);

}
