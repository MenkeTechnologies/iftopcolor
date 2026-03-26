/*
 * test_serv_hash.c: tests for serv_table (port+protocol to service name)
 */

#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "serv_hash.h"
#include "iftop.h"

/* === Creation === */

TEST(serv_table_create_succeeds) {
    serv_table *t = serv_table_create();
    ASSERT_NOT_NULL(t);
    serv_table_destroy(t);
}

TEST(serv_table_create_empty) {
    serv_table *t = serv_table_create();
    ASSERT_EQ(serv_table_count(t), 0);
    serv_table_destroy(t);
}

/* === Insert/Lookup === */

TEST(serv_table_insert_lookup) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 80, 6, "http");
    const char *name = serv_table_lookup(t, 80, 6);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "http");
    serv_table_destroy(t);
}

TEST(serv_table_lookup_miss) {
    serv_table *t = serv_table_create();
    ASSERT(serv_table_lookup(t, 9999, 6) == NULL);
    serv_table_destroy(t);
}

TEST(serv_table_lookup_empty) {
    serv_table *t = serv_table_create();
    ASSERT(serv_table_lookup(t, 80, 6) == NULL);
    serv_table_destroy(t);
}

TEST(serv_table_different_protocols) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 53, 6, "domain-tcp");
    serv_table_insert(t, 53, 17, "domain-udp");
    const char *tcp = serv_table_lookup(t, 53, 6);
    const char *udp = serv_table_lookup(t, 53, 17);
    ASSERT_NOT_NULL(tcp);
    ASSERT_STR_EQ(tcp, "domain-tcp");
    ASSERT_NOT_NULL(udp);
    ASSERT_STR_EQ(udp, "domain-udp");
    serv_table_destroy(t);
}

TEST(serv_table_same_protocol_different_ports) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 22, 6, "ssh");
    serv_table_insert(t, 80, 6, "http");
    serv_table_insert(t, 443, 6, "https");
    ASSERT_STR_EQ(serv_table_lookup(t, 22, 6), "ssh");
    ASSERT_STR_EQ(serv_table_lookup(t, 80, 6), "http");
    ASSERT_STR_EQ(serv_table_lookup(t, 443, 6), "https");
    serv_table_destroy(t);
}

TEST(serv_table_port_zero) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 0, 6, "zero");
    ASSERT_STR_EQ(serv_table_lookup(t, 0, 6), "zero");
    serv_table_destroy(t);
}

TEST(serv_table_port_65535) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 65535, 6, "max-port");
    ASSERT_STR_EQ(serv_table_lookup(t, 65535, 6), "max-port");
    serv_table_destroy(t);
}

TEST(serv_table_multiple_services) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 22, 6, "ssh");
    serv_table_insert(t, 80, 6, "http");
    serv_table_insert(t, 443, 6, "https");
    serv_table_insert(t, 53, 17, "dns");
    serv_table_insert(t, 123, 17, "ntp");
    ASSERT_STR_EQ(serv_table_lookup(t, 22, 6), "ssh");
    ASSERT_STR_EQ(serv_table_lookup(t, 80, 6), "http");
    ASSERT_STR_EQ(serv_table_lookup(t, 443, 6), "https");
    ASSERT_STR_EQ(serv_table_lookup(t, 53, 17), "dns");
    ASSERT_STR_EQ(serv_table_lookup(t, 123, 17), "ntp");
    serv_table_destroy(t);
}

/* === Delete === */

TEST(serv_table_delete_service) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 8080, 6, "http-alt");
    ASSERT_EQ(serv_table_delete(t, 8080, 6), 1);
    ASSERT(serv_table_lookup(t, 8080, 6) == NULL);
    serv_table_destroy(t);
}

TEST(serv_table_delete_missing) {
    serv_table *t = serv_table_create();
    ASSERT_EQ(serv_table_delete(t, 9999, 6), 0);
    serv_table_destroy(t);
}

