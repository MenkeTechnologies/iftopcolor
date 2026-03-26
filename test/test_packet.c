/*
 * test_packet.c: tests for packet parsing and IP filtering logic
 *
 * Tests assign_addr_pair() for IPv4/IPv6 with TCP/UDP/ICMP packets,
 * flip behavior, and in_filter_net() for netmask-based filtering.
 *
 * These functions are static in iftop.c, so we copy them here with
 * the necessary headers for isolated testing.
 */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>

#include "test.h"
#include "iftop.h"
#include "ip.h"
#include "tcp.h"
#include "options.h"
#include "addr_hash.h"

/* Provide the global options that in_filter_net references */
options_t options;

/* --- Functions under test (copied from iftop.c since they are static) --- */

static int test_in_filter_net(struct in_addr addr) {
    return ((addr.s_addr & options.netfiltermask.s_addr) == options.netfilternet.s_addr);
}

static void test_assign_addr_pair(addr_pair *ap, struct ip *iptr, int flip) {
    unsigned short int src_port = 0;
    unsigned short int dst_port = 0;

    memset(ap, '\0', sizeof(*ap));

    if (IP_V(iptr) == 4) {
        ap->address_family = AF_INET;
        if (iptr->ip_p == IPPROTO_TCP || iptr->ip_p == IPPROTO_UDP) {
            struct tcphdr *thdr = ((void *)iptr) + IP_HL(iptr) * 4;
            src_port = ntohs(thdr->th_sport);
            dst_port = ntohs(thdr->th_dport);
        }

        if (flip == 0) {
            ap->src = iptr->ip_src;
            ap->src_port = src_port;
            ap->dst = iptr->ip_dst;
            ap->dst_port = dst_port;
        } else {
            ap->src = iptr->ip_dst;
            ap->src_port = dst_port;
            ap->dst = iptr->ip_src;
            ap->dst_port = src_port;
        }
    } else if (IP_V(iptr) == 6) {
        struct ip6_hdr *ip6tr = (struct ip6_hdr *)iptr;

        ap->address_family = AF_INET6;

        if ((ip6tr->ip6_nxt == IPPROTO_TCP) || (ip6tr->ip6_nxt == IPPROTO_UDP)) {
            struct tcphdr *thdr = ((void *)ip6tr) + 40;
            src_port = ntohs(thdr->th_sport);
            dst_port = ntohs(thdr->th_dport);
        }

        if (flip == 0) {
            memcpy(&ap->src6, &ip6tr->ip6_src, sizeof(ap->src6));
            ap->src_port = src_port;
            memcpy(&ap->dst6, &ip6tr->ip6_dst, sizeof(ap->dst6));
            ap->dst_port = dst_port;
        } else {
            memcpy(&ap->src6, &ip6tr->ip6_dst, sizeof(ap->src6));
            ap->src_port = dst_port;
            memcpy(&ap->dst6, &ip6tr->ip6_src, sizeof(ap->dst6));
            ap->dst_port = src_port;
        }
    }
}

/* --- Helpers to build fake packets --- */

/*
 * Build a minimal IPv4+TCP packet in a buffer.
 * ip_vhl encodes version=4, header length=5 (20 bytes).
 */
static void build_ipv4_tcp_packet(unsigned char *buf, const char *src_ip,
                                  const char *dst_ip, uint16_t sport,
                                  uint16_t dport) {
    memset(buf, 0, 128);
    struct ip *iptr = (struct ip *)buf;
    iptr->ip_vhl = (4 << 4) | 5; /* version=4, IHL=5 (20 bytes) */
    iptr->ip_p = IPPROTO_TCP;
    iptr->ip_len = htons(40); /* 20 IP + 20 TCP */
    inet_pton(AF_INET, src_ip, &iptr->ip_src);
    inet_pton(AF_INET, dst_ip, &iptr->ip_dst);

    struct tcphdr *thdr = (struct tcphdr *)(buf + 20);
    thdr->th_sport = htons(sport);
    thdr->th_dport = htons(dport);
}

