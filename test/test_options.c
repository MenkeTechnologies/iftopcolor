/*
 * test_options.c: tests for options_make interface fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "cfgfile.h"
#include "options.h"
#include "iftop.h"

extern options_t options;

/* === options_make: interface fallback when not configured === */

TEST(options_make_no_interface_configured) {
    config_init();
    /* No "interface" set in config — simulates running without -i */
    options_set_defaults();
    options_make();
    ASSERT_NOT_NULL(options.interface);
}

TEST(options_make_interface_from_config) {
    config_init();
    config_set_string("interface", "eth99");
    options_set_defaults();
    options_make();
    ASSERT_NOT_NULL(options.interface);
    ASSERT_STR_EQ(options.interface, "eth99");
}

TEST(options_make_interface_not_null_after_repeated_calls) {
    config_init();
    options_set_defaults();
    options_make();
    ASSERT_NOT_NULL(options.interface);
    /* Call again with empty config — should still not be NULL */
    config_init();
    options_make();
    ASSERT_NOT_NULL(options.interface);
}

TEST(options_make_interface_overwritten_by_config) {
    config_init();
    options_set_defaults();
    /* First call picks up default */
    options_make();
    ASSERT_NOT_NULL(options.interface);
    /* Now set a specific interface in config and remake */
    config_init();
    config_set_string("interface", "wlan0");
    options_make();
    ASSERT_NOT_NULL(options.interface);
    ASSERT_STR_EQ(options.interface, "wlan0");
}

TEST(options_set_defaults_interface_not_null) {
    options_set_defaults();
    ASSERT_NOT_NULL(options.interface);
}

int main(void) {
    TEST_SUITE("Options Tests");

    RUN(options_make_no_interface_configured);
    RUN(options_make_interface_from_config);
    RUN(options_make_interface_not_null_after_repeated_calls);
    RUN(options_make_interface_overwritten_by_config);
    RUN(options_set_defaults_interface_not_null);

    TEST_REPORT();
}
