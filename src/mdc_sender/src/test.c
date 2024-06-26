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
    struct router *root;
    struct tx_group *grp;
    // create_topo(&root);
    create_testbed(&root);

    reduce_tree(root);
    print_tree(root);
    grp = greedy_grouping(root);
    print_txg(grp);
    rec_reset_tree(root, root);
    exit(EXIT_SUCCESS);
}

void create_topo(struct router **root)
{
    int i, nr, ne;
    struct router *p;

    char n[INET6_ADDRSTRLEN];
    char *rt_pref = "a::";
    char *ep_pref = "f::";

    nr = 12;
    ne = 28;
    
    struct sockaddr_in6 ssa;
    struct sockaddr_in6 rsa[nr + 1];
    struct sockaddr_in6 lsa[ne + 1];
    struct router *r[nr + 1];

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
    }

    for (i = 1; i <= ne; i++) {
        lsa[i].sin6_family = AF_INET6;
        lsa[i].sin6_port = 0;
        snprintf(n, INET6_ADDRSTRLEN, "%s%d", rt_pref, i);
        if (inet_pton(AF_INET6, n, &lsa[i].sin6_addr) != 1) {
            perror("inet_pton (ep)");
            exit(EXIT_FAILURE);
        }
    }

    // root
    *root = create_router(&ssa, NULL);
    r[1] = create_router(&rsa[1], *root);
    r[2] = create_router(&rsa[2], *root);

    // r1
    r[3] = create_router(&rsa[3], r[1]);
    r[4] = create_router(&rsa[4], r[1]);

    create_leaf(&lsa[1], 0, r[1]);
    create_leaf(&lsa[2], 0, r[1]);
    create_leaf(&lsa[3], 0, r[1]);

    
    // r2
    r[5] = create_router(&rsa[5], r[2]);
    r[6] = create_router(&rsa[6], r[2]);

    create_leaf(&lsa[4], 0, r[2]);
    create_leaf(&lsa[5], 0, r[2]);


    // r3
    r[7] = create_router(&rsa[7], r[3]);

    create_leaf(&lsa[6], 0, r[3]);
    create_leaf(&lsa[7], 0, r[3]);
    

    // r4
    r[8] = create_router(&rsa[8], r[4]);
    r[9] = create_router(&rsa[9], r[4]);

    create_leaf(&lsa[8], 0, r[4]);
    create_leaf(&lsa[9], 0, r[4]);
    create_leaf(&lsa[10], 0, r[4]);
    create_leaf(&lsa[11], 0, r[4]);


    // r5
    create_leaf(&lsa[12], 0, r[5]);
    create_leaf(&lsa[13], 0, r[5]);


    // r6
    r[10] = create_router(&rsa[10], r[6]);

    create_leaf(&lsa[14], 0, r[6]);


    // r7
    create_leaf(&lsa[15], 0, r[7]);


    // r8
    r[11] = create_router(&rsa[11], r[8]);

    create_leaf(&lsa[16], 0, r[8]);
    create_leaf(&lsa[17], 0, r[8]);


    // r9
    r[12] = create_router(&rsa[12], r[9]);

    create_leaf(&lsa[18], 0, r[9]);
    create_leaf(&lsa[19], 0, r[9]);


    // r10
    create_leaf(&lsa[20], 0, r[10]);
    create_leaf(&lsa[21], 0, r[10]);
    create_leaf(&lsa[22], 0, r[10]);


    // r11
    create_leaf(&lsa[23], 0, r[11]);
    create_leaf(&lsa[24], 0, r[11]);
    create_leaf(&lsa[25], 0, r[11]);
    create_leaf(&lsa[26], 0, r[11]);


    // r12
    create_leaf(&lsa[27], 0, r[12]);
    create_leaf(&lsa[28], 0, r[12]);

    for (i = 1; i <= nr; i++) {
        p = get_router(r[i]->node.parent);
        r[i]->hops = p->hops + 1;
    }
}

/* Creates file transfer experiment. */
void create_testbed(struct router **root)
{
    int i, nr, ne;
    struct router *p;

    char n[INET6_ADDRSTRLEN];
    char *rt_pref = "a::";
    char *ep_pref = "f::";

    nr = 13;
    ne = 9;

    struct sockaddr_in6 ssa;
    struct sockaddr_in6 rsa[nr + 1];
    struct sockaddr_in6 lsa[ne + 1];
    struct router *r[nr + 1];

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
    }

    for (i = 1; i <= ne; i++) {
        lsa[i].sin6_family = AF_INET6;
        lsa[i].sin6_port = 0;
        snprintf(n, INET6_ADDRSTRLEN, "%s%d", rt_pref, i);
        if (inet_pton(AF_INET6, n, &lsa[i].sin6_addr) != 1) {
            perror("inet_pton (ep)");
            exit(EXIT_FAILURE);
        }
    }

    *root = create_router(&ssa, NULL);      // c4001
    r[1] = create_router(&rsa[1], *root);   // r400
    r[2] = create_router(&rsa[2], r[1]);    // r40
    r[3] = create_router(&rsa[3], r[2]);    // r4
    r[4] = create_router(&rsa[4], r[3]);    // r3

    // r3
    r[5] = create_router(&rsa[5], r[4]);    // r30
    r[6] = create_router(&rsa[6], r[4]);    // r31
    r[7] = create_router(&rsa[7], r[4]);    // r32

    // r30
    r[8] = create_router(&rsa[8], r[5]);    // r300
    r[9] = create_router(&rsa[9], r[5]);    // r301

    // r31
    r[10] = create_router(&rsa[10], r[6]);    // r310
    r[11] = create_router(&rsa[11], r[6]);    // r311

    // r32
    r[12] = create_router(&rsa[12], r[7]);    // r320
    r[13] = create_router(&rsa[13], r[7]);    // r321

    // r300
    create_leaf(&lsa[1], 0, r[8]);   // c3001
    create_leaf(&lsa[2], 0, r[8]);   // c3101

    // r301
    create_leaf(&lsa[3], 0, r[9]);   // c3201

    // r310
    create_leaf(&lsa[4], 0, r[10]);   // c3301
    create_leaf(&lsa[5], 0, r[10]);   // c3401

    // r311
    create_leaf(&lsa[6], 0, r[11]);   // c3501
    create_leaf(&lsa[7], 0, r[11]);   // c3601

    // r320
    create_leaf(&lsa[8], 0, r[12]);   // c3701

    // r321
    create_leaf(&lsa[9], 0, r[13]);   // c3801

    for (i = 1; i <= nr; i++) {
        p = get_router(r[i]->node.parent);
        r[i]->hops = p->hops + 1;
    }
}
