/*
 * test_screenfilter.c: tests for screen filter regex matching
 *
 * Tests screen_filter_set() and screen_filter_match() from screenfilter.c.
 * Verifies regex compilation, case-insensitive matching, and edge cases.
 */

#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "iftop.h"
#include "options.h"
#include "screenfilter.h"

/* Provide the global options that screenfilter.c references */
options_t options;

/* === screen_filter_set === */

TEST(filter_set_valid_pattern) {
    char *pattern = xstrdup("hello");
    int result = screen_filter_set(pattern);
    ASSERT_EQ(result, 1);
    ASSERT_STR_EQ(options.screenfilter, "hello");
    /* Cleanup */
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_set_regex_pattern) {
    char *pattern = xstrdup("^10\\.0\\..*");
    int result = screen_filter_set(pattern);
    ASSERT_EQ(result, 1);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_set_replaces_previous) {
    char *p1 = xstrdup("first");
    char *p2 = xstrdup("second");
    screen_filter_set(p1);
    ASSERT_STR_EQ(options.screenfilter, "first");
    screen_filter_set(p2);
    ASSERT_STR_EQ(options.screenfilter, "second");
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_set_extended_regex) {
    char *pattern = xstrdup("(foo|bar)+");
    int result = screen_filter_set(pattern);
    ASSERT_EQ(result, 1);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

/* === screen_filter_match === */

TEST(filter_match_no_filter_matches_all) {
    options.screenfilter = NULL;
    ASSERT_EQ(screen_filter_match("anything"), 1);
    ASSERT_EQ(screen_filter_match(""), 1);
}

TEST(filter_match_simple_string) {
    char *pattern = xstrdup("example");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match("example.com"), 1);
    ASSERT_EQ(screen_filter_match("test.example.org"), 1);
    ASSERT_EQ(screen_filter_match("nope"), 0);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_match_case_insensitive) {
    char *pattern = xstrdup("hello");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match("HELLO"), 1);
    ASSERT_EQ(screen_filter_match("Hello World"), 1);
    ASSERT_EQ(screen_filter_match("hElLo"), 1);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_match_ip_pattern) {
    char *pattern = xstrdup("^192\\.168\\.");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match("192.168.1.1"), 1);
    ASSERT_EQ(screen_filter_match("192.168.100.5"), 1);
    ASSERT_EQ(screen_filter_match("10.0.0.1"), 0);
    ASSERT_EQ(screen_filter_match("x192.168.1.1"), 0);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_match_alternation) {
    char *pattern = xstrdup("(google|github)");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match("google.com"), 1);
    ASSERT_EQ(screen_filter_match("github.com"), 1);
    ASSERT_EQ(screen_filter_match("example.com"), 0);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_match_dot_matches_any) {
    char *pattern = xstrdup("a.c");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match("abc"), 1);
    ASSERT_EQ(screen_filter_match("aXc"), 1);
    ASSERT_EQ(screen_filter_match("ac"), 0);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_match_empty_text) {
    char *pattern = xstrdup(".");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match(""), 0);
    ASSERT_EQ(screen_filter_match("x"), 1);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

TEST(filter_match_anchor_end) {
    char *pattern = xstrdup("\\.com$");
    screen_filter_set(pattern);
    ASSERT_EQ(screen_filter_match("example.com"), 1);
    ASSERT_EQ(screen_filter_match("example.com.au"), 0);
    xfree(options.screenfilter);
    options.screenfilter = NULL;
}

/* === Main === */

int main(void) {
    /* Ensure clean initial state */
    memset(&options, 0, sizeof(options));

    TEST_SUITE("Screen Filter: Set");
    RUN(filter_set_valid_pattern);
    RUN(filter_set_regex_pattern);
    RUN(filter_set_replaces_previous);
    RUN(filter_set_extended_regex);

    TEST_SUITE("Screen Filter: Match");
    RUN(filter_match_no_filter_matches_all);
    RUN(filter_match_simple_string);
    RUN(filter_match_case_insensitive);
    RUN(filter_match_ip_pattern);
    RUN(filter_match_alternation);
    RUN(filter_match_dot_matches_any);
    RUN(filter_match_empty_text);
    RUN(filter_match_anchor_end);

    TEST_REPORT();
}
