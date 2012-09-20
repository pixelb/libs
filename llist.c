/* Copyright: Pádraig Brady 2002
 * Summary: Doubly linked list
 * License: LGPL
 * History:
 *     02 Sep 2002 : Initial version
 *     10 Nov 2005 : Add llist_reverse()
 */

#include <stdlib.h>
#include "llist.h"

int llist_add(llist_entry **llist, void *val)
{
    llist_entry *e = (llist_entry *) malloc(sizeof(llist_entry));

    if (e == NULL) {
        return 0;
    }

    e->val = val;

    if ((*llist) != NULL) {
        e->prev = NULL;
        e->next = (*llist);
        (*llist)->prev = e;
        (*llist) = e;
    }
    else {
        e->prev = NULL;
        e->next = NULL;
        (*llist) = e;
    }

    return 1;
}

static void * llist_remove(llist_entry **llist, llist_entry *e)
{
    llist_entry *ei;
    void * ret = NULL;

    for (ei = (*llist); ei != NULL; ei = ei->next) {
        if (ei == e) {
            if ((e == (*llist)) && (e->next == NULL)) {
                (*llist) = NULL;
            }
            else if ((e == (*llist)) && (e->next != NULL)) {
                e->next->prev = NULL;
                (*llist) = e->next;
            }
            else if (e->next == NULL) {
                e->prev->next = NULL;
            }
            else {
                e->prev->next = e->next;
                e->next->prev = e->prev;
            }

            ret = e->val;
            free(e);
            break;
        }
    }
    return ret;
}

void * llist_pop(llist_entry **llist, const void *data, const llist_cmp_func lcf)
{
    if (lcf == NULL) {
        return llist_remove(llist, *llist);
    } else {
        llist_entry* ei;

        for (ei = *llist; ei != NULL; ei = ei->next) {
            if (lcf(ei->val, data) == 0) {
                return llist_remove(llist, ei);
            }
        }
        return NULL;
    }
}

void * llist_find(const llist_entry *llist, const void *data, const llist_cmp_func lcf)
{
    const llist_entry* ei;

    for (ei = llist; ei != NULL; ei = ei->next) {
        if (lcf(ei->val, data) == 0) {
            return ei->val;
        }
    }
    return NULL;
}

void llist_apply(llist_entry *llist, void (*llist_func)(void *))
{
    llist_entry *lle = llist;

    while (lle != NULL) {
        llist_func(lle->val);
        lle = lle->next;
    }
}

/*
 * Swap next & prev in each element.
 * O(n)
 */
void llist_reverse(llist_entry **llist)
{
    llist_entry *lle = *llist;
    llist_entry *lle_swap;

    while (lle != NULL) {
        lle_swap = lle->prev;
        lle->prev = lle->next;
        lle->next = lle_swap;
        lle_swap = lle;
        lle = lle->prev;
    }
    (*llist) = lle_swap;
}

/*
 * simple selection sort
 * O(n^2)
 */
void llist_sort(llist_entry *llist, const llist_cmp_func lcf)
{
    llist_entry    *lle1, *lle2;
    void           *tmp_val;

    for (lle1 = llist; lle1 != NULL; lle1 = lle1->next) {
        for (lle2 = lle1->next; lle2 != NULL; lle2 = lle2->next) {
            if (lcf(lle1->val, lle2->val) > 0) {
                tmp_val = lle1->val;
                lle1->val = lle2->val;
                lle2->val = tmp_val;
            }
        }
    }
}
