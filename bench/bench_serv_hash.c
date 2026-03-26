/*
 * bench_serv_hash.c: benchmarks for serv_table (port+protocol to service name)
 *
 * Covers: insert, lookup (hit/miss), delete, and scaling.
 */

#include <stdint.h>
#include "bench.h"
#include "serv_hash.h"
#include "iftop.h"

#define MAX_SERVICES 5000

static void bench_insert(void) {
    BENCH_SECTION("Serv Table - Insert");

    int sizes[] = {50, 100, 200, 500, 1000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d services", n);
        BENCH_RUN(label, 1000, {
            serv_table *t = serv_table_create();
            for (int j = 0; j < n; j++) {
                char name[32];
                snprintf(name, sizeof(name), "svc_%d", j);
                serv_table_insert(t, 1024 + j, (j % 2 == 0) ? 6 : 17, name);
            }
            serv_table_destroy(t);
        });
    }
}

static void bench_lookup_hit(void) {
    BENCH_SECTION("Serv Table - Lookup (hit)");

    int sizes[] = {50, 100, 200, 500};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        serv_table *t = serv_table_create();
        for (int i = 0; i < n; i++) {
            char name[32];
            snprintf(name, sizeof(name), "svc_%d", i);
            serv_table_insert(t, 1024 + i, (i % 2 == 0) ? 6 : 17, name);
        }

        char label[64];
        snprintf(label, sizeof(label), "lookup hit (%d entries)", n);
        BENCH_RUN(label, 1000000, {
            int idx = _i % n;
            bench_use((int)(intptr_t)serv_table_lookup(t, 1024 + idx, (idx % 2 == 0) ? 6 : 17));
        });

        serv_table_destroy(t);
    }
}

static void bench_lookup_miss(void) {
    BENCH_SECTION("Serv Table - Lookup (miss)");

    int n = 200;
    serv_table *t = serv_table_create();
    for (int i = 0; i < n; i++) {
        char name[32];
        snprintf(name, sizeof(name), "svc_%d", i);
        serv_table_insert(t, 1024 + i, 6, name);
    }

    BENCH_RUN("lookup miss (200 entries)", 1000000,
              { bench_use((int)(intptr_t)serv_table_lookup(t, 50000 + (_i % 1000), 6)); });

    serv_table_destroy(t);
}

static void bench_delete(void) {
    BENCH_SECTION("Serv Table - Insert + Delete");

    BENCH_RUN("insert 200 + delete all", 1000, {
        serv_table *t = serv_table_create();
        for (int j = 0; j < 200; j++) {
            char name[32];
            snprintf(name, sizeof(name), "svc_%d", j);
            serv_table_insert(t, 1024 + j, 6, name);
        }
        serv_table_destroy(t);
    });

    BENCH_RUN("insert 200 + delete individually", 1000, {
        serv_table *t = serv_table_create();
        for (int j = 0; j < 200; j++) {
            char name[32];
            snprintf(name, sizeof(name), "svc_%d", j);
            serv_table_insert(t, 1024 + j, 6, name);
        }
        for (int j = 0; j < 200; j++) {
            serv_table_delete(t, 1024 + j, 6);
        }
        serv_table_destroy(t);
    });
}

static void bench_protocol_lookup(void) {
    BENCH_SECTION("Serv Table - Protocol Differentiation");

    serv_table *t = serv_table_create();
    for (int port = 1; port <= 100; port++) {
        char name_tcp[32], name_udp[32];
        snprintf(name_tcp, sizeof(name_tcp), "tcp_%d", port);
        snprintf(name_udp, sizeof(name_udp), "udp_%d", port);
        serv_table_insert(t, port, 6, name_tcp);
        serv_table_insert(t, port, 17, name_udp);
    }

    BENCH_RUN("lookup TCP vs UDP same port (200 entries)", 1000000, {
        int port = (_i % 100) + 1;
        int proto = (_i % 2 == 0) ? 6 : 17;
        bench_use((int)(intptr_t)serv_table_lookup(t, port, proto));
    });

    serv_table_destroy(t);
}

int main(void) {
    BENCH_HEADER("SERV_TABLE SERVICE CACHE PROFILER");

    bench_insert();
    bench_lookup_hit();
    bench_lookup_miss();
    bench_delete();
    bench_protocol_lookup();

    BENCH_FOOTER();
    return 0;
}
