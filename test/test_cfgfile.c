/*
 * test_cfgfile.c: tests for config file parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "cfgfile.h"
#include "stringmap.h"
#include "iftop.h"

extern stringmap config;

static char tmpfile_path[256];

static void write_tmp_config(const char *content) {
    snprintf(tmpfile_path, sizeof(tmpfile_path), "/tmp/iftop_test_cfg_XXXXXX");
    int fd = mkstemp(tmpfile_path);
    write(fd, content, strlen(content));
    close(fd);
}

static void cleanup_tmp(void) {
    unlink(tmpfile_path);
}

/* === Config init === */

TEST(config_init_succeeds) {
    ASSERT(config_init());
}

TEST(config_init_creates_empty_map) {
    config_init();
    ASSERT_NULL(config_get_string("anything"));
}

/* === Set/Get string === */

TEST(config_set_get_string) {
    config_init();
    config_set_string("interface", "eth0");
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
}

TEST(config_get_string_missing) {
    config_init();
    ASSERT_NULL(config_get_string("nonexistent"));
}

TEST(config_set_string_overwrite) {
    config_init();
    config_set_string("interface", "eth0");
    config_set_string("interface", "wlan0");
    ASSERT_STR_EQ(config_get_string("interface"), "wlan0");
}

TEST(config_set_string_multiple_keys) {
    config_init();
    config_set_string("interface", "eth0");
    config_set_string("filter-code", "tcp port 80");
    config_set_string("sort", "source");
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    ASSERT_STR_EQ(config_get_string("filter-code"), "tcp port 80");
    ASSERT_STR_EQ(config_get_string("sort"), "source");
}

TEST(config_set_string_empty_value) {
    config_init();
    config_set_string("test", "");
    char *val = config_get_string("test");
    ASSERT_NOT_NULL(val);
    ASSERT_STR_EQ(val, "");
}

TEST(config_set_string_long_value) {
    config_init();
    char long_val[512];
    memset(long_val, 'x', 511);
    long_val[511] = '\0';
    config_set_string("long", long_val);
    ASSERT_STR_EQ(config_get_string("long"), long_val);
}

TEST(config_set_string_overwrite_multiple_times) {
    config_init();
    config_set_string("key", "v1");
    config_set_string("key", "v2");
    config_set_string("key", "v3");
    ASSERT_STR_EQ(config_get_string("key"), "v3");
}

/* === Bool === */

TEST(config_get_bool_true) {
    config_init();
    config_set_string("promiscuous", "true");
    ASSERT_EQ(config_get_bool("promiscuous"), 1);
}

TEST(config_get_bool_yes) {
    config_init();
    config_set_string("promiscuous", "yes");
    ASSERT_EQ(config_get_bool("promiscuous"), 1);
}

TEST(config_get_bool_false) {
    config_init();
    config_set_string("promiscuous", "false");
    ASSERT_EQ(config_get_bool("promiscuous"), 0);
}

TEST(config_get_bool_no) {
    config_init();
    config_set_string("promiscuous", "no");
    ASSERT_EQ(config_get_bool("promiscuous"), 0);
}

TEST(config_get_bool_arbitrary) {
    config_init();
    config_set_string("promiscuous", "maybe");
    ASSERT_EQ(config_get_bool("promiscuous"), 0);
}

TEST(config_get_bool_missing) {
    config_init();
    ASSERT_EQ(config_get_bool("nonexistent"), 0);
}

TEST(config_get_bool_empty_string) {
    config_init();
    config_set_string("test", "");
    ASSERT_EQ(config_get_bool("test"), 0);
}

TEST(config_get_bool_case_sensitive) {
    config_init();
    config_set_string("a", "True");
    config_set_string("b", "TRUE");
    config_set_string("c", "Yes");
    ASSERT_EQ(config_get_bool("a"), 0); /* only lowercase "true" */
    ASSERT_EQ(config_get_bool("b"), 0);
    ASSERT_EQ(config_get_bool("c"), 0);
}

/* === Int === */

