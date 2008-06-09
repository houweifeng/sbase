#include <unistd.h>
#include "mutex.h"
#ifndef _EVTIMER_H
#define _EVTIMER_H
typedef void (EVTCALLBACK)(void *);
typedef struct _EVTNODE
{
    int id;
    EVTCALLBACK *handler;
    void *arg;
    long long evusec;
    int ison;
    struct _EVTNODE *prev;
    struct _EVTNODE *next;
}EVTNODE;
typedef struct _EVTIMER
{
   struct timeval tv;
   EVTNODE **evlist;
   int nlist;
   EVTNODE *head;
   EVTNODE *tail;
   EVTNODE *node;
   int nq;
   int left;
   long long now;
   int n ;
   void *mutex;
}EVTIMER;
#define PEVT(ptr) ((EVTIMER *)ptr)
#define PEVT_NQ(ptr) (((EVTIMER *)ptr)->nq)
#define PEVT_LEFT(ptr) (((EVTIMER *)ptr)->left)
#define PEVT_NOW(ptr) (((EVTIMER *)ptr)->now)
#define PEVT_N(ptr) (((EVTIMER *)ptr)->n)
#define PEVT_NLIST(ptr) (((EVTIMER *)ptr)->nlist)
#define PEVT_NODE(ptr) (((EVTIMER *)ptr)->node)
#define PEVT_HEAD(ptr) (((EVTIMER *)ptr)->head)
#define PEVT_TAIL(ptr) (((EVTIMER *)ptr)->tail)
#define PEVT_EVLIST(ptr) (((EVTIMER *)ptr)->evlist)
#define PEVT_EVN(ptr, n) (((EVTIMER *)ptr)->evlist[n])
#define EVTIMER_NOW(ptr) ((ptr && gettimeofday(&(PEVT(ptr)->tv), NULL) == 0)?                   \
    (PEVT_NOW(ptr) = PEVT(ptr)->tv.tv_sec * 1000000ll + PEVT(ptr)->tv.tv_usec * 1ll) : -1)

/* evtimer initialize */
#define EVTIMER_INIT(ptr)                                                                       \
do{                                                                                             \
    if((ptr = calloc(1, sizeof(EVTIMER))))                                                      \
    {                                                                                           \
        MUTEX_INIT(PEVT(ptr)->mutex);                                                           \
    }                                                                                           \
}while(0)

/* evtimer resort */
#define EVTIMER_RESORT(ptr, evnode)                                                             \
{                                                                                               \
    if(ptr && evnode)                                                                           \
    {                                                                                           \
        while(evnode->next && evnode->evusec > evnode->next->evusec)                            \
        {                                                                                       \
            if(evnode->prev) evnode->prev->next = evnode->next;                                 \
            evnode->next->prev = evnode->prev;                                                  \
            evnode->prev = evnode->next;                                                        \
            evnode->next = evnode->prev->next;                                                  \
            evnode->prev->next = evnode;                                                        \
            if(evnode->next)evnode->next->prev = evnode;                                        \
        }                                                                                       \
        while(evnode->prev && evnode->evusec < evnode->prev->evusec)                            \
        {                                                                                       \
            if(evnode->next) evnode->next->prev = evnode->prev;                                 \
            evnode->prev->next = evnode->next;                                                  \
            evnode->next = evnode->prev;                                                        \
            evnode->prev = evnode->next->prev;                                                  \
            evnode->next->prev = evnode;                                                        \
            if(evnode->prev)evnode->prev->next = evnode;                                        \
        }                                                                                       \
        if(evnode->next == NULL) PEVT_TAIL(ptr) = evnode;                                       \
        if(evnode->prev == NULL) PEVT_HEAD(ptr) = evnode;                                       \
    }                                                                                           \
}

