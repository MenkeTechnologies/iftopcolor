/*
 * ns_hash.h:
 *
 * Copyright (c) 2026 Jacob Menke
 *
 */

#ifndef __NS_HASH_H_ /* include guard */
#define __NS_HASH_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hash.h"

struct addr_storage {
    int address_family; /* AF_INET or AF_INET6 */
    int addr_len;       /* sizeof(struct in_addr or in6_addr) */
    union {
        struct in_addr addr4;
        struct in6_addr addr6;
    } addr;
#define as_addr4 addr.addr4
#define as_addr6 addr.addr6
};

#define NS_HASH_MAX_ENTRIES 16384

hash_type *ns_hash_create(void);

/*
 * If *n_entries >= max_entries, clear the cache and reset the counter.
 * Returns 1 if eviction occurred, 0 otherwise.
 */
int ns_hash_evict_if_full(hash_type *h, int *n_entries, int max_entries);

#endif /* __NS_HASH_H_ */
