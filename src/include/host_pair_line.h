/*
 * host_pair_line.h:
 *
 * Shared definition for the per-flow display structure used by
 * the TUI (ui.c), export mode (export.c), and tests.
 */

#ifndef __HOST_PAIR_LINE_H_
#define __HOST_PAIR_LINE_H_

#include "addr_hash.h"

#define HISTORY_DIVISIONS 3
#define HOSTNAME_LENGTH   256
#define PROCNAME_LENGTH   64

typedef struct host_pair_line_tag {
    addr_pair ap;
    unsigned long long total_recv;
    unsigned long long total_sent;
    double long recv[HISTORY_DIVISIONS];
    double long sent[HISTORY_DIVISIONS];
    char cached_hostname[HOSTNAME_LENGTH];
    /* Process attribution (populated when -Z is active) */
    int pid;                            /* 0 = unknown */
    char process_name[PROCNAME_LENGTH]; /* empty string = unknown */
} host_pair_line;

#endif /* __HOST_PAIR_LINE_H_ */
