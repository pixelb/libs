/* Copyright: Pádraig Brady 2002
 * Summary: Doubly linked list
 * License: LGPL
 * History:
 *     02 Sep 2002 : Initial version
 *     10 Nov 2005 : Add llist_reverse()
 */

#ifndef LLIST_H
#define LLIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* you manage setting/storage for val */
typedef struct _llist_entry {
    void                    *val;   /* payload */
    struct _llist_entry     *prev;
    struct _llist_entry     *next;
} llist_entry;

typedef int (*llist_cmp_func)(const void *, const void *);

/* adds to start of list
   ret 0 on fail */
int llist_add(llist_entry **llist, void *val);

/* find and return from list first item found */
void * llist_find(const llist_entry *llist, const void *data, const llist_cmp_func lcf);

/* find and remove from list first item found.
   If llist_cmp_func (and data) is NULL then the
   first item in the list is removed */
void * llist_pop(llist_entry **llist, const void *data, const llist_cmp_func lcf);

/* O(n^2) */
void llist_sort(llist_entry *llist, const llist_cmp_func lcf);

/* O(n) */
void llist_reverse(llist_entry **llist);

/* Apply function to each item in list */
void llist_apply(llist_entry *llist, void (*llist_func)(void *));

#ifdef __cplusplus
}
#endif

#endif /* LLIST_H */
