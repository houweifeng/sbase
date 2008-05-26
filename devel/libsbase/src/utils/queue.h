#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifndef _QUEUE_H
#define _QUEUE_H
#ifdef __cplusplus
extern "C" {
#endif
#define QUEUE_BLOCK_SIZE  32
typedef struct _QUEUE
{
    int pos;
    int total;
    int left;
    int count;
    int newcount;
    int head;
    int tail;
    void *table;
    void *newtable;
}QUEUE;

#define Q(ptr) ((QUEUE *)ptr)
#define QTOTAL(ptr) ((Q(ptr)->total))
#define QNTAB(ptr) ((Q(ptr)->newtable))
#define QTAB(ptr) ((Q(ptr)->table))
#define QTI(ptr, type, n) (((type *)(Q(ptr)->table))[n])
#define QNTI(ptr, type, n) (((type *)(Q(ptr)->newtable))[n])
#define QPOS(ptr) (Q(ptr)->pos)
#define QHEAD(ptr) (Q(ptr)->head)
#define QTAIL(ptr) (Q(ptr)->tail)
#define QLEFT(ptr) (Q(ptr)->left)
#define QCOUNT(ptr) (Q(ptr)->count)
#define QNCOUNT(ptr) (Q(ptr)->newcount)
#define QUEUE_INIT() ((calloc(1, sizeof(QUEUE)))) 
#define QUEUE_HEAD(ptr, type, dptr) ((ptr && QTOTAL(ptr) > 0                                \
            && memcpy(dptr, &(QTI(ptr, type, QHEAD(ptr))), sizeof(type)))? 0 : -1)
#define QUEUE_RESIZE(ptr, type)                                                             \
{                                                                                           \
    if(QLEFT(ptr) <= 0)                                                                     \
    {                                                                                       \
        QNCOUNT(ptr) = QCOUNT(ptr) + QUEUE_BLOCK_SIZE;                                      \
        if((QNTAB(ptr) = calloc(QNCOUNT(ptr), sizeof(type))))                               \
        {                                                                                   \
            if(QTAB(ptr) && QCOUNT(ptr) > 0)                                                \
            {                                                                               \
                QPOS(ptr) = 0;                                                              \
                while(QHEAD(ptr) != QTAIL(ptr))                                             \
                {                                                                           \
                    if(QHEAD(ptr) == QCOUNT(ptr)) QHEAD(ptr) = 0;                           \
                    memcpy(&(QNTI(ptr, type, QPOS(ptr))),                                   \
                            &(QTI(ptr, type, QHEAD(ptr))), sizeof(type));                   \
                    QPOS(ptr)++;QHEAD(ptr)++;                                               \
                }                                                                           \
                free(QTAB(ptr));                                                            \
                QTAIL(ptr) = QPOS(ptr);                                                     \
                QHEAD(ptr) = 0;                                                             \
            }                                                                               \
            QLEFT(ptr) += QUEUE_BLOCK_SIZE;                                                 \
            QTAB(ptr) = QNTAB(ptr);                                                         \
            QCOUNT(ptr) = QNCOUNT(ptr);                                                     \
            QNTAB(ptr) = NULL;                                                              \
            QNCOUNT(ptr) = 0;                                                               \
            QPOS(ptr) = 0;                                                                  \
        }                                                                                   \
    }                                                                                       \
}

#define QUEUE_PUSH(ptr, type, dptr)                                                         \
{                                                                                           \
    if(ptr && dptr)                                                                         \
    {                                                                                       \
        QUEUE_RESIZE(ptr, type);                                                            \
        if(QTAB(ptr) && QLEFT(ptr) > 0)                                                     \
        {                                                                                   \
            if(QTAIL(ptr) == QCOUNT(ptr)) QTAIL(ptr) = 0;                                   \
            memcpy(&(QTI(ptr, type, QTAIL(ptr))), dptr, sizeof(type));                      \
            QTAIL(ptr)++;QLEFT(ptr)--;QTOTAL(ptr)++;                                        \
        }                                                                                   \
    }                                                                                       \
}

#define QUEUE_POP(ptr, type, dptr) ((ptr && QLEFT(ptr) < QCOUNT(ptr)                        \
        && (QHEAD(ptr) = ((QHEAD(ptr) == QCOUNT(ptr))? 0 : QHEAD(ptr))) >= 0                \
        && memcpy(dptr, &(QTI(ptr, type, QHEAD(ptr))), sizeof(type))                        \
        && QHEAD(ptr)++ >= 0 && QLEFT(ptr)++ >= 0 && QTOTAL(ptr)--  >= 0)? 0: -1)           

#define QUEUE_RESET(ptr)                                                                    \
{                                                                                           \
    if(ptr)                                                                                 \
    {                                                                                       \
        if(QTAB(ptr)) free(QTAB(ptr));                                                      \
        memset(ptr, 0, sizeof(QUEUE));                                                      \
    }                                                                                       \
}
#define QUEUE_CLEAN(ptr)                                                                    \
{                                                                                           \
    if(ptr)                                                                                 \
    {                                                                                       \
        if(QTAB(ptr)) free(QTAB(ptr));                                                      \
        free(ptr);                                                                          \
        ptr = NULL;                                                                         \
    }                                                                                       \
}
#ifdef __cplusplus
     }
#endif
#endif
