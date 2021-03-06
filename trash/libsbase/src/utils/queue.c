#include <stdio.h>
#include "xmm.h"
#include "mutex.h"
#include "queue.h"

void *queue_init()
{
    QUEUE *q = NULL;

    if((q = (QUEUE *)calloc(1, sizeof(QUEUE)))) 
    {
        MUTEX_RESET(q->mutex);
    }
    return q;
}

void queue_push(void *queue, void *ptr)
{
    QNODE *node = NULL, *tmp = NULL, *nodes = NULL;
    QUEUE *q = (QUEUE *)queue;
    int k = 0, j = 0;

    if(q)
    {
        MUTEX_LOCK(q->mutex);
        if((node = q->left))
        {
            q->left = node->next;
            q->nleft--;
        }
        else 
        {
            if((k = q->nlist) < QNODE_LINE_MAX 
                    && (nodes = (QNODE *)xmm_mnew(QNODE_LINE_NUM * sizeof(QNODE))))
            {
                q->list[k] = nodes;
                q->nlist++;
                j = 1;
                while(j  < QNODE_LINE_NUM)
                {
                    tmp = &(nodes[j]);
                    tmp->next = q->left;
                    q->left = tmp;
                    q->nleft++;
                    ++j;
                }
                q->qtotal += QNODE_LINE_NUM;
                node = nodes;
            }
        }
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
            if((q->first = q->first->next) == NULL)
            {
                q->last = NULL;
            }
            node->next = q->left;
            q->left = node;
            q->nleft++;
            --(q->total);
        }
        MUTEX_UNLOCK(q->mutex);
    } 
    return ptr;
}

void queue_clean(void *queue)
{
    QUEUE *q = (QUEUE *)queue;
    int i = 0;

    if(q)
    {
        //fprintf(stdout, "%s::%d q:%p nleft:%d qtotal:%d qleft:%p\n", __FILE__, __LINE__, q, q->nleft, q->qtotal, q->left);
        for(i = 0; i < q->nlist; i++);
        {
            xmm_free(q->list[i], QNODE_LINE_NUM * sizeof(QNODE));
        }
        MUTEX_DESTROY(q->mutex);
        free(q);
        //xmm_free(q, sizeof(QUEUE));
    }
    return ;
}
