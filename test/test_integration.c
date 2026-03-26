/*
 * test_integration.c: cross-module integration tests
 *
 * Verifies that multiple subsystems work correctly together:
 *   - addr_hash + sorted_list (flow tracking pipeline)
 *   - addr_hash + serv_hash + ns_hash (address/port resolution)
 *   - cfgfile + stringmap (config file end-to-end)
 *   - vector + hash (collect-and-iterate workflows)
 *   - history_type lifecycle (packet accumulation pipeline)
 */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "test.h"
#include "addr_hash.h"
#include "hash.h"
#include "ns_hash.h"
#include "serv_hash.h"
#include "sorted_list.h"
#include "vector.h"
#include "stringmap.h"
#include "cfgfile.h"
#include "iftop.h"

/* === Helpers === */

static addr_pair make_ipv4(const char *src, uint16_t sport,
                           const char *dst, uint16_t dport, uint16_t proto) {
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

static addr_pair make_ipv6(const char *src, uint16_t sport,
                           const char *dst, uint16_t dport, uint16_t proto) {
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

static void addr_hash_teardown(hash_type *h) {
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        addr_hash_delete_node(h, node);
        node = NULL;
    }
    hash_destroy(h);
    free(h);
}

extern stringmap config;

static char tmpfile_path[256];

static void write_tmp_config(const char *content) {
    snprintf(tmpfile_path, sizeof(tmpfile_path), "/tmp/iftop_integ_XXXXXX");
    int fd = mkstemp(tmpfile_path);
    write(fd, content, strlen(content));
    close(fd);
}

static void cleanup_tmp(void) {
    unlink(tmpfile_path);
}

/* ================================================================
 *  SUITE 1: addr_hash + sorted_list (flow tracking pipeline)
 *
 *  Simulates: packets arrive -> stored in addr_hash with history ->
 *  iterated out -> sorted by bandwidth for display
 * ================================================================ */

static int compare_bandwidth_desc(void *a, void *b) {
    long bw_a = *(long *)a;
    long bw_b = *(long *)b;
    /* sorted_list contract: return > 0 when a should go after b.
     * For descending order: a goes after b when a < b. */
    return bw_a < bw_b ? 1 : 0;
}

TEST(flow_track_insert_and_sort) {
    hash_type *flows = addr_hash_create();
    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    /* Simulate 5 flows with different bandwidths */
    struct { const char *src; const char *dst; uint16_t sport; uint16_t dport; long bw; } data[] = {
        {"10.0.0.1", "192.168.1.1",  12345, 80,   5000},
        {"10.0.0.2", "192.168.1.2",  23456, 443,  1000},
        {"10.0.0.3", "192.168.1.3",  34567, 8080, 9000},
        {"10.0.0.4", "192.168.1.4",  45678, 22,   3000},
        {"10.0.0.5", "192.168.1.5",  56789, 53,   7000},
    };
    int n = sizeof(data) / sizeof(data[0]);

    /* Insert flows into addr_hash */
    long *bw_records = xmalloc(n * sizeof(long));
    for (int i = 0; i < n; i++) {
        addr_pair ap = make_ipv4(data[i].src, data[i].sport,
                                 data[i].dst, data[i].dport, 6);
        bw_records[i] = data[i].bw;
        ASSERT_EQ(addr_hash_insert(flows, &ap, &bw_records[i]), HASH_STATUS_OK);
    }

    /* Iterate addr_hash and insert bandwidths into sorted list */
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        long *bw = (long *)node->record;
        sorted_list_insert(&slist, bw);
    }

    /* Verify sorted order: 9000, 7000, 5000, 3000, 1000 */
    long expected[] = {9000, 7000, 5000, 3000, 1000};
    sorted_list_node *snode = NULL;
    int idx = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL) {
        long val = *(long *)snode->data;
        ASSERT_EQ(val, expected[idx]);
        idx++;
    }
    ASSERT_EQ(idx, 5);

    sorted_list_destroy(&slist);
    addr_hash_teardown(flows);
    free(bw_records);
}

TEST(flow_track_update_and_resort) {
    hash_type *flows = addr_hash_create();

    /* Insert a flow */
    addr_pair ap = make_ipv4("10.0.0.1", 1234, "10.0.0.2", 80, 6);
    long bw = 100;
    addr_hash_insert(flows, &ap, &bw);

    /* Find and update */
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(long *)rec, 100);

    /* Simulate bandwidth increase */
    *(long *)rec = 500;

    /* Verify update persisted */
    rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &ap, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(long *)rec, 500);

    addr_hash_teardown(flows);
}

TEST(flow_track_mixed_protocols) {
    hash_type *flows = addr_hash_create();

    /* Same src/dst pair but TCP vs UDP */
    addr_pair tcp_flow = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair udp_flow = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 17);

    long tcp_bw = 1000, udp_bw = 2000;
    addr_hash_insert(flows, &tcp_flow, &tcp_bw);
    addr_hash_insert(flows, &udp_flow, &udp_bw);

    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &tcp_flow, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(long *)rec, 1000);

    rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &udp_flow, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(long *)rec, 2000);

    /* Sort them */
    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        sorted_list_insert(&slist, node->record);
    }

    sorted_list_node *snode = sorted_list_next_item(&slist, NULL);
    ASSERT_NOT_NULL(snode);
    ASSERT_EQ(*(long *)snode->data, 2000); /* UDP first */
    snode = sorted_list_next_item(&slist, snode);
    ASSERT_NOT_NULL(snode);
    ASSERT_EQ(*(long *)snode->data, 1000); /* TCP second */

    sorted_list_destroy(&slist);
    addr_hash_teardown(flows);
}

TEST(flow_track_batch_sort) {
    hash_type *flows = addr_hash_create();
    int n = 200;
    long *bw_records = xmalloc(n * sizeof(long));

    for (int i = 0; i < n; i++) {
        char src[16], dst[16];
        snprintf(src, sizeof(src), "10.0.%d.%d", i / 256, i % 256);
        snprintf(dst, sizeof(dst), "192.168.%d.%d", i / 256, i % 256);
        addr_pair ap = make_ipv4(src, 1000 + i, dst, 80, 6);
        bw_records[i] = (long)(n - i) * 100; /* descending bandwidth */
        addr_hash_insert(flows, &ap, &bw_records[i]);
    }

    /* Collect into array for batch insert */
    void **items = xmalloc(n * sizeof(void *));
    hash_node_type *node = NULL;
    int count = 0;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        items[count++] = node->record;
    }
    ASSERT_EQ(count, n);

    /* Batch insert into sorted list */
    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;
    sorted_list_insert_batch(&slist, items, count);

    /* Verify descending order */
    sorted_list_node *snode = NULL;
    long prev = 999999;
    int verified = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL) {
        long val = *(long *)snode->data;
        ASSERT(val <= prev);
        prev = val;
        verified++;
    }
    ASSERT_EQ(verified, n);

    sorted_list_destroy(&slist);
    addr_hash_teardown(flows);
    free(items);
    free(bw_records);
}

/* ================================================================
 *  SUITE 2: addr_hash + serv_hash + ns_hash (resolution pipeline)
 *
 *  Simulates: flow identified -> resolve port names via serv_hash ->
 *  resolve hostnames via ns_hash -> combine into display string
 * ================================================================ */

TEST(resolve_tcp_flow_ports) {
    hash_type *flows = addr_hash_create();
    serv_table *services = serv_table_create();

    /* Register well-known services */
    serv_table_insert(services, 80, 6, "http");
    serv_table_insert(services, 443, 6, "https");
    serv_table_insert(services, 22, 6, "ssh");

    /* Insert flows */
    addr_pair ap1 = make_ipv4("10.0.0.1", 50000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4("10.0.0.1", 50001, "10.0.0.3", 443, 6);
    addr_pair ap3 = make_ipv4("10.0.0.1", 50002, "10.0.0.4", 22, 6);

    long bw1 = 100, bw2 = 200, bw3 = 300;
    addr_hash_insert(flows, &ap1, &bw1);
    addr_hash_insert(flows, &ap2, &bw2);
    addr_hash_insert(flows, &ap3, &bw3);

    /* Resolve ports for each flow */
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
        ASSERT_NOT_NULL(svc);

        if (key->dst_port == 80) ASSERT_STR_EQ(svc, "http");
        else if (key->dst_port == 443) ASSERT_STR_EQ(svc, "https");
        else if (key->dst_port == 22) ASSERT_STR_EQ(svc, "ssh");
    }

    serv_table_destroy(services);
    addr_hash_teardown(flows);
}

