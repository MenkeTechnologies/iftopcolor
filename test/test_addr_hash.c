/*
 * test_addr_hash.c: tests for optimized addr_hash with pool allocator
 */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "test.h"
#include "addr_hash.h"
#include "iftop.h"

static addr_pair make_ipv4_pair(const char *src, uint16_t sport, const char *dst, uint16_t dport,
                                uint16_t proto) {
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET;
    ap.protocol = proto;
    ap.src_port = sport;
    ap.dst_port = dport;
    inet_pton(AF_INET, src, &ap.src);
    inet_pton(AF_INET, dst, &ap.dst);
    return ap;
}

static addr_pair make_ipv6_pair(const char *src, uint16_t sport, const char *dst, uint16_t dport,
                                uint16_t proto) {
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET6;
    ap.protocol = proto;
    ap.src_port = sport;
    ap.dst_port = dport;
    inet_pton(AF_INET6, src, &ap.src6);
    inet_pton(AF_INET6, dst, &ap.dst6);
    return ap;
}

static void addr_hash_clear(hash_type *h) {
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        addr_hash_delete_node(h, node);
        node = NULL;
    }
}

static void addr_hash_teardown(hash_type *h) {
    addr_hash_clear(h);
    hash_destroy(h);
    free(h);
}

static int addr_hash_count(hash_type *h) {
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    return count;
}

/* === Creation === */

TEST(addr_hash_create_succeeds) {
    hash_type *h = addr_hash_create();
    ASSERT_NOT_NULL(h);
    ASSERT_EQ(h->size, 2048);
    ASSERT_NOT_NULL(h->table);
    addr_hash_teardown(h);
}

TEST(addr_hash_create_table_zeroed) {
    hash_type *h = addr_hash_create();
    for (int i = 0; i < h->size; i++) {
        ASSERT_NULL(h->table[i]);
    }
    addr_hash_teardown(h);
}

TEST(addr_hash_create_has_function_pointers) {
    hash_type *h = addr_hash_create();
    ASSERT_NOT_NULL(h->compare);
    ASSERT_NOT_NULL(h->hash);
    ASSERT_NOT_NULL(h->copy_key);
    ASSERT_NOT_NULL(h->delete_key);
    addr_hash_teardown(h);
}

/* === IPv4 Insert/Find === */

TEST(addr_hash_ipv4_insert_find) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("192.168.1.1", 12345, "10.0.0.1", 80, 6);
    int val = 42;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 42);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_find_miss) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv4_pair("192.168.1.1", 12345, "10.0.0.1", 80, 6);
    addr_pair ap2 = make_ipv4_pair("192.168.1.2", 12345, "10.0.0.1", 80, 6);
    int val = 1;
    addr_hash_insert(h, &ap1, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_different_src_ports) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv4_pair("10.0.0.1", 1000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4_pair("10.0.0.1", 1001, "10.0.0.2", 80, 6);
    int v1 = 1, v2 = 2;
    addr_hash_insert(h, &ap1, &v1);
    addr_hash_insert(h, &ap2, &v2);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 1);
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_different_dst_ports) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv4_pair("10.0.0.1", 80, "10.0.0.2", 2000, 6);
    addr_pair ap2 = make_ipv4_pair("10.0.0.1", 80, "10.0.0.2", 2001, 6);
    int v1 = 10, v2 = 20;
    addr_hash_insert(h, &ap1, &v1);
    addr_hash_insert(h, &ap2, &v2);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 10);
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 20);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_different_src_addr) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 80, 6);
    addr_pair ap2 = make_ipv4_pair("3.3.3.3", 80, "2.2.2.2", 80, 6);
    int v1 = 1, v2 = 2;
    addr_hash_insert(h, &ap1, &v1);
    addr_hash_insert(h, &ap2, &v2);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 1);
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_different_dst_addr) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv4_pair("1.1.1.1", 80, "4.4.4.4", 80, 6);
    addr_pair ap2 = make_ipv4_pair("1.1.1.1", 80, "5.5.5.5", 80, 6);
    int v1 = 1, v2 = 2;
    addr_hash_insert(h, &ap1, &v1);
    addr_hash_insert(h, &ap2, &v2);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 1);
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_different_protocol) {
    hash_type *h = addr_hash_create();
    addr_pair tcp = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 80, 6);
    addr_pair udp = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 80, 17);
    int v1 = 6, v2 = 17;
    addr_hash_insert(h, &tcp, &v1);
    addr_hash_insert(h, &udp, &v2);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &tcp, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 6);
    ASSERT_EQ(addr_hash_find(h, &udp, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 17);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_loopback) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("127.0.0.1", 8080, "127.0.0.1", 80, 6);
    int val = 55;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 55);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_broadcast) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("255.255.255.255", 0, "0.0.0.0", 0, 17);
    int val = 77;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 77);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_zero_ports) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("10.0.0.1", 0, "10.0.0.2", 0, 6);
    int val = 33;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 33);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_high_ports) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("10.0.0.1", 65535, "10.0.0.2", 65534, 6);
    int val = 44;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 44);
    addr_hash_teardown(h);
}

