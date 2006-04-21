#include <lib/base/thread.h>

#include <stdio.h>
#include <unistd.h>
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
	pthread_cleanup_push( thread_completed, (void*)p );
	p->thread();
	pthread_exit(0);
	pthread_cleanup_pop(1);
	return 0;
}

eThread::eThread()
	:the_thread(0), alive(0)
{
}

void eThread::run( int prio, int policy )
{
	if (alive)
	{
		eDebug("thread already running !!");
		return;
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (prio||policy)
	{
		struct sched_param p;
		p.__sched_priority=prio;
		pthread_attr_setschedpolicy(&attr, policy );
		pthread_attr_setschedparam(&attr, &p);
	}
	if ( pthread_create(&the_thread, &attr, wrapper, this) )
	{
		eDebug("couldn't create new thread");
		return;
	}
	alive = 1;
}

eThread::~eThread()
{
	kill();
}

void eThread::kill(bool sendcancel)
{
	if (!the_thread)
		return;
	if ( alive && sendcancel )
	{
		eDebug("send cancel to thread");
		pthread_cancel(the_thread);
	}
	eDebug("thread joined %d", pthread_join(the_thread, 0));
	the_thread=0;
}
