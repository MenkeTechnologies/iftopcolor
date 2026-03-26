/*
 * test_ui_format.c: tests for UI formatting, sorting, and display logic
 *
 * Tests the pure functions from ui.c that don't depend on ncurses:
 * readable_size, convertColorToInt, convertBoldToInt,
 * screen_line_*_compare, history_length, get_bar_interval.
 *
 * Since ui.c cannot be compiled without ncurses, the testable functions
 * are copied here with their necessary types and constants.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "test.h"
#include "iftop.h"
#include "options.h"
#include "addr_hash.h"

/* --- Types and constants from ui.c --- */

#define HISTORY_DIVISIONS 3
#define UNIT_DIVISIONS    4
#define HOSTNAME_LENGTH   256

static char *unit_bits[UNIT_DIVISIONS] = {"b", "kb", "Mb", "Gb"};
static char *unit_bytes[UNIT_DIVISIONS] = {"B", "kB", "MB", "GB"};

int history_divs[HISTORY_DIVISIONS] = {1, 5, 20};

typedef struct host_pair_line_tag {
    addr_pair ap;
    unsigned long long total_recv;
    unsigned long long total_sent;
    double long recv[HISTORY_DIVISIONS];
    double long sent[HISTORY_DIVISIONS];
    char cached_hostname[HOSTNAME_LENGTH];
} host_pair_line;

/* Globals referenced by the functions under test */
options_t options;
int history_len = 1;

/* --- Functions under test (copied from ui.c) --- */

static int test_convertColorToInt(char *color) {
    int i = 0;
    for (i = 0; color[i]; i++) {
        color[i] = tolower(color[i]);
    }
    if (strcmp(color, "green") == 0) return 4;
    if (strcmp(color, "red") == 0) return 5;
    if (strcmp(color, "blue") == 0) return 6;
    if (strcmp(color, "yellow") == 0) return 7;
    if (strcmp(color, "magenta") == 0) return 8;
    if (strcmp(color, "cyan") == 0) return 9;
    if (strcmp(color, "black") == 0) return 10;
    if (strcmp(color, "white") == 0) return 11;
    return -1;
}

static int test_convertBoldToInt(char *bold) {
    int i = 0;
    for (i = 0; bold[i]; i++) {
        bold[i] = tolower(bold[i]);
    }
    if (strcmp(bold, "bold") == 0) return 1;
    if (strcmp(bold, "nonbold") == 0) return 0;
    return 0;
}

static void test_readable_size(float bandwidth, char *buf, int bsize, int ksize, int bytes) {
    int i = 0;
    float size = 1;

    if (bytes == 0) {
        bandwidth *= 8;
    }

    while (1) {
        if (bandwidth < size * 1000 || i >= UNIT_DIVISIONS - 1) {
            snprintf(buf, bsize, " %4.0f%s", bandwidth / size,
                     bytes ? unit_bytes[i] : unit_bits[i]);
            break;
        }
        i++;
        size *= ksize;
        if (bandwidth < size * 10) {
            snprintf(buf, bsize, " %4.2f%s", bandwidth / size,
                     bytes ? unit_bytes[i] : unit_bits[i]);
            break;
        } else if (bandwidth < size * 100) {
            snprintf(buf, bsize, " %4.1f%s", bandwidth / size,
                     bytes ? unit_bytes[i] : unit_bits[i]);
            break;
        }
    }
}

static int test_screen_line_bandwidth_compare(host_pair_line *aa, host_pair_line *bb,
                                              int start_div) {
    int i;
    switch (options.linedisplay) {
    case OPTION_LINEDISPLAY_ONE_LINE_SENT:
        for (i = start_div; i < HISTORY_DIVISIONS; i++) {
            if (aa->sent[i] != bb->sent[i]) {
                return (aa->sent[i] < bb->sent[i]);
            }
        }
        break;
    case OPTION_LINEDISPLAY_ONE_LINE_RECV:
        for (i = start_div; i < HISTORY_DIVISIONS; i++) {
            if (aa->recv[i] != bb->recv[i]) {
                return (aa->recv[i] < bb->recv[i]);
            }
        }
        break;
    case OPTION_LINEDISPLAY_TWO_LINE:
    case OPTION_LINEDISPLAY_ONE_LINE_BOTH:
        break;
    }
    for (i = start_div; i < HISTORY_DIVISIONS; i++) {
        if (aa->recv[i] + aa->sent[i] != bb->recv[i] + bb->sent[i]) {
            return (aa->recv[i] + aa->sent[i] < bb->recv[i] + bb->sent[i]);
        }
    }
    return 1;
}

