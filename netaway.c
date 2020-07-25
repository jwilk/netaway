/* Copyright © 2017 Jakub Wilk <jwilk@jwilk.net>
 * SPDX-License-Identifier: MIT
 */

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void show_usage(FILE *fp)
{
    fprintf(fp,
        "Usage:\n"
        "  netaway COMMAND [ARG...]\n"
        "  netaway -s\n"
        "\n"
        "Options:\n"
        "  -s  run the shell\n"
        "  -h  display this help and exit\n"
    );
}

static void bad_usage()
{
   show_usage(stderr);
   exit(EXIT_FAILURE);
}

void set_if_up(const char *ifname)
{
    struct ifreq ifreq;
    strcpy(ifreq.ifr_name, ifname);
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("netaway: socket()");
        exit(EXIT_FAILURE);
    }
    int rc = ioctl(fd, SIOCGIFFLAGS, &ifreq);
    if (rc < 0) {
        perror("netaway: ioctl(..., SIOCGIFFLAGS, ...)");
        exit(EXIT_FAILURE);
    }
    ifreq.ifr_flags |= IFF_UP;
    rc = ioctl(fd, SIOCSIFFLAGS, &ifreq);
    if (rc < 0) {
        perror("netaway: ioctl(..., SIOCSIFFLAGS, ...)");
        exit(EXIT_FAILURE);
    }
    rc = close(fd);
    if (rc < 0) {
        perror("netaway: close()");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    int opt;
    bool opt_shell = false;
    while ((opt = getopt(argc, argv, "+sh")) != -1)
        switch (opt) {
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
    int rc = unshare(CLONE_NEWNET);
    if (rc < 0) {
        if (errno == EPERM)
            fprintf(stderr, "netaway: CAP_SYS_ADMIN capability is required\n");
        else
            perror("netaway: unshare()");
        exit(EXIT_FAILURE);
    }
    set_if_up("lo");
    execvp(argv[0], argv);
    fprintf(stderr, "netaway: ");
    perror(argv[0]);
    exit(EXIT_FAILURE);
}

/* vim:set ts=4 sts=4 sw=4 et:*/
