/*
 * test_leaks.c: memory leak tests for all data structures
 *
 * Each test exercises a complete lifecycle (create, populate, destroy).
 * Run under macOS `leaks --atExit` to detect unreachable allocations.
 * The process exit code from `leaks` is non-zero if leaks are found.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "test.h"
#include "vector.h"
#include "hash.h"
#include "addr_hash.h"
#include "ns_hash.h"
#include "serv_hash.h"
#include "sorted_list.h"
#include "stringmap.h"
#include "cfgfile.h"
#include "iftop.h"

/* ================================================================
 *  Vector
 * ================================================================ */

TEST(leak_vector_create_destroy) {
    vector v = vector_new();
    vector_delete(v);
}

TEST(leak_vector_push_pop_destroy) {
    vector v = vector_new();
    for (int i = 0; i < 200; i++) {
        vector_push_back(v, item_long(i));
    }
    for (int i = 0; i < 100; i++) {
        vector_pop_back(v);
    }
    vector_delete(v);
}

TEST(leak_vector_with_heap_pointers) {
    vector v = vector_new();
    for (int i = 0; i < 100; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "string_%d", i);
        vector_push_back(v, item_ptr(xstrdup(buf)));
    }
    vector_delete_free(v);
}

TEST(leak_vector_remove_pointers) {
    vector v = vector_new();
    for (int i = 0; i < 20; i++) {
        vector_push_back(v, item_ptr(xstrdup("data")));
    }
    /* Remove half, freeing their pointers */
    for (int i = 0; i < 10; i++) {
        xfree(v->items[0].ptr);
        vector_remove(v, &v->items[0]);
    }
    vector_delete_free(v);
}

TEST(leak_vector_resize_cycles) {
    vector v = vector_new();
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < 100; i++) {
            vector_push_back(v, item_long(i));
        }
        while (v->n_used > 0) {
            vector_pop_back(v);
        }
    }
    vector_delete(v);
}

/* ================================================================
 *  Generic Hash Table
 * ================================================================ */

static int str_compare(void *l, void *r) {
    return strcmp((char *)l, (char *)r) == 0;
}
static int str_hash(void *key) {
    char *s = (char *)key;
    unsigned int h = 0;
    while (*s) {
        h = h * 31 + (unsigned char)*s++;
    }
    return h % 64;
}
static void *str_copy_key(void *key) {
    return xstrdup((char *)key);
}
static void str_delete_key(void *key) {
    free(key);
}

static hash_type *create_str_hash(void) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 64;
    h->compare = str_compare;
    h->hash = str_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    return h;
}

TEST(leak_hash_create_destroy) {
    hash_type *h = create_str_hash();
    hash_destroy(h);
    free(h);
}

TEST(leak_hash_insert_delete_all) {
    hash_type *h = create_str_hash();
    for (int i = 0; i < 200; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        hash_insert(h, key, NULL);
    }
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(leak_hash_insert_delete_all_free) {
    hash_type *h = create_str_hash();
    for (int i = 0; i < 200; i++) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(val, sizeof(val), "val_%d", i);
        hash_insert(h, key, xstrdup(val));
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(leak_hash_insert_delete_by_key) {
    hash_type *h = create_str_hash();
    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        hash_insert(h, key, NULL);
    }
    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        hash_delete(h, key);
    }
    hash_destroy(h);
    free(h);
}

TEST(leak_hash_insert_delete_by_node) {
    hash_type *h = create_str_hash();
    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        hash_insert(h, key, NULL);
    }
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        hash_delete_node(h, node);
        node = NULL;
    }
    hash_destroy(h);
    free(h);
}

TEST(leak_hash_repeated_insert_overwrite) {
    hash_type *h = create_str_hash();
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < 50; i++) {
            char key[32];
            snprintf(key, sizeof(key), "key_%d", i);
            hash_insert(h, key, xstrdup("value"));
        }
        hash_delete_all_free(h);
    }
    hash_destroy(h);
    free(h);
}

/* ================================================================
 *  Address Hash (pool allocator)
 * ================================================================ */

static addr_pair make_ipv4(const char *src, uint16_t sport, const char *dst, uint16_t dport) {
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET;
    ap.protocol = 6;
    ap.src_port = sport;
    ap.dst_port = dport;
    inet_pton(AF_INET, src, &ap.src);
    inet_pton(AF_INET, dst, &ap.dst);
    return ap;
}

static addr_pair make_ipv6(const char *src, uint16_t sport, const char *dst, uint16_t dport) {
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET6;
    ap.protocol = 6;
    ap.src_port = sport;
    ap.dst_port = dport;
    inet_pton(AF_INET6, src, &ap.src6);
    inet_pton(AF_INET6, dst, &ap.dst6);
    return ap;
}

