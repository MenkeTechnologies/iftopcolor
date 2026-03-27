/*
 * procinfo.h:
 *
 * Map network sockets to owning processes.
 * Platform-specific: macOS (libproc) and Linux (/proc).
 */

#ifndef __PROCINFO_H_
#define __PROCINFO_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PROCINFO_NAME_MAX 64

typedef struct {
    pid_t pid;
    char name[PROCINFO_NAME_MAX];
} proc_entry;

/* Rebuild the internal socket-to-pid lookup table.
 * Call once per tick (~2 seconds). */
void procinfo_refresh(void);

/* Look up process info for a given local socket endpoint.
 * Returns 1 and fills *out if found, 0 otherwise. */
int procinfo_lookup(int address_family, const void *local_addr,
                    unsigned short local_port, unsigned short protocol,
                    proc_entry *out);

/* Free all internal state. */
void procinfo_destroy(void);

/* Returns 1 if the platform supports process attribution, 0 otherwise. */
int procinfo_available(void);

#endif /* __PROCINFO_H_ */
