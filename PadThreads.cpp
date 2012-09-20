#include <assert.h>

#include "PadThreads.h"

Thread::Thread()
{

	fThreadRunning = false;
	tid = 0;

	ThreadParam = (long) this; //Pass "this" pointer (object context) to thread starter
	pthread_attr_init(&attr); //TODO: check error returns!
}

Thread::~Thread()
{
	/* Need to kill thread here as bad assumption to
	 * assume that thread objects will only go away
	 * when the process does.
	 *   Also note that destructors for derived classes
	 * shouldn't call anything that might cause the
	 * parallel running thread to fail. If there's
	 * a possibility of this then the destructor should
	 * wait for thread to exit before doing "stuff".
	 */
	if (tid) pthread_cancel(tid);
}

/* Need to check this at various places. posix states all system & libc
   calls should be cancelation points also, but they're not in the
   currently implementation of linuxthreads */
void Thread::ExitIfNeeded(void)
{
	pthread_testcancel();
}

bool Thread::StopThread(void)
{
	if (tid)
	{
		pthread_cancel(tid);
		fThreadRunning = false;
		return true;
	}
	return false;
}

bool Thread::StartThread(void)
{
	assert(fThreadRunning!=true); //logical error to call StartThread when thread already running

	if (fThreadRunning==true) return true; //return (already started) @ runtime.

	/*
	 * Note that you can't call the ("create thread") function
	 * from the constructor of the ABC (Thread), since
	 * the new thread can run before the object is finished
	 * constructing in the origonal thread. I.E. the virtual function
	 * main could be executed before the derived class is
	 * finished constructing in the origonal thread. This will be
	 * caught @ runtime by most compilers with the error "Pure
	 * virtual function call". Note the reason the compiler runs
	 * the pure virtual function (main) and not the "concrete"
	 * one, is because the class in which the concrete one is implemented
	 * hasn't finished constructing yet and so running a member function
	 * on it would result in undefined behaviour (It would drive you mental
	 * trying to find bugs like this).
	 * Note the derived (concrete) class can't call the ("create thread") function
	 * from it's constructor either for the same reason.
	 *
	 * windoze CreateThread is better here since you can start the thread suspended,
	 * but it doesn't really give you anything as you need to explicitly start the
	 * thread anyway.
	 */
	if (pthread_create(&tid, &attr, ThreadStarter, &ThreadParam))
	{
		//TODO: pass errorcode up (log or exception or whatever)
		return false;
	}
	else
	{
		pthread_detach(tid); //Detach so that when this thread exits, resources are reclaimed
		fThreadRunning = true; //So don't start it more than once
		return true; //success
	}
}

/*
 * Note even though this is a member function,
 * it's a static member and hence no "this" pointer is implicitly passed to it.
 * Therefore I have to explicitly pass the "this" pointer so I can
 * call the main (virtual) member function for the appropriate object.
 *
 * Note the reason this is required is that when a thread is created a
 * specified function is called. There obviously must be an agreement
 * between this function and the OS on the format of the stack frame,
 * and a normal class member function doesn't fit the bill.
 *
 * Note even though this is only one line, it can't be inline
 * since the address of the function needs to be taken (passed
 * to the "create thread" function).
 */
void* Thread::ThreadStarter(void* lpParams)
{
	/*
	 * lpParams is a pointer to the "this" pointer, so since calling implicitly
	 * through "this" pointer, the main can be a virtual function.
	 */
	(*(Thread**)lpParams)->main();

	return 0;
}
