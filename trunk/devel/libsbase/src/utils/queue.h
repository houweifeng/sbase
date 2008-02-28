#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifndef _QUEUE_H
#define _QUEUE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TYPEDEF_QUEUE
#define _TYPEDEF_QUEUE
typedef struct _QUEUE_ELEM
{
	void *data;
	struct _QUEUE_ELEM *next;
}QUEUE_ELEM;

typedef struct _QUEUE
{
	QUEUE_ELEM *first;
	QUEUE_ELEM *last;	
	size_t total;
	void *mutex;

	void	(*push)(struct _QUEUE *, void *);		
	void* 	(*pop)(struct _QUEUE *);
	void 	(*del)(struct _QUEUE *);
	void*	(*head)(struct _QUEUE *);
	void*	(*tail)(struct _QUEUE *);
	void	(*clean)(struct _QUEUE **);
	
}QUEUE;
/* Initialize QUEUE */
QUEUE *queue_init();
#endif

/* Push QUEUE_ELEM to QUEUE tail */
void queue_push(QUEUE *, void *);
/* Pop QUEUE_ELEM data from QUEUE head and free it */
void *queue_pop(QUEUE *);
/* Delete QUEUE_ELEM data from QUEUE head and free it */
void queue_del(QUEUE *);
/* Get QUEUE head data don't free */
void *queue_head(QUEUE *);
/* Get QUEUE tail data don't free */
void *queue_tail(QUEUE *);
/* Clean QUEUE */
void queue_clean(QUEUE **);

#define QUEUE_VIEW(queue) \
{ \
	if(queue) \
	{ \
		fprintf(stdout, "queue:%08X\n" \
			"queue->first:%08X\n" \
			"queue->last:%08X\n" \
			"queue->total:%u\n" \
			"queue->push():%08X\n" \
			"queue->pop():%08X\n" \
			"queue->head():%08X\n" \
			"queue->tail()::%08X\n" \
			"queue->clean():%08X\n", \
			queue, queue->first, queue->last, \
			queue->total, queue->push, queue->pop, \
			queue->head, queue->tail, queue->clean \
		);	 \
	}	 \
} 

#define POP_QUEUE(pq) ((QUEUE *)pq)->pop((QUEUE *)pq)
#define PUSH_QUEUE(pq, data) ((QUEUE *)pq)->push((QUEUE *)pq, data)
#define TAIL_QUEUE(pq) ((QUEUE *)pq)->tail((QUEUE *)pq)
#define HEAD_QUEUE(pq) ((QUEUE *)pq)->head((QUEUE *)pq)
#define TOTAL_QUEUE(pq) ((QUEUE *)pq)->total
#define CLEAN_QUEUE(pq) ((QUEUE *)pq)->clean((QUEUE **)&(pq))
#ifdef __cplusplus
 }
#endif
#endif
