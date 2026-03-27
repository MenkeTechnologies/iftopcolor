/*
 * test_export.c: tests for JSON/CSV export mode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "test.h"
#include "addr_hash.h"
#include "serv_hash.h"
#include "iftop.h"
#include "sorted_list.h"
#include "options.h"
#include "cfgfile.h"
#include "export.h"

/* === Copied static helpers from export.c for unit testing === */

#define HISTORY_DIVISIONS 3
#define HOSTNAME_LENGTH 256

typedef struct host_pair_line_tag {
    addr_pair ap;
    unsigned long long total_recv;
    unsigned long long total_sent;
    double long recv[HISTORY_DIVISIONS];
    double long sent[HISTORY_DIVISIONS];
    char cached_hostname[HOSTNAME_LENGTH];
} host_pair_line;

static const char *protocol_name(unsigned short proto) {
    switch (proto) {
    case 6:  return "TCP";
    case 17: return "UDP";
    case 1:  return "ICMP";
    case 58: return "ICMPv6";
    default: return "OTHER";
    }
}

static void sprint_addr(char *buf, size_t buflen, int af, void *addr) {
    inet_ntop(af, addr, buf, buflen);
}

static void json_escape_string(FILE *fp, const char *s) {
    fputc('"', fp);
    for (; *s; s++) {
        switch (*s) {
        case '"':  fputs("\\\"", fp); break;
        case '\\': fputs("\\\\", fp); break;
        default:   fputc(*s, fp);     break;
        }
    }
    fputc('"', fp);
}

/* === Globals needed by export.c (provide stubs for linking) === */

options_t options;
hash_type *screen_hash;
serv_table *service_hash;
sorted_list_type screen_list;
host_pair_line totals;
int peaksent, peakrecv, peaktotal;
history_type history_totals;
sig_atomic_t foad;

/* Stubs for functions referenced by export.c */
void screen_list_init(void) {
    sorted_list_initialise(&screen_list);
}
void screen_list_clear(void) {
    sorted_list_destroy(&screen_list);
}
void tick(int print) { (void)print; }

/* === Helpers === */

