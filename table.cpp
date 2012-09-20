#include <string.h>
#include <stdlib.h>

extern "C" {
#include "llist.h"
}
#include "table.h"
#include "PadThreads.h"


/* Should really turn this into a hash,
both for speed and standard interface */

TableEntry::TableEntry()
{
    name = NULL;
}

TableEntry::TableEntry(const TableEntry & rhs)
{
    if (rhs.name)
        name = strdup(rhs.name);
}

TableEntry::~TableEntry()
{
    if (name)
        free(name);
}

int TableEntry::compare(const void *entry1, const void *entry2)	//Used to sort table
{
    return (strcmp(((TableEntry *) entry1)->name, ((TableEntry *) entry2)->name));
}

int TableEntry::findName(const void *entry, const void *name)
{
    return (strcmp(((TableEntry *) entry)->name, (const char *) name));
}

void TableEntry::acquire(void)
{
    lock.enter();
}

void TableEntry::release(void)
{
    lock.leave();
}

/*
OK the locking here uses a fixed 3 level hierarchy.
This means that if there is any contention on a TableEntry
then the whole table is NOT locked. See the locking
diagram for more info.
*/

Table::Table()
{
    table=NULL;
}

Table::~Table()
{
    tableLock.enter();
    TableEntry* Entry;
    while ((Entry = (TableEntry *) llist_pop(&table, NULL, NULL))) {
        Entry->acquire();
        delete Entry;
    }
    tableLock.leave();
}

/*
NB make sure you don't access an object after
you add it to a table. If this is required??
then ->acquire() first and then ->release()
*/
bool Table::add(TableEntry * Entry)
{
    // Nameless objects CANNOT be part of a table
    if (!Entry->name)
        return false;
    tableLock.enter();
    int result = llist_add(&table, Entry);
    tableLock.leave();
    if (result)
        return true;
    else
        return false;
}

TableEntry *Table::get(const char *Name)
{
    if (!Name)
        return NULL;

    TableEntry *Entry;
    tableLock.enter();
    Entry = (TableEntry *) llist_find(table, Name, TableEntry::findName);
    if (Entry)
        Entry->acquire();
    tableLock.leave();

    return Entry;
}

bool Table::del(const char *Name)
{
    if (!Name)
        return false;

    TableEntry *Entry;
    table_rwlock.writelock();
    tableLock.enter();
    Entry = (TableEntry *) llist_pop(&table, Name, TableEntry::findName);
    if (Entry) {
        Entry->acquire();
        delete Entry;
    }
    tableLock.leave();
    table_rwlock.unlock();

    if (Entry)
        return true;
    else
        return false;
}

TableEntry *Table::getFirst(void **cursor)
{
    TableEntry *Entry;
    table_rwlock.readlock();
    tableLock.enter();
    *cursor = table;
    if (table) {
        Entry = (TableEntry *) table->val;
    } else {
        Entry = NULL;
    }
    if (Entry) {
        Entry->acquire();
    }
    tableLock.leave();
    if (!Entry)
        table_rwlock.unlock(); //nothing in table => allow writers to table to modify
    return Entry;
}

TableEntry *Table::getNext(void **cursor)
{
//Note could do (datanode*)(*cursor)->data.lock.leave();
//here but no as need explicit release in certain cases anyway.
    TableEntry *Entry;
    tableLock.enter();
    *cursor = ((llist_entry *) *cursor)->next;
    if (*cursor) {
        Entry = (TableEntry *) ((llist_entry *) *cursor)->val;
    } else {
        Entry = NULL;
    }
    if (Entry) {
        Entry->acquire();
    }
    tableLock.leave();
    if (!Entry)
        table_rwlock.unlock(); //@ end of table => allow writers to table to modify
    return Entry;
}
