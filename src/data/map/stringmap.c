/*
 * stringmap.c: sucky implementation of binary trees
 *
 * This makes no attempt to balance the tree, so has bad worst-case behaviour.
 * Also, I haven't implemented removal of items from the tree. So sue me.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "../../include/stringmap.h"
#include "../../include/iftop.h"

/* stringmap_new:
 * Allocate memory for a new stringmap. */
stringmap stringmap_new() {
    stringmap map;

    map = xcalloc(sizeof *map, 1);

    return map;
}

/* stringmap_delete:
 * Free memory for a stringmap. */
void stringmap_delete(stringmap map) {
    if (!map) {
        return;
    }
    if (map->lesser) {
        stringmap_delete(map->lesser);
    }
    if (map->greater) {
        stringmap_delete(map->greater);
    }

    xfree(map->key);
    xfree(map);
}

/* stringmap_delete_free:
 * Free memory for a stringmap, and the objects contained in it, assuming that
 * they are pointers to memory allocated by xmalloc(3). */
void stringmap_delete_free(stringmap map) {
    if (!map) {
        return;
    }
    if (map->lesser) {
        stringmap_delete_free(map->lesser);
    }
    if (map->greater) {
        stringmap_delete_free(map->greater);
    }

    xfree(map->key);
    xfree(map->data.ptr);
    xfree(map);
}

/* stringmap_insert:
 * Insert into map an item having key k and value d. Returns a pointer to
 * the existing item value, or NULL if a new item was created.
 */
item *stringmap_insert(stringmap map, const char *key, const item data) {
    if (!map) {
        return NULL;
    }
    if (map->key == NULL) {
        map->key = xstrdup(key);
        map->data = data;
        return NULL;
    } else {
        stringmap current;
        for (current = map;;) {
            int cmp = strcmp(key, current->key);
            if (cmp == 0) {
                return &(current->data);
            } else if (cmp < 0) {
                if (current->lesser) {
                    current = current->lesser;
                } else {
                    if (!(current->lesser = stringmap_new())) {
                        return NULL;
                    }
                    current->lesser->key = xstrdup(key);
                    current->lesser->data = data;
                    return NULL;
                }
            } else if (cmp > 0) {
                if (current->greater) {
                    current = current->greater;
                } else {
                    if (!(current->greater = stringmap_new())) {
                        return NULL;
                    }
                    current->greater->key = xstrdup(key);
                    current->greater->data = data;
                    return NULL;
                }
            }
        }
    }
}

/* stringmap_find:
 * Find in data an item having key in the stringmap, returning the item found
 * on success NULL if no key was found. */
stringmap stringmap_find(const stringmap map, const char *key) {
    stringmap current;
    int cmp;
    if (!map || map->key == NULL) {
        return 0;
    }
    for (current = map;;) {
        cmp = strcmp(key, current->key);
        if (cmp == 0) {
            return current;
        } else if (cmp < 0) {
            if (current->lesser) {
                current = current->lesser;
            } else {
                return NULL;
            }
        } else if (cmp > 0) {
            if (current->greater) {
                current = current->greater;
            } else {
                return NULL;
            }
        }
    }
}