/* evtimer push */
#define EVTIMER_PUSH(ptr, evnode)                                                               \
do{                                                                                             \
    if(ptr && evnode)                                                                           \
    {                                                                                           \
        if(PEVT_HEAD(ptr) == NULL || PEVT_TAIL(ptr) == NULL)                                    \
        {                                                                                       \
            PEVT_HEAD(ptr) = evnode;                                                            \
            PEVT_TAIL(ptr) = evnode;                                                            \
        }                                                                                       \
        else if(PEVT_HEAD(ptr) && evnode->evusec <= PEVT_HEAD(ptr)->evusec)                     \
        {                                                                                       \
            evnode->next = PEVT_HEAD(ptr);                                                      \
            PEVT_HEAD(ptr)->prev = evnode;                                                      \
            PEVT_HEAD(ptr) = evnode;                                                            \
        }                                                                                       \
        else if(PEVT_TAIL(ptr) && evnode->evusec >= PEVT_TAIL(ptr)->evusec)                     \
        {                                                                                       \
            evnode->prev = PEVT_TAIL(ptr);                                                      \
            PEVT_TAIL(ptr)->next = evnode;                                                      \
            PEVT_TAIL(ptr) = evnode;                                                            \
        }                                                                                       \
        else                                                                                    \
        {                                                                                       \
            PEVT_NODE(ptr) = PEVT_TAIL(ptr);                                                    \
            do                                                                                  \
            {                                                                                   \
                PEVT_NODE(ptr) = PEVT_NODE(ptr)->prev;                                          \
            }while(PEVT_NODE(ptr) && PEVT_NODE(ptr)->evusec > evnode->evusec);                  \
            if(PEVT_NODE(ptr))                                                                  \
            {                                                                                   \
                PEVT_NODE(ptr)->next->prev = evnode;                                            \
                evnode->next = PEVT_NODE(ptr)->next;                                            \
                evnode->prev = PEVT_NODE(ptr);                                                  \
                PEVT_NODE(ptr)->next = evnode;                                                  \
            }                                                                                   \
        }                                                                                       \
        evnode->ison = 1;                                                                       \
        PEVT_NQ(ptr)++;                                                                         \
    }                                                                                           \
}while(0)

/* evtimer add */
#define EVTIMER_ADD(ptr, evtime, evhandler, evarg, evid)                                        \
do{                                                                                             \
    evid = -1;                                                                                  \
    MUTEX_LOCK(PEVT(ptr)->mutex);                                                               \
    if(PEVT_LEFT(ptr) == 0)                                                                     \
    {                                                                                           \
        if((PEVT_EVLIST(ptr) = (EVTNODE **)realloc(PEVT_EVLIST(ptr),                            \
                        (PEVT_NLIST(ptr) + 1) * sizeof(EVTNODE *))))                            \
        {                                                                                       \
            evid = PEVT_NLIST(ptr)++;                                                           \
            PEVT_EVN(ptr, evid) = (EVTNODE *)calloc(1, sizeof(EVTNODE));                        \
        }                                                                                       \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        for(PEVT_N(ptr) = 0; PEVT_N(ptr) < PEVT_NLIST(ptr); PEVT_N(ptr)++)                      \
        {                                                                                       \
            if(PEVT_EVN(ptr, PEVT_N(ptr))->id == -1)                                            \
            {                                                                                   \
                evid = PEVT_N(ptr);                                                             \
                break;                                                                          \
            }                                                                                   \
        }                                                                                       \
    }                                                                                           \
    if(evid >= 0 && PEVT_EVLIST(ptr))                                                           \
    {                                                                                           \
        PEVT_EVN(ptr, evid)->id      = evid;                                                    \
        PEVT_EVN(ptr, evid)->handler = evhandler;                                               \
        PEVT_EVN(ptr, evid)->arg     = evarg;                                                   \
        PEVT_EVN(ptr, evid)->evusec  = EVTIMER_NOW(ptr) + evtime;                               \
        EVTIMER_PUSH(ptr, PEVT_EVN(ptr, evid));                                                 \
    }                                                                                           \
    MUTEX_UNLOCK(PEVT(ptr)->mutex);                                                             \
}while(0)

/* evtimer update */
#define EVTIMER_UPDATE(ptr, evid, evtime, evhandler, evarg)                                     \
do{                                                                                             \
    MUTEX_LOCK(PEVT(ptr)->mutex);                                                               \
    if(ptr && evid >= 0 && evid < PEVT_NLIST(ptr) && PEVT_EVLIST(ptr))                          \
    {                                                                                           \
        PEVT_EVN(ptr, evid)->id      = evid;                                                    \
        PEVT_EVN(ptr, evid)->handler = evhandler;                                               \
        PEVT_EVN(ptr, evid)->arg     = evarg;                                                   \
        PEVT_EVN(ptr, evid)->evusec  = EVTIMER_NOW(ptr) + evtime;                               \
        if(PEVT_EVN(ptr, evid)->ison)                                                           \
        {                                                                                       \
            EVTIMER_RESORT(ptr, PEVT_EVN(ptr, evid));                                           \
        }                                                                                       \
        else                                                                                    \
        {                                                                                       \
            PEVT_EVN(ptr, evid)->next = NULL;                                                   \
            PEVT_EVN(ptr, evid)->prev = NULL;                                                   \
            EVTIMER_PUSH(ptr, PEVT_EVN(ptr, evid));                                             \
        }                                                                                       \
    }                                                                                           \
    MUTEX_UNLOCK(PEVT(ptr)->mutex);                                                             \
}while(0)

