/*
 * iftop.h:
 *
 */

#ifndef __IFTOP_H_ /* include guard */
#define __IFTOP_H_

#include "conf.h"

/* 40 / 2  */
#define HISTORY_LENGTH  20
#define RESOLUTION 2
#define DUMP_RESOLUTION 300

typedef struct {
    long recv[HISTORY_LENGTH];
    long sent[HISTORY_LENGTH];
    unsigned long long total_sent;
    unsigned long long total_recv;
    int last_write;
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