TEST(resolve_mixed_protocol_services) {
    serv_table *services = serv_table_create();

    /* Same port, different protocols */
    serv_table_insert(services, 53, 6, "domain-tcp");
    serv_table_insert(services, 53, 17, "domain-udp");

    hash_type *flows = addr_hash_create();
    addr_pair tcp53 = make_ipv4("10.0.0.1", 5000, "8.8.8.8", 53, 6);
    addr_pair udp53 = make_ipv4("10.0.0.1", 5000, "8.8.8.8", 53, 17);

    long bw = 100;
    addr_hash_insert(flows, &tcp53, &bw);
    addr_hash_insert(flows, &udp53, &bw);

    /* TCP DNS flow resolves to domain-tcp */
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &tcp53, &rec), HASH_STATUS_OK);
    ASSERT_STR_EQ(serv_table_lookup(services, 53, 6), "domain-tcp");

    /* UDP DNS flow resolves to domain-udp */
    ASSERT_STR_EQ(serv_table_lookup(services, 53, 17), "domain-udp");

    serv_table_destroy(services);
    addr_hash_teardown(flows);
}

TEST(resolve_ipv6_hostname_cache) {
    hash_type *ns_cache = ns_hash_create();
    hash_type *flows = addr_hash_create();

    /* Insert IPv6 flow */
    addr_pair ap = make_ipv6("2001:db8::1", 5000, "2001:db8::2", 80, 6);
    long bw = 500;
    addr_hash_insert(flows, &ap, &bw);

    /* Cache hostname for the source address */
    struct in6_addr src_addr;
    inet_pton(AF_INET6, "2001:db8::1", &src_addr);
    char *hostname = xstrdup("client.example.com");
    hash_insert(ns_cache, &src_addr, hostname);

    /* Look up hostname from flow's source */
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(flows, &node), HASH_STATUS_OK);
    addr_pair *key = (addr_pair *)node->key;
    ASSERT_EQ(key->address_family, AF_INET6);

    void *cached = NULL;
    ASSERT_EQ(hash_find(ns_cache, &key->src6, &cached), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)cached, "client.example.com");

    hash_delete_all_free(ns_cache);
    hash_destroy(ns_cache);
    free(ns_cache);
    addr_hash_teardown(flows);
}

TEST(resolve_full_pipeline) {
    /* Full resolution: flow -> port name + hostname */
    hash_type *flows = addr_hash_create();
    hash_type *ns_cache = ns_hash_create();
    serv_table *services = serv_table_create();

    serv_table_insert(services, 443, 6, "https");

    /* Cache IPv6 hostnames */
    struct in6_addr addr1, addr2;
    inet_pton(AF_INET6, "2001:db8::10", &addr1);
    inet_pton(AF_INET6, "2001:db8::20", &addr2);
    hash_insert(ns_cache, &addr1, xstrdup("laptop.local"));
    hash_insert(ns_cache, &addr2, xstrdup("server.example.com"));

    /* Insert flow */
    addr_pair ap = make_ipv6("2001:db8::10", 55000, "2001:db8::20", 443, 6);
    long bw = 1500;
    addr_hash_insert(flows, &ap, &bw);

    /* Resolve everything */
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(flows, &node), HASH_STATUS_OK);
    addr_pair *key = (addr_pair *)node->key;

    /* Port resolution */
    const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
    ASSERT_STR_EQ(svc, "https");

    /* Hostname resolution */
    void *src_host = NULL, *dst_host = NULL;
    ASSERT_EQ(hash_find(ns_cache, &key->src6, &src_host), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(ns_cache, &key->dst6, &dst_host), HASH_STATUS_OK);
    ASSERT_STR_EQ((char *)src_host, "laptop.local");
    ASSERT_STR_EQ((char *)dst_host, "server.example.com");

    serv_table_destroy(services);
    hash_delete_all_free(ns_cache);
    hash_destroy(ns_cache);
    free(ns_cache);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 3: cfgfile + stringmap + typed access (config pipeline)
 *
 *  Simulates: config file on disk -> parse -> stringmap -> typed
 *  access (bool, int, float, enum) -> used to drive behavior
 * ================================================================ */

TEST(config_pipeline_full_config) {
    config_init();
    write_tmp_config(
        "interface: eth0\n"
        "dns-resolution: true\n"
        "port-resolution: true\n"
        "show-bars: yes\n"
        "promiscuous: false\n"
        "use-bytes: true\n"
        "max-bandwidth: 1000\n"
        "log-scale: false\n"
    );
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);

    /* String access */
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");

    /* Boolean access */
    ASSERT_EQ(config_get_bool("dns-resolution"), 1);
    ASSERT_EQ(config_get_bool("show-bars"), 1);
    ASSERT_EQ(config_get_bool("promiscuous"), 0);
    ASSERT_EQ(config_get_bool("use-bytes"), 1);
    ASSERT_EQ(config_get_bool("log-scale"), 0);

    /* Integer access */
    int bw = 0;
    ASSERT_EQ(config_get_int("max-bandwidth", &bw), 1);
    ASSERT_EQ(bw, 1000);

    cleanup_tmp();
}

TEST(config_pipeline_cli_overrides_file) {
    config_init();

    /* Simulate CLI setting interface before config file read */
    config_set_string("interface", "wlan0");
    config_set_string("promiscuous", "true");

    write_tmp_config(
        "interface: eth0\n"
        "promiscuous: false\n"
        "dns-resolution: true\n"
    );
    read_config(tmpfile_path, 0);

    /* CLI values take precedence */
    ASSERT_STR_EQ(config_get_string("interface"), "wlan0");
    ASSERT_EQ(config_get_bool("promiscuous"), 1);

    /* File-only values still accessible */
    ASSERT_EQ(config_get_bool("dns-resolution"), 1);

    cleanup_tmp();
}

TEST(config_pipeline_enum_from_file) {
    config_init();
    write_tmp_config("sort: source\n");
    read_config(tmpfile_path, 0);

    config_enumeration_type sort_enum[] = {
        {"2s", 0}, {"10s", 1}, {"40s", 2}, {"source", 3}, {"destination", 4},
        {NULL, -1}
    };
    int val = -1;
    ASSERT_EQ(config_get_enum("sort", sort_enum, &val), 1);
    ASSERT_EQ(val, 3);

    cleanup_tmp();
}

TEST(config_pipeline_reinit_and_reload) {
    config_init();
    write_tmp_config("interface: eth0\nuse-bytes: true\n");
    read_config(tmpfile_path, 0);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    cleanup_tmp();

    /* Reinit and load different config */
    config_init();
    write_tmp_config("interface: wlan1\nuse-bytes: false\n");
    read_config(tmpfile_path, 0);
    ASSERT_STR_EQ(config_get_string("interface"), "wlan1");
    ASSERT_EQ(config_get_bool("use-bytes"), 0);

    /* Old values gone */
    ASSERT_NULL(config_get_string("dns-resolution"));

    cleanup_tmp();
}

TEST(config_pipeline_float_from_file) {
    config_init();
    write_tmp_config("max-bandwidth: 99.5\n");
    read_config(tmpfile_path, 0);

    float val = 0;
    ASSERT_EQ(config_get_float("max-bandwidth", &val), 1);
    ASSERT(val > 99.4 && val < 99.6);

    cleanup_tmp();
}

/* ================================================================
 *  SUITE 4: vector + hash (collect-and-iterate workflows)
 *
 *  Simulates: hash table entries collected into vector for indexed
 *  access (e.g., scrollable display)
 * ================================================================ */

static int str_compare(void *left, void *right) {
    return strcmp((char *)left, (char *)right) == 0;
}
static int str_hash(void *key) {
    char *s = (char *)key;
    unsigned int h = 0;
    while (*s) h = h * 31 + (unsigned char)*s++;
    return h % 64;
}
static void *str_copy_key(void *key) { return xstrdup((char *)key); }
static void str_delete_key(void *key) { free(key); }

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

TEST(hash_to_vector_collect) {
    hash_type *h = create_str_hash();

    hash_insert(h, "alpha", xstrdup("first"));
    hash_insert(h, "beta", xstrdup("second"));
    hash_insert(h, "gamma", xstrdup("third"));
    hash_insert(h, "delta", xstrdup("fourth"));

    /* Collect all values into a vector */
    vector v = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        vector_push_back(v, item_ptr(node->record));
    }
    ASSERT_EQ(v->n_used, 4);

    /* All values should be accessible by index */
    int found_first = 0, found_second = 0, found_third = 0, found_fourth = 0;
    item *it;
    vector_iterate(v, it) {
        char *val = (char *)it->ptr;
        if (strcmp(val, "first") == 0) found_first = 1;
        else if (strcmp(val, "second") == 0) found_second = 1;
        else if (strcmp(val, "third") == 0) found_third = 1;
        else if (strcmp(val, "fourth") == 0) found_fourth = 1;
    }
    ASSERT(found_first && found_second && found_third && found_fourth);

    vector_delete(v);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_to_vector_scrollable_window) {
    hash_type *h = create_str_hash();

    /* Insert 20 entries */
    char keys[20][16], vals[20][32];
    for (int i = 0; i < 20; i++) {
        snprintf(keys[i], sizeof(keys[i]), "host_%02d", i);
        snprintf(vals[i], sizeof(vals[i]), "192.168.1.%d", i);
        hash_insert(h, keys[i], xstrdup(vals[i]));
    }

    /* Collect into vector */
    vector v = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        vector_push_back(v, item_ptr(node->record));
    }
    ASSERT_EQ(v->n_used, 20);

    /* Simulate scrollable window: page of 5 starting at offset 3 */
    int page_start = 3, page_size = 5;
    int visible = 0;
    for (int i = page_start; i < page_start + page_size && (size_t)i < v->n_used; i++) {
        char *val = (char *)v->items[i].ptr;
        ASSERT_NOT_NULL(val);
        visible++;
    }
    ASSERT_EQ(visible, 5);

    vector_delete(v);
    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