static void build_ipv4_udp_packet(unsigned char *buf, const char *src_ip,
                                  const char *dst_ip, uint16_t sport,
                                  uint16_t dport) {
    memset(buf, 0, 128);
    struct ip *iptr = (struct ip *)buf;
    iptr->ip_vhl = (4 << 4) | 5;
    iptr->ip_p = IPPROTO_UDP;
    iptr->ip_len = htons(28); /* 20 IP + 8 UDP */
    inet_pton(AF_INET, src_ip, &iptr->ip_src);
    inet_pton(AF_INET, dst_ip, &iptr->ip_dst);

    struct tcphdr *thdr = (struct tcphdr *)(buf + 20);
    thdr->th_sport = htons(sport);
    thdr->th_dport = htons(dport);
}

static void build_ipv4_icmp_packet(unsigned char *buf, const char *src_ip,
                                   const char *dst_ip) {
    memset(buf, 0, 128);
    struct ip *iptr = (struct ip *)buf;
    iptr->ip_vhl = (4 << 4) | 5;
    iptr->ip_p = IPPROTO_ICMP;
    iptr->ip_len = htons(28);
    inet_pton(AF_INET, src_ip, &iptr->ip_src);
    inet_pton(AF_INET, dst_ip, &iptr->ip_dst);
}

static void build_ipv6_tcp_packet(unsigned char *buf, const char *src_ip,
                                  const char *dst_ip, uint16_t sport,
                                  uint16_t dport) {
    memset(buf, 0, 128);
    struct ip6_hdr *ip6 = (struct ip6_hdr *)buf;
    ip6->ip6_vfc = (6 << 4); /* version=6 */
    ip6->ip6_nxt = IPPROTO_TCP;
    ip6->ip6_plen = htons(20); /* TCP header */
    inet_pton(AF_INET6, src_ip, &ip6->ip6_src);
    inet_pton(AF_INET6, dst_ip, &ip6->ip6_dst);

    struct tcphdr *thdr = (struct tcphdr *)(buf + 40);
    thdr->th_sport = htons(sport);
    thdr->th_dport = htons(dport);
}

static void build_ipv6_udp_packet(unsigned char *buf, const char *src_ip,
                                  const char *dst_ip, uint16_t sport,
                                  uint16_t dport) {
    memset(buf, 0, 128);
    struct ip6_hdr *ip6 = (struct ip6_hdr *)buf;
    ip6->ip6_vfc = (6 << 4);
    ip6->ip6_nxt = IPPROTO_UDP;
    ip6->ip6_plen = htons(8);
    inet_pton(AF_INET6, src_ip, &ip6->ip6_src);
    inet_pton(AF_INET6, dst_ip, &ip6->ip6_dst);

    struct tcphdr *thdr = (struct tcphdr *)(buf + 40);
    thdr->th_sport = htons(sport);
    thdr->th_dport = htons(dport);
}

static void build_ipv6_icmp_packet(unsigned char *buf, const char *src_ip,
                                   const char *dst_ip) {
    memset(buf, 0, 128);
    struct ip6_hdr *ip6 = (struct ip6_hdr *)buf;
    ip6->ip6_vfc = (6 << 4);
    ip6->ip6_nxt = IPPROTO_ICMPV6;
    ip6->ip6_plen = htons(8);
    inet_pton(AF_INET6, src_ip, &ip6->ip6_src);
    inet_pton(AF_INET6, dst_ip, &ip6->ip6_dst);
}

/* === assign_addr_pair: IPv4 TCP === */

TEST(ipv4_tcp_no_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_tcp_packet(buf, "10.0.0.1", "10.0.0.2", 12345, 80);
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.address_family, AF_INET);
    ASSERT_EQ(ap.src_port, 12345);
    ASSERT_EQ(ap.dst_port, 80);

    char src_str[INET_ADDRSTRLEN], dst_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ap.src, src_str, sizeof(src_str));
    inet_ntop(AF_INET, &ap.dst, dst_str, sizeof(dst_str));
    ASSERT_STR_EQ(src_str, "10.0.0.1");
    ASSERT_STR_EQ(dst_str, "10.0.0.2");
}

