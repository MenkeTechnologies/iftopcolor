/*
 * test_ns_hash.c: tests for ns_hash (address to name cache)
 */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "test.h"
#include "ns_hash.h"
#include "iftop.h"

static struct addr_storage make_addr6(const char *str) {
    struct addr_storage a;
    memset(&a, 0, sizeof(a));
    a.address_family = AF_INET6;
    a.addr_len = sizeof(struct in6_addr);
    inet_pton(AF_INET6, str, &a.as_addr6);
    return a;
}

static struct addr_storage make_addr4(const char *str) {
    struct addr_storage a;
    memset(&a, 0, sizeof(a));
    a.address_family = AF_INET;
    a.addr_len = sizeof(struct in_addr);
    inet_pton(AF_INET, str, &a.as_addr4);
    return a;
}

static int ns_hash_count(hash_type *h) {
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    return count;
}

/* === Creation === */

TEST(ns_hash_create_succeeds) {
    hash_type *h = ns_hash_create();
    ASSERT_NOT_NULL(h);
    ASSERT_EQ(h->size, 256);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_create_empty) {
    hash_type *h = ns_hash_create();
    ASSERT_EQ(ns_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_create_has_functions) {
    hash_type *h = ns_hash_create();
    ASSERT_NOT_NULL(h->compare);
    ASSERT_NOT_NULL(h->hash);
    ASSERT_NOT_NULL(h->copy_key);
    ASSERT_NOT_NULL(h->delete_key);
    hash_destroy(h);
    free(h);
}

/* === Insert/Find === */

TEST(ns_hash_insert_find) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("2001:db8::1");
    hash_insert(h, &addr, xstrdup("host1.example.com"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "host1.example.com");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_find_miss) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr1 = make_addr6("::1");
    struct addr_storage addr2 = make_addr6("::2");
    hash_insert(h, &addr1, xstrdup("localhost"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_find_empty) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("::1");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_loopback) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("::1");
    hash_insert(h, &addr, xstrdup("localhost"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "localhost");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_link_local) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("fe80::1");
    hash_insert(h, &addr, xstrdup("link-local-host"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "link-local-host");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_global_unicast) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("2001:0db8:85a3::8a2e:0370:7334");
    hash_insert(h, &addr, xstrdup("global.example.com"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "global.example.com");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_all_zeros) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("::");
    hash_insert(h, &addr, xstrdup("unspecified"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "unspecified");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_multicast) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("ff02::1");
    hash_insert(h, &addr, xstrdup("all-nodes"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "all-nodes");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === IPv4 support === */

TEST(ns_hash_ipv4_insert_find) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr4("192.168.1.1");
    hash_insert(h, &addr, xstrdup("router.local"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "router.local");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_ipv4_find_miss) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr1 = make_addr4("10.0.0.1");
    struct addr_storage addr2 = make_addr4("10.0.0.2");
    hash_insert(h, &addr1, xstrdup("host1"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_ipv4_and_ipv6_coexist) {
    hash_type *h = ns_hash_create();
    struct addr_storage a4 = make_addr4("192.168.1.1");
    struct addr_storage a6 = make_addr6("2001:db8::1");
    hash_insert(h, &a4, xstrdup("ipv4-host"));
    hash_insert(h, &a6, xstrdup("ipv6-host"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &a4, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "ipv4-host");
    ASSERT_EQ(hash_find(h, &a6, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "ipv6-host");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_ipv4_same_bytes_different_family) {
    /* Ensure an IPv4 addr doesn't match an IPv6 addr with same leading bytes */
    hash_type *h = ns_hash_create();
    struct addr_storage a4 = make_addr4("0.0.0.1");
    struct addr_storage a6 = make_addr6("::1");
    hash_insert(h, &a4, xstrdup("v4-loopback"));
    hash_insert(h, &a6, xstrdup("v6-loopback"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &a4, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "v4-loopback");
    ASSERT_EQ(hash_find(h, &a6, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "v6-loopback");
    ASSERT_EQ(ns_hash_count(h), 2);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Multiple entries === */

TEST(ns_hash_multiple_entries) {
    hash_type *h = ns_hash_create();
    struct addr_storage addrs[5];
    char *names[] = {"host0", "host1", "host2", "host3", "host4"};
    for (int i = 0; i < 5; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%d", i + 1);
        addrs[i] = make_addr6(addr_str);
        hash_insert(h, &addrs[i], xstrdup(names[i]));
    }
    for (int i = 0; i < 5; i++) {
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &addrs[i], &rec), HASH_STATUS_OK);
        ASSERT_STR_EQ((char *)rec, names[i]);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_50_entries) {
    hash_type *h = ns_hash_create();
    struct addr_storage addrs[50];
    for (int i = 0; i < 50; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        addrs[i] = make_addr6(addr_str);
        char name[32];
        snprintf(name, sizeof(name), "host-%d.example.com", i);
        hash_insert(h, &addrs[i], xstrdup(name));
    }
    ASSERT_EQ(ns_hash_count(h), 50);
    for (int i = 0; i < 50; i++) {
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &addrs[i], &rec), HASH_STATUS_OK);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_same_suffix_different_prefix) {
    hash_type *h = ns_hash_create();
    struct addr_storage a1 = make_addr6("2001:db8::1");
    struct addr_storage a2 = make_addr6("2001:db9::1");
    hash_insert(h, &a1, xstrdup("net8"));
    hash_insert(h, &a2, xstrdup("net9"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &a1, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "net8");
    ASSERT_EQ(hash_find(h, &a2, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "net9");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Delete === */

TEST(ns_hash_delete_entry) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("fe80::1");
    hash_insert(h, &addr, xstrdup("link-local"));
    ASSERT_EQ(hash_delete(h, &addr), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_delete_missing) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("::1");
    ASSERT_EQ(hash_delete(h, &addr), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_delete_then_reinsert) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("2001:db8::1");
    hash_insert(h, &addr, xstrdup("old"));
    hash_delete(h, &addr);
    hash_insert(h, &addr, xstrdup("new"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "new");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_delete_all_clears) {
    hash_type *h = ns_hash_create();
    for (int i = 0; i < 10; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%d", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("name"));
    }
    hash_delete_all_free(h);
    ASSERT_EQ(ns_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

/* === Iteration === */

TEST(ns_hash_iterate_empty) {
    hash_type *h = ns_hash_create();
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_iterate_single) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("::1");
    hash_insert(h, &addr, xstrdup("only"));
    ASSERT_EQ(ns_hash_count(h), 1);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Key independence === */

TEST(ns_hash_key_is_copied) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("2001:db8::1");
    hash_insert(h, &addr, xstrdup("original"));
    /* Modify the original key */
    memset(&addr, 0xff, sizeof(addr));
    /* Should still find with original address */
    struct addr_storage lookup = make_addr6("2001:db8::1");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "original");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Large scale === */

TEST(ns_hash_200_entries) {
    hash_type *h = ns_hash_create();
    struct addr_storage addrs[200];
    for (int i = 0; i < 200; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8:%x::%x", i / 256, i % 256 + 1);
        addrs[i] = make_addr6(addr_str);
        char name[64];
        snprintf(name, sizeof(name), "host-%d.example.com", i);
        hash_insert(h, &addrs[i], xstrdup(name));
    }
    ASSERT_EQ(ns_hash_count(h), 200);
    /* Spot check */
    for (int i = 0; i < 200; i += 20) {
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &addrs[i], &rec), HASH_STATUS_OK);
        char expected[64];
        snprintf(expected, sizeof(expected), "host-%d.example.com", i);
        ASSERT_STR_EQ((char *)rec, expected);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Hash function distribution === */

TEST(ns_hash_different_subnets) {
    hash_type *h = ns_hash_create();
    /* Addresses from different /48 subnets should hash to different buckets usually */
    struct addr_storage a1 = make_addr6("2001:db8:1::1");
    struct addr_storage a2 = make_addr6("2001:db8:2::1");
    struct addr_storage a3 = make_addr6("2001:db8:3::1");
    hash_insert(h, &a1, xstrdup("net1"));
    hash_insert(h, &a2, xstrdup("net2"));
    hash_insert(h, &a3, xstrdup("net3"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &a1, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "net1");
    ASSERT_EQ(hash_find(h, &a2, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "net2");
    ASSERT_EQ(hash_find(h, &a3, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "net3");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Overwrite === */

TEST(ns_hash_overwrite_value) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("2001:db8::1");
    hash_insert(h, &addr, xstrdup("old_name"));
    hash_delete(h, &addr);
    hash_insert(h, &addr, xstrdup("new_name"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "new_name");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Adjacent addresses === */

TEST(ns_hash_adjacent_addresses) {
    hash_type *h = ns_hash_create();
    struct addr_storage a1 = make_addr6("2001:db8::1");
    struct addr_storage a2 = make_addr6("2001:db8::2");
    struct addr_storage a3 = make_addr6("2001:db8::3");
    hash_insert(h, &a1, xstrdup("first"));
    hash_insert(h, &a2, xstrdup("second"));
    hash_insert(h, &a3, xstrdup("third"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &a1, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "first");
    ASSERT_EQ(hash_find(h, &a2, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "second");
    ASSERT_EQ(hash_find(h, &a3, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "third");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Delete preserves others === */

TEST(ns_hash_delete_preserves_others) {
    hash_type *h = ns_hash_create();
    struct addr_storage a1 = make_addr6("::1");
    struct addr_storage a2 = make_addr6("::2");
    struct addr_storage a3 = make_addr6("::3");
    hash_insert(h, &a1, xstrdup("one"));
    hash_insert(h, &a2, xstrdup("two"));
    hash_insert(h, &a3, xstrdup("three"));
    hash_delete(h, &a2);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &a1, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "one");
    ASSERT_EQ(hash_find(h, &a2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    ASSERT_EQ(hash_find(h, &a3, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "three");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Iteration count === */

TEST(ns_hash_iterate_count_matches) {
    hash_type *h = ns_hash_create();
    for (int i = 0; i < 30; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%d", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("name"));
    }
    ASSERT_EQ(ns_hash_count(h), 30);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Long hostname values === */

TEST(ns_hash_long_hostname) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("2001:db8::1");
    char long_name[256];
    memset(long_name, 'x', 255);
    long_name[255] = '\0';
    hash_insert(h, &addr, xstrdup(long_name));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, long_name);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Empty string value === */

TEST(ns_hash_empty_name) {
    hash_type *h = ns_hash_create();
    struct addr_storage addr = make_addr6("::1");
    hash_insert(h, &addr, xstrdup(""));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === 500 entries === */

TEST(ns_hash_500_entries) {
    hash_type *h = ns_hash_create();
    struct addr_storage addrs[500];
    for (int i = 0; i < 500; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8:%x::%x", i / 256, i % 256 + 1);
        addrs[i] = make_addr6(addr_str);
        char name[64];
        snprintf(name, sizeof(name), "host-%d.test.com", i);
        hash_insert(h, &addrs[i], xstrdup(name));
    }
    ASSERT_EQ(ns_hash_count(h), 500);
    /* Spot check every 50th */
    for (int i = 0; i < 500; i += 50) {
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &addrs[i], &rec), HASH_STATUS_OK);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Cache eviction === */

TEST(ns_hash_evict_no_op_when_under_limit) {
    hash_type *h = ns_hash_create();
    int count = 0;
    for (int i = 0; i < 10; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%d", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("name"));
        count++;
    }
    int evicted = ns_hash_evict_if_full(h, &count, 100);
    ASSERT_EQ(evicted, 0);
    ASSERT_EQ(count, 10);
    ASSERT_EQ(ns_hash_count(h), 10);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_clears_at_limit) {
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 50;
    for (int i = 0; i < limit; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("name"));
        count++;
    }
    ASSERT_EQ(count, limit);
    int evicted = ns_hash_evict_if_full(h, &count, limit);
    ASSERT_EQ(evicted, 1);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(ns_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_clears_above_limit) {
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 30;
    /* Insert more than the limit */
    for (int i = 0; i < limit + 10; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("name"));
        count++;
    }
    int evicted = ns_hash_evict_if_full(h, &count, limit);
    ASSERT_EQ(evicted, 1);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(ns_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_one_below_limit) {
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 50;
    for (int i = 0; i < limit - 1; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("name"));
        count++;
    }
    int evicted = ns_hash_evict_if_full(h, &count, limit);
    ASSERT_EQ(evicted, 0);
    ASSERT_EQ(count, limit - 1);
    ASSERT_EQ(ns_hash_count(h), limit - 1);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_repopulate_after_eviction) {
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 20;
    /* Fill to limit and evict */
    for (int i = 0; i < limit; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("old"));
        count++;
    }
    ns_hash_evict_if_full(h, &count, limit);
    ASSERT_EQ(count, 0);
    /* Re-populate with new entries */
    for (int i = 0; i < 5; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8:1::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("new"));
        count++;
    }
    ASSERT_EQ(count, 5);
    ASSERT_EQ(ns_hash_count(h), 5);
    /* Old entries should be gone */
    struct addr_storage old_addr = make_addr6("2001:db8::1");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &old_addr, &rec), HASH_STATUS_KEY_NOT_FOUND);
    /* New entries should be findable */
    struct addr_storage new_addr = make_addr6("2001:db8:1::1");
    ASSERT_EQ(hash_find(h, &new_addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "new");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_multiple_cycles) {
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 25;
    for (int cycle = 0; cycle < 5; cycle++) {
        for (int i = 0; i < limit; i++) {
            ns_hash_evict_if_full(h, &count, limit);
            char addr_str[64];
            snprintf(addr_str, sizeof(addr_str), "2001:db8:%x::%x", cycle, i + 1);
            struct addr_storage addr = make_addr6(addr_str);
            hash_insert(h, &addr, xstrdup("name"));
            count++;
        }
        /* At this point count == limit, next evict call should clear */
        ASSERT_EQ(count, limit);
        ASSERT_EQ(ns_hash_count(h), limit);
    }
    /* One final eviction */
    int evicted = ns_hash_evict_if_full(h, &count, limit);
    ASSERT_EQ(evicted, 1);
    ASSERT_EQ(count, 0);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_counter_tracks_external_deletes) {
    /* Simulates resolver_worker pattern: find+delete+insert (replace) doesn't change count,
       but insert after eviction (find miss) increments count */
    hash_type *h = ns_hash_create();
    int count = 0;
    int limit = 20;
    /* Insert entries with placeholder values */
    for (int i = 0; i < 10; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        hash_insert(h, &addr, xstrdup("1.2.3.4")); /* numeric placeholder */
        count++;
    }
    /* Replace some entries (delete + insert, no count change) */
    for (int i = 0; i < 5; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8::%x", i + 1);
        struct addr_storage addr = make_addr6(addr_str);
        void *old = NULL;
        ASSERT_EQ(hash_find(h, &addr, &old), HASH_STATUS_OK);
        hash_delete(h, &addr);
        xfree(old);
        hash_insert(h, &addr, xstrdup("resolved.example.com"));
        /* count stays the same for a replace */
    }
    ASSERT_EQ(count, 10);
    ASSERT_EQ(ns_hash_count(h), 10);
    /* Now evict should not trigger (10 < 20) */
    ASSERT_EQ(ns_hash_evict_if_full(h, &count, limit), 0);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_evict_with_limit_1) {
    hash_type *h = ns_hash_create();
    int count = 0;
    /* Limit of 1: every insert triggers eviction of previous */
    struct addr_storage addr1 = make_addr6("2001:db8::1");
    hash_insert(h, &addr1, xstrdup("first"));
    count++;
    /* count is now 1, eviction should trigger */
    int evicted = ns_hash_evict_if_full(h, &count, 1);
    ASSERT_EQ(evicted, 1);
    ASSERT_EQ(count, 0);
    ASSERT_EQ(ns_hash_count(h), 0);
    /* Insert again */
    struct addr_storage addr2 = make_addr6("2001:db8::2");
    hash_insert(h, &addr2, xstrdup("second"));
    count++;
    ASSERT_EQ(ns_hash_count(h), 1);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr2, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "second");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Delete all then reuse === */

TEST(ns_hash_reuse_after_clear) {
    hash_type *h = ns_hash_create();
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 20; i++) {
            char addr_str[64];
            snprintf(addr_str, sizeof(addr_str), "2001:db8::%d", i + 1);
            struct addr_storage addr = make_addr6(addr_str);
            hash_insert(h, &addr, xstrdup("name"));
        }
        ASSERT_EQ(ns_hash_count(h), 20);
        hash_delete_all_free(h);
        ASSERT_EQ(ns_hash_count(h), 0);
    }
    hash_destroy(h);
    free(h);
}

int main(void) {
    TEST_SUITE("NS Hash Tests");

    RUN(ns_hash_create_succeeds);
    RUN(ns_hash_create_empty);
    RUN(ns_hash_create_has_functions);
    RUN(ns_hash_insert_find);
    RUN(ns_hash_find_miss);
    RUN(ns_hash_find_empty);
    RUN(ns_hash_loopback);
    RUN(ns_hash_link_local);
    RUN(ns_hash_global_unicast);
    RUN(ns_hash_all_zeros);
    RUN(ns_hash_multicast);
    RUN(ns_hash_ipv4_insert_find);
    RUN(ns_hash_ipv4_find_miss);
    RUN(ns_hash_ipv4_and_ipv6_coexist);
    RUN(ns_hash_ipv4_same_bytes_different_family);
    RUN(ns_hash_multiple_entries);
    RUN(ns_hash_50_entries);
    RUN(ns_hash_same_suffix_different_prefix);
    RUN(ns_hash_delete_entry);
    RUN(ns_hash_delete_missing);
    RUN(ns_hash_delete_then_reinsert);
    RUN(ns_hash_delete_all_clears);
    RUN(ns_hash_iterate_empty);
    RUN(ns_hash_iterate_single);
    RUN(ns_hash_key_is_copied);
    RUN(ns_hash_200_entries);
    RUN(ns_hash_different_subnets);
    RUN(ns_hash_overwrite_value);
    RUN(ns_hash_adjacent_addresses);
    RUN(ns_hash_delete_preserves_others);
    RUN(ns_hash_iterate_count_matches);
    RUN(ns_hash_long_hostname);
    RUN(ns_hash_empty_name);
    RUN(ns_hash_500_entries);
    RUN(ns_hash_evict_no_op_when_under_limit);
    RUN(ns_hash_evict_clears_at_limit);
    RUN(ns_hash_evict_clears_above_limit);
    RUN(ns_hash_evict_one_below_limit);
    RUN(ns_hash_evict_repopulate_after_eviction);
    RUN(ns_hash_evict_multiple_cycles);
    RUN(ns_hash_evict_counter_tracks_external_deletes);
    RUN(ns_hash_evict_with_limit_1);
    RUN(ns_hash_reuse_after_clear);

    TEST_REPORT();
}
