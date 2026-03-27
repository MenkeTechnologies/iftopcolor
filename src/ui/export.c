/*
 * export.c:
 *
 * Non-interactive JSON/CSV export mode.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../include/addr_hash.h"
#include "../include/serv_hash.h"
#include "../include/iftop.h"
#include "../include/sorted_list.h"
#include "../include/options.h"
#include "../include/host_pair_line.h"

/* Globals defined in ui.c */
extern hash_type *screen_hash;
extern serv_table *service_hash;
extern sorted_list_type screen_list;
extern host_pair_line totals;
extern int peaksent, peakrecv, peaktotal;

/* From iftop.c */
extern history_type history_totals;
extern options_t options;
extern sig_atomic_t foad;

/* Forward declarations for functions in ui.c */
void screen_list_init(void);
void screen_list_clear(void);

static int csv_header_printed = 0;

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

static void export_print_json(void) {
    sorted_list_node *nn = NULL;
    char src[INET6_ADDRSTRLEN], dst[INET6_ADDRSTRLEN];
    char timebuf[64];
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    int first = 1;

    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", tm);

    printf("{\n");
    printf("  \"timestamp\": \"%s\",\n", timebuf);
    printf("  \"interface\": ");
    json_escape_string(stdout, options.interface);
    printf(",\n");
    printf("  \"flows\": [\n");

    while ((nn = sorted_list_next_item(&screen_list, nn)) != NULL) {
        host_pair_line *line = (host_pair_line *)nn->data;

        sprint_addr(src, sizeof(src), line->ap.address_family, &line->ap.src6);
        sprint_addr(dst, sizeof(dst), line->ap.address_family, &line->ap.dst6);

        if (!first) printf(",\n");
        first = 0;

        printf("    {\n");
        printf("      \"source\": \"%s\",\n", src);
        printf("      \"source_port\": %u,\n", line->ap.src_port);
        printf("      \"destination\": \"%s\",\n", dst);
        printf("      \"destination_port\": %u,\n", line->ap.dst_port);
        printf("      \"protocol\": \"%s\",\n", protocol_name(line->ap.protocol));
        if (options.show_processes) {
            printf("      \"pid\": %d,\n", line->pid);
            printf("      \"process_name\": ");
            json_escape_string(stdout, line->process_name[0] ? line->process_name : "");
            printf(",\n");
        }
        printf("      \"sent_2s\": %.0Lf,\n", line->sent[0]);
        printf("      \"sent_10s\": %.0Lf,\n", line->sent[1]);
        printf("      \"sent_40s\": %.0Lf,\n", line->sent[2]);
        printf("      \"recv_2s\": %.0Lf,\n", line->recv[0]);
        printf("      \"recv_10s\": %.0Lf,\n", line->recv[1]);
        printf("      \"recv_40s\": %.0Lf,\n", line->recv[2]);
        printf("      \"total_sent\": %llu,\n", line->total_sent);
        printf("      \"total_recv\": %llu\n", line->total_recv);
        printf("    }");
    }

    printf("\n  ],\n");
    printf("  \"totals\": {\n");
    printf("    \"sent_2s\": %.0Lf,\n", totals.sent[0]);
    printf("    \"sent_10s\": %.0Lf,\n", totals.sent[1]);
    printf("    \"sent_40s\": %.0Lf,\n", totals.sent[2]);
    printf("    \"recv_2s\": %.0Lf,\n", totals.recv[0]);
    printf("    \"recv_10s\": %.0Lf,\n", totals.recv[1]);
    printf("    \"recv_40s\": %.0Lf,\n", totals.recv[2]);
    printf("    \"cumulative_sent\": %llu,\n", history_totals.total_sent);
    printf("    \"cumulative_recv\": %llu,\n", history_totals.total_recv);
    printf("    \"peak_sent\": %d,\n", peaksent);
    printf("    \"peak_recv\": %d\n", peakrecv);
    printf("  }\n");
    printf("}\n");
    fflush(stdout);
}

static void export_print_csv(void) {
    sorted_list_node *nn = NULL;
    char src[INET6_ADDRSTRLEN], dst[INET6_ADDRSTRLEN];
    char timebuf[64];
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);

    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", tm);

    if (!csv_header_printed) {
        printf("timestamp,source,source_port,destination,destination_port,"
               "protocol,");
        if (options.show_processes)
            printf("pid,process_name,");
        printf("sent_2s,sent_10s,sent_40s,recv_2s,recv_10s,recv_40s,"
               "total_sent,total_recv\n");
        csv_header_printed = 1;
    }

    while ((nn = sorted_list_next_item(&screen_list, nn)) != NULL) {
        host_pair_line *line = (host_pair_line *)nn->data;

        sprint_addr(src, sizeof(src), line->ap.address_family, &line->ap.src6);
        sprint_addr(dst, sizeof(dst), line->ap.address_family, &line->ap.dst6);

        printf("%s,%s,%u,%s,%u,%s,",
               timebuf, src, line->ap.src_port, dst, line->ap.dst_port,
               protocol_name(line->ap.protocol));
        if (options.show_processes)
            printf("%d,%s,", line->pid,
                   line->process_name[0] ? line->process_name : "");
        printf("%.0Lf,%.0Lf,%.0Lf,%.0Lf,%.0Lf,%.0Lf,%llu,%llu\n",
               line->sent[0], line->sent[1], line->sent[2],
               line->recv[0], line->recv[1], line->recv[2],
               line->total_sent, line->total_recv);
    }
    fflush(stdout);
}

void export_print(void) {
    if (options.export_mode == 1) {
        export_print_json();
    } else if (options.export_mode == 2) {
        export_print_csv();
    }
}

void export_init(void) {
    screen_list_init();
    screen_hash = addr_hash_create();
    service_hash = serv_table_create();
    serv_table_init(service_hash);
}

void export_loop(void) {
    time_t start = time(NULL);

    while (foad == 0) {
        if (options.export_duration > 0) {
            time_t elapsed = time(NULL) - start;
            if (elapsed >= options.export_duration) {
                break;
            }
        }
        usleep(DISPLAY_RESOLUTION_US);
        tick(1);
    }
}

void export_finish(void) {
    /* Nothing to clean up beyond what the OS does on exit */
}
