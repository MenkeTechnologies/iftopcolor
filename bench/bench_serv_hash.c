/*
 * bench_serv_hash.c: benchmarks for serv_hash (port+protocol to service name)
 *
 * Covers: insert, find (hit/miss), delete, iteration, and scaling.
 */

#include "bench.h"
#include "serv_hash.h"
#include "hash.h"
#include "iftop.h"

#define MAX_SERVICES 5000
static ip_service services[MAX_SERVICES];
static char svc_names[MAX_SERVICES][32];

static void generate_services(void) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        services[i].port = (unsigned short)(1024 + i);
        services[i].protocol = (i % 2 == 0) ? 6 : 17; /* alternate TCP/UDP */
        snprintf(svc_names[i], sizeof(svc_names[i]), "svc_%d", i);
    }
}

static int serv_hash_count(hash_type *h) {
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) count++;
    return count;
}

static void bench_insert(void) {
    BENCH_SECTION("Serv Hash - Insert");

    int sizes[] = {50, 100, 200, 500, 1000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d services", n);
        BENCH_RUN(label, 1000, {
            hash_type *h = serv_hash_create();
            for (int j = 0; j < n; j++)
                hash_insert(h, &services[j], xstrdup(svc_names[j]));
            hash_delete_all_free(h);
            hash_destroy(h);
            free(h);
        });
    }
}

static void bench_find_hit(void) {
    BENCH_SECTION("Serv Hash - Find (hit)");

    int sizes[] = {50, 100, 200, 500};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        hash_type *h = serv_hash_create();
        for (int i = 0; i < n; i++)
            hash_insert(h, &services[i], xstrdup(svc_names[i]));

        char label[64];
        snprintf(label, sizeof(label), "find hit (%d entries)", n);
        BENCH_RUN(label, 1000000, {
            void *rec;
            bench_use(hash_find(h, &services[_i % n], &rec));
        });

        hash_delete_all_free(h);
        hash_destroy(h);
        free(h);
    }
}

static void bench_find_miss(void) {
    BENCH_SECTION("Serv Hash - Find (miss)");

    int n = 200;
    hash_type *h = serv_hash_create();
    for (int i = 0; i < n; i++)
        hash_insert(h, &services[i], xstrdup(svc_names[i]));

    BENCH_RUN("find miss (200 entries)", 1000000, {
        void *rec;
        ip_service miss;
        miss.port = (unsigned short)(50000 + (_i % 1000));
        miss.protocol = 6;
        bench_use(hash_find(h, &miss, &rec));
    });

    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

static void bench_delete(void) {
    BENCH_SECTION("Serv Hash - Insert + Delete");

    BENCH_RUN("insert 200 + delete all", 1000, {
        hash_type *h = serv_hash_create();
        for (int j = 0; j < 200; j++)
            hash_insert(h, &services[j], xstrdup(svc_names[j]));
        hash_delete_all_free(h);
        hash_destroy(h);
        free(h);
    });

    BENCH_RUN("insert 200 + delete individually", 1000, {
        hash_type *h = serv_hash_create();
        for (int j = 0; j < 200; j++)
            hash_insert(h, &services[j], xstrdup(svc_names[j]));
        for (int j = 0; j < 200; j++)
            hash_delete(h, &services[j]);
        hash_destroy(h);
        free(h);
    });
}

static void bench_iteration(void) {
    BENCH_SECTION("Serv Hash - Iteration");

    int n = 500;
    hash_type *h = serv_hash_create();
    for (int i = 0; i < n; i++)
        hash_insert(h, &services[i], xstrdup(svc_names[i]));

    BENCH_RUN("iterate 500 entries", 100000, {
        int count = 0;
        hash_node_type *node = NULL;
        while (hash_next_item(h, &node) == HASH_STATUS_OK)
            count++;
        bench_use(count);
    });

    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

static void bench_protocol_lookup(void) {
    BENCH_SECTION("Serv Hash - Protocol Differentiation");

    /* Same port, different protocols */
    hash_type *h = serv_hash_create();
    for (int port = 1; port <= 100; port++) {
        ip_service tcp = {(unsigned short)port, 6};
        ip_service udp = {(unsigned short)port, 17};
        char name_tcp[32], name_udp[32];
        snprintf(name_tcp, sizeof(name_tcp), "tcp_%d", port);
        snprintf(name_udp, sizeof(name_udp), "udp_%d", port);
        hash_insert(h, &tcp, xstrdup(name_tcp));
        hash_insert(h, &udp, xstrdup(name_udp));
    }

    BENCH_RUN("find TCP vs UDP same port (200 entries)", 1000000, {
        void *rec;
        int port = (_i % 100) + 1;
        ip_service lookup;
        lookup.port = (unsigned short)port;
        lookup.protocol = (_i % 2 == 0) ? 6 : 17;
        bench_use(hash_find(h, &lookup, &rec));
    });

    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

int main(void) {
    printf("iftopcolor serv_hash benchmark suite\n");
    printf("=====================================\n");

    generate_services();

    bench_insert();
    bench_find_hit();
    bench_find_miss();
    bench_delete();
    bench_iteration();
    bench_protocol_lookup();

    printf("\nDone.\n");
    return 0;
}