TEST(addr_hash_to_vector_with_service_names) {
    hash_type *flows = addr_hash_create();
    serv_table *services = serv_table_create();

    serv_table_insert(services, 80, 6, "http");
    serv_table_insert(services, 443, 6, "https");
    serv_table_insert(services, 22, 6, "ssh");

    addr_pair ap1 = make_ipv4("10.0.0.1", 50000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4("10.0.0.3", 50001, "10.0.0.4", 443, 6);
    addr_pair ap3 = make_ipv4("10.0.0.5", 50002, "10.0.0.6", 22, 6);

    long bw1 = 100, bw2 = 200, bw3 = 300;
    addr_hash_insert(flows, &ap1, &bw1);
    addr_hash_insert(flows, &ap2, &bw2);
    addr_hash_insert(flows, &ap3, &bw3);

    /* Collect flows into vector, verify service lookup for each */
    vector v = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        vector_push_back(v, item_ptr(node));
    }
    ASSERT_EQ(v->n_used, 3);

    int resolved = 0;
    item *it;
    vector_iterate(v, it) {
        hash_node_type *n = (hash_node_type *)it->ptr;
        addr_pair *key = (addr_pair *)n->key;
        const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
        ASSERT_NOT_NULL(svc);
        resolved++;
    }
    ASSERT_EQ(resolved, 3);

    vector_delete(v);
    serv_table_destroy(services);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 5: history_type lifecycle (packet accumulation)
 *
 *  Simulates: the core data flow where packets accumulate into
 *  history_type records stored in addr_hash, then get sorted
 * ================================================================ */

TEST(history_accumulate_and_sort) {
    hash_type *flows = addr_hash_create();

    /* Create history records for 3 flows */
    history_type *h1 = xcalloc(1, sizeof(history_type));
    history_type *h2 = xcalloc(1, sizeof(history_type));
    history_type *h3 = xcalloc(1, sizeof(history_type));

    /* Simulate packet accumulation */
    h1->recv[0] = 5000; h1->sent[0] = 3000; h1->total_recv = 5000; h1->total_sent = 3000;
    h2->recv[0] = 1000; h2->sent[0] = 500;  h2->total_recv = 1000; h2->total_sent = 500;
    h3->recv[0] = 9000; h3->sent[0] = 7000; h3->total_recv = 9000; h3->total_sent = 7000;

    addr_pair ap1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 443, 6);
    addr_pair ap3 = make_ipv4("10.0.0.5", 5002, "10.0.0.6", 22, 6);

    addr_hash_insert(flows, &ap1, h1);
    addr_hash_insert(flows, &ap2, h2);
    addr_hash_insert(flows, &ap3, h3);

    /* Verify retrieval */
    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &ap1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(((history_type *)rec)->total_recv, 5000);

    ASSERT_EQ(addr_hash_find(flows, &ap3, &rec), HASH_STATUS_OK);
    ASSERT_EQ(((history_type *)rec)->total_recv, 9000);

    /* Accumulate more packets into h1 */
    h1->recv[1] = 3000; h1->total_recv += 3000;
    h1->sent[1] = 2000; h1->total_sent += 2000;

    ASSERT_EQ(addr_hash_find(flows, &ap1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(((history_type *)rec)->total_recv, 8000);
    ASSERT_EQ(((history_type *)rec)->total_sent, 5000);

    /* Clean up - free history records before teardown */
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        free(node->record);
        node->record = NULL;
    }
    addr_hash_teardown(flows);
}

TEST(history_circular_buffer) {
    history_type h;
    memset(&h, 0, sizeof(h));

    /* Fill circular buffer */
    for (int i = 0; i < HISTORY_LENGTH; i++) {
        h.recv[i] = (i + 1) * 100;
        h.sent[i] = (i + 1) * 50;
        h.total_recv += h.recv[i];
        h.total_sent += h.sent[i];
    }

    /* Verify totals */
    long expected_recv = 0, expected_sent = 0;
    for (int i = 0; i < HISTORY_LENGTH; i++) {
        expected_recv += (i + 1) * 100;
        expected_sent += (i + 1) * 50;
    }
    ASSERT_EQ((long)h.total_recv, expected_recv);
    ASSERT_EQ((long)h.total_sent, expected_sent);

    /* Overwrite oldest slot (simulating circular use) */
    h.total_recv -= h.recv[0];
    h.recv[0] = 9999;
    h.total_recv += 9999;
    ASSERT_EQ(h.recv[0], 9999);
}

TEST(history_multiple_flows_sorted_by_total) {
    hash_type *flows = addr_hash_create();
    int n = 10;
    history_type *histories = xcalloc(n, sizeof(history_type));

    for (int i = 0; i < n; i++) {
        char src[16];
        snprintf(src, sizeof(src), "10.0.0.%d", i + 1);
        addr_pair ap = make_ipv4(src, 1000 + i, "10.0.0.100", 80, 6);
        histories[i].total_recv = (i + 1) * 1000;
        histories[i].total_sent = (i + 1) * 500;
        addr_hash_insert(flows, &ap, &histories[i]);
    }

    /* Collect total bandwidth into sorted list */
    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    /* Use a temporary array to hold total values */
    long *totals = xmalloc(n * sizeof(long));
    hash_node_type *node = NULL;
    int idx = 0;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        history_type *h = (history_type *)node->record;
        totals[idx] = (long)(h->total_recv + h->total_sent);
        sorted_list_insert(&slist, &totals[idx]);
        idx++;
    }

    /* Verify descending order */
    sorted_list_node *snode = NULL;
    long prev = 999999;
    int count = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL) {
        long val = *(long *)snode->data;
        ASSERT(val <= prev);
        prev = val;
        count++;
    }
    ASSERT_EQ(count, n);

    sorted_list_destroy(&slist);
    free(totals);
    hash_node_type *cleanup_node = NULL;
    while (hash_next_item(flows, &cleanup_node) == HASH_STATUS_OK) {
        cleanup_node->record = NULL;
        /* Don't free - histories is a single block */
    }
    addr_hash_teardown(flows);
    free(histories);
}

/* ================================================================
 *  SUITE 6: IPv4 + IPv6 mixed flow table
 *
 *  Verifies that both address families coexist in the same hash
 *  and resolve correctly through the full pipeline
 * ================================================================ */

TEST(mixed_af_flow_table) {
    hash_type *flows = addr_hash_create();
    serv_table *services = serv_table_create();

    serv_table_insert(services, 80, 6, "http");
    serv_table_insert(services, 443, 6, "https");

    /* Mix of IPv4 and IPv6 flows */
    addr_pair v4_http = make_ipv4("10.0.0.1", 50000, "10.0.0.2", 80, 6);
    addr_pair v6_https = make_ipv6("2001:db8::1", 50001, "2001:db8::2", 443, 6);
    addr_pair v4_https = make_ipv4("172.16.0.1", 50002, "172.16.0.2", 443, 6);
    addr_pair v6_http = make_ipv6("fe80::1", 50003, "fe80::2", 80, 6);

    long bw1 = 100, bw2 = 200, bw3 = 300, bw4 = 400;
    addr_hash_insert(flows, &v4_http, &bw1);
    addr_hash_insert(flows, &v6_https, &bw2);
    addr_hash_insert(flows, &v4_https, &bw3);
    addr_hash_insert(flows, &v6_http, &bw4);

    /* Verify all 4 flows found with correct service names */
    hash_node_type *node = NULL;
    int v4_count = 0, v6_count = 0;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
        ASSERT_NOT_NULL(svc);

        if (key->address_family == AF_INET) v4_count++;
        else if (key->address_family == AF_INET6) v6_count++;

        if (key->dst_port == 80) ASSERT_STR_EQ(svc, "http");
        else ASSERT_STR_EQ(svc, "https");
    }
    ASSERT_EQ(v4_count, 2);
    ASSERT_EQ(v6_count, 2);

    serv_table_destroy(services);
    addr_hash_teardown(flows);
}