static int test_screen_line_host_compare(host_pair_line *aa, host_pair_line *bb) {
    int cmp_result = strcmp(aa->cached_hostname, bb->cached_hostname);
    if (cmp_result == 0) {
        return test_screen_line_bandwidth_compare(aa, bb, 2);
    } else {
        return (cmp_result > 0);
    }
}

static int test_history_length(const int division) {
    if (history_len < history_divs[division]) {
        return history_len * RESOLUTION;
    } else {
        return history_divs[division] * RESOLUTION;
    }
}

static int test_get_bar_interval(float bandwidth) {
    int i = 10;
    if (bandwidth > 100000000) {
        i = 100;
    }
    return i;
}

/* --- Helper --- */

static host_pair_line make_line(double long s0, double long s1, double long s2,
                                double long r0, double long r1, double long r2,
                                const char *hostname) {
    host_pair_line l;
    memset(&l, 0, sizeof(l));
    l.sent[0] = s0;
    l.sent[1] = s1;
    l.sent[2] = s2;
    l.recv[0] = r0;
    l.recv[1] = r1;
    l.recv[2] = r2;
    if (hostname) {
        strncpy(l.cached_hostname, hostname, HOSTNAME_LENGTH - 1);
    }
    return l;
}

/* === convertColorToInt === */

TEST(color_green) {
    char c[] = "green";
    ASSERT_EQ(test_convertColorToInt(c), 4);
}

TEST(color_red) {
    char c[] = "red";
    ASSERT_EQ(test_convertColorToInt(c), 5);
}

TEST(color_blue) {
    char c[] = "blue";
    ASSERT_EQ(test_convertColorToInt(c), 6);
}

TEST(color_yellow) {
    char c[] = "yellow";
    ASSERT_EQ(test_convertColorToInt(c), 7);
}

TEST(color_magenta) {
    char c[] = "magenta";
    ASSERT_EQ(test_convertColorToInt(c), 8);
}

TEST(color_cyan) {
    char c[] = "cyan";
    ASSERT_EQ(test_convertColorToInt(c), 9);
}

TEST(color_black) {
    char c[] = "black";
    ASSERT_EQ(test_convertColorToInt(c), 10);
}

TEST(color_white) {
    char c[] = "white";
    ASSERT_EQ(test_convertColorToInt(c), 11);
}

TEST(color_uppercase) {
    char c[] = "GREEN";
    ASSERT_EQ(test_convertColorToInt(c), 4);
}

TEST(color_mixed_case) {
    char c[] = "MaGeNtA";
    ASSERT_EQ(test_convertColorToInt(c), 8);
}

TEST(color_invalid) {
    char c[] = "orange";
    ASSERT_EQ(test_convertColorToInt(c), -1);
}

TEST(color_empty_string) {
    char c[] = "";
    ASSERT_EQ(test_convertColorToInt(c), -1);
}

/* === convertBoldToInt === */

TEST(bold_bold) {
    char b[] = "bold";
    ASSERT_EQ(test_convertBoldToInt(b), 1);
}

TEST(bold_nonbold) {
    char b[] = "nonbold";
    ASSERT_EQ(test_convertBoldToInt(b), 0);
}

TEST(bold_uppercase) {
    char b[] = "BOLD";
    ASSERT_EQ(test_convertBoldToInt(b), 1);
}

TEST(bold_invalid_defaults_zero) {
    char b[] = "something";
    ASSERT_EQ(test_convertBoldToInt(b), 0);
}

TEST(bold_dash_defaults_zero) {
    char b[] = "-";
    ASSERT_EQ(test_convertBoldToInt(b), 0);
}

/* === readable_size === */

TEST(readable_size_zero_bits) {
    char buf[32];
    test_readable_size(0, buf, sizeof(buf), 1024, 0);
    ASSERT(strstr(buf, "0") != NULL);
    ASSERT(strstr(buf, "b") != NULL);
}

TEST(readable_size_small_bits) {
    char buf[32];
    /* 100 bytes/s -> 800 bits/s */
    test_readable_size(100, buf, sizeof(buf), 1024, 0);
    ASSERT(strstr(buf, "800") != NULL);
    ASSERT(strstr(buf, "b") != NULL);
}

TEST(readable_size_kilobits) {
    char buf[32];
    /* 12500 bytes/s = 100000 bits/s = 100 kb */
    test_readable_size(12500, buf, sizeof(buf), 1000, 0);
    ASSERT(strstr(buf, "100") != NULL);
    ASSERT(strstr(buf, "kb") != NULL);
}