TEST(serv_table_delete_then_reinsert) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 80, 6, "http");
    serv_table_delete(t, 80, 6);
    serv_table_insert(t, 80, 6, "http-new");
    ASSERT_STR_EQ(serv_table_lookup(t, 80, 6), "http-new");
    serv_table_destroy(t);
}

TEST(serv_table_delete_preserves_others) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 22, 6, "ssh");
    serv_table_insert(t, 80, 6, "http");
    serv_table_insert(t, 443, 6, "https");
    serv_table_delete(t, 80, 6);
    ASSERT_STR_EQ(serv_table_lookup(t, 22, 6), "ssh");
    ASSERT(serv_table_lookup(t, 80, 6) == NULL);
    ASSERT_STR_EQ(serv_table_lookup(t, 443, 6), "https");
    serv_table_destroy(t);
}

/* === Count === */

TEST(serv_table_count_tracks) {
    serv_table *t = serv_table_create();
    for (int i = 0; i < 10; i++) {
        serv_table_insert(t, 1000 + i, 6, "svc");
    }
    ASSERT_EQ(serv_table_count(t), 10);
    serv_table_destroy(t);
}

/* === System services === */

TEST(serv_table_init_from_system) {
    serv_table *t = serv_table_create();
    serv_table_init(t);
    ASSERT(serv_table_count(t) > 0);
    serv_table_destroy(t);
}

TEST(serv_table_init_does_not_crash) {
    serv_table *t = serv_table_create();
    serv_table_init(t);
    /* Count may be 0 if /etc/services is empty or sandbox-restricted */
    ASSERT(serv_table_count(t) >= 0);
    serv_table_destroy(t);
}

/* === Overwrite === */

TEST(serv_table_overwrite) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 80, 6, "http");
    serv_table_insert(t, 80, 6, "http-new");
    ASSERT_STR_EQ(serv_table_lookup(t, 80, 6), "http-new");
    serv_table_destroy(t);
}

/* === Large scale === */

TEST(serv_table_50_entries) {
    serv_table *t = serv_table_create();
    for (int i = 0; i < 50; i++) {
        char name[32];
        snprintf(name, sizeof(name), "service_%d", i);
        serv_table_insert(t, 2000 + i, 6, name);
    }
    ASSERT_EQ(serv_table_count(t), 50);
    for (int i = 0; i < 50; i++) {
        char expected[32];
        snprintf(expected, sizeof(expected), "service_%d", i);
        ASSERT_STR_EQ(serv_table_lookup(t, 2000 + i, 6), expected);
    }
    serv_table_destroy(t);
}

/* === Protocol 0 edge case (uses fallback hash) === */

TEST(serv_table_protocol_zero) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 80, 0, "proto0");
    ASSERT_STR_EQ(serv_table_lookup(t, 80, 0), "proto0");
    serv_table_destroy(t);
}

/* === Adjacent ports === */

TEST(serv_table_adjacent_ports) {
    serv_table *t = serv_table_create();
    for (int i = 0; i < 10; i++) {
        char name[16];
        snprintf(name, sizeof(name), "port_%d", 8080 + i);
        serv_table_insert(t, 8080 + i, 6, name);
    }
    for (int i = 0; i < 10; i++) {
        char expected[16];
        snprintf(expected, sizeof(expected), "port_%d", 8080 + i);
        ASSERT_STR_EQ(serv_table_lookup(t, 8080 + i, 6), expected);
    }
    serv_table_destroy(t);
}

/* === Delete multiple preserves remainder === */

TEST(serv_table_delete_multiple_preserves) {
    serv_table *t = serv_table_create();
    for (int i = 0; i < 5; i++) {
        char name[16];
        snprintf(name, sizeof(name), "svc_%d", i);
        serv_table_insert(t, 5000 + i, 6, name);
    }
    serv_table_delete(t, 5000, 6);
    serv_table_delete(t, 5004, 6);
    ASSERT_EQ(serv_table_count(t), 3);
    ASSERT(serv_table_lookup(t, 5000, 6) == NULL);
    ASSERT_STR_EQ(serv_table_lookup(t, 5002, 6), "svc_2");
    ASSERT(serv_table_lookup(t, 5004, 6) == NULL);
    serv_table_destroy(t);
}