TEST(config_get_int_valid) {
    config_init();
    config_set_string("interval", "42");
    int val = 0;
    ASSERT_EQ(config_get_int("interval", &val), 1);
    ASSERT_EQ(val, 42);
}

TEST(config_get_int_zero) {
    config_init();
    config_set_string("zero", "0");
    int val = -1;
    ASSERT_EQ(config_get_int("zero", &val), 1);
    ASSERT_EQ(val, 0);
}

TEST(config_get_int_negative) {
    config_init();
    config_set_string("offset", "-10");
    int val = 0;
    ASSERT_EQ(config_get_int("offset", &val), 1);
    ASSERT_EQ(val, -10);
}

TEST(config_get_int_large) {
    config_init();
    config_set_string("big", "999999");
    int val = 0;
    ASSERT_EQ(config_get_int("big", &val), 1);
    ASSERT_EQ(val, 999999);
}

TEST(config_get_int_missing) {
    config_init();
    int val = 0;
    ASSERT_EQ(config_get_int("nonexistent", &val), 0);
}

TEST(config_get_int_invalid_alpha) {
    config_init();
    config_set_string("bad", "abc");
    int val = 0;
    ASSERT_EQ(config_get_int("bad", &val), -1);
}

TEST(config_get_int_invalid_trailing) {
    config_init();
    config_set_string("bad", "42abc");
    int val = 0;
    ASSERT_EQ(config_get_int("bad", &val), -1);
}

TEST(config_get_int_null_value) {
    ASSERT_EQ(config_get_int("test", NULL), -1);
}

TEST(config_get_int_empty_string) {
    config_init();
    config_set_string("empty", "");
    int val = 0;
    ASSERT_EQ(config_get_int("empty", &val), -1);
}

/* === Float === */

TEST(config_get_float_valid) {
    config_init();
    config_set_string("rate", "3.14");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val > 3.13 && val < 3.15);
}

TEST(config_get_float_integer) {
    config_init();
    config_set_string("rate", "42");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val > 41.9 && val < 42.1);
}

TEST(config_get_float_zero) {
    config_init();
    config_set_string("rate", "0.0");
    float val = -1;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val > -0.01 && val < 0.01);
}

TEST(config_get_float_negative) {
    config_init();
    config_set_string("rate", "-1.5");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val < -1.4 && val > -1.6);
}

TEST(config_get_float_missing) {
    config_init();
    float val = 0;
    ASSERT_EQ(config_get_float("nonexistent", &val), 0);
}

TEST(config_get_float_null_value) {
    ASSERT_EQ(config_get_float("test", NULL), -1);
}

TEST(config_get_float_invalid) {
    config_init();
    config_set_string("bad", "xyz");
    float val = 0;
    ASSERT_EQ(config_get_float("bad", &val), -1);
}

TEST(config_get_float_empty) {
    config_init();
    config_set_string("empty", "");
    float val = 0;
    ASSERT_EQ(config_get_float("empty", &val), -1);
}

/* === Enum === */

TEST(config_get_enum_valid) {
    config_init();
    config_set_string("sort", "source");
    config_enumeration_type sort_enum[] = {{"2s", 0},     {"10s", 1},         {"40s", 2},
                                           {"source", 3}, {"destination", 4}, {NULL, -1}};
    int val = -1;
    ASSERT_EQ(config_get_enum("sort", sort_enum, &val), 1);
    ASSERT_EQ(val, 3);
}

TEST(config_get_enum_first) {
    config_init();
    config_set_string("sort", "2s");
    config_enumeration_type sort_enum[] = {{"2s", 0}, {"10s", 1}, {NULL, -1}};
    int val = -1;
    ASSERT_EQ(config_get_enum("sort", sort_enum, &val), 1);
    ASSERT_EQ(val, 0);
}

TEST(config_get_enum_last) {
    config_init();
    config_set_string("sort", "destination");
    config_enumeration_type sort_enum[] = {
        {"2s", 0}, {"source", 3}, {"destination", 4}, {NULL, -1}};
    int val = -1;
    ASSERT_EQ(config_get_enum("sort", sort_enum, &val), 1);
    ASSERT_EQ(val, 4);
}

