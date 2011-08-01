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
    sem_t mutex;
    sem_t cond;
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
#define __MM__(m) (&(((MUTEX *)m)->mutex))
#define __MC__(m) (&(((MUTEX *)m)->cond))
#define MUTEX_RESET(m) do{if(m){sem_init(__MM__(m), 0, 1);sem_init(__MC__(m), 0, 1);}}while(0)
#define MUTEX_INIT(m) do{if((m = (MUTEX *)calloc(1, sizeof(MUTEX)))){MUTEX_RESET(m);}}while(0)
#define MUTEX_LOCK(m) ((m)?sem_wait(__MM__(m)):-1)
#define MUTEX_UNLOCK(m) ((m)?sem_post(__MM__(m)):-1)
#define MUTEX_WAIT(m) ((m)?sem_wait(__MC__(m)):-1)
#define MUTEX_TIMEDWAIT(m, ts) ((m)?sem_timedwait(__MC__(m), &ts):-1)
#define MUTEX_SIGNAL(m) ((m)?sem_post(__MC__(m)):-1)
#define MUTEX_DESTROY(m) do{if(m){sem_destroy(__MM__(m));sem_destroy(__MC__(m));free(m);}}while(0)
#else
#define __M__(m) ((MUTEX *)m)
#define __MM__(m) (&(((MUTEX *)m)->mutex))
#define __MC__(m) (&(((MUTEX *)m)->cond))
#define MUTEX_RESET(m)                                                              \
{if(m)                                                                              \
{                                                                                   \
        pthread_mutex_init(__MM__(m), NULL);                                        \
        pthread_cond_init(__MC__(m), NULL);                                         \
}}
#define MUTEX_INIT(m) do{if((m = (MUTEX *)calloc(1, sizeof(MUTEX)))){MUTEX_RESET(m);}}while(0)
#define MUTEX_LOCK(m) pthread_mutex_lock(__MM__(m))
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(__MM__(m))
#define MUTEX_WAIT(m)                                                               \
{if(m)                                                                              \
{                                                                                   \
    pthread_mutex_lock(__MM__(m));                                                  \
    if(__M__(m)->nowait == 0)                                                       \
    pthread_cond_wait(__MC__(m), __MM__(m));                                        \
    __M__(m)->nowait = 0;                                                           \
    pthread_mutex_unlock(__MM__(m));                                                \
}}
#define MUTEX_TIMEDWAIT(m, ts)                                                      \
{if(m)                                                                              \
{                                                                                   \
    pthread_mutex_lock(__MM__(m));                                                  \
    if(__M__(m)->nowait == 0)                                                       \
    pthread_cond_timedwait(__MC__(m), __MM__(m), &ts);                              \
    __M__(m)->nowait = 0;                                                           \
    pthread_mutex_unlock(__MM__(m));                                                \
}}
#define MUTEX_SIGNAL(m)                                                             \
{if(m)                                                                              \
{                                                                                   \
    pthread_mutex_lock(__MM__(m));                                                  \
    if(__M__(m)->nowait == 0)                                                       \
        pthread_cond_signal(__MC__(m));                                             \
    __M__(m)->nowait = 1;                                                           \
    pthread_mutex_unlock(__MM__(m));                                                \
}}
#define MUTEX_DESTROY(m)                                                            \
{if(m)                                                                              \
{															                        \
    pthread_mutex_destroy(__MM__(m)); 						                        \
    pthread_cond_destroy(__MC__(m)); 							                    \
    free(m);                                                                        \
}}
#endif
#ifdef __cplusplus
 }
#endif
#endif