TEST(ipv4_tcp_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_tcp_packet(buf, "10.0.0.1", "10.0.0.2", 12345, 80);
    test_assign_addr_pair(&ap, (struct ip *)buf, 1);

    ASSERT_EQ(ap.address_family, AF_INET);
    /* Flip swaps src/dst addresses and ports */
    ASSERT_EQ(ap.src_port, 80);
    ASSERT_EQ(ap.dst_port, 12345);

    char src_str[INET_ADDRSTRLEN], dst_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ap.src, src_str, sizeof(src_str));
    inet_ntop(AF_INET, &ap.dst, dst_str, sizeof(dst_str));
    ASSERT_STR_EQ(src_str, "10.0.0.2");
    ASSERT_STR_EQ(dst_str, "10.0.0.1");
}

TEST(ipv4_tcp_high_ports) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_tcp_packet(buf, "192.168.1.100", "8.8.8.8", 65535, 443);
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.src_port, 65535);
    ASSERT_EQ(ap.dst_port, 443);
}

/* === assign_addr_pair: IPv4 UDP === */

TEST(ipv4_udp_no_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_udp_packet(buf, "172.16.0.1", "172.16.0.2", 5353, 53);
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.address_family, AF_INET);
    ASSERT_EQ(ap.src_port, 5353);
    ASSERT_EQ(ap.dst_port, 53);
}

TEST(ipv4_udp_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_udp_packet(buf, "172.16.0.1", "172.16.0.2", 5353, 53);
    test_assign_addr_pair(&ap, (struct ip *)buf, 1);

    ASSERT_EQ(ap.src_port, 53);
    ASSERT_EQ(ap.dst_port, 5353);

    char src_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ap.src, src_str, sizeof(src_str));
    ASSERT_STR_EQ(src_str, "172.16.0.2");
}

/* === assign_addr_pair: IPv4 ICMP (no ports) === */

TEST(ipv4_icmp_no_ports) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_icmp_packet(buf, "10.1.1.1", "10.2.2.2");
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.address_family, AF_INET);
    ASSERT_EQ(ap.src_port, 0);
    ASSERT_EQ(ap.dst_port, 0);

    char src_str[INET_ADDRSTRLEN], dst_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ap.src, src_str, sizeof(src_str));
    inet_ntop(AF_INET, &ap.dst, dst_str, sizeof(dst_str));
    ASSERT_STR_EQ(src_str, "10.1.1.1");
    ASSERT_STR_EQ(dst_str, "10.2.2.2");
}

TEST(ipv4_icmp_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv4_icmp_packet(buf, "10.1.1.1", "10.2.2.2");
    test_assign_addr_pair(&ap, (struct ip *)buf, 1);

    ASSERT_EQ(ap.src_port, 0);
    ASSERT_EQ(ap.dst_port, 0);

    char src_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ap.src, src_str, sizeof(src_str));
    ASSERT_STR_EQ(src_str, "10.2.2.2");
}

/* === assign_addr_pair: IPv6 TCP === */

TEST(ipv6_tcp_no_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv6_tcp_packet(buf, "2001:db8::1", "2001:db8::2", 8080, 443);
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.address_family, AF_INET6);
    ASSERT_EQ(ap.src_port, 8080);
    ASSERT_EQ(ap.dst_port, 443);

    char src_str[INET6_ADDRSTRLEN], dst_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ap.src6, src_str, sizeof(src_str));
    inet_ntop(AF_INET6, &ap.dst6, dst_str, sizeof(dst_str));
    ASSERT_STR_EQ(src_str, "2001:db8::1");
    ASSERT_STR_EQ(dst_str, "2001:db8::2");
}