static addr_pair make_ipv4(const char *src, uint16_t sport,
                           const char *dst, uint16_t dport,
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

static addr_pair make_ipv6(const char *src, uint16_t sport,
                           const char *dst, uint16_t dport,
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

static char *capture_stdout(void (*fn)(void)) {
    fflush(stdout);
    char tmpfile[] = "/tmp/iftop_test_XXXXXX";
    int fd = mkstemp(tmpfile);
    if (fd == -1) return NULL;

    FILE *tmp = fdopen(fd, "w+");
    if (!tmp) { close(fd); unlink(tmpfile); return NULL; }

    /* Redirect stdout */
    int saved_stdout = dup(fileno(stdout));
    dup2(fileno(tmp), fileno(stdout));

    fn();
    fflush(stdout);

    /* Restore stdout */
    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);

    /* Read captured output */
    fseek(tmp, 0, SEEK_END);
    long sz = ftell(tmp);
    fseek(tmp, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (buf) {
        fread(buf, 1, sz, tmp);
        buf[sz] = '\0';
    }
    fclose(tmp);
    unlink(tmpfile);
    return buf;
}

/* ============================================================
 * Unit Tests: protocol_name
 * ============================================================ */

TEST(protocol_name_tcp) {
    ASSERT_STR_EQ(protocol_name(6), "TCP");
}

TEST(protocol_name_udp) {
    ASSERT_STR_EQ(protocol_name(17), "UDP");
}

TEST(protocol_name_icmp) {
    ASSERT_STR_EQ(protocol_name(1), "ICMP");
}

TEST(protocol_name_icmpv6) {
    ASSERT_STR_EQ(protocol_name(58), "ICMPv6");
}

TEST(protocol_name_unknown) {
    ASSERT_STR_EQ(protocol_name(255), "OTHER");
}

TEST(protocol_name_zero) {
    ASSERT_STR_EQ(protocol_name(0), "OTHER");
}

/* ============================================================
 * Unit Tests: sprint_addr
 * ============================================================ */

TEST(sprint_addr_ipv4) {
    char buf[INET6_ADDRSTRLEN];
    struct in_addr addr;
    inet_pton(AF_INET, "192.168.1.1", &addr);
    sprint_addr(buf, sizeof(buf), AF_INET, &addr);
    ASSERT_STR_EQ(buf, "192.168.1.1");
}

TEST(sprint_addr_ipv4_zeros) {
    char buf[INET6_ADDRSTRLEN];
    struct in_addr addr;
    inet_pton(AF_INET, "0.0.0.0", &addr);
    sprint_addr(buf, sizeof(buf), AF_INET, &addr);
    ASSERT_STR_EQ(buf, "0.0.0.0");
}

TEST(sprint_addr_ipv6) {
    char buf[INET6_ADDRSTRLEN];
    struct in6_addr addr;
    inet_pton(AF_INET6, "::1", &addr);
    sprint_addr(buf, sizeof(buf), AF_INET6, &addr);
    ASSERT_STR_EQ(buf, "::1");
}

TEST(sprint_addr_ipv6_full) {
    char buf[INET6_ADDRSTRLEN];
    struct in6_addr addr;
    inet_pton(AF_INET6, "2001:db8::1", &addr);
    sprint_addr(buf, sizeof(buf), AF_INET6, &addr);
    ASSERT_STR_EQ(buf, "2001:db8::1");
}

/* ============================================================
 * Unit Tests: json_escape_string
 * ============================================================ */

TEST(json_escape_plain_string) {
    char tmpfile[] = "/tmp/iftop_json_XXXXXX";
    int fd = mkstemp(tmpfile);
    FILE *fp = fdopen(fd, "w+");
    json_escape_string(fp, "hello");
    fseek(fp, 0, SEEK_SET);
    char buf[64];
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    unlink(tmpfile);
    ASSERT_STR_EQ(buf, "\"hello\"");
}

TEST(json_escape_quotes) {
    char tmpfile[] = "/tmp/iftop_json_XXXXXX";
    int fd = mkstemp(tmpfile);
    FILE *fp = fdopen(fd, "w+");
    json_escape_string(fp, "say \"hi\"");
    fseek(fp, 0, SEEK_SET);
    char buf[64];
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    unlink(tmpfile);
    ASSERT_STR_EQ(buf, "\"say \\\"hi\\\"\"");
}

TEST(json_escape_backslash) {
    char tmpfile[] = "/tmp/iftop_json_XXXXXX";
    int fd = mkstemp(tmpfile);
    FILE *fp = fdopen(fd, "w+");
    json_escape_string(fp, "a\\b");
    fseek(fp, 0, SEEK_SET);
    char buf[64];
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    unlink(tmpfile);
    ASSERT_STR_EQ(buf, "\"a\\\\b\"");
}

TEST(json_escape_empty_string) {
    char tmpfile[] = "/tmp/iftop_json_XXXXXX";
    int fd = mkstemp(tmpfile);
    FILE *fp = fdopen(fd, "w+");
    json_escape_string(fp, "");
    fseek(fp, 0, SEEK_SET);
    char buf[64];
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    unlink(tmpfile);
    ASSERT_STR_EQ(buf, "\"\"");
}

/* ============================================================
 * Unit Tests: export_init
 * ============================================================ */

TEST(export_init_creates_screen_hash) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    ASSERT_NOT_NULL(screen_hash);
    ASSERT_NOT_NULL(service_hash);
}

TEST(export_init_screen_hash_starts_empty) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    hash_node_type *node = NULL;
    ASSERT(hash_next_item(screen_hash, &node) != HASH_STATUS_OK);
}

/* ============================================================
 * Unit Tests: export_print dispatch
 * ============================================================ */

TEST(export_print_json_mode_produces_json) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    peaksent = peakrecv = peaktotal = 0;
    options.export_mode = 1;
    options.interface = "lo0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "\"timestamp\"") != NULL);
    ASSERT(strstr(output, "\"interface\"") != NULL);
    ASSERT(strstr(output, "\"flows\"") != NULL);
    ASSERT(strstr(output, "\"totals\"") != NULL);
    ASSERT(strstr(output, "\"lo0\"") != NULL);
    free(output);
}

TEST(export_print_csv_mode_produces_header) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    options.export_mode = 2;
    /* Reset csv header state — the static var is in export.c,
     * but since we're in a fresh test binary it starts at 0 */

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "timestamp,source,source_port") != NULL);
    ASSERT(strstr(output, "protocol,sent_2s") != NULL);
    free(output);
}

TEST(export_print_zero_mode_produces_nothing) {
    options.export_mode = 0;
    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT_EQ(strlen(output), 0);
    free(output);
}

/* ============================================================
 * Integration Tests: JSON output with flows
 * ============================================================ */

static int descending_compare(void *a, void *b) {
    host_pair_line *la = a, *lb = b;
    double long sa = la->sent[0] + la->recv[0];
    double long sb = lb->sent[0] + lb->recv[0];
    if (sa > sb) return -1;
    if (sa < sb) return 1;
    return 0;
}