/* === IPv6 === */

TEST(addr_hash_ipv6_insert_find) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv6_pair("::1", 80, "fe80::1", 443, 6);
    int val = 99;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 99);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv6_find_miss) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv6_pair("::1", 80, "::2", 443, 6);
    addr_pair ap2 = make_ipv6_pair("::1", 80, "::3", 443, 6);
    int val = 1;
    addr_hash_insert(h, &ap1, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv6_link_local) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv6_pair("fe80::1", 1234, "fe80::2", 5678, 6);
    int val = 88;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 88);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv6_global_unicast) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv6_pair("2001:db8::1", 80, "2001:db8::2", 443, 6);
    int val = 66;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 66);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv6_multicast) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv6_pair("ff02::1", 0, "::1", 0, 17);
    int val = 22;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 22);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv6_full_address) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv6_pair("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 80,
                                  "2001:0db8:85a3:0000:0000:8a2e:0370:7335", 443, 6);
    int val = 11;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 11);
    addr_hash_teardown(h);
}

/* === Delete === */

TEST(addr_hash_delete_node_works) {
    hash_type *h = addr_hash_create();
    addr_pair ap1 = make_ipv4_pair("1.1.1.1", 100, "2.2.2.2", 200, 6);
    addr_pair ap2 = make_ipv4_pair("3.3.3.3", 300, "4.4.4.4", 400, 6);
    int v1 = 10, v2 = 20;
    addr_hash_insert(h, &ap1, &v1);
    addr_hash_insert(h, &ap2, &v2);
    hash_node_type *node = NULL;
    hash_node_type *target = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        if (memcmp(node->key, &ap1, sizeof(addr_pair)) == 0) {
            target = node;
            break;
        }
    }
    ASSERT_NOT_NULL(target);
    addr_hash_delete_node(h, target);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap1, &rec), HASH_STATUS_KEY_NOT_FOUND);
    ASSERT_EQ(addr_hash_find(h, &ap2, &rec), HASH_STATUS_OK);
    addr_hash_teardown(h);
}

TEST(addr_hash_delete_only_entry) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("1.2.3.4", 80, "5.6.7.8", 443, 6);
    int val = 1;
    addr_hash_insert(h, &ap, &val);
    hash_node_type *node = NULL;
    hash_next_item(h, &node);
    ASSERT_NOT_NULL(node);
    addr_hash_delete_node(h, node);
    ASSERT_EQ(addr_hash_count(h), 0);
    addr_hash_teardown(h);
}

TEST(addr_hash_delete_all_free_clears) {
    hash_type *h = addr_hash_create();
    for (int i = 0; i < 50; i++) {
        char src[16];
        snprintf(src, sizeof(src), "10.0.%d.%d", i / 256, i % 256);
        addr_pair ap = make_ipv4_pair(src, 1000, "10.0.0.1", 80, 6);
        int *val = malloc(sizeof(int));
        *val = i;
        addr_hash_insert(h, &ap, val);
    }
    addr_hash_delete_all_free(h);
    ASSERT_EQ(addr_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

/* === Generic interface === */

TEST(addr_hash_generic_find) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("8.8.8.8", 53, "192.168.0.1", 5000, 17);
    int val = 55;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 55);
    addr_hash_teardown(h);
}

TEST(addr_hash_generic_insert_then_fast_find) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("10.0.0.1", 80, "10.0.0.2", 443, 6);
    int val = 77;
    hash_insert(h, &ap, &val);
    void *rec = NULL;
    /* generic insert uses copy_key; fast find uses memcmp */
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 77);
    /* Need to use generic delete since generic insert used malloc for node */
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === Iteration === */