static void addr_hash_teardown(hash_type *h) {
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        addr_hash_delete_node(h, node);
        node = NULL;
    }
    hash_destroy(h);
    free(h);
}

TEST(leak_addr_hash_create_destroy) {
    hash_type *h = addr_hash_create();
    hash_destroy(h);
    free(h);
}

TEST(leak_addr_hash_insert_teardown) {
    hash_type *h = addr_hash_create();
    for (int i = 0; i < 500; i++) {
        char src[32], dst[32];
        snprintf(src, sizeof(src), "10.0.%d.%d", i / 256, i % 256);
        snprintf(dst, sizeof(dst), "10.1.%d.%d", i / 256, i % 256);
        addr_pair ap = make_ipv4(src, 1000 + i, dst, 80);
        addr_hash_insert(h, &ap, NULL);
    }
    addr_hash_teardown(h);
}

TEST(leak_addr_hash_insert_with_records) {
    hash_type *h = addr_hash_create();
    for (int i = 0; i < 200; i++) {
        char src[32], dst[32];
        snprintf(src, sizeof(src), "10.0.%d.%d", i / 256, i % 256);
        snprintf(dst, sizeof(dst), "10.1.%d.%d", i / 256, i % 256);
        addr_pair ap = make_ipv4(src, 1000 + i, dst, 80);
        int *rec = xmalloc(sizeof(int));
        *rec = i;
        addr_hash_insert(h, &ap, rec);
    }
    addr_hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(leak_addr_hash_ipv6) {
    hash_type *h = addr_hash_create();
    for (int i = 0; i < 100; i++) {
        char src[64], dst[64];
        snprintf(src, sizeof(src), "2001:db8::%x", i);
        snprintf(dst, sizeof(dst), "2001:db8::1:%x", i);
        addr_pair ap = make_ipv6(src, 1000 + i, dst, 443);
        addr_hash_insert(h, &ap, NULL);
    }
    addr_hash_teardown(h);
}

TEST(leak_addr_hash_insert_delete_cycles) {
    hash_type *h = addr_hash_create();
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 100; i++) {
            char src[32];
            snprintf(src, sizeof(src), "10.0.0.%d", i);
            addr_pair ap = make_ipv4(src, 5000, "10.0.1.1", 80);
            addr_hash_insert(h, &ap, NULL);
        }
        hash_node_type *node = NULL;
        while (hash_next_item(h, &node) == HASH_STATUS_OK) {
            addr_hash_delete_node(h, node);
            node = NULL;
        }
    }
    hash_destroy(h);
    free(h);
}

/* ================================================================
 *  NS Hash (IPv6 name cache)
 * ================================================================ */

TEST(leak_ns_hash_create_destroy) {
    hash_type *h = ns_hash_create();
    hash_destroy(h);
    free(h);
}

TEST(leak_ns_hash_insert_cleanup) {
    hash_type *h = ns_hash_create();
    for (int i = 0; i < 100; i++) {
        struct in6_addr addr;
        char str[64];
        snprintf(str, sizeof(str), "2001:db8::%x", i);
        inet_pton(AF_INET6, str, &addr);
        char name[64];
        snprintf(name, sizeof(name), "host%d.example.com", i);
        hash_insert(h, &addr, xstrdup(name));
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(leak_ns_hash_evict_cycles) {
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 50;
    for (int cycle = 0; cycle < 10; cycle++) {
        for (int i = 0; i < limit; i++) {
            ns_hash_evict_if_full(h, &count, limit);
            struct in6_addr addr;
            char str[64];
            snprintf(str, sizeof(str), "2001:db8:%x::%x", cycle, i + 1);
            inet_pton(AF_INET6, str, &addr);
            hash_insert(h, &addr, xstrdup("hostname"));
            count++;
        }
    }
    /* Final eviction to clean up */
    ns_hash_evict_if_full(h, &count, limit);
    hash_destroy(h);
    free(h);
}

/* ================================================================
 *  Service Hash (port+proto to name)
 * ================================================================ */

TEST(leak_serv_table_create_destroy) {
    serv_table *t = serv_table_create();
    serv_table_destroy(t);
}

TEST(leak_serv_table_insert_cleanup) {
    serv_table *t = serv_table_create();
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "svc_%d", i);
        serv_table_insert(t, 80 + i, 6, name);
    }
    serv_table_destroy(t);
}

/* ================================================================
 *  Sorted List
 * ================================================================ */

static int int_compare(void *l, void *r) {
    long la = (long)l, lb = (long)r;
    return (la > lb) - (la < lb);
}

TEST(leak_sorted_list_single_inserts) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    for (long i = 0; i < 200; i++) {
        sorted_list_insert(&list, (void *)i);
    }
    sorted_list_destroy(&list);
}