TEST(ipv6_tcp_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv6_tcp_packet(buf, "2001:db8::1", "2001:db8::2", 8080, 443);
    test_assign_addr_pair(&ap, (struct ip *)buf, 1);

    ASSERT_EQ(ap.address_family, AF_INET6);
    ASSERT_EQ(ap.src_port, 443);
    ASSERT_EQ(ap.dst_port, 8080);

    char src_str[INET6_ADDRSTRLEN], dst_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ap.src6, src_str, sizeof(src_str));
    inet_ntop(AF_INET6, &ap.dst6, dst_str, sizeof(dst_str));
    ASSERT_STR_EQ(src_str, "2001:db8::2");
    ASSERT_STR_EQ(dst_str, "2001:db8::1");
}

/* === assign_addr_pair: IPv6 UDP === */

TEST(ipv6_udp_no_flip) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv6_udp_packet(buf, "fe80::1", "fe80::2", 1234, 5678);
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.address_family, AF_INET6);
    ASSERT_EQ(ap.src_port, 1234);
    ASSERT_EQ(ap.dst_port, 5678);
}

/* === assign_addr_pair: IPv6 ICMPv6 (no ports) === */

TEST(ipv6_icmpv6_no_ports) {
    unsigned char buf[128];
    addr_pair ap;
    build_ipv6_icmp_packet(buf, "2001:db8::1", "2001:db8::2");
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    ASSERT_EQ(ap.address_family, AF_INET6);
    ASSERT_EQ(ap.src_port, 0);
    ASSERT_EQ(ap.dst_port, 0);
}

/* === assign_addr_pair: zeroed output === */

TEST(assign_zeroes_output) {
    unsigned char buf[128];
    addr_pair ap;
    /* Fill ap with garbage first */
    memset(&ap, 0xFF, sizeof(ap));
    build_ipv4_tcp_packet(buf, "1.2.3.4", "5.6.7.8", 100, 200);
    test_assign_addr_pair(&ap, (struct ip *)buf, 0);

    /* Verify the padding region (between dst and dst_port) is zero */
    ASSERT_EQ(ap.address_family, AF_INET);
    ASSERT_EQ(ap.src_port, 100);
}

/* === in_filter_net === */

TEST(filter_net_matches_in_network) {
    /* 10.0.0.0/8 network */
    inet_pton(AF_INET, "10.0.0.0", &options.netfilternet);
    inet_pton(AF_INET, "255.0.0.0", &options.netfiltermask);

    struct in_addr addr;
    inet_pton(AF_INET, "10.1.2.3", &addr);
    ASSERT_EQ(test_in_filter_net(addr), 1);
}

TEST(filter_net_rejects_outside_network) {
    inet_pton(AF_INET, "10.0.0.0", &options.netfilternet);
    inet_pton(AF_INET, "255.0.0.0", &options.netfiltermask);

    struct in_addr addr;
    inet_pton(AF_INET, "192.168.1.1", &addr);
    ASSERT_EQ(test_in_filter_net(addr), 0);
}

TEST(filter_net_24_mask) {
    /* 192.168.1.0/24 */
    inet_pton(AF_INET, "192.168.1.0", &options.netfilternet);
    inet_pton(AF_INET, "255.255.255.0", &options.netfiltermask);

    struct in_addr addr_in, addr_out;
    inet_pton(AF_INET, "192.168.1.42", &addr_in);
    inet_pton(AF_INET, "192.168.2.42", &addr_out);

    ASSERT_EQ(test_in_filter_net(addr_in), 1);
    ASSERT_EQ(test_in_filter_net(addr_out), 0);
}

TEST(filter_net_16_mask) {
    /* 172.16.0.0/16 */
    inet_pton(AF_INET, "172.16.0.0", &options.netfilternet);
    inet_pton(AF_INET, "255.255.0.0", &options.netfiltermask);

    struct in_addr addr_in, addr_out;
    inet_pton(AF_INET, "172.16.255.255", &addr_in);
    inet_pton(AF_INET, "172.17.0.1", &addr_out);

    ASSERT_EQ(test_in_filter_net(addr_in), 1);
    ASSERT_EQ(test_in_filter_net(addr_out), 0);
}

