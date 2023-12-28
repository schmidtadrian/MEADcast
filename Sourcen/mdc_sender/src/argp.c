#include <argp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include "argp.h"


#define DEF_TIFNAME "tun0"
#define DEF_BIFNAME NULL
#define DEF_TADDR "fd15::1"
#define DEF_BADDR "::"
#define DEF_BPORT 9999
#define DEF_PPORT 0
#define DEF_PADDR NULL

#define DEF_DCVR_TOUT_V_S  0
#define DEF_DCVR_TOUT_V_NS 0
#define DEF_DCVR_TOUT_I_S  2
#define DEF_DCVR_TOUT_I_NS 0

#define DEF_DCVR_INT_V_S   5
#define DEF_DCVR_INT_V_NS  0
#define DEF_DCVR_INT_I_S   5
#define DEF_DCVR_INT_I_NS  0


static char doc[] = "Exemplary MEADcast sender";
static char args_doc[] = "[PEER ADDRESSES] [PEER PORT]";

static struct argp_option options[] = {
    { "tun",         't', "ifname", 0, "Specify name of the created TUN device."},
    { "tun-address", 'A', "ipv6",   0, "Specify host route to send traffic to."},
    { "interface",   'i', "ifname", 0, "Specify interface to use."},
    { "address",     'a', "ipv6",   0, "Specify source address."},
    { "port",        'p', "port",   0, "Specify source port."},
    { "interval",    'I', "secs",   0, "Specify discovery interval."},
    { "delay",       'd', "secs",   0, "Specify delay until initial discovery phase."},
    { "timeout",     'T', "secs",   0, "Specify discovery timeout."},
    { 0 }
};

const char *help_info = "See --help for more information.";
const char *argp_program_version = "MEADcast sender v1";


void _parse_if(char **dst, char *arg, struct argp_state *state, char *info)
{
    if (strlen(arg) > IFNAMSIZ) {
        argp_failure(state, 1, 0, "Max %s ifname length is %d. %s",
                     info, IFNAMSIZ, help_info);
        exit(ARGP_KEY_ERROR);
    }
    *dst = arg;
}

void _parse_addr(struct in6_addr *addr, char *arg, struct argp_state *state,
                char *info)
{
    int ret;

    ret = inet_pton(AF_INET6, arg, addr);
    if (ret < 0) {
        argp_failure(state, 1, 0, "Invalid %s address family. %s",
                     info, help_info);
        exit(ARGP_KEY_ERROR);
    }

    if (ret < 1) {
        argp_failure(state, 1, 0, "Invalid %s address. %s",
                     info, help_info);
        exit(ARGP_KEY_ERROR);
    }
}

void _parse_port(uint16_t *port, char *arg, struct argp_state *state,
                char *info)
{
    if ((*port = atoi(arg)) > 0)
        return;

    argp_failure(state, 1, 0, "Invalid %s port: %s. %s", info, arg, help_info);
    exit(ARGP_KEY_ERROR);
}

void _parse_peer(struct paddr **head, char *arg, struct argp_state *state)
{
    int ret;
    struct paddr *new, *i;
    new = malloc(sizeof(*new));

    ret = inet_pton(AF_INET6, arg, &new->v);
    if (!ret) {
        argp_failure(state, 1, 0, "Invalid peer address at index %d: %s. %s",
                     state->arg_num, arg, help_info);
        exit(ARGP_KEY_ERROR);
    }

    if (!*head) {
        new->n = NULL;
        *head = new;
        return;
    }

    for (i = *head; i->n; i = i->n) {}
    i->n = new;
}

void null_check(void *arg, struct argp_state *state, char *info)
{
    if (arg)
        return;

    argp_failure(state, 1, 0, "%s is required. %s", info, help_info);
    exit(ARGP_KEY_ERROR);
}

void empty_check(char *arg, struct argp_state *state, char *info)
{
    if (arg && strlen(arg) <= IFNAMSIZ)
        return;

    argp_failure(state, 1, 0, "%s is required. %s", info, help_info);
    exit(ARGP_KEY_ERROR);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
        case 't':
            _parse_if((void *) &arguments->tifname, arg, state, "TUN");
            break;
        case 'i':
            _parse_if((void *) &arguments->bifname, arg, state, "bind");
            break;
        case 'a':
            _parse_addr(&arguments->baddr, arg, state, "bind");
            break;
        case 'A':
            _parse_addr(&arguments->taddr, arg, state, "tun");
            break;
        case 'p':
            _parse_port(&arguments->bport, arg, state, "bind");
            break;
        case 'I':
            arguments->dcvr_int.it_interval.tv_sec = atoi(arg);
            break;
        case 'T':
            arguments->dcvr_tout.it_interval.tv_sec = atoi(arg);
            break;
        case 'd':
            arguments->dcvr_int.it_value.tv_sec = atoi(arg);
            break;
        case ARGP_KEY_ARG:
            if (state->argc != state->next)
                _parse_peer(&arguments->paddr, arg, state);
            else
                _parse_port(&arguments->pport, arg, state, "peer");
            break;
        case ARGP_KEY_END:
            empty_check(arguments->tifname, state, "TUN interface");
            empty_check(arguments->bifname, state, "Bind interface");
            null_check(arguments->paddr, state, "At least one destination");
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

void print_args(struct arguments *args)
{
    char addr[INET6_ADDRSTRLEN];
    struct paddr *peer;

    if (!inet_ntop(AF_INET6, &args->baddr, addr, sizeof(addr))) {
        char *msg = "(Invalid bind addr)";
        strncpy(addr, msg, strlen(msg));
    }

    printf("TUN name is %s\n"
           "Bind to interface %s\n"
           "Bind addr: %s\n"
           "Bind port: %d\n"
           "Peer port: %d\n",
           args->tifname,
           args->bifname,
           addr,
           args->bport,
           args->pport);

    printf("Peers: [");
    for (peer = args->paddr; peer; peer = peer->n) {
        inet_ntop(AF_INET6, &peer->v, addr, sizeof(addr));
        printf("%s,", addr);
    }
    printf("]\n");
}


static struct argp argp = { options, parse_opt, args_doc, doc };

void set_default_args(struct arguments *args)
{
    args->tifname = malloc(IFNAMSIZ);
    strncpy(args->tifname, DEF_TIFNAME, sizeof(*args->tifname));
    inet_pton(AF_INET6, DEF_TADDR, &args->taddr);

    args->bifname = DEF_BIFNAME;
    args->bport = DEF_BPORT;
    args->pport = DEF_PPORT;
    args->paddr = DEF_PADDR;
    inet_pton(AF_INET6, DEF_BADDR, &args->baddr);

    args->dcvr_int.it_value.tv_sec     = DEF_DCVR_INT_V_S;
    args->dcvr_int.it_value.tv_nsec    = DEF_DCVR_INT_V_NS;
    args->dcvr_int.it_interval.tv_sec  = DEF_DCVR_INT_I_S;
    args->dcvr_int.it_interval.tv_nsec = DEF_DCVR_INT_I_NS;

    args->dcvr_tout.it_value.tv_sec     = DEF_DCVR_TOUT_V_S;
    args->dcvr_tout.it_value.tv_nsec    = DEF_DCVR_TOUT_V_NS;
    args->dcvr_tout.it_interval.tv_sec  = DEF_DCVR_TOUT_I_S;
    args->dcvr_tout.it_interval.tv_nsec = DEF_DCVR_TOUT_I_NS;
}

void get_args(struct arguments *args, int argc, char *argv[])
{
    set_default_args(args);
    argp_parse(&argp, argc, argv, 0, 0, args);
    // print_args(args);
}