TEST(readable_size_megabits) {
    char buf[32];
    /* 12500000 bytes/s = 100Mb */
    test_readable_size(12500000, buf, sizeof(buf), 1000, 0);
    ASSERT(strstr(buf, "100") != NULL);
    ASSERT(strstr(buf, "Mb") != NULL);
}

TEST(readable_size_bytes_mode) {
    char buf[32];
    /* 500 bytes/s in byte mode */
    test_readable_size(500, buf, sizeof(buf), 1024, 1);
    ASSERT(strstr(buf, "500") != NULL);
    ASSERT(strstr(buf, "B") != NULL);
}

TEST(readable_size_kilobytes) {
    char buf[32];
    /* 100000 bytes/s in byte mode -> ~97.6 kB (ksize=1024) */
    test_readable_size(100000, buf, sizeof(buf), 1024, 1);
    ASSERT(strstr(buf, "kB") != NULL);
}

TEST(readable_size_megabytes) {
    char buf[32];
    test_readable_size(50000000, buf, sizeof(buf), 1000, 1);
    ASSERT(strstr(buf, "MB") != NULL);
}

TEST(readable_size_gigabits) {
    char buf[32];
    /* 200000000 bytes/s = 1.6 Gb */
    test_readable_size(200000000, buf, sizeof(buf), 1000, 0);
    ASSERT(strstr(buf, "Gb") != NULL);
}

/* === screen_line_bandwidth_compare === */

TEST(bandwidth_compare_sent_mode_higher_wins) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_SENT;
    host_pair_line a = make_line(1000, 500, 100, 0, 0, 0, NULL);
    host_pair_line b = make_line(2000, 500, 100, 0, 0, 0, NULL);

    /* a < b in sent, so compare(a, b) should return 1 (a sorts lower) */
    ASSERT_EQ(test_screen_line_bandwidth_compare(&a, &b, 0), 1);
    /* b > a, so compare(b, a) should return 0 */
    ASSERT_EQ(test_screen_line_bandwidth_compare(&b, &a, 0), 0);
}

TEST(bandwidth_compare_recv_mode) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_RECV;
    host_pair_line a = make_line(0, 0, 0, 100, 200, 300, NULL);
    host_pair_line b = make_line(0, 0, 0, 200, 200, 300, NULL);

    ASSERT_EQ(test_screen_line_bandwidth_compare(&a, &b, 0), 1);
    ASSERT_EQ(test_screen_line_bandwidth_compare(&b, &a, 0), 0);
}

TEST(bandwidth_compare_both_mode_uses_sum) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_BOTH;
    host_pair_line a = make_line(100, 0, 0, 100, 0, 0, NULL); /* sum=200 */
    host_pair_line b = make_line(200, 0, 0, 200, 0, 0, NULL); /* sum=400 */

    ASSERT_EQ(test_screen_line_bandwidth_compare(&a, &b, 0), 1);
}

TEST(bandwidth_compare_equal_returns_one) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_SENT;
    host_pair_line a = make_line(100, 200, 300, 0, 0, 0, NULL);
    host_pair_line b = make_line(100, 200, 300, 0, 0, 0, NULL);

    ASSERT_EQ(test_screen_line_bandwidth_compare(&a, &b, 0), 1);
}

TEST(bandwidth_compare_start_div_skips_columns) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_SENT;
    host_pair_line a = make_line(9999, 100, 50, 0, 0, 0, NULL);
    host_pair_line b = make_line(1, 200, 50, 0, 0, 0, NULL);

    /* Starting from div 1, ignore div 0 */
    ASSERT_EQ(test_screen_line_bandwidth_compare(&a, &b, 1), 1);
    ASSERT_EQ(test_screen_line_bandwidth_compare(&b, &a, 1), 0);
}

TEST(bandwidth_compare_two_line_uses_combined) {
    options.linedisplay = OPTION_LINEDISPLAY_TWO_LINE;
    host_pair_line a = make_line(100, 0, 0, 50, 0, 0, NULL);  /* sum=150 */
    host_pair_line b = make_line(200, 0, 0, 100, 0, 0, NULL); /* sum=300 */

    ASSERT_EQ(test_screen_line_bandwidth_compare(&a, &b, 0), 1);
}

/* === screen_line_host_compare === */

TEST(host_compare_alphabetical) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_BOTH;
    host_pair_line a = make_line(0, 0, 0, 0, 0, 0, "alpha.example.com");
    host_pair_line b = make_line(0, 0, 0, 0, 0, 0, "beta.example.com");

    /* "alpha" < "beta" lexicographically, strcmp returns negative,
     * so cmp_result > 0 is false, returns 0 */
    ASSERT_EQ(test_screen_line_host_compare(&a, &b), 0);
    ASSERT_EQ(test_screen_line_host_compare(&b, &a), 1);
}

