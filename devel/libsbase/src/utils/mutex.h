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
#define __MM__(ptr) (&(((MUTEX *)ptr)->mutex))
#define MUTEX_RESET(ptr) ((ptr)?sem_init(__MM__(ptr), 0, 1):-1)
#define MUTEX_INIT(ptr) do{if((ptr = (MUTEX *)calloc(1, sizeof(MUTEX)))){MUTEX_RESET(ptr);}}while(0)
#define MUTEX_LOCK(ptr) ((ptr)?sem_wait(__MM__(ptr)):-1)
#define MUTEX_UNLOCK(ptr) ((ptr)?sem_post(__MM__(ptr)):-1)
#define MUTEX_WAIT(ptr) ((ptr)?sem_wait(__MM__(ptr)):-1)
#define MUTEX_TIMEDWAIT(ptr, ts) ((ptr)?sem_timedwait(__MM__(ptr), &ts):-1)
#define MUTEX_SIGNAL(ptr) ((ptr)?sem_post(__MM__(ptr)):-1)
#define MUTEX_DESTROY(ptr) do{if(ptr){sem_destroy(__MM__(ptr));free(ptr);}}while(0)
#else
#define __M__(ptr) ((MUTEX *)ptr)
#define __MM__(ptr) (&(((MUTEX *)ptr)->mutex))
#define __MC__(ptr) (&(((MUTEX *)ptr)->cond))
#define MUTEX_RESET(ptr)                                                                \
{if(ptr)                                                                                \
{                                                                                       \
        pthread_mutex_init(__MM__(ptr), NULL);                                          \
        pthread_cond_init(__MC__(ptr), NULL);                                           \
}}
#define MUTEX_INIT(ptr) do{if((ptr = (MUTEX *)calloc(1, sizeof(MUTEX)))){MUTEX_RESET(ptr);}}while(0)
#define MUTEX_LOCK(ptr) ((ptr)?pthread_mutex_lock(__MM__(ptr)):-1)
#define MUTEX_UNLOCK(ptr) ((ptr)?pthread_mutex_unlock(__MM__(ptr)):-1)
#define MUTEX_WAIT(ptr)                                                                 \
{if(ptr)                                                                                \
{                                                                                       \
    pthread_mutex_lock(__MM__(ptr));                                                    \
    if(__M__(ptr)->nowait == 0)                                                         \
    pthread_cond_wait(__MC__(ptr), __MM__(ptr));                                        \
    __M__(ptr)->nowait = 0;                                                             \
    pthread_mutex_unlock(__MM__(ptr));                                                  \
}}
#define MUTEX_TIMEDWAIT(ptr, ts)                                                        \
{if(ptr)                                                                                \
{                                                                                       \
    pthread_mutex_lock(__MM__(ptr));                                                    \
    if(__M__(ptr)->nowait == 0)                                                         \
    pthread_cond_timedwait(__MC__(ptr), __MM__(ptr), &ts);                              \
    __M__(ptr)->nowait = 0;                                                             \
    pthread_mutex_unlock(__MM__(ptr));                                                  \
}}
#define MUTEX_SIGNAL(ptr)                                                               \
{if(ptr)                                                                                \
{                                                                                       \
    pthread_mutex_lock(__MM__(ptr));                                                    \
    pthread_cond_signal(__MC__(ptr));                                                   \
    __M__(ptr)->nowait = 1;                                                             \
    pthread_mutex_unlock(__MM__(ptr));                                                  \
}}
#define MUTEX_DESTROY(ptr)                                                              \
{if(ptr)                                                                                \
{															                            \
    pthread_mutex_destroy(__MM__(ptr)); 						                        \
    pthread_cond_destroy(__MC__(ptr)); 							                        \
    free(ptr);                                                                          \
}}
#endif
#ifdef __cplusplus
 }
#endif
#endif
