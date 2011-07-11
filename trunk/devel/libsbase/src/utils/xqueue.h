#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "mutex.h"
#ifndef _XQUEUE_H
#define _XQUEUE_H
#ifdef __cplusplus
extern "C" {
#endif
#define XQ_ROOTS_MAX  65536
typedef struct _XQNODE
{
    void *ptr;
    struct _XQNODE *next;
}XQNODE;
typedef struct _XQROOT
{
    int total;
    int status;
    XQNODE *first;
    XQNODE *last;
}XQROOT;
typedef struct _XQUEUE
{
    int nleft;
    int qtotal;
    int nwaits;
    int total;
    MUTEX mutex;
    XQNODE *left;
    int waits[XQ_ROOTS_MAX];
    XQROOT roots[XQ_ROOTS_MAX];
}XQUEUE;
void *xqueue_init();
int xqueue_new(void *q);
int xqueue_total(void *q, int qid);
void xqueue_push(void *q, int qid, void *ptr);
void *xqueue_pop(void *q, int qid);
void xqueue_close(void *q, int qid);
void *xqueue_head(void *q, int qid);
void xqueue_clean(void *q);
#ifdef __cplusplus
     }
#endif
#endif
