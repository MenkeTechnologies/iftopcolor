/*
 * bench_cfgfile.c: benchmarks for config file parsing
 *
 * Covers: config_set_string, config_get_string, config_get_bool,
 * config_get_int, config_get_float, config_get_enum, and read_config.
 */

#include <stdio.h>
#include <unistd.h>
#include "bench.h"
#include "cfgfile.h"
#include "stringmap.h"
#include "iftop.h"

static char tmpfile_path[256];

static void write_tmp_config(const char *content) {
    snprintf(tmpfile_path, sizeof(tmpfile_path), "/tmp/iftop_bench_cfg_XXXXXX");
    int fd = mkstemp(tmpfile_path);
    write(fd, content, strlen(content));
    close(fd);
}

static void cleanup_tmp(void) {
    unlink(tmpfile_path);
}

static config_enumeration_type sort_enum[] = {
    {"2s", 0}, {"10s", 1}, {"40s", 2}, {"source", 3}, {"destination", 4}, {NULL, -1}
};

static void bench_set_get_string(void) {
    BENCH_SECTION("Config - set/get string");

    BENCH_RUN("config_set_string (20 keys)", 100000, {
        config_init();
        for (int j = 0; j < 20; j++) {
            char key[32];
            char val[32];
            snprintf(key, sizeof(key), "key_%d", j);
            snprintf(val, sizeof(val), "value_%d", j);
            config_set_string(key, val);
        }
    });

    config_init();
    for (int j = 0; j < 20; j++) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "key_%d", j);
        snprintf(val, sizeof(val), "value_%d", j);
        config_set_string(key, val);
    }

    BENCH_RUN("config_get_string (20 keys, 1000 lookups)", 100000, {
        for (int j = 0; j < 1000; j++) {
            char key[32];
            snprintf(key, sizeof(key), "key_%d", j % 20);
            bench_use(config_get_string(key) != NULL);
        }
    });
}

static void bench_get_bool(void) {
    BENCH_SECTION("Config - get bool");

    config_init();
    config_set_string("flag_true", "true");
    config_set_string("flag_false", "false");
    config_set_string("flag_yes", "yes");
    config_set_string("flag_no", "no");

    BENCH_RUN("config_get_bool (4 keys)", 1000000, {
        bench_use(config_get_bool("flag_true"));
        bench_use(config_get_bool("flag_false"));
        bench_use(config_get_bool("flag_yes"));
        bench_use(config_get_bool("flag_no"));
    });
}

static void bench_get_int(void) {
    BENCH_SECTION("Config - get int");

    config_init();
    config_set_string("int_val", "42");
    config_set_string("int_neg", "-100");
    config_set_string("int_zero", "0");

    BENCH_RUN("config_get_int (3 keys)", 1000000, {
        int val;
        bench_use(config_get_int("int_val", &val));
        bench_use(config_get_int("int_neg", &val));
        bench_use(config_get_int("int_zero", &val));
    });
}

static void bench_get_float(void) {
    BENCH_SECTION("Config - get float");

    config_init();
    config_set_string("float_val", "3.14");
    config_set_string("float_neg", "-1.5");

    BENCH_RUN("config_get_float (2 keys)", 1000000, {
        float val;
        bench_use(config_get_float("float_val", &val));
        bench_use(config_get_float("float_neg", &val));
    });
}

static void bench_get_enum(void) {
    BENCH_SECTION("Config - get enum");

    config_init();
    config_set_string("sort", "source");

    BENCH_RUN("config_get_enum", 1000000, {
        int val;
        bench_use(config_get_enum("sort", sort_enum, &val));
    });
}

static void bench_read_file(void) {
    BENCH_SECTION("Config - read_config from file");

    write_tmp_config(
        "interface: eth0\n"
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
        "port-display: on\n"
    );

    BENCH_RUN("read_config (14 directives)", 10000, {
        config_init();
        read_config(tmpfile_path, 0);
    });

    /* Large config with comments and whitespace */
    char big_config[4096];
    int offset = 0;
    for (int i = 0; i < 50; i++) {
        offset += snprintf(big_config + offset, sizeof(big_config) - offset,
                           "# comment line %d\n"
                           "key_%d: value_%d\n",
                           i, i, i);
    }
    cleanup_tmp();
    write_tmp_config(big_config);

    BENCH_RUN("read_config (50 directives + comments)", 10000, {
        config_init();
        read_config(tmpfile_path, 0);
    });

    cleanup_tmp();
}

static void bench_config_workload(void) {
    BENCH_SECTION("Config - Realistic Workload (init + set + read + query)");

    write_tmp_config(
        "interface: eth0\n"
        "dns-resolution: true\n"
        "promiscuous: false\n"
        "show-bars: true\n"
        "sort: source\n"
        "max-bandwidth: 100M\n"
    );

    BENCH_RUN("full config lifecycle", 10000, {
        config_init();
        /* Command-line overrides */
        config_set_string("interface", "wlan0");
        config_set_string("promiscuous", "true");
        /* Read file */
        read_config(tmpfile_path, 0);
        /* Query all values */
        bench_use(config_get_string("interface") != NULL);
        bench_use(config_get_bool("dns-resolution"));
        bench_use(config_get_bool("promiscuous"));
        bench_use(config_get_bool("show-bars"));
        int val;
        bench_use(config_get_enum("sort", sort_enum, &val));
    });

    cleanup_tmp();
}

int main(void) {
    BENCH_HEADER("CONFIG FILE PARSER PROFILER");

    bench_set_get_string();
    bench_get_bool();
    bench_get_int();
    bench_get_float();
    bench_get_enum();
    bench_read_file();
    bench_config_workload();

    BENCH_FOOTER();
    return 0;
}
