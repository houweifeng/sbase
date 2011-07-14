#ifndef __MUTEX_H__
#define __MUTEX_H__
#ifdef __cplusplus
extern "C" {
#endif
#ifndef __TYPEDEF__MUTEX
#define __TYPEDEF__MUTEX
#ifdef HAVE_SEMAPHORE
#include <semaphore.h>
typedef struct _MUTEX
{
    sem_t sem;
}MUTEX;
#else
#include <pthread.h>
typedef struct _MUTEX
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int nowait;
    int bits;
}MUTEX;
#endif
#endif
#ifdef HAVE_SEMAPHORE
#define MUTEX_RESET(m) sem_init(&(m.sem), 0, 1)
#define MUTEX_LOCK(m) sem_wait(&(m.sem))
#define MUTEX_UNLOCK(m) sem_post(&(m.sem))
#define MUTEX_WAIT(m) sem_wait(&(m.sem))
#define MUTEX_TIMEDWAIT(m, ts) sem_timedwait(&(m.sem), &ts)
#define MUTEX_SIGNAL(m) sem_post(&(m.sem))
#define MUTEX_DESTROY(m) sem_destroy(&(m.sem))
#else
#define MUTEX_RESET(m)                                                      \
do                                                                          \
{                                                                           \
        pthread_mutex_init(&(m.mutex), NULL);                               \
        pthread_cond_init(&(m.cond), NULL);                                 \
}while(0)
#define MUTEX_LOCK(m) pthread_mutex_lock(&(m.mutex))
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(&(m.mutex))
#define MUTEX_WAIT(m)                                                       \
do                                                                          \
{                                                                           \
    pthread_mutex_lock(&(m.mutex));                                         \
    if(m.nowait == 0)                                                       \
    pthread_cond_wait(&(m.cond), &(m.mutex));                               \
    m.nowait = 0;                                                           \
    pthread_mutex_unlock(&(m.mutex));                                       \
}while(0)
#define MUTEX_TIMEDWAIT(m, ts)                                              \
do                                                                          \
{                                                                           \
    pthread_mutex_lock(&(m.mutex));                                         \
    if(m.nowait == 0)                                                       \
    pthread_cond_timedwait(&(m.cond), &(m.mutex), &ts);                     \
    m.nowait = 0;                                                           \
    pthread_mutex_unlock(&(m.mutex));                                       \
}while(0)
#define MUTEX_SIGNAL(m)                                                     \
do                                                                          \
{                                                                           \
    pthread_mutex_lock(&(m.mutex));                                         \
    pthread_cond_signal(&(m.cond));                                         \
    m.nowait = 1;                                                           \
    pthread_mutex_unlock(&(m.mutex));                                       \
}while(0)
#define MUTEX_DESTROY(m)                                                    \
do                                                                          \
{															                \
    pthread_mutex_destroy(&(m.mutex)); 						                \
    pthread_cond_destroy(&(m.cond)); 							            \
}while(0)
#endif
#ifdef __cplusplus
 }
#endif
#endif