TEST(addr_hash_iterate_empty) {
    hash_type *h = addr_hash_create();
    ASSERT_EQ(addr_hash_count(h), 0);
    addr_hash_teardown(h);
}

TEST(addr_hash_iterate_single) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 80, 6);
    int val = 1;
    addr_hash_insert(h, &ap, &val);
    ASSERT_EQ(addr_hash_count(h), 1);
    addr_hash_teardown(h);
}

TEST(addr_hash_iterate_10) {
    hash_type *h = addr_hash_create();
    int vals[10];
    for (int i = 0; i < 10; i++) {
        char src[16];
        snprintf(src, sizeof(src), "10.0.0.%d", i + 1);
        addr_pair ap = make_ipv4_pair(src, 1000 + i, "10.0.0.100", 80, 6);
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 10);
    addr_hash_teardown(h);
}

TEST(addr_hash_iterate_100) {
    hash_type *h = addr_hash_create();
    int vals[100];
    for (int i = 0; i < 100; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(0x0a000000 + i);
        ap.dst.s_addr = htonl(0x0a000100);
        ap.src_port = 1000 + i;
        ap.dst_port = 80;
        ap.protocol = 6;
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 100);
    addr_hash_teardown(h);
}

/* === Pool stress === */

TEST(addr_hash_pool_stress) {
    hash_type *h = addr_hash_create();
    for (int round = 0; round < 3; round++) {
        int vals[500];
        for (int i = 0; i < 500; i++) {
            addr_pair ap;
            memset(&ap, 0, sizeof(ap));
            ap.address_family = AF_INET;
            ap.src.s_addr = htonl(i);
            ap.dst.s_addr = htonl(i + 1000);
            ap.src_port = i;
            ap.dst_port = 80;
            ap.protocol = 6;
            vals[i] = i;
            addr_hash_insert(h, &ap, &vals[i]);
        }
        ASSERT_EQ(addr_hash_count(h), 500);
        addr_hash_clear(h);
    }
    hash_destroy(h);
    free(h);
}

TEST(addr_hash_pool_block_boundary) {
    /* Pool allocates 256 items per block; test crossing that boundary */
    hash_type *h = addr_hash_create();
    int vals[300];
    for (int i = 0; i < 300; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(i);
        ap.src_port = i;
        ap.protocol = 6;
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 300);
    /* Verify all findable */
    for (int i = 0; i < 300; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(i);
        ap.src_port = i;
        ap.protocol = 6;
        void *rec = NULL;
        ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    addr_hash_teardown(h);
}

/* === Mixed IPv4/IPv6 === */

TEST(addr_hash_mixed_protocols) {
    hash_type *h = addr_hash_create();
    addr_pair ipv4 = make_ipv4_pair("1.2.3.4", 80, "5.6.7.8", 443, 6);
    addr_pair ipv6 = make_ipv6_pair("2001:db8::1", 80, "2001:db8::2", 443, 6);
    int v4 = 4, v6 = 6;
    addr_hash_insert(h, &ipv4, &v4);
    addr_hash_insert(h, &ipv6, &v6);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ipv4, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 4);
    ASSERT_EQ(addr_hash_find(h, &ipv6, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 6);
    addr_hash_teardown(h);
}

TEST(addr_hash_mixed_many) {
    hash_type *h = addr_hash_create();
    int vals[20];
    for (int i = 0; i < 10; i++) {
        char src[16];
        snprintf(src, sizeof(src), "10.0.0.%d", i + 1);
        addr_pair ap = make_ipv4_pair(src, 1000 + i, "10.0.0.100", 80, 6);
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    for (int i = 0; i < 10; i++) {
        char src[64];
        snprintf(src, sizeof(src), "2001:db8::%d", i + 1);
        addr_pair ap = make_ipv6_pair(src, 2000 + i, "2001:db8::100", 443, 6);
        vals[10 + i] = 10 + i;
        addr_hash_insert(h, &ap, &vals[10 + i]);
    }
    ASSERT_EQ(addr_hash_count(h), 20);
    addr_hash_teardown(h);
}

TEST(addr_hash_ipv4_vs_ipv6_same_ports) {
    /* Same ports but different AF should be different entries */
    hash_type *h = addr_hash_create();
    addr_pair ipv4 = make_ipv4_pair("0.0.0.1", 80, "0.0.0.2", 443, 6);
    addr_pair ipv6 = make_ipv6_pair("::1", 80, "::2", 443, 6);
    int v4 = 4, v6 = 6;
    addr_hash_insert(h, &ipv4, &v4);
    addr_hash_insert(h, &ipv6, &v6);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ipv4, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 4);
    ASSERT_EQ(addr_hash_find(h, &ipv6, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 6);
    ASSERT_EQ(addr_hash_count(h), 2);
    addr_hash_teardown(h);
}

/* === Edge cases === */

TEST(addr_hash_find_empty) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 80, 6);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_KEY_NOT_FOUND);
    addr_hash_teardown(h);
}

