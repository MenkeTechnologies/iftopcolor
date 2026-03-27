/*
 * test_procinfo.c: tests for per-process socket attribution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "test.h"
#include "procinfo.h"

/* ── Platform availability ──────────────────────────────────────── */

TEST(procinfo_available_returns_expected) {
#if defined(__APPLE__) || defined(__linux__)
    ASSERT_EQ(procinfo_available(), 1);
#else
    ASSERT_EQ(procinfo_available(), 0);
#endif
}

/* ── Lookup with no refresh returns 0 ───────────────────────────── */

TEST(lookup_before_refresh_returns_not_found) {
    struct in_addr addr;
    inet_aton("127.0.0.1", &addr);
    proc_entry pe;
    ASSERT_EQ(procinfo_lookup(AF_INET, &addr, 12345, IPPROTO_TCP, &pe), 0);
}

/* ── Refresh + lookup against a bound socket ────────────────────── */

TEST(refresh_finds_own_tcp_socket) {
    if (!procinfo_available()) return;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(sock >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; /* let OS pick */

    int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    ASSERT_EQ(ret, 0);

    ret = listen(sock, 1);
    ASSERT_EQ(ret, 0);

    /* Find out which port was assigned */
    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    unsigned short port = ntohs(addr.sin_port);

    procinfo_refresh();

    proc_entry pe;
    memset(&pe, 0, sizeof(pe));
    struct in_addr loopback;
    loopback.s_addr = htonl(INADDR_LOOPBACK);

    int found = procinfo_lookup(AF_INET, &loopback, port, IPPROTO_TCP, &pe);
    ASSERT_EQ(found, 1);
    ASSERT_EQ(pe.pid, getpid());
    ASSERT(pe.name[0] != '\0');

    close(sock);
}

TEST(refresh_finds_own_udp_socket) {
    if (!procinfo_available()) return;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT(sock >= 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    ASSERT_EQ(ret, 0);

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    unsigned short port = ntohs(addr.sin_port);

    procinfo_refresh();

    proc_entry pe;
    memset(&pe, 0, sizeof(pe));
    struct in_addr loopback;
    loopback.s_addr = htonl(INADDR_LOOPBACK);

    int found = procinfo_lookup(AF_INET, &loopback, port, IPPROTO_UDP, &pe);
    ASSERT_EQ(found, 1);
    ASSERT_EQ(pe.pid, getpid());

    close(sock);
}

/* ── Lookup for unused port returns not found ───────────────────── */

TEST(lookup_unused_port_returns_not_found) {
    if (!procinfo_available()) return;

    procinfo_refresh();

    proc_entry pe;
    struct in_addr loopback;
    loopback.s_addr = htonl(INADDR_LOOPBACK);

    /* Port 1 is unlikely to be in use by this test process */
    int found = procinfo_lookup(AF_INET, &loopback, 1, IPPROTO_TCP, &pe);
    ASSERT_EQ(found, 0);
}

/* ── IPv6 socket lookup ─────────────────────────────────────────── */

TEST(refresh_finds_own_tcp6_socket) {
    if (!procinfo_available()) return;

    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0) return; /* IPv6 not available */

    /* Disable dual-stack so we get a pure IPv6 socket */
    int off = 1;
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_loopback;
    addr.sin6_port = 0;

    int ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) { close(sock); return; } /* bind failed, skip */

    ret = listen(sock, 1);
    ASSERT_EQ(ret, 0);

    socklen_t addrlen = sizeof(addr);
    getsockname(sock, (struct sockaddr *)&addr, &addrlen);
    unsigned short port = ntohs(addr.sin6_port);

    procinfo_refresh();

    proc_entry pe;
    memset(&pe, 0, sizeof(pe));

    int found = procinfo_lookup(AF_INET6, &in6addr_loopback, port, IPPROTO_TCP, &pe);
    ASSERT_EQ(found, 1);
    ASSERT_EQ(pe.pid, getpid());

    close(sock);
}

/* ── Destroy does not crash ─────────────────────────────────────── */

TEST(destroy_is_safe) {
    procinfo_refresh();
    procinfo_destroy();
    /* Lookup after destroy should return 0, not crash */
    proc_entry pe;
    struct in_addr loopback;
    loopback.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_EQ(procinfo_lookup(AF_INET, &loopback, 80, IPPROTO_TCP, &pe), 0);
}

TEST(double_destroy_is_safe) {
    procinfo_destroy();
    procinfo_destroy();
}

/* ── Multiple refreshes do not leak ─────────────────────────────── */

TEST(multiple_refreshes) {
    if (!procinfo_available()) return;

    for (int i = 0; i < 10; i++) {
        procinfo_refresh();
    }
    procinfo_destroy();
}

/* ── Main ───────────────────────────────────────────────────────── */

int main(void) {
    TEST_SUITE("procinfo");

    RUN(procinfo_available_returns_expected);
    RUN(lookup_before_refresh_returns_not_found);
    RUN(refresh_finds_own_tcp_socket);
    RUN(refresh_finds_own_udp_socket);
    RUN(lookup_unused_port_returns_not_found);
    RUN(refresh_finds_own_tcp6_socket);
    RUN(destroy_is_safe);
    RUN(double_destroy_is_safe);
    RUN(multiple_refreshes);

    TEST_REPORT();
}