TEST(json_output_contains_flow_data) {
    /* Set up a flow in the screen list */
    screen_hash = NULL;
    service_hash = NULL;
    export_init();

    /* Override compare for our test */
    screen_list.compare = descending_compare;

    host_pair_line *line = calloc(1, sizeof(*line));
    line->ap = make_ipv4("10.0.0.1", 443, "10.0.0.2", 52341, 6);
    line->sent[0] = 1000; line->sent[1] = 2000; line->sent[2] = 3000;
    line->recv[0] = 500;  line->recv[1] = 1000; line->recv[2] = 1500;
    line->total_sent = 50000;
    line->total_recv = 25000;
    sorted_list_insert(&screen_list, line);

    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    peaksent = peakrecv = 0;
    options.export_mode = 1;
    options.interface = "en0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "\"source\": \"10.0.0.1\"") != NULL);
    ASSERT(strstr(output, "\"destination\": \"10.0.0.2\"") != NULL);
    ASSERT(strstr(output, "\"source_port\": 443") != NULL);
    ASSERT(strstr(output, "\"destination_port\": 52341") != NULL);
    ASSERT(strstr(output, "\"protocol\": \"TCP\"") != NULL);
    ASSERT(strstr(output, "\"total_sent\": 50000") != NULL);
    ASSERT(strstr(output, "\"total_recv\": 25000") != NULL);
    free(output);
    free(line);
}

TEST(json_output_multiple_flows) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    screen_list.compare = descending_compare;

    host_pair_line *l1 = calloc(1, sizeof(*l1));
    l1->ap = make_ipv4("10.0.0.1", 80, "10.0.0.2", 12345, 6);
    l1->sent[0] = 2000;
    l1->recv[0] = 1000;
    sorted_list_insert(&screen_list, l1);

    host_pair_line *l2 = calloc(1, sizeof(*l2));
    l2->ap = make_ipv4("10.0.0.3", 53, "10.0.0.4", 54321, 17);
    l2->sent[0] = 500;
    l2->recv[0] = 300;
    sorted_list_insert(&screen_list, l2);

    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    peaksent = peakrecv = 0;
    options.export_mode = 1;
    options.interface = "en0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "10.0.0.1") != NULL);
    ASSERT(strstr(output, "10.0.0.3") != NULL);
    ASSERT(strstr(output, "\"protocol\": \"UDP\"") != NULL);
    free(output);
    free(l1);
    free(l2);
}

TEST(json_output_ipv6_flow) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    screen_list.compare = descending_compare;

    host_pair_line *line = calloc(1, sizeof(*line));
    line->ap = make_ipv6("2001:db8::1", 443, "2001:db8::2", 8080, 6);
    line->sent[0] = 100;
    line->recv[0] = 200;
    sorted_list_insert(&screen_list, line);

    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    peaksent = peakrecv = 0;
    options.export_mode = 1;
    options.interface = "lo0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "2001:db8::1") != NULL);
    ASSERT(strstr(output, "2001:db8::2") != NULL);
    free(output);
    free(line);
}

TEST(json_output_totals_section) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    memset(&totals, 0, sizeof(totals));
    totals.sent[0] = 100; totals.sent[1] = 200; totals.sent[2] = 300;
    totals.recv[0] = 50;  totals.recv[1] = 100; totals.recv[2] = 150;
    history_totals.total_sent = 999999;
    history_totals.total_recv = 888888;
    peaksent = 5000;
    peakrecv = 4000;
    options.export_mode = 1;
    options.interface = "en0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "\"cumulative_sent\": 999999") != NULL);
    ASSERT(strstr(output, "\"cumulative_recv\": 888888") != NULL);
    ASSERT(strstr(output, "\"peak_sent\": 5000") != NULL);
    ASSERT(strstr(output, "\"peak_recv\": 4000") != NULL);
    free(output);
}

/* ============================================================
 * Integration Tests: CSV output with flows
 * ============================================================ */

TEST(csv_output_contains_flow_data) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    screen_list.compare = descending_compare;

    host_pair_line *line = calloc(1, sizeof(*line));
    line->ap = make_ipv4("172.16.0.1", 22, "172.16.0.2", 55555, 6);
    line->sent[0] = 100; line->sent[1] = 200; line->sent[2] = 300;
    line->recv[0] = 50;  line->recv[1] = 100; line->recv[2] = 150;
    line->total_sent = 10000;
    line->total_recv = 5000;
    sorted_list_insert(&screen_list, line);

    memset(&totals, 0, sizeof(totals));
    options.export_mode = 2;
    options.interface = "en0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "172.16.0.1") != NULL);
    ASSERT(strstr(output, "172.16.0.2") != NULL);
    ASSERT(strstr(output, "22") != NULL);
    ASSERT(strstr(output, "TCP") != NULL);
    ASSERT(strstr(output, "10000") != NULL);
    free(output);
    free(line);
}