TEST(addr_hash_insert_same_twice) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 80, 6);
    int v1 = 1, v2 = 2;
    addr_hash_insert(h, &ap, &v1);
    addr_hash_insert(h, &ap, &v2);
    /* Most recent should be found first */
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    ASSERT_EQ(addr_hash_count(h), 2);
    addr_hash_teardown(h);
}

TEST(addr_hash_zero_addresses) {
    hash_type *h = addr_hash_create();
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET;
    int val = 0;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 0);
    addr_hash_teardown(h);
}

/* === Key independence === */

TEST(addr_hash_key_is_copied) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 443, 6);
    int val = 55;
    addr_hash_insert(h, &ap, &val);
    /* Modify the original key */
    ap.src_port = 9999;
    ap.dst.s_addr = 0;
    /* Original should still be findable */
    addr_pair lookup = make_ipv4_pair("1.1.1.1", 80, "2.2.2.2", 443, 6);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &lookup, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 55);
    addr_hash_teardown(h);
}

/* === Large scale === */

TEST(addr_hash_1000_entries) {
    hash_type *h = addr_hash_create();
    int vals[1000];
    for (int i = 0; i < 1000; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(0x0a000000 + i);
        ap.dst.s_addr = htonl(0xc0a80001);
        ap.src_port = 1024 + i;
        ap.dst_port = 80;
        ap.protocol = 6;
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 1000);
    /* Spot check */
    for (int i = 0; i < 1000; i += 100) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(0x0a000000 + i);
        ap.dst.s_addr = htonl(0xc0a80001);
        ap.src_port = 1024 + i;
        ap.dst_port = 80;
        ap.protocol = 6;
        void *rec = NULL;
        ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    addr_hash_teardown(h);
}

/* === Delete and reinsert cycle === */

TEST(addr_hash_delete_reinsert_cycle) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("10.0.0.1", 80, "10.0.0.2", 443, 6);
    for (int i = 0; i < 50; i++) {
        int val = i;
        addr_hash_insert(h, &ap, &val);
        hash_node_type *node = NULL;
        hash_next_item(h, &node);
        ASSERT_NOT_NULL(node);
        addr_hash_delete_node(h, node);
        ASSERT_EQ(addr_hash_count(h), 0);
    }
    addr_hash_teardown(h);
}

/* === Pool allocator multi-block === */

TEST(addr_hash_pool_multi_block) {
    /* Allocate more than 2 blocks worth (256 items per block) */
    hash_type *h = addr_hash_create();
    int vals[600];
    for (int i = 0; i < 600; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(i);
        ap.dst.s_addr = htonl(i + 10000);
        ap.src_port = i;
        ap.dst_port = 80;
        ap.protocol = 6;
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 600);
    /* Delete all and verify empty */
    addr_hash_clear(h);
    ASSERT_EQ(addr_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

/* === IPv6 large scale === */

TEST(addr_hash_ipv6_100_entries) {
    hash_type *h = addr_hash_create();
    int vals[100];
    for (int i = 0; i < 100; i++) {
        char src[64], dst[64];
        snprintf(src, sizeof(src), "2001:db8::%x", i + 1);
        snprintf(dst, sizeof(dst), "2001:db8::ff%x", i + 1);
        addr_pair ap = make_ipv6_pair(src, 1000 + i, dst, 443, 6);
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 100);
    /* Verify a few */
    char src[64], dst[64];
    snprintf(src, sizeof(src), "2001:db8::1");
    snprintf(dst, sizeof(dst), "2001:db8::ff1");
    addr_pair lookup = make_ipv6_pair(src, 1000, dst, 443, 6);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &lookup, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 0);
    addr_hash_teardown(h);
}

/* === UDP protocol === */

TEST(addr_hash_udp_flow) {
    hash_type *h = addr_hash_create();
    addr_pair ap = make_ipv4_pair("8.8.8.8", 53, "192.168.1.1", 5000, 17);
    int val = 100;
    addr_hash_insert(h, &ap, &val);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 100);
    addr_hash_teardown(h);
}

