#include <lib/base/thread.h>
#include <stdio.h>
#include <lib/base/eerror.h>

void eThread::thread_completed(void *ptr)
{
	eThread *p = (eThread*) ptr;
	eDebug("thread has completed..");
	p->alive=0;
	p->thread_finished();
}

void *eThread::wrapper(void *ptr)
{
	eThread *p = (eThread*)ptr;
	p->alive=1;
	pthread_cleanup_push( thread_completed, (void*)p );
	p->thread();
	pthread_exit(0);
	pthread_cleanup_pop(0);
}

eThread::eThread()
	:alive(0)
{
}

void eThread::run( int prio, int policy )
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (prio||policy)
	{
		struct sched_param p;
		p.__sched_priority=prio;
		pthread_attr_setschedpolicy(&attr, policy );
		pthread_attr_setschedparam(&attr, &p);
	}
	pthread_create(&the_thread, &attr, wrapper, this);
}                     

eThread::~eThread()
{
	if (alive)
		kill();
}

void eThread::kill(bool hard)
{
	if ( hard )
	{
		eDebug("killing the thread...");
		pthread_cancel(the_thread);
		alive=0;
	}
	else
	{
		eDebug("waiting for thread shutdown...");
		pthread_join(the_thread, 0);
		eDebug("ok");
	}
}