TEST(mixed_af_sort_by_bandwidth) {
    hash_type *flows = addr_hash_create();

    addr_pair v4 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair v6 = make_ipv6("2001:db8::1", 5001, "2001:db8::2", 443, 6);

    long bw_v4 = 500, bw_v6 = 1500;
    addr_hash_insert(flows, &v4, &bw_v4);
    addr_hash_insert(flows, &v6, &bw_v6);

    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        sorted_list_insert(&slist, node->record);
    }

    /* IPv6 flow should sort first (higher bandwidth) */
    sorted_list_node *snode = sorted_list_next_item(&slist, NULL);
    ASSERT_NOT_NULL(snode);
    ASSERT_EQ(*(long *)snode->data, 1500);

    snode = sorted_list_next_item(&slist, snode);
    ASSERT_NOT_NULL(snode);
    ASSERT_EQ(*(long *)snode->data, 500);

    sorted_list_destroy(&slist);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 7: stringmap + vector (config-driven list building)
 * ================================================================ */

TEST(stringmap_keys_to_vector) {
    stringmap sm = stringmap_new();

    stringmap_insert(sm, "interface", item_ptr(xstrdup("eth0")));
    stringmap_insert(sm, "dns-resolution", item_ptr(xstrdup("true")));
    stringmap_insert(sm, "port-resolution", item_ptr(xstrdup("false")));
    stringmap_insert(sm, "show-bars", item_ptr(xstrdup("yes")));

    /* Verify all keys findable */
    ASSERT_NOT_NULL(stringmap_find(sm, "interface"));
    ASSERT_NOT_NULL(stringmap_find(sm, "dns-resolution"));
    ASSERT_NOT_NULL(stringmap_find(sm, "port-resolution"));
    ASSERT_NOT_NULL(stringmap_find(sm, "show-bars"));
    ASSERT_NULL(stringmap_find(sm, "nonexistent"));

    /* Access values */
    ASSERT_STR_EQ((char *)stringmap_find(sm, "interface")->data.ptr, "eth0");
    ASSERT_STR_EQ((char *)stringmap_find(sm, "dns-resolution")->data.ptr, "true");

    stringmap_delete_free(sm);
}

TEST(config_drives_serv_table) {
    config_init();
    write_tmp_config("port-resolution: true\n");
    read_config(tmpfile_path, 0);

    int do_port_res = config_get_bool("port-resolution");
    ASSERT_EQ(do_port_res, 1);

    serv_table *services = serv_table_create();
    if (do_port_res) {
        serv_table_insert(services, 80, 6, "http");
        serv_table_insert(services, 443, 6, "https");
    }

    ASSERT_STR_EQ(serv_table_lookup(services, 80, 6), "http");
    ASSERT_STR_EQ(serv_table_lookup(services, 443, 6), "https");

    serv_table_destroy(services);
    cleanup_tmp();
}

TEST(config_disabled_port_resolution) {
    config_init();
    write_tmp_config("port-resolution: false\n");
    read_config(tmpfile_path, 0);

    int do_port_res = config_get_bool("port-resolution");
    ASSERT_EQ(do_port_res, 0);

    serv_table *services = serv_table_create();
    if (do_port_res) {
        serv_table_insert(services, 80, 6, "http");
    }

    /* No services registered when disabled */
    ASSERT_NULL(serv_table_lookup(services, 80, 6));

    serv_table_destroy(services);
    cleanup_tmp();
}

/* ================================================================
 *  SUITE 8: end-to-end flow lifecycle
 *
 *  Full pipeline: config -> flow capture -> history -> sort -> display
 * ================================================================ */

TEST(end_to_end_flow_lifecycle) {
    /* 1. Load config */
    config_init();
    write_tmp_config(
        "dns-resolution: true\n"
        "port-resolution: true\n"
        "use-bytes: true\n"
        "sort: source\n"
    );
    read_config(tmpfile_path, 0);

    /* 2. Set up service table based on config */
    serv_table *services = serv_table_create();
    if (config_get_bool("port-resolution")) {
        serv_table_insert(services, 80, 6, "http");
        serv_table_insert(services, 443, 6, "https");
        serv_table_insert(services, 53, 17, "domain");
    }

    /* 3. Set up flow tracking */
    hash_type *flows = addr_hash_create();

    /* 4. Simulate packet arrivals */
    addr_pair web = make_ipv4("192.168.1.10", 50000, "93.184.216.34", 80, 6);
    addr_pair tls = make_ipv4("192.168.1.10", 50001, "93.184.216.34", 443, 6);
    addr_pair dns = make_ipv4("192.168.1.10", 50002, "8.8.8.8", 53, 17);

    history_type *h_web = xcalloc(1, sizeof(history_type));
    history_type *h_tls = xcalloc(1, sizeof(history_type));
    history_type *h_dns = xcalloc(1, sizeof(history_type));

    h_web->recv[0] = 5000; h_web->sent[0] = 500; h_web->total_recv = 5000; h_web->total_sent = 500;
    h_tls->recv[0] = 8000; h_tls->sent[0] = 1000; h_tls->total_recv = 8000; h_tls->total_sent = 1000;
    h_dns->recv[0] = 200;  h_dns->sent[0] = 100;  h_dns->total_recv = 200;  h_dns->total_sent = 100;

    addr_hash_insert(flows, &web, h_web);
    addr_hash_insert(flows, &tls, h_tls);
    addr_hash_insert(flows, &dns, h_dns);

    /* 5. Collect flows into vector for display */
    vector display = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        vector_push_back(display, item_ptr(node));
    }
    ASSERT_EQ(display->n_used, 3);

    /* 6. Resolve service names for each flow */
    item *it;
    vector_iterate(display, it) {
        hash_node_type *n = (hash_node_type *)it->ptr;
        addr_pair *key = (addr_pair *)n->key;
        const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
        ASSERT_NOT_NULL(svc);
    }

    /* 7. Sort by total bandwidth */
    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    long totals[3];
    int idx = 0;
    vector_iterate(display, it) {
        hash_node_type *n = (hash_node_type *)it->ptr;
        history_type *h = (history_type *)n->record;
        totals[idx] = (long)(h->total_recv + h->total_sent);
        sorted_list_insert(&slist, &totals[idx]);
        idx++;
    }

    /* TLS (9000) > HTTP (5500) > DNS (300) */
    sorted_list_node *snode = sorted_list_next_item(&slist, NULL);
    ASSERT_EQ(*(long *)snode->data, 9000);
    snode = sorted_list_next_item(&slist, snode);
    ASSERT_EQ(*(long *)snode->data, 5500);
    snode = sorted_list_next_item(&slist, snode);
    ASSERT_EQ(*(long *)snode->data, 300);

    /* 8. Cleanup */
    sorted_list_destroy(&slist);
    vector_delete(display);

    node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        free(node->record);
        node->record = NULL;
    }
    addr_hash_teardown(flows);
    serv_table_destroy(services);
    cleanup_tmp();
}

/* ================================================================
 *  SUITE 9: Flow churn (insert/delete/reinsert)
 *
 *  Simulates: connections come and go, addr_hash must handle
 *  the full lifecycle of entries being added, removed, re-added
 * ================================================================ */

