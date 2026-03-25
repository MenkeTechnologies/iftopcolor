/*
 * test_serv_hash.c: tests for serv_hash (port+protocol to service name)
 */

#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "serv_hash.h"
#include "iftop.h"

static int serv_hash_count(hash_type *h) {
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) count++;
    return count;
}

/* === Creation === */

TEST(serv_hash_create_succeeds) {
    hash_type *h = serv_hash_create();
    ASSERT_NOT_NULL(h);
    ASSERT_EQ(h->size, 123);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_create_empty) {
    hash_type *h = serv_hash_create();
    ASSERT_EQ(serv_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_create_has_functions) {
    hash_type *h = serv_hash_create();
    ASSERT_NOT_NULL(h->compare);
    ASSERT_NOT_NULL(h->hash);
    ASSERT_NOT_NULL(h->copy_key);
    ASSERT_NOT_NULL(h->delete_key);
    hash_destroy(h);
    free(h);
}

/* === Insert/Find === */

TEST(serv_hash_insert_find) {
    hash_type *h = serv_hash_create();
    ip_service svc = {80, 6};
    hash_insert(h, &svc, xstrdup("http"));
    void *rec = NULL;
    ip_service lookup = {80, 6};
    ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "http");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_find_miss) {
    hash_type *h = serv_hash_create();
    ip_service svc = {9999, 6};
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_find_empty) {
    hash_type *h = serv_hash_create();
    ip_service svc = {80, 6};
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_different_protocols) {
    hash_type *h = serv_hash_create();
    ip_service tcp = {53, 6};
    ip_service udp = {53, 17};
    hash_insert(h, &tcp, xstrdup("domain-tcp"));
    hash_insert(h, &udp, xstrdup("domain-udp"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &tcp, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "domain-tcp");
    ASSERT_EQ(hash_find(h, &udp, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "domain-udp");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_same_protocol_different_ports) {
    hash_type *h = serv_hash_create();
    ip_service s1 = {22, 6};
    ip_service s2 = {80, 6};
    ip_service s3 = {443, 6};
    hash_insert(h, &s1, xstrdup("ssh"));
    hash_insert(h, &s2, xstrdup("http"));
    hash_insert(h, &s3, xstrdup("https"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &s1, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "ssh");
    ASSERT_EQ(hash_find(h, &s2, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "http");
    ASSERT_EQ(hash_find(h, &s3, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "https");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_port_zero) {
    hash_type *h = serv_hash_create();
    ip_service svc = {0, 6};
    hash_insert(h, &svc, xstrdup("zero"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "zero");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_port_65535) {
    hash_type *h = serv_hash_create();
    ip_service svc = {65535, 6};
    hash_insert(h, &svc, xstrdup("max-port"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "max-port");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_multiple_services) {
    hash_type *h = serv_hash_create();
    ip_service services[] = {{22, 6}, {80, 6}, {443, 6}, {53, 17}, {123, 17}};
    char *names[] = {"ssh", "http", "https", "dns", "ntp"};
    for (int i = 0; i < 5; i++)
        hash_insert(h, &services[i], xstrdup(names[i]));
    for (int i = 0; i < 5; i++) {
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &services[i], &rec), HASH_STATUS_OK);
        ASSERT_STR_EQ((char *)rec, names[i]);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Delete === */

TEST(serv_hash_delete_service) {
    hash_type *h = serv_hash_create();
    ip_service svc = {8080, 6};
    hash_insert(h, &svc, xstrdup("http-alt"));
    ASSERT_EQ(hash_delete(h, &svc), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_delete_missing) {
    hash_type *h = serv_hash_create();
    ip_service svc = {9999, 6};
    ASSERT_EQ(hash_delete(h, &svc), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_delete_then_reinsert) {
    hash_type *h = serv_hash_create();
    ip_service svc = {80, 6};
    hash_insert(h, &svc, xstrdup("http"));
    hash_delete(h, &svc);
    hash_insert(h, &svc, xstrdup("http-new"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "http-new");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_delete_preserves_others) {
    hash_type *h = serv_hash_create();
    ip_service s1 = {22, 6}, s2 = {80, 6}, s3 = {443, 6};
    hash_insert(h, &s1, xstrdup("ssh"));
    hash_insert(h, &s2, xstrdup("http"));
    hash_insert(h, &s3, xstrdup("https"));
    hash_delete(h, &s2);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &s1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, &s2, &rec), HASH_STATUS_KEY_NOT_FOUND);
    ASSERT_EQ(hash_find(h, &s3, &rec), HASH_STATUS_OK);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Iteration === */

TEST(serv_hash_iterate_count) {
    hash_type *h = serv_hash_create();
    for (int i = 0; i < 10; i++) {
        ip_service svc = {1000 + i, 6};
        char name[16];
        snprintf(name, sizeof(name), "svc_%d", i);
        hash_insert(h, &svc, xstrdup(name));
    }
    ASSERT_EQ(serv_hash_count(h), 10);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_iterate_empty) {
    hash_type *h = serv_hash_create();
    ASSERT_EQ(serv_hash_count(h), 0);
    hash_destroy(h);
    free(h);
}

/* === System services === */

TEST(serv_hash_initialise_from_system) {
    hash_type *h = serv_hash_create();
    serv_hash_initialise(h);
    ASSERT(serv_hash_count(h) > 0);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(serv_hash_initialise_does_not_crash) {
    hash_type *h = serv_hash_create();
    serv_hash_initialise(h);
    /* Count may be 0 if /etc/services is empty or sandbox-restricted */
    ASSERT(serv_hash_count(h) >= 0);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Key copy === */

TEST(serv_hash_key_is_copied) {
    hash_type *h = serv_hash_create();
    ip_service svc = {80, 6};
    hash_insert(h, &svc, xstrdup("http"));
    svc.port = 443; /* modify original */
    ip_service lookup = {80, 6};
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "http");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Large scale === */

TEST(serv_hash_50_entries) {
    hash_type *h = serv_hash_create();
    for (int i = 0; i < 50; i++) {
        ip_service svc = {(unsigned short)(2000 + i), 6};
        char name[32];
        snprintf(name, sizeof(name), "service_%d", i);
        hash_insert(h, &svc, xstrdup(name));
    }
    ASSERT_EQ(serv_hash_count(h), 50);
    for (int i = 0; i < 50; i++) {
        ip_service lookup = {(unsigned short)(2000 + i), 6};
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_OK);
        char expected[32];
        snprintf(expected, sizeof(expected), "service_%d", i);
        ASSERT_STR_EQ((char *)rec, expected);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Delete all then reuse === */

TEST(serv_hash_reuse_after_clear) {
    hash_type *h = serv_hash_create();
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 10; i++) {
            ip_service svc = {(unsigned short)(3000 + i), 6};
            char name[16];
            snprintf(name, sizeof(name), "svc_%d", i);
            hash_insert(h, &svc, xstrdup(name));
        }
        ASSERT_EQ(serv_hash_count(h), 10);
        hash_delete_all_free(h);
        ASSERT_EQ(serv_hash_count(h), 0);
    }
    hash_destroy(h);
    free(h);
}

/* === Protocol 0 edge case === */

TEST(serv_hash_protocol_zero) {
    hash_type *h = serv_hash_create();
    ip_service svc = {80, 0};
    hash_insert(h, &svc, xstrdup("proto0"));
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &svc, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)rec, "proto0");
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

/* === Many protocols same port === */

TEST(serv_hash_many_protocols_same_port) {
    hash_type *h = serv_hash_create();
    /* Insert same port with different protocol numbers */
    for (int p = 0; p < 10; p++) {
        ip_service svc = {80, (unsigned short)p};
        char name[16];
        snprintf(name, sizeof(name), "proto_%d", p);
        hash_insert(h, &svc, xstrdup(name));
    }
    ASSERT_EQ(serv_hash_count(h), 10);
    for (int p = 0; p < 10; p++) {
        ip_service lookup = {80, (unsigned short)p};
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_OK);
        char expected[16];
        snprintf(expected, sizeof(expected), "proto_%d", p);
        ASSERT_STR_EQ((char *)rec, expected);
    }
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

int main(void) {
    TEST_SUITE("Serv Hash Tests");

    RUN(serv_hash_create_succeeds);
    RUN(serv_hash_create_empty);
    RUN(serv_hash_create_has_functions);
    RUN(serv_hash_insert_find);
    RUN(serv_hash_find_miss);
    RUN(serv_hash_find_empty);
    RUN(serv_hash_different_protocols);
    RUN(serv_hash_same_protocol_different_ports);
    RUN(serv_hash_port_zero);
    RUN(serv_hash_port_65535);
    RUN(serv_hash_multiple_services);
    RUN(serv_hash_delete_service);
    RUN(serv_hash_delete_missing);
    RUN(serv_hash_delete_then_reinsert);
    RUN(serv_hash_delete_preserves_others);
    RUN(serv_hash_iterate_count);
    RUN(serv_hash_iterate_empty);
    RUN(serv_hash_initialise_from_system);
    RUN(serv_hash_initialise_does_not_crash);
    RUN(serv_hash_key_is_copied);
    RUN(serv_hash_50_entries);
    RUN(serv_hash_reuse_after_clear);
    RUN(serv_hash_protocol_zero);
    RUN(serv_hash_many_protocols_same_port);

    TEST_REPORT();
}
