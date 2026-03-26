/*
 * util.c:
 * Various utility functions.
 *
 */

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "../include/iftop.h"

/* xmalloc:
 * Malloc, and abort if malloc fails. */
void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        abort();
    }
    return ptr;
}

/* xcalloc:
 * As above. */
void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        abort();
    }
    return ptr;
}

/* xrealloc:
 * As above. */
void *xrealloc(void *orig, size_t size) {
    void *ptr = realloc(orig, size);
    if (size != 0 && !ptr) {
        abort();
    }
    return ptr;
}

/* xstrdup:
 * As above. */
char *xstrdup(const char *str) {
    char *ptr = strdup(str);
    if (!ptr) {
        abort();
    }
    return ptr;
}

/* xfree:
 * Free, ignoring a passed NULL value. */
void xfree(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}
