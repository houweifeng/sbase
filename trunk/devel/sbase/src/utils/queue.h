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
#ifdef HAVE_PTHREAD
	pthread_mutex_t mutex;
#endif
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

#define QUEUE_INIT() \
{ \
	QUEUE *queue = (QUEUE *)calloc(1, sizeof(QUEUE)); \
	if(queue) \
	{ \
		queue->push	= queue_push; \
		queue->pop		= queue_pop; \
		queue->head	= queue_head; \
		queue->tail	= queue_tail; \
		queue->clean	= queue_clean; \
		 \
	} \
	return(queue); \
} 
#define QUEUE_PUSH(queue, data) \
{ \
	if(queue) \
	{ \
		if(queue->last) \
		{ \
			queue->last->next = (QUEUE_ELEM *)calloc(1, sizeof(QUEUE_ELEM)); \
			queue->last = queue->last->next; \
		}	 \
		else \
		{ \
			queue->last = (QUEUE_ELEM *)calloc(1, sizeof(QUEUE_ELEM)); \
			if(queue->first == NULL) queue->first = queue->last; \
		} \
		queue->last->data = data; \
		queue->total++; \
	}	 \
} 
#define QUEUE_POP(queue) \
{ \
	void *p = NULL; \
	QUEUE_ELEM *elem = NULL; \
	if(queue) \
	{ \
		if(queue->first) \
		{ \
			elem = queue->fist; \
			if(queue->fist == queue->last)  \
			{ \
				queue->fist = queue->last = NULL; \
			} \
			else \
			{ \
				queue->first = queue->first->next; \
			} \
			p = elem->data; \
			free(elem); \
		} \
	} \
	return(p);\
} 
#define QUEUE_HEAD(queue) \
{ \
	void *p = NULL; \
	if(queue && queue->fist) \
	{ \
		 p = queue->fist->data; \
	} \
	return(p); \
} 
#define QUEUE_TAIL(queue) \
{ \
        void *p = NULL; \
        if(queue && queue->last) \
        { \
                 p = queue->last->data; \
        } \
        return(p); \
} 
#define QUEUE_CLEAN(queue) \
{ \
	QUEUE_ELEM *elem = NULL, p = NULL; \
	if(queue) \
	{ \
		p = elem = queue->first; \
		while(elem != queue_last->last) \
		{ \
			elem = elem->next; \
			free(p);	 \
			p = elem; \
		}					 \
		free(queue); \
	} \
}

#ifdef __cplusplus
 }
#endif
#endif