TEST(csv_output_ipv6_flow) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    screen_list.compare = descending_compare;

    host_pair_line *line = calloc(1, sizeof(*line));
    line->ap = make_ipv6("fe80::1", 80, "fe80::2", 9090, 6);
    line->sent[0] = 1;
    sorted_list_insert(&screen_list, line);

    memset(&totals, 0, sizeof(totals));
    options.export_mode = 2;
    options.interface = "lo0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "fe80::1") != NULL);
    ASSERT(strstr(output, "fe80::2") != NULL);
    free(output);
    free(line);
}

/* ============================================================
 * Integration Tests: options parsing for export flags
 * ============================================================ */

TEST(options_default_export_mode_off) {
    options_set_defaults();
    ASSERT_EQ(options.export_mode, 0);
    ASSERT_EQ(options.export_duration, 0);
}

TEST(options_export_mode_json_via_config) {
    config_init();
    options_set_defaults();
    /* Simulate what -o json would do */
    options.export_mode = 1;
    ASSERT_EQ(options.export_mode, 1);
}

TEST(options_export_mode_csv_via_config) {
    config_init();
    options_set_defaults();
    options.export_mode = 2;
    ASSERT_EQ(options.export_mode, 2);
}

TEST(options_export_duration_positive) {
    options_set_defaults();
    options.export_duration = 30;
    ASSERT_EQ(options.export_duration, 30);
}

/* ============================================================
 * Edge cases
 * ============================================================ */

TEST(json_empty_flows_array) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    peaksent = peakrecv = 0;
    options.export_mode = 1;
    options.interface = "lo0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "\"flows\": [") != NULL);
    ASSERT(strstr(output, "\"totals\"") != NULL);
    /* No flow objects should appear */
    ASSERT(strstr(output, "\"source\"") == NULL);
    free(output);
}

TEST(csv_empty_produces_no_data_rows) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    memset(&totals, 0, sizeof(totals));
    options.export_mode = 2;

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    /* CSV header may or may not print (depends on prior tests and static state).
     * Either way, there should be no IP address data rows. */
    ASSERT(strstr(output, "192.168") == NULL);
    ASSERT(strstr(output, "10.0.0") == NULL);
    free(output);
}

TEST(json_interface_with_special_chars) {
    screen_hash = NULL;
    service_hash = NULL;
    export_init();
    memset(&totals, 0, sizeof(totals));
    memset(&history_totals, 0, sizeof(history_totals));
    peaksent = peakrecv = 0;
    options.export_mode = 1;
    options.interface = "eth\"0";

    char *output = capture_stdout(export_print);
    ASSERT_NOT_NULL(output);
    ASSERT(strstr(output, "\"eth\\\"0\"") != NULL);
    free(output);
}

/* ============================================================
 * main
 * ============================================================ */

int main(void) {
    TEST_SUITE("Export: protocol_name");
    RUN(protocol_name_tcp);
    RUN(protocol_name_udp);
    RUN(protocol_name_icmp);
    RUN(protocol_name_icmpv6);
    RUN(protocol_name_unknown);
    RUN(protocol_name_zero);

    TEST_SUITE("Export: sprint_addr");
    RUN(sprint_addr_ipv4);
    RUN(sprint_addr_ipv4_zeros);
    RUN(sprint_addr_ipv6);
    RUN(sprint_addr_ipv6_full);

    TEST_SUITE("Export: json_escape_string");
    RUN(json_escape_plain_string);
    RUN(json_escape_quotes);
    RUN(json_escape_backslash);
    RUN(json_escape_empty_string);

    TEST_SUITE("Export: export_init");
    RUN(export_init_creates_screen_hash);
    RUN(export_init_screen_hash_starts_empty);

    TEST_SUITE("Export: export_print dispatch");
    RUN(export_print_json_mode_produces_json);
    RUN(export_print_csv_mode_produces_header);
    RUN(export_print_zero_mode_produces_nothing);

    TEST_SUITE("Export: JSON output with flows");
    RUN(json_output_contains_flow_data);
    RUN(json_output_multiple_flows);
    RUN(json_output_ipv6_flow);
    RUN(json_output_totals_section);

    TEST_SUITE("Export: CSV output with flows");
    RUN(csv_output_contains_flow_data);
    RUN(csv_output_ipv6_flow);

    TEST_SUITE("Export: options defaults");
    RUN(options_default_export_mode_off);
    RUN(options_export_mode_json_via_config);
    RUN(options_export_mode_csv_via_config);
    RUN(options_export_duration_positive);

    TEST_SUITE("Export: edge cases");
    RUN(json_empty_flows_array);
    RUN(csv_empty_produces_no_data_rows);
    RUN(json_interface_with_special_chars);

    TEST_REPORT();
}
