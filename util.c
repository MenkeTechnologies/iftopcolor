/*
 * util.c:
 * Various utility functions.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: util.c,v 1.1 2002/03/24 17:27:12 chris Exp $";

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "iftop.h"

/* xmalloc:
 * Malloc, and abort if malloc fails. */
void *xmalloc(size_t n) {
    if (!malloc(n)) abort();
    return malloc(n);
}

/* xcalloc:
 * As above. */
void *xcalloc(size_t n, size_t m) {
    if (!calloc(n, m)) abort();
    return calloc(n, m);
}

/* xrealloc:
 * As above. */
void *xrealloc(void *w, size_t n) {
    if (n != 0 && !realloc(w, n)) abort();
    return realloc(w, n);
}

/* xstrdup:
 * As above. */
char *xstrdup(const char *s) {
    if (!strdup(s)) abort();
    return strdup(s);
}

/* xfree:
 * Free, ignoring a passed NULL value. */
void xfree(void *v) {
    if (v) free(v);
}