TEST(flow_churn_delete_and_reinsert) {
    hash_type *flows = addr_hash_create();

    /* Insert 20 flows */
    addr_pair pairs[20];
    long bw_records[20];
    for (int i = 0; i < 20; i++) {
        char src[16];
        snprintf(src, sizeof(src), "10.0.0.%d", i + 1);
        pairs[i] = make_ipv4(src, 5000 + i, "10.0.0.100", 80, 6);
        bw_records[i] = (i + 1) * 100;
        addr_hash_insert(flows, &pairs[i], &bw_records[i]);
    }

    /* Delete odd-indexed flows */
    for (int i = 1; i < 20; i += 2) {
        hash_node_type *node = NULL;
        while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
            addr_pair *key = (addr_pair *)node->key;
            if (key->src_port == (uint16_t)(5000 + i)) {
                addr_hash_delete_node(flows, node);
                break;
            }
        }
    }

    /* Verify 10 remain */
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) count++;
    ASSERT_EQ(count, 10);

    /* Reinsert deleted flows with new bandwidth */
    long new_bw[10];
    for (int i = 1; i < 20; i += 2) {
        new_bw[i / 2] = 9999;
        addr_hash_insert(flows, &pairs[i], &new_bw[i / 2]);
    }

    /* Verify all 20 present again */
    count = 0;
    node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) count++;
    ASSERT_EQ(count, 20);

    /* Verify reinserted flows have new bandwidth */
    for (int i = 1; i < 20; i += 2) {
        void *rec = NULL;
        ASSERT_EQ(addr_hash_find(flows, &pairs[i], &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(long *)rec, 9999);
    }

    /* Verify original flows kept their bandwidth */
    for (int i = 0; i < 20; i += 2) {
        void *rec = NULL;
        ASSERT_EQ(addr_hash_find(flows, &pairs[i], &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(long *)rec, (i + 1) * 100);
    }

    addr_hash_teardown(flows);
}

TEST(flow_churn_complete_replacement) {
    hash_type *flows = addr_hash_create();

    /* Insert initial set */
    addr_pair p1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair p2 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 443, 6);
    long bw1 = 100, bw2 = 200;
    addr_hash_insert(flows, &p1, &bw1);
    addr_hash_insert(flows, &p2, &bw2);

    /* Delete all */
    addr_hash_teardown(flows);

    /* Fresh hash, new flows */
    flows = addr_hash_create();
    addr_pair p3 = make_ipv6("2001:db8::1", 6000, "2001:db8::2", 22, 6);
    addr_pair p4 = make_ipv6("2001:db8::3", 6001, "2001:db8::4", 53, 17);
    long bw3 = 300, bw4 = 400;
    addr_hash_insert(flows, &p3, &bw3);
    addr_hash_insert(flows, &p4, &bw4);

    void *rec = NULL;
    ASSERT_EQ(addr_hash_find(flows, &p3, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(long *)rec, 300);
    ASSERT_EQ(addr_hash_find(flows, &p4, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(long *)rec, 400);

    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 10: Vector filtering (collect from hash, filter in vector)
 *
 *  Simulates: all flows collected into display vector, then
 *  low-bandwidth entries removed before rendering
 * ================================================================ */

TEST(vector_filter_low_bandwidth) {
    hash_type *flows = addr_hash_create();

    long bw_vals[] = {50, 5000, 10, 8000, 200, 12000, 3};
    int n = sizeof(bw_vals) / sizeof(bw_vals[0]);
    long bw_records[7];

    for (int i = 0; i < n; i++) {
        char src[16];
        snprintf(src, sizeof(src), "10.0.0.%d", i + 1);
        addr_pair ap = make_ipv4(src, 3000 + i, "10.1.1.1", 80, 6);
        bw_records[i] = bw_vals[i];
        addr_hash_insert(flows, &ap, &bw_records[i]);
    }

    /* Collect bandwidth values into vector */
    vector v = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK)
        vector_push_back(v, item_long(*(long *)node->record));
    ASSERT_EQ(v->n_used, 7);

    /* Filter: remove entries below 1000 */
    item *it = v->items;
    while (it < v->items + v->n_used) {
        if (it->num < 1000)
            it = vector_remove(v, it);
        else
            it++;
    }

    /* Should have 3 entries: 5000, 8000, 12000 */
    ASSERT_EQ(v->n_used, 3);
    long sum = 0;
    vector_iterate(v, it) { sum += it->num; }
    ASSERT_EQ(sum, 5000 + 8000 + 12000);

    vector_delete(v);
    addr_hash_teardown(flows);
}

TEST(vector_filter_by_protocol) {
    hash_type *flows = addr_hash_create();

    /* Mix of TCP and UDP */
    addr_pair tcp1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair udp1 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 53, 17);
    addr_pair tcp2 = make_ipv4("10.0.0.5", 5002, "10.0.0.6", 443, 6);
    addr_pair udp2 = make_ipv4("10.0.0.7", 5003, "10.0.0.8", 123, 17);

    long bw1 = 100, bw2 = 200, bw3 = 300, bw4 = 400;
    addr_hash_insert(flows, &tcp1, &bw1);
    addr_hash_insert(flows, &udp1, &bw2);
    addr_hash_insert(flows, &tcp2, &bw3);
    addr_hash_insert(flows, &udp2, &bw4);

    /* Collect flow nodes into vector */
    vector v = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK)
        vector_push_back(v, item_ptr(node));

    ASSERT_EQ(v->n_used, 4);

    /* Filter: keep only TCP (protocol 6) */
    item *it = v->items;
    while (it < v->items + v->n_used) {
        hash_node_type *n = (hash_node_type *)it->ptr;
        addr_pair *key = (addr_pair *)n->key;
        if (key->protocol != 6)
            it = vector_remove(v, it);
        else
            it++;
    }

    ASSERT_EQ(v->n_used, 2);

    /* Verify remaining are all TCP */
    vector_iterate(v, it) {
        hash_node_type *n = (hash_node_type *)it->ptr;
        addr_pair *key = (addr_pair *)n->key;
        ASSERT_EQ(key->protocol, 6);
    }

    vector_delete(v);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 11: Top-N display (config-driven limit on sorted output)
 *
 *  Simulates: user sets max-display in config, sorted list is
 *  truncated to top-N for rendering
 * ================================================================ */

TEST(top_n_from_config) {
    config_init();
    write_tmp_config("max-bandwidth: 3\n");
    read_config(tmpfile_path, 0);

    int max_display = 0;
    ASSERT_EQ(config_get_int("max-bandwidth", &max_display), 1);
    ASSERT_EQ(max_display, 3);

    /* Create sorted flows */
    long bw_vals[] = {100, 9000, 500, 7000, 3000, 200, 8500};
    int n = sizeof(bw_vals) / sizeof(bw_vals[0]);

    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    for (int i = 0; i < n; i++)
        sorted_list_insert(&slist, &bw_vals[i]);

    /* Take top-N into vector */
    vector top = vector_new();
    sorted_list_node *snode = NULL;
    int taken = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL && taken < max_display) {
        vector_push_back(top, item_long(*(long *)snode->data));
        taken++;
    }

    ASSERT_EQ(top->n_used, 3);
    ASSERT_EQ(top->items[0].num, 9000);
    ASSERT_EQ(top->items[1].num, 8500);
    ASSERT_EQ(top->items[2].num, 7000);

    vector_delete(top);
    sorted_list_destroy(&slist);
    cleanup_tmp();
}

TEST(top_n_more_than_available) {
    /* Request more entries than exist */
    hash_type *flows = addr_hash_create();

    addr_pair p1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair p2 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 443, 6);
    long bw1 = 500, bw2 = 1000;
    addr_hash_insert(flows, &p1, &bw1);
    addr_hash_insert(flows, &p2, &bw2);

    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK)
        sorted_list_insert(&slist, node->record);

    /* Try to take top 10 but only 2 exist */
    vector top = vector_new();
    sorted_list_node *snode = NULL;
    int taken = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL && taken < 10) {
        vector_push_back(top, item_long(*(long *)snode->data));
        taken++;
    }

    ASSERT_EQ(top->n_used, 2);
    ASSERT_EQ(top->items[0].num, 1000);
    ASSERT_EQ(top->items[1].num, 500);

    vector_delete(top);
    sorted_list_destroy(&slist);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 12: Combined hostname + service resolution
 *
 *  Simulates: for each flow, resolve both the hostname (ns_hash)
 *  and the service name (serv_hash) simultaneously
 * ================================================================ */

TEST(resolve_hostname_and_service_together) {
    hash_type *flows = addr_hash_create();
    hash_type *ns_cache = ns_hash_create();
    serv_table *services = serv_table_create();

    /* Set up hostname cache for IPv6 addresses */
    struct in6_addr src_addr, dst_addr;
    inet_pton(AF_INET6, "2001:db8::10", &src_addr);
    inet_pton(AF_INET6, "2001:db8::20", &dst_addr);
    hash_insert(ns_cache, &src_addr, xstrdup("workstation.local"));
    hash_insert(ns_cache, &dst_addr, xstrdup("cdn.example.com"));

    /* Set up services */
    serv_table_insert(services, 443, 6, "https");
    serv_table_insert(services, 80, 6, "http");

    /* Insert flows */
    addr_pair ap1 = make_ipv6("2001:db8::10", 55000, "2001:db8::20", 443, 6);
    addr_pair ap2 = make_ipv6("2001:db8::10", 55001, "2001:db8::20", 80, 6);
    long bw1 = 5000, bw2 = 1000;
    addr_hash_insert(flows, &ap1, &bw1);
    addr_hash_insert(flows, &ap2, &bw2);

    /* Resolve everything for each flow */
    hash_node_type *node = NULL;
    int resolved = 0;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;

        /* Service resolution */
        const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
        ASSERT_NOT_NULL(svc);

        /* Hostname resolution (source) */
        void *src_name = NULL;
        ASSERT_EQ(hash_find(ns_cache, &key->src6, &src_name), HASH_STATUS_OK);
        ASSERT_STR_EQ((char *)src_name, "workstation.local");

        /* Hostname resolution (destination) */
        void *dst_name = NULL;
        ASSERT_EQ(hash_find(ns_cache, &key->dst6, &dst_name), HASH_STATUS_OK);
        ASSERT_STR_EQ((char *)dst_name, "cdn.example.com");

        if (key->dst_port == 443) ASSERT_STR_EQ(svc, "https");
        else ASSERT_STR_EQ(svc, "http");

        resolved++;
    }
    ASSERT_EQ(resolved, 2);

    serv_table_destroy(services);
    hash_delete_all_free(ns_cache);
    hash_destroy(ns_cache);
    free(ns_cache);
    addr_hash_teardown(flows);
}

