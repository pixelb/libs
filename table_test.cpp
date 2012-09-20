#include <string.h>
#include <stdlib.h>
#include "table.h"
#include "PadThreads.h"
#include "pad.h"

/* This simple app shows all the operations
   on a table, and then creates some threads
   to both demonstrate the Thread class and
   also how access to the table is automatically
   handled in the class */

/* TableEntry is an abstract base class.
   You must define your concrete "record"
   type by deriving from it */
class myRecord: public TableEntry
{
    public:
        int field1;
        int field2;

        bool populate(void *line) {
            typedef enum {
                mr_Unknown,
                mr_Name,
                mr_Field1,
                mr_Field2
            } myRecordFields_t;
            myRecordFields_t curr_field = mr_Unknown;
            while (char *field = strsep((char**)&line, " ")) {

                curr_field = (myRecordFields_t) (1 + (int) curr_field);

                switch (curr_field) {
                case mr_Name:
                    if (!name)
                        name = strdup(field);
                    break;
                case mr_Field1: field1 = atoi(field); break;
                case mr_Field2: field2 = atoi(field); break;
                default: break;
                }
            }
            return true;
        };
        void print(void) { printf("record[%s] = %d, %d\n", name, field1, field2); };
};

Table myTable;

/* define thread type that must contain the main() function */
class myThread: public Thread
{
public:
    int id;
private:
    void main(void) {
        for(;;) {
            //Traverse table
            void *cursor;
            myRecord *mr = (myRecord *) myTable.getFirst(&cursor);
            while (mr) {
                printf("thread=%02d ", id); mr->print();
                mr->release();
                unsigned int rand_state;
                if ((rand_r(&rand_state) % 5)==0) { /* mix it up a bit */
                   usleep(500000);
                }
                mr = (myRecord *) myTable.getNext(&cursor);
            }
        }
    }
};

int main(void) {
    myRecord *mr;

    //Add items to table
    for (int i=0; i<10; i++) {
        char buf[200];
        sprintf(buf, "Name%d %d %d", i, i, i*i);
        TableEntry *te = new myRecord;
        if (te->populate(buf)) {
            if (!myTable.add(te)) {
                delete te;
            }
        } else {
            delete te;
        }
    }

    //Del item from table
    myTable.del("Name3");

    //Get item from table
    mr = (myRecord *) myTable.get("Name4");
    if (mr) {
        mr->print();
        mr->release();
    }

    //Traverse table
    void *cursor;
    mr = (myRecord *) myTable.getFirst(&cursor);
    bool dont_need_to_continue=false;
    while (mr) {
        mr->print();
        if (!strcmp(mr->name, "abort condition")) {
            dont_need_to_continue=true;
        }
        mr->release();
        if (dont_need_to_continue) {
            myTable.abortWalk();
        }
        mr = (myRecord *) myTable.getNext(&cursor);
    }

    //create threads that traverse tables
    myThread myThreads[100];
    for (unsigned i=0; i<lengthof(myThreads); i++) {
        myThreads[i].id=i;
        myThreads[i].StartThread();
    }
    while(1) sleep(1);
}
