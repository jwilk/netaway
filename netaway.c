/* Copyright © 2017-2023 Jakub Wilk <jwilk@jwilk.net>
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

static void show_usage(FILE *fp)
{
    fprintf(fp,
        "Usage:\n"
        "  netaway [-u] COMMAND [ARG...]\n"
        "  netaway [-u] -s\n"
        "\n"
        "Options:\n"
        "  -u  create new user namespace\n"
        "  -s  run the shell\n"
        "  -h  show this help message and exit\n"
    );
}

static void bad_usage()
{
    show_usage(stderr);
    exit(EXIT_FAILURE);
}

static void fatal(const char *context)
{
    int orig_errno = errno;
    fprintf(stderr, "netaway: ");
    errno = orig_errno;
    perror(context);
    exit(EXIT_FAILURE);
}

static void set_if_up(const char *ifname)
{
    struct ifreq ifreq;
    strcpy(ifreq.ifr_name, ifname);
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
        fatal("socket()");
    int rc = ioctl(fd, SIOCGIFFLAGS, &ifreq);
    if (rc < 0)
        fatal("ioctl(..., SIOCGIFFLAGS, ...)");
    ifreq.ifr_flags |= IFF_UP;
    rc = ioctl(fd, SIOCSIFFLAGS, &ifreq);
    if (rc < 0)
        fatal("ioctl(..., SIOCSIFFLAGS, ...)");
    rc = close(fd);
    if (rc < 0)
        fatal("close()");
}

static void pprintf(const char *path, const char *fmt, ...)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
        fatal(path);
    va_list ap;
    va_start(ap, fmt);
    int rc = vdprintf(fd, fmt, ap);
    if (rc < 0)
        fatal(path);
    va_end(ap);
    rc = close(fd);
    if (rc < 0)
        fatal(path);
}

int main(int argc, char **argv)
{
    int opt;
    bool opt_shell = false;
    int unshare_flags = CLONE_NEWNET;
    while ((opt = getopt(argc, argv, "+ush")) != -1)
        switch (opt) {
        case 'u':
            unshare_flags |= CLONE_NEWUSER;
            break;
        case 's':
            opt_shell = true;
            break;
        case 'h':
            show_usage(stdout);
            exit(EXIT_SUCCESS);
        default:
            bad_usage();
        }
    assert(optind <= argc);
    argc -= optind;
    argv += optind;
    if (opt_shell) {
        if (argc != 0)
            bad_usage();
        static char * shell_argv[2] = {NULL, NULL};
        argv = shell_argv;
        argv[0] = getenv("SHELL");
        if (argv[0] == NULL)
            argv[0] = "sh";
    } else if (argc == 0)
        bad_usage();
    int uid = geteuid();
    int gid = getegid();
    int rc = unshare(unshare_flags);
    if (rc < 0) {
        if (errno == EPERM)
            fprintf(stderr, "netaway: CAP_SYS_ADMIN capability is required\n");
        else
            fatal("unshare()");
        exit(EXIT_FAILURE);
    }
    if (unshare_flags & CLONE_NEWUSER) {
        pprintf("/proc/self/uid_map", "%d %d 1", uid, uid);
        pprintf("/proc/self/setgroups", "deny");
        pprintf("/proc/self/gid_map", "%d %d 1", gid, gid);
    }
    set_if_up("lo");
    execvp(argv[0], argv);
    fatal(argv[0]);
}

/* vim:set ts=4 sts=4 sw=4 et:*/