TEST(resolve_partial_hostname_cache) {
    /* Some hosts cached, some not — resolver would be called for missing */
    hash_type *flows = addr_hash_create();
    hash_type *ns_cache = ns_hash_create();

    struct in6_addr cached_addr;
    inet_pton(AF_INET6, "2001:db8::1", &cached_addr);
    hash_insert(ns_cache, &cached_addr, xstrdup("known-host.local"));

    /* Two flows: one src cached, one not */
    addr_pair ap1 = make_ipv6("2001:db8::1", 5000, "2001:db8::99", 80, 6);
    addr_pair ap2 = make_ipv6("2001:db8::2", 5001, "2001:db8::99", 80, 6);
    long bw1 = 100, bw2 = 200;
    addr_hash_insert(flows, &ap1, &bw1);
    addr_hash_insert(flows, &ap2, &bw2);

    int cached_count = 0, uncached_count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        void *hostname = NULL;
        if (hash_find(ns_cache, &key->src6, &hostname) == HASH_STATUS_OK) {
            ASSERT_STR_EQ((char *)hostname, "known-host.local");
            cached_count++;
        } else {
            uncached_count++;
        }
    }
    ASSERT_EQ(cached_count, 1);
    ASSERT_EQ(uncached_count, 1);

    hash_delete_all_free(ns_cache);
    hash_destroy(ns_cache);
    free(ns_cache);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 13: Stringmap as runtime config overlay
 *
 *  Simulates: config file provides defaults, CLI args stored in a
 *  stringmap override specific keys at runtime
 * ================================================================ */

TEST(config_overlay_cli_wins) {
    config_init();
    write_tmp_config(
        "interface: eth0\n"
        "dns-resolution: true\n"
        "port-resolution: true\n"
    );
    read_config(tmpfile_path, 0);

    /* CLI overrides stored in stringmap */
    stringmap cli = stringmap_new();
    stringmap_insert(cli, "interface", item_ptr(xstrdup("wlan0")));
    stringmap_insert(cli, "promiscuous", item_ptr(xstrdup("true")));

    /* Lookup with override precedence */
    stringmap ov;

    ov = stringmap_find(cli, "interface");
    const char *iface = ov ? (char *)ov->data.ptr : config_get_string("interface");
    ASSERT_STR_EQ(iface, "wlan0");

    ov = stringmap_find(cli, "dns-resolution");
    const char *dns = ov ? (char *)ov->data.ptr : config_get_string("dns-resolution");
    ASSERT_STR_EQ(dns, "true"); /* from config file */

    ov = stringmap_find(cli, "promiscuous");
    ASSERT_NOT_NULL(ov);
    ASSERT_STR_EQ((char *)ov->data.ptr, "true"); /* CLI-only key */

    /* Config file key not in CLI */
    ASSERT_EQ(config_get_bool("port-resolution"), 1);

    stringmap_delete_free(cli);
    cleanup_tmp();
}

TEST(config_overlay_multiple_layers) {
    /* Defaults -> config file -> CLI args: three-layer resolution */
    stringmap defaults = stringmap_new();
    stringmap_insert(defaults, "interface", item_ptr(xstrdup("lo")));
    stringmap_insert(defaults, "dns-resolution", item_ptr(xstrdup("false")));
    stringmap_insert(defaults, "port-resolution", item_ptr(xstrdup("true")));
    stringmap_insert(defaults, "promiscuous", item_ptr(xstrdup("false")));

    config_init();
    write_tmp_config(
        "interface: eth0\n"
        "dns-resolution: true\n"
    );
    read_config(tmpfile_path, 0);

    stringmap cli = stringmap_new();
    stringmap_insert(cli, "interface", item_ptr(xstrdup("wlan0")));

    /* Resolution order: CLI > config file > defaults */
    const char *keys[] = {"interface", "dns-resolution", "port-resolution", "promiscuous"};
    const char *expected[] = {"wlan0", "true", "true", "false"};

    for (int i = 0; i < 4; i++) {
        stringmap ov = stringmap_find(cli, keys[i]);
        const char *val;
        if (ov) {
            val = (char *)ov->data.ptr;
        } else {
            val = config_get_string(keys[i]);
            if (!val) {
                stringmap def = stringmap_find(defaults, keys[i]);
                val = def ? (char *)def->data.ptr : NULL;
            }
        }
        ASSERT_NOT_NULL(val);
        ASSERT_STR_EQ(val, expected[i]);
    }

    stringmap_delete_free(cli);
    stringmap_delete_free(defaults);
    cleanup_tmp();
}

/* ================================================================
 *  SUITE 14: Large-scale pipeline stress
 *
 *  Simulates: a busy network with many concurrent flows going
 *  through the full pipeline
 * ================================================================ */

TEST(stress_500_flows_through_pipeline) {
    hash_type *flows = addr_hash_create();
    int n = 500;
    long *bw_records = xmalloc(n * sizeof(long));

    for (int i = 0; i < n; i++) {
        char src[16], dst[16];
        snprintf(src, sizeof(src), "10.%d.%d.%d", (i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF);
        snprintf(dst, sizeof(dst), "192.%d.%d.%d", (i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF);
        addr_pair ap = make_ipv4(src, 1024 + (i % 64000), dst, 80 + (i % 3), 6);
        bw_records[i] = (long)(n - i) * 10;
        addr_hash_insert(flows, &ap, &bw_records[i]);
    }

    /* Collect into sorted list */
    void **items = xmalloc(n * sizeof(void *));
    hash_node_type *node = NULL;
    int count = 0;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK)
        items[count++] = node->record;
    ASSERT_EQ(count, n);

    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;
    sorted_list_insert_batch(&slist, items, count);

    /* Verify descending order */
    sorted_list_node *snode = NULL;
    long prev = 999999;
    int verified = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL) {
        long val = *(long *)snode->data;
        ASSERT(val <= prev);
        prev = val;
        verified++;
    }
    ASSERT_EQ(verified, n);

    /* Take top 10 into vector */
    vector top = vector_new();
    snode = NULL;
    int taken = 0;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL && taken < 10) {
        vector_push_back(top, item_long(*(long *)snode->data));
        taken++;
    }
    ASSERT_EQ(top->n_used, 10);
    /* First should be the highest bandwidth */
    ASSERT_EQ(top->items[0].num, n * 10);

    vector_delete(top);
    sorted_list_destroy(&slist);
    free(items);
    addr_hash_teardown(flows);
    free(bw_records);
}

TEST(stress_mixed_af_200_flows) {
    hash_type *flows = addr_hash_create();
    int n = 200;
    long bw_records[200];

    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            char src[16], dst[16];
            snprintf(src, sizeof(src), "10.0.%d.%d", (i >> 8) & 0xFF, i & 0xFF);
            snprintf(dst, sizeof(dst), "10.1.%d.%d", (i >> 8) & 0xFF, i & 0xFF);
            addr_pair ap = make_ipv4(src, 2000 + i, dst, 80, 6);
            bw_records[i] = (i + 1) * 50;
            addr_hash_insert(flows, &ap, &bw_records[i]);
        } else {
            char src[48], dst[48];
            snprintf(src, sizeof(src), "2001:db8::%x", i);
            snprintf(dst, sizeof(dst), "2001:db8::1:%x", i);
            addr_pair ap = make_ipv6(src, 2000 + i, dst, 443, 6);
            bw_records[i] = (i + 1) * 50;
            addr_hash_insert(flows, &ap, &bw_records[i]);
        }
    }

    /* Count by address family */
    int v4 = 0, v6 = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        if (key->address_family == AF_INET) v4++;
        else v6++;
    }
    ASSERT_EQ(v4, 100);
    ASSERT_EQ(v6, 100);

    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 15: History rotation across time slots
 *
 *  Simulates: bandwidth accumulation across multiple time intervals
 *  with sorting reflecting the latest window
 * ================================================================ */

