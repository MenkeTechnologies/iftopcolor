/*
 * util.c:
 * Various utility functions.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "../include/iftop.h"

/* xmalloc:
 * Malloc, and abort if malloc fails. */
void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) abort();
    return p;
}

/* xcalloc:
 * As above. */
void *xcalloc(size_t n, size_t m) {
    void *p = calloc(n, m);
    if (!p) abort();
    return p;
}

/* xrealloc:
 * As above. */
void *xrealloc(void *w, size_t n) {
    void *p = realloc(w, n);
    if (n != 0 && !p) abort();
    return p;
}

/* xstrdup:
 * As above. */
char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) abort();
    return p;
}

/* xfree:
 * Free, ignoring a passed NULL value. */
void xfree(void *v) {
    if (v) free(v);
}

