#ifndef _MUTEX_H
#define _MUTEX_H
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
//#ifdef USE_PTHREAD_MUTEX
#ifdef HAVE_PTHREAD
typedef struct _MUTEX
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
}MUTEX;
#define MT(ptr) ((MUTEX *)ptr)
#ifdef HAVE_MMAP
#define MUTEX_INIT(ptr)                                                     \
do                                                                          \
{                                                                           \
    if((ptr = mmap(NULL, sizeof(MUTEX), PROT_READ|PROT_WRITE,               \
                    MAP_ANON|MAP_PRIVATE, -1, 0)) && ptr != (void *)-1)     \
    {                                                                       \
        memset(ptr, 0, sizeof(MUTEX));                                      \
        pthread_mutex_init(&(MT(ptr)->mutex), NULL);                        \
        pthread_cond_init(&(MT(ptr)->cond), NULL);                          \
    }                                                                       \
    else ptr = NULL;                                                        \
}while(0)
#else
#define MUTEX_INIT(ptr)                                                     \
do                                                                          \
{                                                                           \
    if((ptr = calloc(1, sizeof(MUTEX))))                                    \
    {                                                                       \
        pthread_mutex_init(&(MT(ptr)->mutex), NULL);                        \
        pthread_cond_init(&(MT(ptr)->cond), NULL);                          \
    }                                                                       \
}while(0)
#endif
#define MUTEX_LOCK(ptr) ((ptr) ? pthread_mutex_lock(&(MT(ptr)->mutex)): -1)
#define MUTEX_UNLOCK(ptr) ((ptr) ? pthread_mutex_unlock(&(MT(ptr)->mutex)): -1)
#define MUTEX_WAIT(ptr) ((ptr && pthread_mutex_lock(&(MT(ptr)->mutex)) == 0 && pthread_cond_wait(&(MT(ptr)->cond), &(MT(ptr)->mutex)) == 0 && pthread_mutex_unlock(&(MT(ptr)->mutex)) == 0)? 0 : -1)
#define MUTEX_SIGNAL(ptr) ((ptr)?pthread_cond_signal(&(MT(ptr)->cond)): -1)
#ifdef HAVE_MMAP
#define MUTEX_DESTROY(ptr)                                                  \
do                                                                          \
{															                \
	if(ptr)													                \
	{														                \
		pthread_mutex_destroy(&(MT(ptr)->mutex)); 						    \
		pthread_cond_destroy(&(MT(ptr)->cond)); 							\
		munmap(ptr, sizeof(MUTEX));											\
		ptr = NULL;												            \
	}														                \
}while(0)
#else
#define MUTEX_DESTROY(ptr)                                                  \
do                                                                          \
{															                \
	if(ptr)													                \
	{														                \
		pthread_mutex_destroy(&(MT(ptr)->mutex)); 						    \
		pthread_cond_destroy(&(MT(ptr)->cond)); 							\
		free(ptr);											                \
		ptr = NULL;												            \
	}														                \
}while(0)
#endif
#else
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
    if((ptr = calloc(1, sizeof(MUTEX))))                                    \
    {                                                                       \
        sem_init(&(MT(ptr)->sem), 0, 1);                                    \
    }                                                                       \
}while(0)
#define MUTEX_LOCK(ptr) ((ptr)?sem_wait(&(MT(ptr)->sem)):-1)
#define MUTEX_UNLOCK(ptr) ((ptr)?sem_post(&(MT(ptr)->sem)):-1)
#define MUTEX_WAIT(ptr) ((ptr)?sem_wait(&(MT(ptr)->sem)):-1)
#define MUTEX_SIGNAL(ptr) ((ptr)?sem_post(&(MT(ptr)->sem)):-1) 
#define MUTEX_DESTROY(ptr)                                                  \
do                                                                          \
{                                                                           \
    if(ptr){sem_destroy(&(MT(ptr)->sem));free(ptr);}                        \
}while(0)
#else
#define MUTEX_INIT(ptr)
#define MUTEX_LOCK(ptr)
#define MUTEX_UNLOCK(ptr)
#define MUTEX_WAIT(ptr)
#define MUTEX_SIGNAL(ptr)
#define MUTEX_DESTROY(ptr)
#endif
#endif
#ifdef __cplusplus
 }
#endif
#endif