TEST(history_multi_slot_accumulation) {
    hash_type *flows = addr_hash_create();

    history_type *h1 = xcalloc(1, sizeof(history_type));
    history_type *h2 = xcalloc(1, sizeof(history_type));
    history_type *h3 = xcalloc(1, sizeof(history_type));

    addr_pair ap1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 443, 6);
    addr_pair ap3 = make_ipv4("10.0.0.5", 5002, "10.0.0.6", 22, 6);

    addr_hash_insert(flows, &ap1, h1);
    addr_hash_insert(flows, &ap2, h2);
    addr_hash_insert(flows, &ap3, h3);

    /* Simulate 5 time slots of packet accumulation */
    for (int slot = 0; slot < 5; slot++) {
        h1->recv[slot] = 1000 * (slot + 1);
        h1->sent[slot] = 500 * (slot + 1);
        h1->total_recv += h1->recv[slot];
        h1->total_sent += h1->sent[slot];

        h2->recv[slot] = 500;
        h2->sent[slot] = 100;
        h2->total_recv += 500;
        h2->total_sent += 100;

        h3->recv[slot] = 200 * (5 - slot);
        h3->sent[slot] = 100 * (5 - slot);
        h3->total_recv += h3->recv[slot];
        h3->total_sent += h3->sent[slot];
    }

    /* Sort by most recent slot (slot 4) recv bandwidth */
    long recent_bw[3];
    recent_bw[0] = h1->recv[4]; /* 5000 */
    recent_bw[1] = h2->recv[4]; /* 500 */
    recent_bw[2] = h3->recv[4]; /* 200 */

    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    for (int i = 0; i < 3; i++)
        sorted_list_insert(&slist, &recent_bw[i]);

    sorted_list_node *snode = sorted_list_next_item(&slist, NULL);
    ASSERT_EQ(*(long *)snode->data, 5000); /* h1 highest recently */
    snode = sorted_list_next_item(&slist, snode);
    ASSERT_EQ(*(long *)snode->data, 500);
    snode = sorted_list_next_item(&slist, snode);
    ASSERT_EQ(*(long *)snode->data, 200);

    /* Verify totals */
    ASSERT_EQ((long)h1->total_recv, 1000 + 2000 + 3000 + 4000 + 5000);
    ASSERT_EQ((long)h2->total_recv, 2500);

    sorted_list_destroy(&slist);
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        free(node->record);
        node->record = NULL;
    }
    addr_hash_teardown(flows);
}

TEST(history_last_write_tracking) {
    history_type h;
    memset(&h, 0, sizeof(h));

    /* Simulate writes cycling through the buffer */
    for (int i = 0; i < HISTORY_LENGTH * 3; i++) {
        int slot = i % HISTORY_LENGTH;
        h.recv[slot] = i * 100;
        h.sent[slot] = i * 50;
        h.last_write = slot;
    }

    /* Last write should point to the most recent slot */
    ASSERT_EQ(h.last_write, (HISTORY_LENGTH * 3 - 1) % HISTORY_LENGTH);

    /* Most recent value */
    ASSERT_EQ(h.recv[h.last_write], (HISTORY_LENGTH * 3 - 1) * 100);
}

/* ================================================================
 *  SUITE 16: Service table maintenance during operation
 *
 *  Simulates: services being added/removed while flows reference them
 * ================================================================ */

TEST(serv_table_update_during_flow_iteration) {
    serv_table *services = serv_table_create();
    hash_type *flows = addr_hash_create();

    serv_table_insert(services, 80, 6, "http");
    serv_table_insert(services, 443, 6, "https");

    addr_pair ap1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 443, 6);
    addr_pair ap3 = make_ipv4("10.0.0.5", 5002, "10.0.0.6", 8080, 6);
    long bw1 = 100, bw2 = 200, bw3 = 300;
    addr_hash_insert(flows, &ap1, &bw1);
    addr_hash_insert(flows, &ap2, &bw2);
    addr_hash_insert(flows, &ap3, &bw3);

    /* Initially port 8080 is unknown */
    ASSERT_NULL(serv_table_lookup(services, 8080, 6));

    /* Add it dynamically */
    serv_table_insert(services, 8080, 6, "http-alt");

    /* Now all flows can be resolved */
    hash_node_type *node = NULL;
    int resolved = 0;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        const char *svc = serv_table_lookup(services, key->dst_port, key->protocol);
        ASSERT_NOT_NULL(svc);
        resolved++;
    }
    ASSERT_EQ(resolved, 3);

    /* Remove a service and verify it's gone */
    serv_table_delete(services, 80, 6);
    ASSERT_NULL(serv_table_lookup(services, 80, 6));
    ASSERT_STR_EQ(serv_table_lookup(services, 443, 6), "https");
    ASSERT_STR_EQ(serv_table_lookup(services, 8080, 6), "http-alt");

    serv_table_destroy(services);
    addr_hash_teardown(flows);
}

TEST(serv_table_replace_service_name) {
    serv_table *services = serv_table_create();

    serv_table_insert(services, 80, 6, "http");
    ASSERT_STR_EQ(serv_table_lookup(services, 80, 6), "http");

    /* Replace with updated name */
    serv_table_delete(services, 80, 6);
    serv_table_insert(services, 80, 6, "http/1.1");
    ASSERT_STR_EQ(serv_table_lookup(services, 80, 6), "http/1.1");

    serv_table_destroy(services);
}

/* ================================================================
 *  SUITE 17: Config error handling
 *
 *  Simulates: graceful handling of missing/bad config data
 * ================================================================ */

TEST(config_missing_file_no_crash) {
    config_init();
    /* Reading a non-existent file should not crash */
    int result = read_config("/tmp/nonexistent_iftop_config_xyz", 0);
    /* Should return 0 (failure) but not crash */
    ASSERT_EQ(result, 0);
}

TEST(config_missing_keys_return_defaults) {
    config_init();
    write_tmp_config("interface: eth0\n");
    read_config(tmpfile_path, 0);

    /* Existing key works */
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");

    /* Missing keys return safe defaults */
    ASSERT_NULL(config_get_string("nonexistent-key"));
    ASSERT_EQ(config_get_bool("nonexistent-key"), 0);

    int ival = -1;
    ASSERT_EQ(config_get_int("nonexistent-key", &ival), 0);
    ASSERT_EQ(ival, -1); /* unchanged */

    float fval = -1.0;
    ASSERT_EQ(config_get_float("nonexistent-key", &fval), 0);

    cleanup_tmp();
}

TEST(config_empty_file) {
    config_init();
    write_tmp_config("");
    int result = read_config(tmpfile_path, 0);
    ASSERT_EQ(result, 1); /* success, just no directives */
    ASSERT_NULL(config_get_string("interface"));
    cleanup_tmp();
}

/* ================================================================
 *  SUITE 18: Vector + sorted_list direct pipeline
 *
 *  Simulates: data collected in vector, transferred to sorted_list
 *  for ordered display, then back to vector for indexed access
 * ================================================================ */

TEST(vector_to_sorted_list_round_trip) {
    /* Start with unsorted vector data */
    vector v = vector_new();
    long vals[] = {42, 7, 99, 1, 55, 23, 88, 3, 67, 15};
    int n = sizeof(vals) / sizeof(vals[0]);
    for (int i = 0; i < n; i++)
        vector_push_back(v, item_long(vals[i]));

    /* Transfer to sorted list */
    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    long sorted_storage[10];
    for (int i = 0; i < n; i++) {
        sorted_storage[i] = v->items[i].num;
        sorted_list_insert(&slist, &sorted_storage[i]);
    }

    /* Collect back into new vector in sorted order */
    vector sorted_v = vector_new();
    sorted_list_node *snode = NULL;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL)
        vector_push_back(sorted_v, item_long(*(long *)snode->data));

    ASSERT_EQ(sorted_v->n_used, n);

    /* Verify descending order */
    for (size_t i = 1; i < sorted_v->n_used; i++)
        ASSERT(sorted_v->items[i - 1].num >= sorted_v->items[i].num);

    /* First and last */
    ASSERT_EQ(sorted_v->items[0].num, 99);
    ASSERT_EQ(sorted_v->items[n - 1].num, 1);

    vector_delete(v);
    vector_delete(sorted_v);
    sorted_list_destroy(&slist);
}

TEST(vector_sorted_list_with_duplicates) {
    long vals[] = {500, 100, 500, 300, 100, 500};
    int n = sizeof(vals) / sizeof(vals[0]);

    sorted_list_type slist;
    sorted_list_initialise(&slist);
    slist.compare = compare_bandwidth_desc;

    for (int i = 0; i < n; i++)
        sorted_list_insert(&slist, &vals[i]);

    vector v = vector_new();
    sorted_list_node *snode = NULL;
    while ((snode = sorted_list_next_item(&slist, snode)) != NULL)
        vector_push_back(v, item_long(*(long *)snode->data));

    ASSERT_EQ(v->n_used, n);

    /* All 500s first, then 300, then 100s */
    ASSERT_EQ(v->items[0].num, 500);
    ASSERT_EQ(v->items[1].num, 500);
    ASSERT_EQ(v->items[2].num, 500);
    ASSERT_EQ(v->items[3].num, 300);
    ASSERT_EQ(v->items[4].num, 100);
    ASSERT_EQ(v->items[5].num, 100);

    vector_delete(v);
    sorted_list_destroy(&slist);
}

/* ================================================================
 *  SUITE 19: Config enum drives protocol filtering
 *
 *  Simulates: config specifies protocol filter, used to select
 *  which flows to display and which services to resolve
 * ================================================================ */

