/*
 * serv_hash.c: port+protocol to service name lookup
 *
 * Uses direct-index arrays for TCP (protocol 6) and UDP (protocol 17)
 * for O(1) lookup. Falls back to a generic hash for other protocols.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "../../include/serv_hash.h"
#include "../../include/iftop.h"

/* --- Fallback hash for non-TCP/UDP protocols --- */

#define OTHER_HASH_SIZE 127

static int other_compare(void *left, void *right) {
    ip_service *a = (ip_service *) left;
    ip_service *b = (ip_service *) right;
    return (a->port == b->port && a->protocol == b->protocol);
}

static int other_hash(void *key) {
    ip_service *s = (ip_service *) key;
    return (unsigned)((s->port * 31) + s->protocol) % OTHER_HASH_SIZE;
}

static void *other_copy_key(void *orig) {
    ip_service *copy = xmalloc(sizeof *copy);
    *copy = *(ip_service *) orig;
    return copy;
}

static void other_delete_key(void *key) {
    free(key);
}

static hash_type *other_hash_create(void) {
    hash_type *h = xcalloc(OTHER_HASH_SIZE, sizeof *h);
    h->size = OTHER_HASH_SIZE;
    h->compare = &other_compare;
    h->hash = &other_hash;
    h->copy_key = &other_copy_key;
    h->delete_key = &other_delete_key;
    hash_initialise(h);
    return h;
}

/* --- Direct-index slot helper --- */

static inline char **serv_slot(serv_table *t, int port, int protocol) {
    if (port < 0 || port >= SERV_NUM_PORTS) return NULL;
    if (protocol == 6)  return &t->tcp[port];
    if (protocol == 17) return &t->udp[port];
    return NULL;
}

/* --- Public API --- */

serv_table *serv_table_create(void) {
    serv_table *t = xcalloc(1, sizeof *t);
    t->other = other_hash_create();
    return t;
}

void serv_table_insert(serv_table *t, int port, int protocol, const char *name) {
    char **slot = serv_slot(t, port, protocol);
    if (slot) {
        free(*slot);
        *slot = xstrdup(name);
    } else {
        ip_service key = {port, protocol};
        void *existing = NULL;
        if (hash_find(t->other, &key, &existing) == HASH_STATUS_OK) {
            hash_delete(t->other, &key);
            free(existing);
        }
        hash_insert(t->other, &key, xstrdup(name));
    }
    t->count++;
}

const char *serv_table_lookup(serv_table *t, int port, int protocol) {
    char **slot = serv_slot(t, port, protocol);
    if (slot) return *slot;

    ip_service key = {port, protocol};
    void *rec = NULL;
    if (hash_find(t->other, &key, &rec) == HASH_STATUS_OK)
        return (const char *) rec;
    return NULL;
}

int serv_table_delete(serv_table *t, int port, int protocol) {
    char **slot = serv_slot(t, port, protocol);
    if (slot) {
        if (*slot == NULL) return 0;
        free(*slot);
        *slot = NULL;
        t->count--;
        return 1;
    }

    ip_service key = {port, protocol};
    void *existing = NULL;
    if (hash_find(t->other, &key, &existing) == HASH_STATUS_OK) {
        hash_delete(t->other, &key);
        free(existing);
        t->count--;
        return 1;
    }
    return 0;
}

int serv_table_count(serv_table *t) {
    return t->count;
}

void serv_table_destroy(serv_table *t) {
    int i;
    for (i = 0; i < SERV_NUM_PORTS; i++) {
        free(t->tcp[i]);
        free(t->udp[i]);
    }
    hash_delete_all_free(t->other);
    hash_destroy(t->other);
    free(t->other);
    free(t);
}

void serv_table_init(serv_table *t) {
    struct servent *ent;
    struct protoent *pent;
    setprotoent(1);
    while ((ent = getservent()) != NULL) {
        pent = getprotobyname(ent->s_proto);
        if (pent != NULL) {
            serv_table_insert(t, ntohs(ent->s_port), pent->p_proto, ent->s_name);
        }
    }
    endprotoent();
}
