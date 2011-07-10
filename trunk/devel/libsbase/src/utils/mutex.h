#ifndef __MUTEX_H__
#define __MUTEX_H__
#ifdef __cplusplus
extern "C" {
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "xmm.h"
#ifdef HAVE_SEMAPHORE
#include <semaphore.h>
typedef struct _MUTEX
{
    sem_t sem;
}MUTEX;
#define MT(ptr) ((MUTEX *)ptr)
#define MUTEX_INIT(ptr)                                                     \
do                                                                          \
{                                                                           \
    if((ptr = xmm_mnew(sizeof(MUTEX))))                                     \
    {                                                                       \
        sem_init(&(MT(ptr)->sem), 0, 1);                                    \
    }                                                                       \
}while(0)
#define MUTEX_LOCK(ptr) ((ptr)?sem_wait(&(MT(ptr)->sem)):-1)
#define MUTEX_UNLOCK(ptr) ((ptr)?sem_post(&(MT(ptr)->sem)):-1)
#define MUTEX_WAIT(ptr) ((ptr)?sem_wait(&(MT(ptr)->sem)):-1)
#define MUTEX_TIMEDWAIT(ptr, ts) ((ptr)?sem_timedwait(&(MT(ptr)->sem), &ts):-1)
#define MUTEX_SIGNAL(ptr) ((ptr)?sem_post(&(MT(ptr)->sem)):-1) 
#define MUTEX_DESTROY(ptr)                                                  \
do                                                                          \
{                                                                           \
    if(ptr){sem_destroy(&(MT(ptr)->sem));xmm_free(ptr, sizeof(MUTEX));}     \
}while(0)
#else
#ifdef HAVE_PTHREAD
typedef struct _MUTEX
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int nowait;
}MUTEX;
#define MT(ptr) ((MUTEX *)ptr)
#define MUTEX_INIT(ptr)                                                     \
do                                                                          \
{                                                                           \
    if((ptr = xmm_new(sizeof(MUTEX))))                                      \
    {                                                                       \
        pthread_mutex_init(&(MT(ptr)->mutex), NULL);                        \
        pthread_cond_init(&(MT(ptr)->cond), NULL);                          \
    }                                                                       \
}while(0)
#define MUTEX_LOCK(ptr) ((ptr) ? pthread_mutex_lock(&(MT(ptr)->mutex)): -1)
#define MUTEX_UNLOCK(ptr) ((ptr) ? pthread_mutex_unlock(&(MT(ptr)->mutex)): -1)
#define MUTEX_WAIT(ptr)                                                                 \
do                                                                                      \
{                                                                                       \
    if(ptr)                                                                             \
    {                                                                                   \
        pthread_mutex_lock(&(MT(ptr)->mutex));                                          \
        if(MT(ptr)->nowait == 0)                                                        \
            pthread_cond_wait(&(MT(ptr)->cond), &(MT(ptr)->mutex));                     \
        MT(ptr)->nowait = 0;                                                            \
        pthread_mutex_unlock(&(MT(ptr)->mutex));                                        \
    }                                                                                   \
}while(0)
#define MUTEX_TIMEDWAIT(ptr, ts)                                                        \
do                                                                                      \
{                                                                                       \
    if(ptr)                                                                             \
    {                                                                                   \
        pthread_mutex_lock(&(MT(ptr)->mutex));                                          \
        if(MT(ptr)->nowait == 0)                                                        \
            pthread_cond_timedwait(&(MT(ptr)->cond), &(MT(ptr)->mutex), &ts);           \
        MT(ptr)->nowait = 0;                                                            \
        pthread_mutex_unlock(&(MT(ptr)->mutex));                                        \
    }                                                                                   \
}while(0)
#define MUTEX_SIGNAL(ptr)                                                               \
do                                                                                      \
{                                                                                       \
    if(ptr)                                                                             \
    {                                                                                   \
        pthread_mutex_lock(&(MT(ptr)->mutex));                                          \
        pthread_cond_signal(&(MT(ptr)->cond));                                          \
        MT(ptr)->nowait = 1;                                                            \
        pthread_mutex_unlock(&(MT(ptr)->mutex));                                        \
    }                                                                                   \
}while(0)
#define MUTEX_DESTROY(ptr)                                                              \
do                                                                                      \
{															                            \
	if(ptr)													                            \
	{														                            \
		pthread_mutex_destroy(&(MT(ptr)->mutex)); 						                \
		pthread_cond_destroy(&(MT(ptr)->cond)); 							            \
		xmm_free(ptr, sizeof(MUTEX));										            \
		ptr = NULL;												                        \
	}														                            \
}while(0)
#endif
#endif
#ifdef __cplusplus
 }
#endif
#endif