/* === Long service name === */

TEST(serv_table_long_service_name) {
    serv_table *t = serv_table_create();
    char long_name[256];
    memset(long_name, 'z', 255);
    long_name[255] = '\0';
    serv_table_insert(t, 8888, 6, long_name);
    ASSERT_STR_EQ(serv_table_lookup(t, 8888, 6), long_name);
    serv_table_destroy(t);
}

/* === 100 entries === */

TEST(serv_table_100_entries) {
    serv_table *t = serv_table_create();
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "service_%d_%s", i, (i % 2 == 0) ? "tcp" : "udp");
        serv_table_insert(t, 4000 + i, (i % 2 == 0) ? 6 : 17, name);
    }
    ASSERT_EQ(serv_table_count(t), 100);
    /* Spot check */
    ASSERT_STR_EQ(serv_table_lookup(t, 4050, 6), "service_50_tcp");
    serv_table_destroy(t);
}

/* === Empty name === */

TEST(serv_table_empty_name) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 7777, 6, "");
    ASSERT_STR_EQ(serv_table_lookup(t, 7777, 6), "");
    serv_table_destroy(t);
}

/* === Many protocols same port (uses fallback hash for non-TCP/UDP) === */

TEST(serv_table_many_protocols_same_port) {
    serv_table *t = serv_table_create();
    for (int p = 0; p < 10; p++) {
        char name[16];
        snprintf(name, sizeof(name), "proto_%d", p);
        serv_table_insert(t, 80, p, name);
    }
    ASSERT_EQ(serv_table_count(t), 10);
    for (int p = 0; p < 10; p++) {
        char expected[16];
        snprintf(expected, sizeof(expected), "proto_%d", p);
        ASSERT_STR_EQ(serv_table_lookup(t, 80, p), expected);
    }
    serv_table_destroy(t);
}

/* === UDP direct array === */

TEST(serv_table_udp_lookup) {
    serv_table *t = serv_table_create();
    serv_table_insert(t, 53, 17, "domain");
    serv_table_insert(t, 123, 17, "ntp");
    serv_table_insert(t, 5353, 17, "mdns");
    ASSERT_STR_EQ(serv_table_lookup(t, 53, 17), "domain");
    ASSERT_STR_EQ(serv_table_lookup(t, 123, 17), "ntp");
    ASSERT_STR_EQ(serv_table_lookup(t, 5353, 17), "mdns");
    serv_table_destroy(t);
}

int main(void) {
    TEST_SUITE("Serv Table Tests");

    RUN(serv_table_create_succeeds);
    RUN(serv_table_create_empty);
    RUN(serv_table_insert_lookup);
    RUN(serv_table_lookup_miss);
    RUN(serv_table_lookup_empty);
    RUN(serv_table_different_protocols);
    RUN(serv_table_same_protocol_different_ports);
    RUN(serv_table_port_zero);
    RUN(serv_table_port_65535);
    RUN(serv_table_multiple_services);
    RUN(serv_table_delete_service);
    RUN(serv_table_delete_missing);
    RUN(serv_table_delete_then_reinsert);
    RUN(serv_table_delete_preserves_others);
    RUN(serv_table_count_tracks);
    RUN(serv_table_init_from_system);
    RUN(serv_table_init_does_not_crash);
    RUN(serv_table_overwrite);
    RUN(serv_table_50_entries);
    RUN(serv_table_protocol_zero);
    RUN(serv_table_adjacent_ports);
    RUN(serv_table_delete_multiple_preserves);
    RUN(serv_table_long_service_name);
    RUN(serv_table_100_entries);
    RUN(serv_table_empty_name);
    RUN(serv_table_many_protocols_same_port);
    RUN(serv_table_udp_lookup);

    TEST_REPORT();
}
