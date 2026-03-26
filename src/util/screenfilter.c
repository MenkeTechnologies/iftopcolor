/*
 * screenfilter.c:
 *
 * Copyright (c) 2002 Jacob Menke
 *
 */

#include "../include/conf.h"

#ifdef HAVE_REGCOMP

#    include <sys/types.h>
#    include <regex.h>
#    include <stdio.h>
#    include "../include/iftop.h"
#    include "../include/options.h"

extern options_t options;

regex_t preg;

int screen_filter_set(char *pattern) {
    int result;

    if (options.screenfilter != NULL) {
        xfree(options.screenfilter);
        options.screenfilter = NULL;
        regfree(&preg);
    }

    result = regcomp(&preg, pattern, REG_ICASE | REG_EXTENDED);

    if (result == 0) {
        options.screenfilter = pattern;
        return 1;
    } else {
        xfree(pattern);
        return 0;
    }
}

int screen_filter_match(char *text) {
    int result;
    if (options.screenfilter == NULL) {
        return 1;
    }

    result = regexec(&preg, text, 0, NULL, 0);
    if (result == 0) {
        return 1;
    } else {
        return 0;
    }
}

#endif /* HAVE_REGCOMP */
