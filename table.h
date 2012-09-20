#ifndef _TABLE_H
#define _TABLE_H

#include "PadThreads.h"
#include "llist.h"

struct TableEntry
{
    char *name;

    CriticalSection lock;

    TableEntry();
    TableEntry(const TableEntry & rhs);
    virtual ~TableEntry();

    virtual bool populate(void *) = 0;
    virtual void print(void) = 0;

    static int compare(const void *entry1, const void *entry2);
    static int findName(const void *entry, const void *name);

    void acquire(void);
    void release(void);
};

struct Table {
    Table();
    virtual ~Table();
    bool add(TableEntry * Entry);
    TableEntry *get(const char *Name);
    bool del(const char *Name);

    TableEntry *getFirst(void **cursor);
    TableEntry *getNext(void **cursor);
    /* Note use abort walk if exiting a monacoTable walk before the last item.
       You can also use the abort/resume combination if you want to delete the
       current item and you're sure that no other thread could be deleteing from
       the table. Note be careful when resuming that there is another entry in the
       table (i.e. if the last getNext() returned NULL, then you should not resumeWalk() */
    void abortWalk(void) { table_rwlock.unlock(); }
    void resumeWalk(void) { table_rwlock.readlock(); }

  protected:
    llist_entry* table;
    CriticalSection tableLock;
    rwlock table_rwlock;
};

#endif //_TABLE_H
