#ifndef PAD_THREADS_H
#define PAD_THREADS_H

// If GETOUT_CLAUSE is defined, a lock can only be refused for that number of seconds
//#define GETOUT_CLAUSE 5

#ifdef GETOUT_CLAUSE
    static volatile int mutexCount = 0;
    #include "tracer.h"
#endif //GETOUT_CLAUSE

// Number of MICROseconds between attempts to access a lock
#define LOCK_CHECK_DELAY    (5000)

#ifdef WIN32
    #include <cygnus\pthread.h>
#else
    #include <pthread.h>
#endif /* WIN32 */

    #include <stdio.h>
    #include <errno.h>
    #include <syslog.h>
    #include <unistd.h>

class CriticalSection
{
public:

#ifdef GETOUT_CLAUSE //useful for debugging deadlocks
    CriticalSection() {
        pthread_mutex_init(&mutex, NULL);
//      mutexNum = mutexCount++;
    }

    ~CriticalSection() {
        pthread_mutex_destroy(&mutex);
    }

    void enter(void){
        int result;
        int retryCount = 0;

        while((result=pthread_mutex_trylock(&mutex)) != 0) {
            //DebugMsg(0, "Lock %3d failed for pid %d - retrying\n", mutexNum, (int) getpid());
            usleep(LOCK_CHECK_DELAY);
            if ((GETOUT_CLAUSE * 1000000) <= (LOCK_CHECK_DELAY * ++retryCount)) {
                DebugMsg(5, "Process %d failed to acquire lock %d - using GETOUT_CLAUSE\n", getpid(), mutexNum);
                break;
            }
        }
        //DebugMsg(0, "Lock %3d acquired by %d\n", mutexNum, (int) getpid());
    }

    void leave(void) {
        pthread_mutex_unlock(&mutex);
        //DebugMsg(0, "Lock %3d released by %d\n", mutexNum, (int) getpid());
    }
#else
	CriticalSection() {pthread_mutex_init   (&mutex, NULL);}/*TODO: set attr for 1 process*/
	~CriticalSection(){pthread_mutex_destroy(&mutex);      }
	void enter(void)  {pthread_mutex_lock   (&mutex);      }/*TODO: return bool (false if EINVAL(mutex destroyed)), retry on EINTR?*/
	void leave(void)  {pthread_mutex_unlock (&mutex);      }
    pthread_mutex_t* pthread_mutex(void) { return &mutex; }
#endif

private:
    pthread_mutex_t mutex;
//  int mutexNum;
};

class rwlock
{
    pthread_rwlock_t lock;

    public:
    rwlock()            {pthread_rwlock_init(&lock, NULL);}
    ~rwlock()           {pthread_rwlock_destroy(&lock);}
    void readlock(void) {pthread_rwlock_rdlock(&lock);}
    void writelock(void){pthread_rwlock_wrlock(&lock);}
    void unlock(void)   {pthread_rwlock_unlock(&lock);}
};

class Thread
{
public:
	Thread();
	virtual ~Thread();

	bool StartThread(void);
	bool StopThread(void);
	bool PauseThread(void);
	bool UnPauseThread(void);
	void ExitIfNeeded(void);

	bool fThreadRunning;

private:


	pthread_t tid;
	pthread_attr_t attr;
	long ThreadParam;

	/* This pure virtual function makes this class an ABC.
	 * I.E. this function must be implemented for each class
	 * that derives from this.
	 *    Note that since the code in main will be running
	 * in a parallel thread of execution to everything else
	 * on the system it should release processing time when
	 * it's not doing anything useful. I.E. it should use
	 * the select(), sleep(), getMessage() calls etc. to yield
	 * the processor to the OS. An e.g. implementation of
	 * main could be:
	 *
	 * for(;;) {
	 *    select(socket_descriptor); //Wait for data
	 *    process(data);
	 * }
	 */
	virtual void main(void)=0;

	//static member function means a global function,
	//IE no this pointer is passed to it on the stack.
	static void* ThreadStarter(void* lpParams);
};

#endif //PAD_THREADS_H
