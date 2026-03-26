/*
 * addr_hash.h:
 *
 */

#ifndef __ADDR_HASH_H_ /* include guard */
#define __ADDR_HASH_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "hash.h"

typedef struct {
    int address_family;
    unsigned short int protocol;
    unsigned short int src_port;
    union {
        struct in_addr src;
        struct in6_addr src6;
    };
    unsigned short int dst_port;
    union {
        struct in_addr dst;
        struct in6_addr dst6;
    };
} addr_pair;

typedef addr_pair key_type; /* index into hash table */

hash_type *addr_hash_create(void);

/* Specialized fast-path operations (bypass function pointer indirection) */
hash_status_enum addr_hash_find(hash_type *hash_table, addr_pair *key, void **rec);
hash_status_enum addr_hash_insert(hash_type *hash_table, addr_pair *key, void *rec);
void addr_hash_delete_node(hash_type *hash_table, hash_node_type *node);
void addr_hash_delete_all_free(hash_type *hash_table);

int get_addrs_dlpi(char *interface, char if_hw_addr[], struct in_addr *if_ip_addr);

int get_addrs_ioctl(char *interface, char if_hw_addr[], struct in_addr *if_ip_addr,
                    struct in6_addr *if_ip6_addr);

#endif /* __ADDR_HASH_H_ */
