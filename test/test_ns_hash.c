/*
 * test_ns_hash.c: tests for ns_hash (IPv6 address to name cache)
 */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "test.h"
#include "ns_hash.h"
#include "iftop.h"

static struct in6_addr make_addr6(const char *str) {
    struct in6_addr addr;
    memset(&addr, 0, sizeof(addr));
    inet_pton(AF_INET6, str, &addr);
    return addr;
}

static int ns_hash_count(hash_type *h) {
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) count++;
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
    struct in6_addr addr = make_addr6("2001:db8::1");
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
    struct in6_addr addr1 = make_addr6("::1");
    struct in6_addr addr2 = make_addr6("::2");
    hash_insert(h, &addr1, xstrdup("localhost"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_find_empty) {
    hash_type *h = ns_hash_create();
    struct in6_addr addr = make_addr6("::1");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_loopback) {
    hash_type *h = ns_hash_create();
    struct in6_addr addr = make_addr6("::1");
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
    struct in6_addr addr = make_addr6("fe80::1");
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
    struct in6_addr addr = make_addr6("2001:0db8:85a3::8a2e:0370:7334");
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
    struct in6_addr addr = make_addr6("::");
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
    struct in6_addr addr = make_addr6("ff02::1");
    hash_insert(h, &addr, xstrdup("all-nodes"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "all-nodes");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Multiple entries === */

TEST(ns_hash_multiple_entries) {
    hash_type *h = ns_hash_create();
    struct in6_addr addrs[5];
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
    struct in6_addr addrs[50];
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
    struct in6_addr a1 = make_addr6("2001:db8::1");
    struct in6_addr a2 = make_addr6("2001:db9::1");
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
    struct in6_addr addr = make_addr6("fe80::1");
    hash_insert(h, &addr, xstrdup("link-local"));
    ASSERT_EQ(hash_delete(h, &addr), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &addr, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_delete_missing) {
    hash_type *h = ns_hash_create();
    struct in6_addr addr = make_addr6("::1");
    ASSERT_EQ(hash_delete(h, &addr), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(ns_hash_delete_then_reinsert) {
    hash_type *h = ns_hash_create();
    struct in6_addr addr = make_addr6("2001:db8::1");
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
        struct in6_addr addr = make_addr6(addr_str);
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
    struct in6_addr addr = make_addr6("::1");
    hash_insert(h, &addr, xstrdup("only"));
    ASSERT_EQ(ns_hash_count(h), 1);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Key independence === */

TEST(ns_hash_key_is_copied) {
    hash_type *h = ns_hash_create();
    struct in6_addr addr = make_addr6("2001:db8::1");
    hash_insert(h, &addr, xstrdup("original"));
    /* Modify the original key */
    memset(&addr, 0xff, sizeof(addr));
    /* Should still find with original address */
    struct in6_addr lookup = make_addr6("2001:db8::1");
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
    struct in6_addr addrs[200];
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
    struct in6_addr a1 = make_addr6("2001:db8:1::1");
    struct in6_addr a2 = make_addr6("2001:db8:2::1");
    struct in6_addr a3 = make_addr6("2001:db8:3::1");
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
    struct in6_addr addr = make_addr6("2001:db8::1");
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
    struct in6_addr a1 = make_addr6("2001:db8::1");
    struct in6_addr a2 = make_addr6("2001:db8::2");
    struct in6_addr a3 = make_addr6("2001:db8::3");
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
    struct in6_addr a1 = make_addr6("::1");
    struct in6_addr a2 = make_addr6("::2");
    struct in6_addr a3 = make_addr6("::3");
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
        struct in6_addr addr = make_addr6(addr_str);
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
    struct in6_addr addr = make_addr6("2001:db8::1");
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
    struct in6_addr addr = make_addr6("::1");
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
    struct in6_addr addrs[500];
    for (int i = 0; i < 500; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8:%x::%x",
                 i / 256, i % 256 + 1);
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

/* === Delete all then reuse === */

TEST(ns_hash_reuse_after_clear) {
    hash_type *h = ns_hash_create();
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 20; i++) {
            char addr_str[64];
            snprintf(addr_str, sizeof(addr_str), "2001:db8::%d", i + 1);
            struct in6_addr addr = make_addr6(addr_str);
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
    RUN(ns_hash_reuse_after_clear);

    TEST_REPORT();
}