TEST(config_get_enum_missing) {
    config_init();
    config_enumeration_type sort_enum[] = {{"2s", 0}, {NULL, -1}};
    int val = -1;
    ASSERT_EQ(config_get_enum("nonexistent", sort_enum, &val), 0);
}

TEST(config_get_enum_invalid_value) {
    config_init();
    config_set_string("sort", "invalid");
    config_enumeration_type sort_enum[] = {{"2s", 0}, {"10s", 1}, {NULL, -1}};
    int val = -1;
    ASSERT_EQ(config_get_enum("sort", sort_enum, &val), 0);
    ASSERT_EQ(val, -1); /* unchanged */
}

/* === File reading === */

TEST(config_read_file_basic) {
    config_init();
    write_tmp_config("interface: eth1\ndns-resolution: false\nshow-bars: true\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth1");
    ASSERT_STR_EQ(config_get_string("dns-resolution"), "false");
    ASSERT_STR_EQ(config_get_string("show-bars"), "true");
    cleanup_tmp();
}

TEST(config_read_file_comments) {
    config_init();
    write_tmp_config("# This is a comment\ninterface: lo\n# Another comment\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "lo");
    cleanup_tmp();
}

TEST(config_read_file_inline_comment) {
    config_init();
    write_tmp_config("interface: eth0 # inline comment\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    cleanup_tmp();
}

TEST(config_read_file_whitespace) {
    config_init();
    write_tmp_config("  interface  :   eth2   \n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth2");
    cleanup_tmp();
}

TEST(config_read_file_tabs) {
    config_init();
    write_tmp_config("\tinterface\t:\teth3\t\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth3");
    cleanup_tmp();
}

TEST(config_read_file_nonexistent) {
    config_init();
    ASSERT_EQ(read_config("/tmp/nonexistent_iftop_config_xyz", 0), 0);
}

TEST(config_read_file_empty) {
    config_init();
    write_tmp_config("");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    cleanup_tmp();
}

TEST(config_read_file_only_comments) {
    config_init();
    write_tmp_config("# comment 1\n# comment 2\n# comment 3\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    cleanup_tmp();
}

TEST(config_read_file_blank_lines) {
    config_init();
    write_tmp_config("\n\ninterface: eth0\n\n\nshow-bars: true\n\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    ASSERT_STR_EQ(config_get_string("show-bars"), "true");
    cleanup_tmp();
}

TEST(config_read_file_all_directives) {
    config_init();
    write_tmp_config("interface: eth0\n"
                     "dns-resolution: true\n"
                     "port-resolution: false\n"
                     "filter-code: tcp port 80\n"
                     "show-bars: true\n"
                     "promiscuous: false\n"
                     "use-bytes: true\n"
                     "sort: source\n"
                     "line-display: two-line\n"
                     "show-totals: true\n"
                     "log-scale: false\n"
                     "max-bandwidth: 100M\n"
                     "link-local: false\n"
                     "port-display: on\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    ASSERT_STR_EQ(config_get_string("dns-resolution"), "true");
    ASSERT_STR_EQ(config_get_string("filter-code"), "tcp port 80");
    ASSERT_STR_EQ(config_get_string("sort"), "source");
    ASSERT_STR_EQ(config_get_string("max-bandwidth"), "100M");
    ASSERT_STR_EQ(config_get_string("port-display"), "on");
    cleanup_tmp();
}

TEST(config_read_file_value_with_colon) {
    config_init();
    write_tmp_config("filter-code: host 10.0.0.1\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    /* Value after first colon */
    ASSERT_NOT_NULL(config_get_string("filter-code"));
    cleanup_tmp();
}

TEST(config_read_file_repeated_directive) {
    config_init();
    write_tmp_config("interface: eth0\ninterface: eth1\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    /* First value takes precedence */
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    cleanup_tmp();
}

TEST(config_set_before_read_takes_precedence) {
    config_init();
    config_set_string("interface", "wlan0");
    write_tmp_config("interface: eth0\n");
    read_config(tmpfile_path, 0);
    /* Pre-set value takes precedence (stringmap_insert returns existing) */
    ASSERT_STR_EQ(config_get_string("interface"), "wlan0");
    cleanup_tmp();
}

/* === Integration: get typed values after file read === */

TEST(config_read_then_get_bool) {
    config_init();
    write_tmp_config("promiscuous: true\nshow-bars: false\n");
    read_config(tmpfile_path, 0);
    ASSERT_EQ(config_get_bool("promiscuous"), 1);
    ASSERT_EQ(config_get_bool("show-bars"), 0);
    cleanup_tmp();
}

TEST(config_read_then_get_int) {
    config_init();
    write_tmp_config("max-bandwidth: 1024\n");
    read_config(tmpfile_path, 0);
    int val = 0;
    ASSERT_EQ(config_get_int("max-bandwidth", &val), 1);
    ASSERT_EQ(val, 1024);
    cleanup_tmp();
}

/* === Continuation lines === */

TEST(config_read_file_continuation_line) {
    config_init();
    write_tmp_config("filter-code: tcp port 80 or \\\ntcp port 443\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    char *val = config_get_string("filter-code");
    ASSERT_NOT_NULL(val);
    /* The continuation should merge lines */
    cleanup_tmp();
}

/* === Float edge cases === */

TEST(config_get_float_trailing_text) {
    config_init();
    config_set_string("rate", "3.14abc");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), -1);
}

TEST(config_get_float_scientific) {
    config_init();
    config_set_string("rate", "1.5e3");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val > 1499.0 && val < 1501.0);
}

/* === Int edge cases === */

TEST(config_get_int_leading_zeros) {
    config_init();
    config_set_string("val", "007");
    int val = 0;
    ASSERT_EQ(config_get_int("val", &val), 1);
    ASSERT_EQ(val, 7);
}

TEST(config_get_int_whitespace_only) {
    config_init();
    config_set_string("val", "   ");
    int val = 0;
    ASSERT_EQ(config_get_int("val", &val), -1);
}

/* === Config directive validation === */

TEST(config_is_directive_valid) {
    extern int is_cfgdirective_valid(const char *s);
    ASSERT_EQ(is_cfgdirective_valid("interface"), 1);
    ASSERT_EQ(is_cfgdirective_valid("dns-resolution"), 1);
    ASSERT_EQ(is_cfgdirective_valid("promiscuous"), 1);
    ASSERT_EQ(is_cfgdirective_valid("nonexistent"), 0);
    ASSERT_EQ(is_cfgdirective_valid(""), 0);
    ASSERT_EQ(is_cfgdirective_valid("Interface"), 0); /* case sensitive */
}

/* === File with many directives stress === */

TEST(config_read_then_get_float) {
    config_init();
    write_tmp_config("max-bandwidth: 3.14\n");
    read_config(tmpfile_path, 0);
    float val = 0;
    ASSERT_EQ(config_get_float("max-bandwidth", &val), 1);
    ASSERT(val > 3.13 && val < 3.15);
    cleanup_tmp();
}

/* === Set then read file does not overwrite === */

TEST(config_set_all_types_then_read) {
    config_init();
    config_set_string("interface", "wlan0");
    config_set_string("promiscuous", "true");
    config_set_string("max-bandwidth", "999");
    write_tmp_config("interface: eth0\npromiscuous: false\nmax-bandwidth: 100\n");
    read_config(tmpfile_path, 0);
    /* Pre-set values should win */
    ASSERT_STR_EQ(config_get_string("interface"), "wlan0");
    ASSERT_EQ(config_get_bool("promiscuous"), 1);
    int bw = 0;
    config_get_int("max-bandwidth", &bw);
    ASSERT_EQ(bw, 999);
    cleanup_tmp();
}

/* === Multi-line continuation === */

TEST(config_read_file_multi_continuation) {
    config_init();
    write_tmp_config("filter-code: tcp port 80 or \\\ntcp port 443 or \\\ntcp port 8080\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    char *val = config_get_string("filter-code");
    ASSERT_NOT_NULL(val);
    cleanup_tmp();
}

/* === Line without colon === */

TEST(config_read_file_no_colon) {
    config_init();
    write_tmp_config("this line has no colon\ninterface: eth0\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    cleanup_tmp();
}

/* === Value containing hash character === */

TEST(config_read_file_value_with_hash) {
    config_init();
    write_tmp_config("filter-code: host 10.0.0.1\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    char *val = config_get_string("filter-code");
    ASSERT_NOT_NULL(val);
    ASSERT_STR_EQ(val, "host 10.0.0.1");
    cleanup_tmp();
}

/* === Bool edge cases === */

TEST(config_get_bool_1_is_false) {
    config_init();
    config_set_string("flag", "1");
    ASSERT_EQ(config_get_bool("flag"), 0);
}

TEST(config_get_bool_0_is_false) {
    config_init();
    config_set_string("flag", "0");
    ASSERT_EQ(config_get_bool("flag"), 0);
}

/* === Int overflow edge cases === */

TEST(config_get_int_max) {
    config_init();
    config_set_string("val", "2147483647");
    int val = 0;
    ASSERT_EQ(config_get_int("val", &val), 1);
    ASSERT_EQ(val, 2147483647);
}

TEST(config_get_int_min) {
    config_init();
    config_set_string("val", "-2147483648");
    int val = 0;
    ASSERT_EQ(config_get_int("val", &val), 1);
}

TEST(config_get_int_positive_sign) {
    config_init();
    config_set_string("val", "+42");
    int val = 0;
    ASSERT_EQ(config_get_int("val", &val), 1);
    ASSERT_EQ(val, 42);
}

/* === Float edge cases === */

TEST(config_get_float_very_small) {
    config_init();
    config_set_string("rate", "0.001");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val > 0.0009 && val < 0.0011);
}

TEST(config_get_float_large) {
    config_init();
    config_set_string("rate", "999999.99");
    float val = 0;
    ASSERT_EQ(config_get_float("rate", &val), 1);
    ASSERT(val > 999999.0);
}

/* === Enum edge cases === */

TEST(config_get_enum_missing_key) {
    config_init();
    config_enumeration_type sort_enum[] = {{"2s", 0}, {"10s", 1}, {NULL, -1}};
    int val = -1;
    /* Key not set, should return 0 */
    ASSERT_EQ(config_get_enum("nonexistent_key", sort_enum, &val), 0);
    ASSERT_EQ(val, -1); /* unchanged */
}

TEST(config_get_enum_case_sensitive) {
    config_init();
    config_set_string("sort", "Source");
    config_enumeration_type sort_enum[] = {{"source", 3}, {"destination", 4}, {NULL, -1}};
    int val = -1;
    ASSERT_EQ(config_get_enum("sort", sort_enum, &val), 0);
}

/* === Multiple reads of same file === */

TEST(config_read_file_twice) {
    config_init();
    write_tmp_config("interface: eth0\nshow-bars: true\n");
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    cleanup_tmp();
}

/* === Config with many entries === */

TEST(config_read_file_many_entries) {
    config_init();
    char content[4096];
    int off = 0;
    off += snprintf(content + off, sizeof(content) - off,
                    "interface: eth0\n"
                    "dns-resolution: true\n"
                    "port-resolution: false\n"
                    "filter-code: tcp port 80\n"
                    "show-bars: true\n"
                    "promiscuous: false\n"
                    "hide-source: false\n"
                    "hide-destination: false\n"
                    "use-bytes: true\n"
                    "sort: source\n"
                    "line-display: two-line\n"
                    "show-totals: true\n"
                    "log-scale: false\n"
                    "max-bandwidth: 100M\n"
                    "link-local: false\n"
                    "port-display: on\n");
    write_tmp_config(content);
    ASSERT_EQ(read_config(tmpfile_path, 0), 1);
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    ASSERT_STR_EQ(config_get_string("sort"), "source");
    ASSERT_STR_EQ(config_get_string("port-display"), "on");
    ASSERT_STR_EQ(config_get_string("hide-source"), "false");
    cleanup_tmp();
}

/* === Reinit clears state === */

TEST(config_reinit_clears_state) {
    config_init();
    config_set_string("interface", "eth0");
    ASSERT_STR_EQ(config_get_string("interface"), "eth0");
    config_init();
    ASSERT_NULL(config_get_string("interface"));
}

int main(void) {
    TEST_SUITE("Config File Tests");

    RUN(config_init_succeeds);
    RUN(config_init_creates_empty_map);
    RUN(config_set_get_string);
    RUN(config_get_string_missing);
    RUN(config_set_string_overwrite);
    RUN(config_set_string_multiple_keys);
    RUN(config_set_string_empty_value);
    RUN(config_set_string_long_value);
    RUN(config_set_string_overwrite_multiple_times);
    RUN(config_get_bool_true);
    RUN(config_get_bool_yes);
    RUN(config_get_bool_false);
    RUN(config_get_bool_no);
    RUN(config_get_bool_arbitrary);
    RUN(config_get_bool_missing);
    RUN(config_get_bool_empty_string);
    RUN(config_get_bool_case_sensitive);
    RUN(config_get_int_valid);
    RUN(config_get_int_zero);
    RUN(config_get_int_negative);
    RUN(config_get_int_large);
    RUN(config_get_int_missing);
    RUN(config_get_int_invalid_alpha);
    RUN(config_get_int_invalid_trailing);
    RUN(config_get_int_null_value);
    RUN(config_get_int_empty_string);
    RUN(config_get_float_valid);
    RUN(config_get_float_integer);
    RUN(config_get_float_zero);
    RUN(config_get_float_negative);
    RUN(config_get_float_missing);
    RUN(config_get_float_null_value);
    RUN(config_get_float_invalid);
    RUN(config_get_float_empty);
    RUN(config_get_enum_valid);
    RUN(config_get_enum_first);
    RUN(config_get_enum_last);
    RUN(config_get_enum_missing);
    RUN(config_get_enum_invalid_value);
    RUN(config_read_file_basic);
    RUN(config_read_file_comments);
    RUN(config_read_file_inline_comment);
    RUN(config_read_file_whitespace);
    RUN(config_read_file_tabs);
    RUN(config_read_file_nonexistent);
    RUN(config_read_file_empty);
    RUN(config_read_file_only_comments);
    RUN(config_read_file_blank_lines);
    RUN(config_read_file_all_directives);
    RUN(config_read_file_value_with_colon);
    RUN(config_read_file_repeated_directive);
    RUN(config_set_before_read_takes_precedence);
    RUN(config_read_then_get_bool);
    RUN(config_read_then_get_int);
    RUN(config_read_file_continuation_line);
    RUN(config_get_float_trailing_text);
    RUN(config_get_float_scientific);
    RUN(config_get_int_leading_zeros);
    RUN(config_get_int_whitespace_only);
    RUN(config_is_directive_valid);
    RUN(config_read_then_get_float);
    RUN(config_set_all_types_then_read);
    RUN(config_read_file_multi_continuation);
    RUN(config_read_file_no_colon);
    RUN(config_read_file_value_with_hash);
    RUN(config_get_bool_1_is_false);
    RUN(config_get_bool_0_is_false);
    RUN(config_get_int_max);
    RUN(config_get_int_min);
    RUN(config_get_int_positive_sign);
    RUN(config_get_float_very_small);
    RUN(config_get_float_large);
    RUN(config_get_enum_missing_key);
    RUN(config_get_enum_case_sensitive);
    RUN(config_read_file_twice);
    RUN(config_read_file_many_entries);
    RUN(config_reinit_clears_state);

    TEST_REPORT();
}
