#ifndef __MUTEX_H__
#define __MUTEX_H__
#include <semaphore.h>
#include <time.h>
#ifdef HAVE_SEMAPHORE
#define MUTEX_INIT(mutex) sem_init(&mutex, 0, 1)
#define MUTEX_LOCK(mutex) sem_wait(&mutex)
#define MUTEX_UNLOCK(mutex) sem_post(&mutex)
#define MUTEX_DESTROY(mutex) sem_destroy(&mutex)
#define MUTEX_COND_INIT(mutex_cond) sem_init(&mutex_cond, 0, 1)
#define MUTEX_COND_WAIT(mutex_cond) sem_wait(&mutex_cond)
#define MUTEX_COND_TIMEDWAIT(mutex_cond, tspec) sem_timedwait(&mutex_cond, &tspec)
#define MUTEX_COND_SIGNAL(mutex_cond) sem_post(&mutex_cond)
#define MUTEX_COND_DESTROY(mutex_cond) sem_destroy(&mutex_cond)
#else
#define MUTEX_INIT(mutex) pthread_mutex_init(&mutex)
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex)
#define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&mutex)
#define MUTEX_COND_INIT(mutex_cond) pthread_cond_init(&mutex_cond)
#define MUTEX_COND_WAIT(mutex_cond) pthread_cond_wait(&mutex_cond)
#define MUTEX_COND_TIMEDWAIT(mutex_cond, tspec) pthread_cond_timedwait(&mutex_cond, &tspec)
#define MUTEX_COND_SIGNAL(mutex_cond) pthread_cond_signal(&mutex_cond)
#define MUTEX_COND_DESTROY(mutex_cond) pthread_cond_destroy(&mutex_cond)
#endif
#endif
