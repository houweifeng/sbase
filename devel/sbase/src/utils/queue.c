#include "queue.h"
/* Initialize QUEUE */
QUEUE *queue_init()
{
	QUEUE *queue = (QUEUE *)calloc(1, sizeof(QUEUE));
	if(queue)
	{
		queue->push	= queue_push;
		queue->pop		= queue_pop;
		queue->del		= queue_del;
		queue->head	= queue_head;
		queue->tail	= queue_tail;
		queue->clean	= queue_clean;
#ifdef HAVE_PTHREAD
        	pthread_mutex_init(&(queue->mutex), NULL);
#endif
		
	}
	return queue;
}
/* Push QUEUE_ELEM to QUEUE tail */
void queue_push(QUEUE *queue, void *data)
{
	if(queue)
	{
#ifdef HAVE_PTHREAD
                pthread_mutex_lock(&(queue->mutex));
#endif
		if(queue->last)
		{
			queue->last->next = (QUEUE_ELEM *)calloc(1, sizeof(QUEUE_ELEM));
			queue->last = queue->last->next;
		}	
		else
		{
			queue->last = (QUEUE_ELEM *)calloc(1, sizeof(QUEUE_ELEM));
			if(queue->first == NULL) queue->first = queue->last;
		}
		queue->last->data = data;
		queue->total++;
#ifdef HAVE_PTHREAD
                pthread_mutex_unlock(&(queue->mutex));
#endif
	}	
}
/* Pop QUEUE_ELEM data from QUEUE head and free it */
void *queue_pop(QUEUE *queue)
{
	void *p = NULL;
	QUEUE_ELEM *elem = NULL;
	if(queue)
	{
#ifdef HAVE_PTHREAD
                pthread_mutex_lock(&(queue->mutex));
#endif
		if(queue->total > 0 && (elem = queue->first) )
		{
			p = elem->data;
			if(queue->first == queue->last)
			{
				queue->first = queue->last = NULL;
			}
			else
			{
				queue->first = queue->first->next;
			}
			free(elem);
			queue->total--;
		}
#ifdef HAVE_PTHREAD
                pthread_mutex_unlock(&(queue->mutex));
#endif
	}
	return p;
}

/* Delete QUEUE_ELEM data from QUEUE head and free it */
void queue_del(QUEUE *queue)
{
        QUEUE_ELEM *elem = NULL; 
        if(queue)
        {
#ifdef HAVE_PTHREAD
                pthread_mutex_lock(&(queue->mutex));
#endif
		if(queue->total > 0 && (elem = queue->first) )
		{
			elem = queue->first;
			if(queue->first == queue->last)
			{
				queue->first = queue->last = NULL;
			}
			else
			{
				queue->first = queue->first->next;
			}
			free(elem);
			queue->total--;
		}
#ifdef HAVE_PTHREAD
                pthread_mutex_unlock(&(queue->mutex));
#endif
        }
        return ;
}

/* Get QUEUE head data don't free */
void *queue_head(QUEUE *queue)
{
	void *p = NULL;
	if(queue && queue->first)
	{
		 p = queue->first->data;
	}
	return p;
}

/* Get QUEUE tail data don't free */
void *queue_tail(QUEUE *queue)
{
        void *p = NULL;
        if(queue && queue->last)
        {
                 p = queue->last->data;
        }
        return p;
}

/* Clean QUEUE */
void queue_clean(QUEUE **queue)
{
	QUEUE_ELEM *elem = NULL, *p = NULL;
	if(queue)
	{
#ifdef HAVE_PTHREAD
		pthread_mutex_lock(&((*queue)->mutex));
#endif
		p = elem = (*queue)->first;
		while((*queue)->total > 0 && elem)
		{
			elem = elem->next;
			free(p);	
			p = elem;
			(*queue)->total--;
		}					
#ifdef HAVE_PTHREAD
		pthread_mutex_unlock(&((*queue)->mutex));
		pthread_mutex_destroy(&((*queue)->mutex));
#endif
		free((*queue));
		(*queue) = NULL;
	}
}
#ifdef _DEBUG_QUEUE
int main()
{
	void *s = NULL;
	int size = 1024;
	char buf[size][8];
	int n = 0;
	int i = 0, j = 0 ;
	QUEUE *queue = queue_init();
	if(queue)
	{
		for(i = 0; i < size; i++)
		{
			n = sprintf(buf[i], "%d", i);			
			queue->push(queue, buf[i]);
			if((i % 5) == 0 )
			{
				fprintf(stdout, "id:%d => data:%s\n",
					 j++, (char *)queue->pop(queue));
				fprintf(stdout, "head:%s => tail:%s\n",
					(char *) (queue->head(queue)),
					(char *) (queue->tail(queue)) );
				QUEUE_VIEW(queue);
			}
		}	
		QUEUE_VIEW(queue);
		queue->clean(&queue);			
	}
}
#endif