/* === Same addresses different protocols are distinct === */

TEST(addr_hash_tcp_udp_distinct) {
    hash_type *h = addr_hash_create();
    addr_pair tcp = make_ipv4_pair("10.0.0.1", 80, "10.0.0.2", 443, 6);
    addr_pair udp = make_ipv4_pair("10.0.0.1", 80, "10.0.0.2", 443, 17);
    int v_tcp = 1, v_udp = 2;
    addr_hash_insert(h, &tcp, &v_tcp);
    addr_hash_insert(h, &udp, &v_udp);
    ASSERT_EQ(addr_hash_count(h), 2);
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(h, &tcp, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 1);
    ASSERT_EQ(addr_hash_find(h, &udp, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    addr_hash_teardown(h);
}

/* === Pool reuse after clear === */

TEST(addr_hash_pool_reuse_after_clear) {
    hash_type *h = addr_hash_create();
    /* Fill then clear, then fill again - pool should reuse freed nodes */
    for (int round = 0; round < 5; round++) {
        int vals[100];
        for (int i = 0; i < 100; i++) {
            addr_pair ap;
            memset(&ap, 0, sizeof(ap));
            ap.address_family = AF_INET;
            ap.src.s_addr = htonl(i);
            ap.dst.s_addr = htonl(i + 1000);
            ap.src_port = i;
            ap.dst_port = 80;
            ap.protocol = 6;
            vals[i] = round * 100 + i;
            addr_hash_insert(h, &ap, &vals[i]);
        }
        ASSERT_EQ(addr_hash_count(h), 100);
        addr_hash_clear(h);
        ASSERT_EQ(addr_hash_count(h), 0);
    }
    hash_destroy(h);
    free(h);
}

/* === 2000 entries stress === */

TEST(addr_hash_2000_entries) {
    hash_type *h = addr_hash_create();
    int vals[2000];
    for (int i = 0; i < 2000; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(0x0a000000 + i);
        ap.dst.s_addr = htonl(0xc0a80001 + (i % 256));
        ap.src_port = 1024 + (i % 60000);
        ap.dst_port = 80 + (i % 10);
        ap.protocol = 6;
        vals[i] = i;
        addr_hash_insert(h, &ap, &vals[i]);
    }
    ASSERT_EQ(addr_hash_count(h), 2000);
    /* Spot check */
    for (int i = 0; i < 2000; i += 200) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(0x0a000000 + i);
        ap.dst.s_addr = htonl(0xc0a80001 + (i % 256));
        ap.src_port = 1024 + (i % 60000);
        ap.dst_port = 80 + (i % 10);
        ap.protocol = 6;
        void *rec = NULL;
        ASSERT_EQ(addr_hash_find(h, &ap, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    addr_hash_teardown(h);
}

/* === addr_hash_delete_all_free === */

TEST(addr_hash_delete_all_free_with_malloc_recs) {
    hash_type *h = addr_hash_create();
    for (int i = 0; i < 20; i++) {
        addr_pair ap;
        memset(&ap, 0, sizeof(ap));
        ap.address_family = AF_INET;
        ap.src.s_addr = htonl(i);
        ap.protocol = 6;
        int *val = malloc(sizeof(int));
        *val = i;
        addr_hash_insert(h, &ap, val);
    }
    addr_hash_delete_all_free(h);
    ASSERT_EQ(addr_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

/* === IPv6 multiple subnets === */

TEST(addr_hash_ipv6_multiple_subnets) {
    hash_type *h = addr_hash_create();
    int vals[30];
    char *subnets[] = {"2001:db8:1::", "2001:db8:2::", "2001:db8:3::"};
    for (int s = 0; s < 3; s++) {
        for (int i = 0; i < 10; i++) {
            char src[64], dst[64];
            snprintf(src, sizeof(src), "%s%d", subnets[s], i + 1);
            snprintf(dst, sizeof(dst), "%s%d", subnets[s], i + 100);
            addr_pair ap = make_ipv6_pair(src, 1000 + i, dst, 443, 6);
            vals[s * 10 + i] = s * 10 + i;
            addr_hash_insert(h, &ap, &vals[s * 10 + i]);
        }
    }
    ASSERT_EQ(addr_hash_count(h), 30);
    addr_hash_teardown(h);
}

/* === Delete half then verify remainder === */

TEST(addr_hash_delete_half) {
    hash_type *h = addr_hash_create();
    int vals[20];
    addr_pair pairs[20];
    for (int i = 0; i < 20; i++) {
        memset(&pairs[i], 0, sizeof(addr_pair));
        pairs[i].address_family = AF_INET;
        pairs[i].src.s_addr = htonl(i);
        pairs[i].dst.s_addr = htonl(i + 100);
        pairs[i].src_port = i;
        pairs[i].dst_port = 80;
        pairs[i].protocol = 6;
        vals[i] = i;
        addr_hash_insert(h, &pairs[i], &vals[i]);
    }
    /* Delete even entries via iteration */
    for (int i = 0; i < 20; i += 2) {
        void *rec = NULL;
        if (addr_hash_find(h, &pairs[i], &rec) == HASH_STATUS_OK) {
            hash_node_type *node = NULL;
            while (hash_next_item(h, &node) == HASH_STATUS_OK) {
                if (node->record == &vals[i]) {
                    addr_hash_delete_node(h, node);
                    break;
                }
            }
        }
    }
    ASSERT_EQ(addr_hash_count(h), 10);
    /* Verify odd entries remain */
    for (int i = 1; i < 20; i += 2) {
        void *rec = NULL;
        ASSERT_EQ(addr_hash_find(h, &pairs[i], &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    addr_hash_teardown(h);
}

int main(void) {
    TEST_SUITE("Addr Hash Tests");

    RUN(addr_hash_create_succeeds);
    RUN(addr_hash_create_table_zeroed);
    RUN(addr_hash_create_has_function_pointers);
    RUN(addr_hash_ipv4_insert_find);
    RUN(addr_hash_ipv4_find_miss);
    RUN(addr_hash_ipv4_different_src_ports);
    RUN(addr_hash_ipv4_different_dst_ports);
    RUN(addr_hash_ipv4_different_src_addr);
    RUN(addr_hash_ipv4_different_dst_addr);
    RUN(addr_hash_ipv4_different_protocol);
    RUN(addr_hash_ipv4_loopback);
    RUN(addr_hash_ipv4_broadcast);
    RUN(addr_hash_ipv4_zero_ports);
    RUN(addr_hash_ipv4_high_ports);
    RUN(addr_hash_ipv6_insert_find);
    RUN(addr_hash_ipv6_find_miss);
    RUN(addr_hash_ipv6_link_local);
    RUN(addr_hash_ipv6_global_unicast);
    RUN(addr_hash_ipv6_multicast);
    RUN(addr_hash_ipv6_full_address);
    RUN(addr_hash_delete_node_works);
    RUN(addr_hash_delete_only_entry);
    RUN(addr_hash_delete_all_free_clears);
    RUN(addr_hash_generic_find);
    RUN(addr_hash_generic_insert_then_fast_find);
    RUN(addr_hash_iterate_empty);
    RUN(addr_hash_iterate_single);
    RUN(addr_hash_iterate_10);
    RUN(addr_hash_iterate_100);
    RUN(addr_hash_pool_stress);
    RUN(addr_hash_pool_block_boundary);
    RUN(addr_hash_mixed_protocols);
    RUN(addr_hash_mixed_many);
    RUN(addr_hash_ipv4_vs_ipv6_same_ports);
    RUN(addr_hash_find_empty);
    RUN(addr_hash_insert_same_twice);
    RUN(addr_hash_zero_addresses);
    RUN(addr_hash_key_is_copied);
    RUN(addr_hash_1000_entries);
    RUN(addr_hash_delete_reinsert_cycle);
    RUN(addr_hash_pool_multi_block);
    RUN(addr_hash_ipv6_100_entries);
    RUN(addr_hash_udp_flow);
    RUN(addr_hash_tcp_udp_distinct);
    RUN(addr_hash_pool_reuse_after_clear);
    RUN(addr_hash_2000_entries);
    RUN(addr_hash_delete_all_free_with_malloc_recs);
    RUN(addr_hash_ipv6_multiple_subnets);
    RUN(addr_hash_delete_half);

    TEST_REPORT();
}
