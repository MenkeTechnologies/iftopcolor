/*
 * cfgfile.c:
 *
 * Copyright (c) 2003 Jacob Menke
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../include/stringmap.h"
#include "../include/iftop.h"
#include "../include/options.h"
#include "../include/cfgfile.h"

#define CONFIG_TYPE_STRING 0
#define CONFIG_TYPE_BOOL   1
#define CONFIG_TYPE_INT    2

#define MAX_CONFIG_LINE     2048

char *config_directives[] = {
        "interface",
        "dns-resolution",
        "port-resolution",
        "filter-code",
        "show-bars",
        "promiscuous",
        "hide-source",
        "hide-destination",
        "use-bytes",
        "sort",
        "line-display",
        "show-totals",
        "log-scale",
        "max-bandwidth",
        "net-filter",
        "net-filter6",
        "link-local",
        "port-display",
        NULL
};

stringmap config;

extern options_t options;

int is_cfgdirective_valid(const char *directive) {
    int idx;
    for (idx = 0; config_directives[idx] != NULL; idx++)
        if (strcmp(directive, config_directives[idx]) == 0) return 1;
    return 0;
}

int config_init() {
    config = stringmap_new();
    return config != NULL;
}

/* read_config_file:
 * Read a configuration file consisting of key: value tuples, returning a
 * stringmap of the results. Prints errors to stderr, rather than using
 * syslog, since this file is called at program startup. Returns 1 on success
 * or 0 on failure. */
int read_config_file(const char *filepath, int whinge) {
    int ret = 0;
    FILE *fp;
    char *line;
    int line_num = 1;

    line = xmalloc(MAX_CONFIG_LINE);

    fp = fopen(filepath, "rt");
    if (!fp) {
        if (whinge) fprintf(stderr, "%s: %s\n", filepath, strerror(errno));
        goto fail;
    }

    while (fgets(line, MAX_CONFIG_LINE, fp)) {
        char *key, *value, *end;

        for (end = line + strlen(line) - 1; end > line && *end == '\n'; *(end--) = 0);

        /* Get continuation lines. Ugly. */
        while (*(line + strlen(line) - 1) == '\\') {
            if (!fgets(line + strlen(line) - 1, MAX_CONFIG_LINE - strlen(line), fp))
                break;
            for (end = line + strlen(line) - 1; end > line && *end == '\n'; *(end--) = 0);
        }

        /* Strip comment. */
        key = strpbrk(line, "#\n");
        if (key) *key = 0;

        /*    foo  : bar baz quux
         * key^    ^value          */
        key = line + strspn(line, " \t");
        value = strchr(line, ':');

        if (value) {
            /*    foo  : bar baz quux
             * key^  ^end ^value         */
            ++value;

            end = key + strcspn(key, " \t:");
            if (end != key) {
                item *existing;
                *end = 0;

                /*    foo\0: bar baz quux
                 * key^      ^value      ^end */
                value += strspn(value, " \t");
                end = value + strlen(value) - 1;
                while (strchr(" \t", *end) && end > value) --end;
                *(end + 1) = 0;

                /* (Removed check for zero length value.) */

                /* Check that this is a valid key. */
                if (!is_cfgdirective_valid(key))
                    fprintf(stderr, "%s:%d: warning: unknown directive \"%s\"\n", filepath, line_num, key);
                else {
                    char *dup = xstrdup(value);
                    if ((existing = stringmap_insert(config, key, item_ptr(dup)))) {
                        /* Don't warn of repeated directives, because they
                         * may have been specified via the command line
                         * Previous option takes precedence.
                         */
                        xfree(dup);
                        fprintf(stderr, "%s:%d: warning: repeated directive \"%s\"\n", filepath, line_num, key);
                    }
                }
            }
        }

        memset(line, 0, MAX_CONFIG_LINE); /* security paranoia */

        ++line_num;
    }

    ret = 1;

    fail:
    if (fp) fclose(fp);
    if (line) xfree(line);

    return ret;
}

int config_get_int(const char *directive, int *value) {
    stringmap entry;
    char *str, *endptr;

    if (!value) return -1;

    entry = stringmap_find(config, directive);
    if (!entry) return 0;

    str = (char *) entry->data.ptr;
    if (!*str) return -1;
    errno = 0;
    *value = strtol(str, &endptr, 10);
    if (*endptr) return -1;

    return errno == ERANGE ? -1 : 1;
}

/* config_get_float:
 * Get an integer value from a config string. Returns 1 on success, -1 on
 * failure, or 0 if no value was found. */
int config_get_float(const char *directive, float *value) {
    stringmap entry;
    char *str, *endptr;

    if (!value) return -1;

    if (!(entry = stringmap_find(config, directive)))
        return 0;

    str = (char *) entry->data.ptr;
    if (!*str) return -1;
    errno = 0;
    *value = strtod(str, &endptr);
    if (*endptr) return -1;

    return errno == ERANGE ? -1 : 1;
}

/* config_get_string;
 * Get a string value from the config file. Returns NULL if it is not
 * present. */
char *config_get_string(const char *directive) {
    stringmap entry;

    entry = stringmap_find(config, directive);
    if (entry) return (char *) entry->data.ptr;
    else return NULL;
}

/* config_get_bool:
 * Get a boolean value from the config file. Returns false if not present. */
int config_get_bool(const char *directive) {
    char *str;

    str = config_get_string(directive);
    if (str && (strcmp(str, "yes") == 0 || strcmp(str, "true") == 0))
        return 1;
    else
        return 0;
}

/* config_get_enum:
 * Get an enumeration value from the config file. Returns false if not
 * present or an invalid value is found. */
int config_get_enum(const char *directive, config_enumeration_type *enumeration, int *value) {
    char *str;
    config_enumeration_type *entry;
    str = config_get_string(directive);
    if (str) {
        for (entry = enumeration; entry->name; entry++) {
            if (strcmp(str, entry->name) == 0) {
                *value = entry->value;
                return 1;
            }
        }
        fprintf(stderr, "Invalid enumeration value \"%s\" for directive \"%s\"\n", str, directive);
    }
    return 0;
}

/* config_set_string; Sets a value in the config, possibly overriding
 * an existing value
 */
void config_set_string(const char *directive, const char *str) {
    stringmap entry;

    entry = stringmap_find(config, directive);
    if (entry) {
        xfree(entry->data.ptr);
        entry->data = item_ptr(xstrdup(str));
    } else {
        stringmap_insert(config, directive, item_ptr(xstrdup(str)));
    }
}

int read_config(char *file, int whinge_on_error) {
    return read_config_file(file, whinge_on_error);
}