TEST(leak_sorted_list_batch_insert) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[500];
    for (int i = 0; i < 500; i++) {
        items[i] = (void *)(long)(500 - i);
    }
    sorted_list_insert_batch(&list, items, 500);
    sorted_list_destroy(&list);
}

TEST(leak_sorted_list_destroy_reinit) {
    sorted_list_type list;
    list.compare = int_compare;
    for (int round = 0; round < 5; round++) {
        sorted_list_initialise(&list);
        void *items[100];
        for (int i = 0; i < 100; i++) {
            items[i] = (void *)(long)(100 - i);
        }
        sorted_list_insert_batch(&list, items, 100);
        sorted_list_destroy(&list);
    }
}

/* ================================================================
 *  Stringmap
 * ================================================================ */

TEST(leak_stringmap_create_delete) {
    stringmap s = stringmap_new();
    stringmap_delete(s);
}

TEST(leak_stringmap_insert_delete) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 200; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        stringmap_insert(s, key, item_long(i));
    }
    stringmap_delete(s);
}

TEST(leak_stringmap_insert_ptr_delete_free) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 200; i++) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(val, sizeof(val), "val_%d", i);
        stringmap_insert(s, key, item_ptr(xstrdup(val)));
    }
    stringmap_delete_free(s);
}

TEST(leak_stringmap_duplicate_insert) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        stringmap_insert(s, key, item_ptr(xstrdup("first")));
    }
    /* Re-insert same keys: stringmap_insert returns existing, so we must
     * free the new value ourselves since it wasn't stored */
    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_%d", i);
        char *new_val = xstrdup("second");
        item *existing = stringmap_insert(s, key, item_ptr(new_val));
        if (existing) {
            xfree(new_val);
        }
    }
    stringmap_delete_free(s);
}

/* ================================================================
 *  Config (stringmap-backed)
 * ================================================================ */

TEST(leak_config_init_set_reinit) {
    for (int round = 0; round < 5; round++) {
        config_init();
        for (int i = 0; i < 20; i++) {
            char key[32], val[32];
            snprintf(key, sizeof(key), "interface");
            snprintf(val, sizeof(val), "eth%d", i);
            config_set_string(key, val);
        }
    }
}

TEST(leak_config_set_overwrite) {
    config_init();
    for (int i = 0; i < 100; i++) {
        config_set_string("interface", "eth0");
        config_set_string("interface", "wlan0");
    }
}

TEST(leak_config_read_file) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/iftop_leak_test_XXXXXX");
    int fd = mkstemp(path);
    const char *content = "interface: eth0\n"
                          "dns-resolution: true\n"
                          "port-resolution: false\n"
                          "show-bars: true\n"
                          "sort: source\n";
    write(fd, content, strlen(content));
    close(fd);

    for (int round = 0; round < 5; round++) {
        config_init();
        read_config(path, 0);
    }
    unlink(path);
}

/* ================================================================
 *  Main
 * ================================================================ */

int main(void) {
    TEST_SUITE("Memory Leak Tests");

    /* Vector */
    RUN(leak_vector_create_destroy);
    RUN(leak_vector_push_pop_destroy);
    RUN(leak_vector_with_heap_pointers);
    RUN(leak_vector_remove_pointers);
    RUN(leak_vector_resize_cycles);

    /* Generic Hash */
    RUN(leak_hash_create_destroy);
    RUN(leak_hash_insert_delete_all);
    RUN(leak_hash_insert_delete_all_free);
    RUN(leak_hash_insert_delete_by_key);
    RUN(leak_hash_insert_delete_by_node);
    RUN(leak_hash_repeated_insert_overwrite);

    /* Address Hash */
    RUN(leak_addr_hash_create_destroy);
    RUN(leak_addr_hash_insert_teardown);
    RUN(leak_addr_hash_insert_with_records);
    RUN(leak_addr_hash_ipv6);
    RUN(leak_addr_hash_insert_delete_cycles);

    /* NS Hash */
    RUN(leak_ns_hash_create_destroy);
    RUN(leak_ns_hash_insert_cleanup);
    RUN(leak_ns_hash_evict_cycles);

    /* Service Hash */
    RUN(leak_serv_table_create_destroy);
    RUN(leak_serv_table_insert_cleanup);

    /* Sorted List */
    RUN(leak_sorted_list_single_inserts);
    RUN(leak_sorted_list_batch_insert);
    RUN(leak_sorted_list_destroy_reinit);

    /* Stringmap */
    RUN(leak_stringmap_create_delete);
    RUN(leak_stringmap_insert_delete);
    RUN(leak_stringmap_insert_ptr_delete_free);
    RUN(leak_stringmap_duplicate_insert);

    /* Config */
    RUN(leak_config_init_set_reinit);
    RUN(leak_config_set_overwrite);
    RUN(leak_config_read_file);

    TEST_REPORT();
}