/* evtimer delete */
#define EVTIMER_DEL(ptr, evid)                                                                  \
do{                                                                                             \
    MUTEX_LOCK(PEVT(ptr)->mutex);                                                               \
    if(ptr && evid >= 0 && evid < PEVT_NLIST(ptr) && PEVT_EVLIST(ptr) && PEVT_EVN(ptr, evid))   \
    {                                                                                           \
        PEVT_EVN(ptr, evid)->id      = -1;                                                      \
    }                                                                                           \
    MUTEX_UNLOCK(PEVT(ptr)->mutex);                                                             \
}while(0)

/* evtimer list */
#define EVTIMER_LIST(ptr, fp)                                                                   \
do{                                                                                             \
    MUTEX_LOCK(PEVT(ptr)->mutex);                                                               \
    if(ptr && (PEVT_NODE(ptr) = PEVT_HEAD(ptr)))                                                \
    {                                                                                           \
        do{                                                                                     \
            fprintf(fp, "prev:%08x id:%d ison:%d handler:%08x arg:%08x next:%08x\n",            \
                    PEVT_NODE(ptr)->prev, PEVT_NODE(ptr)->id, PEVT_NODE(ptr)->ison,             \
                    PEVT_NODE(ptr)->handler, PEVT_NODE(ptr)->arg, PEVT_NODE(ptr)->next);        \
        }while((PEVT_NODE(ptr) = PEVT_NODE(ptr)->next));                                        \
    }                                                                                           \
    MUTEX_UNLOCK(PEVT(ptr)->mutex);                                                             \
}while(0)

/* evtimer check */
#define EVTIMER_CHECK(ptr)                                                                      \
do{                                                                                             \
    MUTEX_LOCK(PEVT(ptr)->mutex);                                                               \
    if(ptr && PEVT_HEAD(ptr) && PEVT_NQ(ptr) > 0)                                               \
    {                                                                                           \
        EVTIMER_NOW(ptr);                                                                       \
        while((PEVT_NODE(ptr) = PEVT_HEAD(ptr))  && PEVT_NODE(ptr)->evusec <= PEVT_NOW(ptr))    \
        {                                                                                       \
            if(PEVT_NODE(ptr)->handler)                                                         \
            {                                                                                   \
                PEVT_NODE(ptr)->handler(PEVT_NODE(ptr)->arg);                                   \
            }                                                                                   \
            PEVT_NQ(ptr)--;                                                                     \
            PEVT_HEAD(ptr) = PEVT_NODE(ptr)->next;                                              \
            PEVT_NODE(ptr)->ison = 0;                                                           \
            PEVT_NODE(ptr)->prev = NULL;                                                        \
            PEVT_NODE(ptr)->next = NULL;                                                        \
            if(PEVT_HEAD(ptr) == NULL) PEVT_TAIL(ptr) = NULL;                                   \
        }                                                                                       \
    }                                                                                           \
    MUTEX_UNLOCK(PEVT(ptr)->mutex);                                                             \
}while(0)

/* evtimer reset */
#define EVTIMER_RESET(ptr)                                                                      \
do{                                                                                             \
    if(ptr)                                                                                     \
    {                                                                                           \
        if(PEVT_EVLIST(ptr))                                                                    \
        {                                                                                       \
            for(PEVT_N(ptr) = 0; PEVT_N(ptr) < PEVT_NLIST(ptr); PEVT_N(ptr)++)                  \
            {                                                                                   \
                if(PEVT_EVN(ptr, PEVT_N(ptr)))                                                  \
                {                                                                               \
                    memset(PEVT_EVN(ptr, PEVT_N(ptr)), 0, sizeof(EVTNODE));                     \
                    PEVT_EVN(ptr, PEVT_N(ptr))->id = -1;                                        \
                }                                                                               \
            }                                                                                   \
            PEVT_LEFT(ptr) = PEVT_NLIST(ptr);                                                   \
        }                                                                                       \
        PEVT_NQ(ptr) = 0;                                                                       \
        PEVT_HEAD(ptr) = NULL;                                                                  \
        PEVT_TAIL(ptr) = NULL;                                                                  \
    }                                                                                           \
}while(0)

/* evtimer clean */
#define EVTIMER_CLEAN(ptr)                                                                      \
{                                                                                               \
    if(ptr)                                                                                     \
    {                                                                                           \
        if(PEVT_EVLIST(ptr))                                                                    \
        {                                                                                       \
            for(PEVT_N(ptr) = 0; PEVT_N(ptr) < PEVT_NLIST(ptr); PEVT_N(ptr)++)                  \
            {                                                                                   \
                if(PEVT_EVN(ptr, PEVT_N(ptr))) free(PEVT_EVN(ptr, PEVT_N(ptr)));                \
            }                                                                                   \
            free(PEVT_EVLIST(ptr));                                                             \
        }                                                                                       \
        free(ptr);                                                                              \
        ptr = NULL;                                                                             \
    }                                                                                           \
}
#endif
