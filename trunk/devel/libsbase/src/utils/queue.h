#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "mutex.h"
#ifndef _QUEUE_H
#define _QUEUE_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _QNODE
{
    void *ptr;
    struct _QNODE *next;
}QNODE;
typedef struct _QUEUE
{
    int nleft;
    int qtotal;
    int total;
    void *mutex;
    QNODE *left;
    QNODE *first;
    QNODE *last;
}QUEUE;
void *queue_init();
void queue_push(void *q, void *ptr);
void *queue_pop(void *q);
void *queue_head(void *q);
void queue_clean(void *q);
#define QTOTAL(q) (((QUEUE *)q)->total)
#ifdef __cplusplus
     }
#endif
#endif
