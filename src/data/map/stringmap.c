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
    stringmap S;

    S = xcalloc(sizeof *S, 1);

    return S;
}

/* stringmap_delete:
 * Free memory for a stringmap. */
void stringmap_delete(stringmap S) {
    if (!S) return;
    if (S->lesser) stringmap_delete(S->lesser);
    if (S->greater) stringmap_delete(S->greater);

    xfree(S->key);
    xfree(S);
}

/* stringmap_delete_free:
 * Free memory for a stringmap, and the objects contained in it, assuming that
 * they are pointers to memory allocated by xmalloc(3). */
void stringmap_delete_free(stringmap S) {
    if (!S) return;
    if (S->lesser) stringmap_delete_free(S->lesser);
    if (S->greater) stringmap_delete_free(S->greater);

    xfree(S->key);
    xfree(S->data.ptr);
    xfree(S);
}

/* stringmap_insert:
 * Insert into S an item having key k and value d. Returns a pointer to
 * the existing item value, or NULL if a new item was created.
 */
item *stringmap_insert(stringmap S, const char *k, const item d) {
    if (!S) return NULL;
    if (S->key == NULL) {
        S->key = xstrdup(k);
        S->data = d;
        return NULL;
    } else {
        stringmap S2;
        for (S2 = S;;) {
            int i = strcmp(k, S2->key);
            if (i == 0) {
                return &(S2->data);
            } else if (i < 0) {
                if (S2->lesser) S2 = S2->lesser;
                else {
                    if (!(S2->lesser = stringmap_new())) return NULL;
                    S2->lesser->key = xstrdup(k);
                    S2->lesser->data = d;
                    return NULL;
                }
            } else if (i > 0) {
                if (S2->greater) S2 = S2->greater;
                else {
                    if (!(S2->greater = stringmap_new())) return NULL;
                    S2->greater->key = xstrdup(k);
                    S2->greater->data = d;
                    return NULL;
                }
            }
        }
    }
}

/* stringmap_find:
 * Find in d an item having key k in the stringmap S, returning the item found
 * on success NULL if no key was found. */
stringmap stringmap_find(const stringmap S, const char *k) {
    stringmap S2;
    int i;
    if (!S || S->key == NULL) return 0;
    for (S2 = S;;) {
        i = strcmp(k, S2->key);
        if (i == 0) return S2;
        else if (i < 0) {
            if (S2->lesser) S2 = S2->lesser;
            else return NULL;
        } else if (i > 0) {
            if (S2->greater) S2 = S2->greater;
            else return NULL;
        }
    }
}
