#include "discover.h"
#include "group.h"
#include "rx.h"
#include "tree.h"
#include "tx.h"
#include "util.h"
#include <bits/types/struct_itimerspec.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

int init_epoll(int mdc_fd, int tout_fd, int tint_fd)
{
    int fd, ret;
    size_t n;
    struct epoll_event ev;

    n = 3;
    int fds[3] = { mdc_fd, tout_fd, tint_fd };

    if (mdc_fd < 1 || tout_fd < 1 || tint_fd < 1)
        return -1;

    ev.events = EPOLLIN;
    fd = epoll_create(n);

    if (fd < 0) {
        perror("epoll_create");
        return fd;
    }

    for (int i = 0; i < n; i++) {
        ev.data.fd = fds[i];
        ret = epoll_ctl(fd, EPOLL_CTL_ADD, fds[i], &ev);

        if (ret < 0) {
            perror("epoll_ctl");
            return ret;
        }
    }

    return fd;
}

int init_timerfd(struct timespec *ts, long tval_s, long tval_ns,
                 long tint_s, long tint_ns)
{
    int fd, ret;
    struct itimerspec tspec;

    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd < 0) {
        perror("timerfd_create");
        return fd;
    }
    
    tspec.it_value.tv_sec = tval_s;
    tspec.it_value.tv_nsec = tval_ns;
    tspec.it_interval.tv_sec = tint_s;
    tspec.it_interval.tv_nsec = tint_ns;

    ts->tv_sec = tint_s;
    ts->tv_nsec = tint_ns;

    ret = timerfd_settime(fd, 0, &tspec, NULL);
    if (ret < 0) {
        perror("timerfd_settime");
        return ret;
    }

    return fd;
}

int update_timer(int t, long sec, long nsec)
{
    int ret;
    struct itimerspec tspec;

    ret = timerfd_gettime(t, &tspec);
    if (ret < 0) {
        perror("timerfd_gettime");
        return ret;
    }

    tspec.it_value.tv_sec = sec;
    tspec.it_value.tv_nsec = nsec;

    ret = timerfd_settime(t, 0, &tspec, NULL);
    if (ret < 0) {
        perror("timerfd_settime");
        return ret;
    }

    return 0;
}
int flip_timers(int t1, int t2, struct timespec *ts)
{
    int ret;

    ret = update_timer(t1, 0, 0);
    if (ret < 0)
        return ret;

    return update_timer(t2, ts->tv_sec, ts->tv_nsec);
}

int tint_handler(int mdc_fd, int tint_fd, int tout_fd)
{
    int n;
    uint64_t buf;

    printf("Starting discovery phase\n");

    n = read(tint_fd, &buf, sizeof(buf));
    if (n < 1) {
        perror("read tint_fd");
        return n;
    }

    send_disc(mdc_fd);

    n = flip_timers(tint_fd, tout_fd, &tout_ts);
    if (n < 0)
        return n;

    return 0;
}

int tout_handler(int fd, int tint_fd)
{
    int n;
    uint64_t buf;
    struct tx_group *grp;
    struct router *r;

    printf("Ending discovery phase\n");

    n = read(fd, &buf, sizeof(buf));
    if (n < 1) {
        perror("End discovery");
        return -1;
    }

    // TODO clean old group
    r = get_root();
    reduce_tree(r);
    grp = greedy_grouping(r);
    if (!grp) {
        printf("Grouping failed\n");
        return -1;
    }

    set_txg(&grp);
    rec_reset_tree(r, r);

    n = flip_timers(fd, tint_fd, &tint_ts);
    if (n < 0)
        return n;

    return 0;    
}

int dx_loop(int epoll_fd, int mdc_fd, struct epoll_event *ev, size_t evlen)
{
    int n, i, fd;
    uint64_t buf;

    for (;;) {

        n = epoll_wait(epoll_fd, ev, evlen, -1);
        if (n < 0) {
            perror("epoll_wait");
            return n;
        }

        for (i = 0; i < n; i++) {
            fd = ev[i].data.fd;

            if (fd == mdc_fd) {
                /* TODO skip as long as we are not in discovery phase,
                 * and keep data in kernel buffer till next discovery phase.
                 * This can be done by setting EPOLLET. */
                rx_disc(mdc_fd);
            }
            else if (fd == tout_fd) {
                tout_handler(tout_fd, tint_fd);
                print_txg(get_txg());
                // exit(EXIT_SUCCESS);
            }
            else if (fd == tint_fd) {
                tint_handler(mdc_fd, tint_fd, tout_fd);
            }
        }
    }
}

int start_dx(struct dx_targs *args)
{
    int ret;
    int epoll_fd;
    struct epoll_event ev[args->evlen];

    tout_fd = init_timerfd(&tout_ts, 0, 0, args->tout, 0);
    if (tout_fd < 0)
        return tout_fd;

    tint_fd = init_timerfd(&tint_ts, 5, 1, args->tint, 0);
    if (tint_fd < 0)
        return tint_fd;

    epoll_fd = init_epoll(args->fd, tout_fd, tint_fd);
    if (epoll_fd < 1)
        return epoll_fd;



    ret = dx_loop(epoll_fd, args->fd, ev, args->evlen);
    if (ret < 0)
        return ret;

    return 0;
}
