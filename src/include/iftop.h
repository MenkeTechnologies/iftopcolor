/*
 * iftop.h:
 *
 */

#ifndef __IFTOP_H_ /* include guard */
#define __IFTOP_H_

#include "conf.h"
#include "procinfo.h"

/* 40 / 1 */
#define HISTORY_LENGTH  40
#define RESOLUTION      1        /* history slot rotation interval (seconds) */
#define DISPLAY_RESOLUTION 0.03  /* UI repaint interval */
#define DISPLAY_RESOLUTION_US 30000
#define PROCINFO_RESOLUTION 0.25 /* process table refresh interval */
#define DUMP_RESOLUTION 300

typedef struct {
    long recv[HISTORY_LENGTH];
    long sent[HISTORY_LENGTH];
    unsigned long long total_sent;
    unsigned long long total_recv;
    int last_write;
    /* Cached process info — persists after socket closes */
    int proc_resolved;            /* 1 if we've found a process for this flow */
    int proc_pid;                 /* cached PID (0 = unknown) */
    char proc_name[PROCINFO_NAME_MAX]; /* cached process name */
} history_type;

void tick(int print);

void *xmalloc(size_t size);

void *xcalloc(size_t count, size_t size);

void *xrealloc(void *orig, size_t size);

char *xstrdup(const char *str);

void xfree(void *ptr);

/* options.c */
void options_read(int argc, char **argv);

struct pfloghdr {
    unsigned char length;
    unsigned char address_family;
    unsigned char action;
    unsigned char reason;
    char ifname[16];
    char ruleset[16];
    unsigned int rulenr;
    unsigned int subrulenr;
    unsigned char direction;
    unsigned char padding[3];
};

#endif /* __IFTOP_H_ */
