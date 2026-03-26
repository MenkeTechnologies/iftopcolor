/*
 * serv_hash.h: port+protocol to service name lookup
 *
 * Direct-index arrays for TCP/UDP (O(1) lookup), with a generic hash
 * fallback for rare protocols.
 */

#ifndef __SERV_HASH_H_ /* include guard */
#define __SERV_HASH_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hash.h"

#define SERV_NUM_PORTS 65536

typedef struct {
    int port;
    int protocol;
} ip_service;

typedef struct {
    char *tcp[SERV_NUM_PORTS];   /* protocol 6: port -> name */
    char *udp[SERV_NUM_PORTS];   /* protocol 17: port -> name */
    hash_type *other;            /* fallback for other protocols */
    int count;
} serv_table;

serv_table *serv_table_create(void);
void serv_table_init(serv_table *t);
const char *serv_table_lookup(serv_table *t, int port, int protocol);
void serv_table_insert(serv_table *t, int port, int protocol, const char *name);
int serv_table_delete(serv_table *t, int port, int protocol);
int serv_table_count(serv_table *t);
void serv_table_destroy(serv_table *t);

#endif /* __SERV_HASH_H_ */