TEST(config_enum_protocol_filter) {
    config_init();
    config_set_string("protocol-filter", "tcp");

    config_enumeration_type proto_enum[] = {
        {"tcp", 6}, {"udp", 17}, {"all", 0}, {NULL, -1}
    };
    int proto = -1;
    ASSERT_EQ(config_get_enum("protocol-filter", proto_enum, &proto), 1);
    ASSERT_EQ(proto, 6);

    /* Set up flows with mixed protocols */
    hash_type *flows = addr_hash_create();
    addr_pair tcp1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair udp1 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 53, 17);
    addr_pair tcp2 = make_ipv4("10.0.0.5", 5002, "10.0.0.6", 443, 6);
    long bw1 = 100, bw2 = 200, bw3 = 300;
    addr_hash_insert(flows, &tcp1, &bw1);
    addr_hash_insert(flows, &udp1, &bw2);
    addr_hash_insert(flows, &tcp2, &bw3);

    /* Filter by configured protocol */
    vector display = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        if (proto == 0 || key->protocol == proto)
            vector_push_back(display, item_ptr(node));
    }

    ASSERT_EQ(display->n_used, 2); /* only TCP flows */

    item *it;
    vector_iterate(display, it) {
        hash_node_type *n = (hash_node_type *)it->ptr;
        addr_pair *key = (addr_pair *)n->key;
        ASSERT_EQ(key->protocol, 6);
    }

    vector_delete(display);
    addr_hash_teardown(flows);
}

TEST(config_enum_all_protocols) {
    config_init();
    config_set_string("protocol-filter", "all");

    config_enumeration_type proto_enum[] = {
        {"tcp", 6}, {"udp", 17}, {"all", 0}, {NULL, -1}
    };
    int proto = -1;
    ASSERT_EQ(config_get_enum("protocol-filter", proto_enum, &proto), 1);
    ASSERT_EQ(proto, 0);

    /* All protocols pass the filter */
    hash_type *flows = addr_hash_create();
    addr_pair tcp1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair udp1 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 53, 17);
    long bw1 = 100, bw2 = 200;
    addr_hash_insert(flows, &tcp1, &bw1);
    addr_hash_insert(flows, &udp1, &bw2);

    vector display = vector_new();
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        if (proto == 0 || key->protocol == proto)
            vector_push_back(display, item_ptr(node));
    }

    ASSERT_EQ(display->n_used, 2);

    vector_delete(display);
    addr_hash_teardown(flows);
}

/* ================================================================
 *  SUITE 20: Stringmap + vector multi-table index
 *
 *  Simulates: using stringmap to index vector entries by name
 *  for O(log n) lookup into a sequentially-stored display list
 * ================================================================ */

TEST(stringmap_indexes_vector) {
    vector v = vector_new();
    stringmap index = stringmap_new();

    /* Build a list of host entries */
    const char *hosts[] = {"alpha.local", "beta.local", "gamma.local",
                           "delta.local", "epsilon.local"};
    long bw[] = {5000, 1200, 8000, 300, 4500};

    for (int i = 0; i < 5; i++) {
        vector_push_back(v, item_long(bw[i]));
        stringmap_insert(index, hosts[i], item_long(i));
    }

    /* Look up by name, get vector index */
    stringmap found = stringmap_find(index, "gamma.local");
    ASSERT_NOT_NULL(found);
    int idx = (int)found->data.num;
    ASSERT_EQ(v->items[idx].num, 8000);

    found = stringmap_find(index, "delta.local");
    ASSERT_NOT_NULL(found);
    idx = (int)found->data.num;
    ASSERT_EQ(v->items[idx].num, 300);

    found = stringmap_find(index, "nonexistent.local");
    ASSERT_NULL(found);

    stringmap_delete(index);
    vector_delete(v);
}

TEST(stringmap_indexes_flow_nodes) {
    hash_type *flows = addr_hash_create();
    stringmap index = stringmap_new();

    /* Insert flows and index by a display label */
    addr_pair ap1 = make_ipv4("10.0.0.1", 5000, "10.0.0.2", 80, 6);
    addr_pair ap2 = make_ipv4("10.0.0.3", 5001, "10.0.0.4", 443, 6);
    addr_pair ap3 = make_ipv4("10.0.0.5", 5002, "10.0.0.6", 22, 6);

    long bw1 = 1000, bw2 = 2000, bw3 = 3000;
    addr_hash_insert(flows, &ap1, &bw1);
    addr_hash_insert(flows, &ap2, &bw2);
    addr_hash_insert(flows, &ap3, &bw3);

    /* Build label -> bandwidth index */
    hash_node_type *node = NULL;
    while (hash_next_item(flows, &node) == HASH_STATUS_OK) {
        addr_pair *key = (addr_pair *)node->key;
        char label[64];
        char src_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &key->src, src_str, sizeof(src_str));
        snprintf(label, sizeof(label), "%s:%d", src_str, key->src_port);
        stringmap_insert(index, label, item_ptr(node->record));
    }

    /* Look up by label */
    stringmap found = stringmap_find(index, "10.0.0.1:5000");
    ASSERT_NOT_NULL(found);
    ASSERT_EQ(*(long *)found->data.ptr, 1000);

    found = stringmap_find(index, "10.0.0.5:5002");
    ASSERT_NOT_NULL(found);
    ASSERT_EQ(*(long *)found->data.ptr, 3000);

    found = stringmap_find(index, "10.0.0.99:9999");
    ASSERT_NULL(found);

    stringmap_delete(index);
    addr_hash_teardown(flows);
}

/* === main === */

int main(void) {
    TEST_SUITE("Integration: Flow Tracking Pipeline");
    RUN(flow_track_insert_and_sort);
    RUN(flow_track_update_and_resort);
    RUN(flow_track_mixed_protocols);
    RUN(flow_track_batch_sort);

    TEST_SUITE("Integration: Address/Port Resolution");
    RUN(resolve_tcp_flow_ports);
    RUN(resolve_mixed_protocol_services);
    RUN(resolve_ipv6_hostname_cache);
    RUN(resolve_full_pipeline);

    TEST_SUITE("Integration: Config Pipeline");
    RUN(config_pipeline_full_config);
    RUN(config_pipeline_cli_overrides_file);
    RUN(config_pipeline_enum_from_file);
    RUN(config_pipeline_reinit_and_reload);
    RUN(config_pipeline_float_from_file);

    TEST_SUITE("Integration: Vector + Hash Workflows");
    RUN(hash_to_vector_collect);
    RUN(hash_to_vector_scrollable_window);
    RUN(addr_hash_to_vector_with_service_names);

    TEST_SUITE("Integration: History Lifecycle");
    RUN(history_accumulate_and_sort);
    RUN(history_circular_buffer);
    RUN(history_multiple_flows_sorted_by_total);

    TEST_SUITE("Integration: Mixed Address Family");
    RUN(mixed_af_flow_table);
    RUN(mixed_af_sort_by_bandwidth);

    TEST_SUITE("Integration: Config-Driven Behavior");
    RUN(stringmap_keys_to_vector);
    RUN(config_drives_serv_table);
    RUN(config_disabled_port_resolution);

    TEST_SUITE("Integration: End-to-End Flow Lifecycle");
    RUN(end_to_end_flow_lifecycle);

    TEST_SUITE("Integration: Flow Churn");
    RUN(flow_churn_delete_and_reinsert);
    RUN(flow_churn_complete_replacement);

    TEST_SUITE("Integration: Vector Filtering");
    RUN(vector_filter_low_bandwidth);
    RUN(vector_filter_by_protocol);

    TEST_SUITE("Integration: Top-N Display");
    RUN(top_n_from_config);
    RUN(top_n_more_than_available);

    TEST_SUITE("Integration: Combined Resolution");
    RUN(resolve_hostname_and_service_together);
    RUN(resolve_partial_hostname_cache);

    TEST_SUITE("Integration: Config Overlay");
    RUN(config_overlay_cli_wins);
    RUN(config_overlay_multiple_layers);

    TEST_SUITE("Integration: Large-Scale Stress");
    RUN(stress_500_flows_through_pipeline);
    RUN(stress_mixed_af_200_flows);

    TEST_SUITE("Integration: History Rotation");
    RUN(history_multi_slot_accumulation);
    RUN(history_last_write_tracking);

    TEST_SUITE("Integration: Service Table Maintenance");
    RUN(serv_table_update_during_flow_iteration);
    RUN(serv_table_replace_service_name);

    TEST_SUITE("Integration: Config Error Handling");
    RUN(config_missing_file_no_crash);
    RUN(config_missing_keys_return_defaults);
    RUN(config_empty_file);

    TEST_SUITE("Integration: Vector + Sorted List Pipeline");
    RUN(vector_to_sorted_list_round_trip);
    RUN(vector_sorted_list_with_duplicates);

    TEST_SUITE("Integration: Config Enum Protocol Filter");
    RUN(config_enum_protocol_filter);
    RUN(config_enum_all_protocols);

    TEST_SUITE("Integration: Stringmap Index");
    RUN(stringmap_indexes_vector);
    RUN(stringmap_indexes_flow_nodes);

    TEST_REPORT();
}
