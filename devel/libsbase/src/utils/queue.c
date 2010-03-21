#include "mutex.h"
#include "queue.h"

void *queue_init()
{
    QUEUE *q = NULL;

    if((q = (QUEUE *)calloc(1, sizeof(QUEUE))))
    {
        MUTEX_INIT(q->mutex);
    }
    return q;
}

void queue_push(void *queue, void *ptr)
{
    QUEUE *q = (QUEUE *)queue;
    QNODE *node = NULL;

    if(q)
    {
        MUTEX_LOCK(q->mutex);
        if((node = q->left))
        {
            q->left = node->next;
        }
        else node = (QNODE *)calloc(1, sizeof(QNODE));
        if(node)
        {
            node->ptr = ptr;
            if(q->last)
            {
                q->last->next = node;
                q->last = node;
            }
            else
            {
                q->first = q->last = node;
            }
            node->next = NULL;
            q->total++;
        }
        MUTEX_UNLOCK(q->mutex);
    }
    return ;
}

void *queue_head(void *queue)
{
    QUEUE *q = (QUEUE *)queue;
    QNODE *node = NULL;
    void *ptr = NULL;

    if(q)
    {
        if((node = q->first))
        {
            ptr = node->ptr;
        }
    }
    return ptr;
}

void *queue_pop(void *queue)
{
    QUEUE *q = (QUEUE *)queue;
    QNODE *node = NULL;
    void *ptr = NULL;

    if(q)
    {
        MUTEX_LOCK(q->mutex);
        if((node = q->first))
        {
            ptr = node->ptr;
            if(node->next == NULL)
            {
                q->first = q->last = NULL;
            }
            else q->first = node->next;
            node->next = q->left;
            q->left = node;
            q->total--;
        }
        MUTEX_UNLOCK(q->mutex);
    } 
    return ptr;
}

void queue_clean(void *queue)
{
    QUEUE *q = (QUEUE *)queue;
    QNODE *node = NULL;

    if(q)
    {
        while((node = q->first))
        {
            q->first = node->next;
            free(node);
        }
        while((node = q->left))
        {
            q->left = node->next;
            free(node);
        }
        MUTEX_DESTROY(q->mutex);
        free(q);
    }
    return ;
}
