/*
 * ns_hash.h:
 *
 * Copyright (c) 2002 Jacob Menke
 *
 */

#ifndef __NS_HASH_H_ /* include guard */
#define __NS_HASH_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hash.h"

#define NS_HASH_MAX_ENTRIES 16384

hash_type *ns_hash_create(void);

/*
 * If *n_entries >= max_entries, clear the cache and reset the counter.
 * Returns 1 if eviction occurred, 0 otherwise.
 */
int ns_hash_evict_if_full(hash_type *h, int *n_entries, int max_entries);

#endif /* __NS_HASH_H_ */