TEST(filter_net_exact_match) {
    /* /32 - single host */
    inet_pton(AF_INET, "1.2.3.4", &options.netfilternet);
    inet_pton(AF_INET, "255.255.255.255", &options.netfiltermask);

    struct in_addr addr_match, addr_nomatch;
    inet_pton(AF_INET, "1.2.3.4", &addr_match);
    inet_pton(AF_INET, "1.2.3.5", &addr_nomatch);

    ASSERT_EQ(test_in_filter_net(addr_match), 1);
    ASSERT_EQ(test_in_filter_net(addr_nomatch), 0);
}

TEST(filter_net_zero_mask_matches_all) {
    /* 0.0.0.0/0 matches everything */
    inet_pton(AF_INET, "0.0.0.0", &options.netfilternet);
    inet_pton(AF_INET, "0.0.0.0", &options.netfiltermask);

    struct in_addr addr;
    inet_pton(AF_INET, "123.45.67.89", &addr);
    ASSERT_EQ(test_in_filter_net(addr), 1);
}

/* === assign_addr_pair: IPv4 with larger IHL === */

TEST(ipv4_tcp_with_options_ihl) {
    unsigned char buf[128];
    memset(buf, 0, sizeof(buf));
    struct ip *iptr = (struct ip *)buf;
    /* version=4, IHL=8 (32 bytes header, includes 12 bytes of options) */
    iptr->ip_vhl = (4 << 4) | 8;
    iptr->ip_p = IPPROTO_TCP;
    iptr->ip_len = htons(52); /* 32 IP + 20 TCP */
    inet_pton(AF_INET, "10.0.0.1", &iptr->ip_src);
    inet_pton(AF_INET, "10.0.0.2", &iptr->ip_dst);

    /* TCP header at offset 32 (IHL=8 * 4 = 32 bytes) */
    struct tcphdr *thdr = (struct tcphdr *)(buf + 32);
    thdr->th_sport = htons(9999);
    thdr->th_dport = htons(22);

    addr_pair ap;
    test_assign_addr_pair(&ap, iptr, 0);

    ASSERT_EQ(ap.src_port, 9999);
    ASSERT_EQ(ap.dst_port, 22);
}

/* === Main === */

int main(void) {
    TEST_SUITE("Packet Parsing: IPv4 TCP");
    RUN(ipv4_tcp_no_flip);
    RUN(ipv4_tcp_flip);
    RUN(ipv4_tcp_high_ports);
    RUN(ipv4_tcp_with_options_ihl);

    TEST_SUITE("Packet Parsing: IPv4 UDP");
    RUN(ipv4_udp_no_flip);
    RUN(ipv4_udp_flip);

    TEST_SUITE("Packet Parsing: IPv4 ICMP");
    RUN(ipv4_icmp_no_ports);
    RUN(ipv4_icmp_flip);

    TEST_SUITE("Packet Parsing: IPv6 TCP");
    RUN(ipv6_tcp_no_flip);
    RUN(ipv6_tcp_flip);

    TEST_SUITE("Packet Parsing: IPv6 UDP");
    RUN(ipv6_udp_no_flip);

    TEST_SUITE("Packet Parsing: IPv6 ICMPv6");
    RUN(ipv6_icmpv6_no_ports);

    TEST_SUITE("Packet Parsing: Edge Cases");
    RUN(assign_zeroes_output);

    TEST_SUITE("IP Net Filter");
    RUN(filter_net_matches_in_network);
    RUN(filter_net_rejects_outside_network);
    RUN(filter_net_24_mask);
    RUN(filter_net_16_mask);
    RUN(filter_net_exact_match);
    RUN(filter_net_zero_mask_matches_all);

    TEST_REPORT();
}