TEST(host_compare_same_falls_back_to_bandwidth) {
    options.linedisplay = OPTION_LINEDISPLAY_ONE_LINE_BOTH;
    host_pair_line a = make_line(0, 0, 100, 0, 0, 100, "same.host");
    host_pair_line b = make_line(0, 0, 200, 0, 0, 200, "same.host");

    /* Same hostname -> falls back to bandwidth compare at div 2 */
    ASSERT_EQ(test_screen_line_host_compare(&a, &b), 1);
}

/* === history_length === */

TEST(history_length_short) {
    history_len = 1;
    /* Division 0: divs[0]=1, history_len == 1, not < 1, so returns 1*RESOLUTION=2 */
    ASSERT_EQ(test_history_length(0), 1 * RESOLUTION);
    /* Division 1: divs[1]=5, history_len=1 < 5, returns 1*RESOLUTION=2 */
    ASSERT_EQ(test_history_length(1), 1 * RESOLUTION);
}

TEST(history_length_medium) {
    history_len = 5;
    /* Division 0: history_len=5 >= divs[0]=1, returns 1*RESOLUTION=2 */
    ASSERT_EQ(test_history_length(0), history_divs[0] * RESOLUTION);
    /* Division 1: history_len=5 >= divs[1]=5, returns 5*RESOLUTION=10 */
    ASSERT_EQ(test_history_length(1), history_divs[1] * RESOLUTION);
    /* Division 2: history_len=5 < divs[2]=20, returns 5*RESOLUTION=10 */
    ASSERT_EQ(test_history_length(2), 5 * RESOLUTION);
}

TEST(history_length_full) {
    history_len = HISTORY_LENGTH;
    ASSERT_EQ(test_history_length(0), history_divs[0] * RESOLUTION);
    ASSERT_EQ(test_history_length(1), history_divs[1] * RESOLUTION);
    ASSERT_EQ(test_history_length(2), history_divs[2] * RESOLUTION);
}

/* === get_bar_interval === */

TEST(bar_interval_low_bandwidth) {
    ASSERT_EQ(test_get_bar_interval(1000), 10);
    ASSERT_EQ(test_get_bar_interval(50000000), 10);
    ASSERT_EQ(test_get_bar_interval(100000000), 10);
}

TEST(bar_interval_high_bandwidth) {
    /* 100000001 loses precision as a float and becomes 100000000,
     * so use a value clearly above the threshold */
    ASSERT_EQ(test_get_bar_interval(200000000), 100);
    ASSERT_EQ(test_get_bar_interval(1000000000), 100);
}

/* === Main === */

int main(void) {
    TEST_SUITE("UI: Color Conversion");
    RUN(color_green);
    RUN(color_red);
    RUN(color_blue);
    RUN(color_yellow);
    RUN(color_magenta);
    RUN(color_cyan);
    RUN(color_black);
    RUN(color_white);
    RUN(color_uppercase);
    RUN(color_mixed_case);
    RUN(color_invalid);
    RUN(color_empty_string);

    TEST_SUITE("UI: Bold Conversion");
    RUN(bold_bold);
    RUN(bold_nonbold);
    RUN(bold_uppercase);
    RUN(bold_invalid_defaults_zero);
    RUN(bold_dash_defaults_zero);

    TEST_SUITE("UI: Readable Size Formatting");
    RUN(readable_size_zero_bits);
    RUN(readable_size_small_bits);
    RUN(readable_size_kilobits);
    RUN(readable_size_megabits);
    RUN(readable_size_bytes_mode);
    RUN(readable_size_kilobytes);
    RUN(readable_size_megabytes);
    RUN(readable_size_gigabits);

    TEST_SUITE("UI: Bandwidth Sorting");
    RUN(bandwidth_compare_sent_mode_higher_wins);
    RUN(bandwidth_compare_recv_mode);
    RUN(bandwidth_compare_both_mode_uses_sum);
    RUN(bandwidth_compare_equal_returns_one);
    RUN(bandwidth_compare_start_div_skips_columns);
    RUN(bandwidth_compare_two_line_uses_combined);

    TEST_SUITE("UI: Host Sorting");
    RUN(host_compare_alphabetical);
    RUN(host_compare_same_falls_back_to_bandwidth);

    TEST_SUITE("UI: History Length");
    RUN(history_length_short);
    RUN(history_length_medium);
    RUN(history_length_full);

    TEST_SUITE("UI: Bar Interval");
    RUN(bar_interval_low_bandwidth);
    RUN(bar_interval_high_bandwidth);

    TEST_REPORT();
}
