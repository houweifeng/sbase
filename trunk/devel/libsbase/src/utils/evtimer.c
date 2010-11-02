#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "xmm.h"
#include "mutex.h"
/* add event timer */
int evtimer_add(EVTIMER *evtimer, off_t timeout, EVTCALLBACK *handler, void *arg)
{
    EVTNODE *node = NULL, tmp = NULL;
    int evid = -1, x = 0, i = 0;
    struct timeval tv = {0};

    if(evtimer && handler && timeout > 0)
    {
        MUTEX_LOCK(evtimer->mutex);
        if((node = evtimer->left))
        {
            evtimer->left = node->next; 
        }
        else
        {
            if((x = evtimer->nevlist) < EVNODE_LINE_MAX 
                    && (node = (EVTNODE *)xmm_new(sizeof(EVTNODE) * EVTNODE_LINE_NUM)))
            {
                evtimer->evlist[x] = node;
                evtimer->nevlist++;
                x *= EVTNODE_LINE_NUM;
                node[0].id = x;
                i = 1;
                while(i < EVTNODE_LINE_NUM)
                {
                    node[i].id = i + x;
                    node[i].next = evtimer->left;
                    evtimer->left = &(node[i]);
                    ++i;
                }
            }
        }
        if(node)
        {
            evid = node->id;
            node->handler = handler;
            node->arg = arg;
            node->ison = 1;
            gettimeofday(&tv, NULL);
            node->evusec = (off_t)(tv.tv_sec * 1000000ll + tv.tv_usec * 1ll) + timeout;
            if(evtimer->tail  == NULL || evtimer->head == NULL)
                evtimer->head = evtimer->tail = node;
            else
            {
                if(node->evusec >= evtimer->tail->evusec)
                {
                    node->prev = evtimer->tail;
                    evtimer->tail->next = node;
                    evtimer->tail = node;
                }
                else if(node->evusec < evtimer->head->evusec)
                {
                    node->next = evtimer->head;
                    evtimer->head->prev = node;
                    evtimer->head = node;
                }
                else
                {
                    tmp = evtimer->tail->prev;
                    while(tmp && tmp->evusec > node->evusec)
                        tmp = tmp->prev;
                    node->prev = tmp;
                    node->next = tmp->next;
                    tmp->next->prev = node;
                    tmp->next = node;
                }
            }
        }
        MUTEX_UNLOCK(evtimer->mutex);
    }
    return evid;
}

/* update event timer */
int evtimer_update(EVTIMER *evtimer, int evid, off_t timeout, EVTCALLBACK *handler, void *arg)
{
    EVTNODE *nodes = NULL, *node = NULL, *tmp = NULL;
    int x = 0, i = 0, ret = -1;
    struct timeval tv = {0};

    if(evtimer && evid >= 0 && evid < ((evtimer->nevlist+1) * EVTNODE_LINE_NUM))
    {
        MUTEX_LOCK(evtimer->mutex);
        x = evid / EVTNODE_LINE_NUM;
        i = evid % EVTNODE_LINE_NUM;
        if((nodes = evtimer->evlist[x]) && (node = nodes[i]) && node->ison)
        {
            node->handler = handler;
            node->arg = arg;
            gettimeofday(&tv, NULL);
            node->evusec = (off_t)(tv.tv_sec * 1000000ll + tv.tv_usec * 1ll) + timeout;
            if((tmp = node->next) && node->evusec > tmp->evusec)
            {
                tmp->prev = node->prev;
                if(node->prev == NULL)
                {
                    evtimer->head = tmp;
                }
                else
                {
                    node->prev->next = tmp;
                }
                do
                {
                    tmp = tmp->next;
                }while(tmp && tmp->evusec < node->evusec);
                if(tmp == NULL)
                {
                    evtimer->tail->next = node;
                    evtimer->tail = node;
                }
                else
                {
                    node->prev = tmp->prev;
                    node->next = tmp;
                    tmp->prev->next = tmp;
                    tmp->prev = node;
                }
            }
            ret = 0;
        }
        MUTEX_UNLOCK(evtimer->mutex);
    }
    return ret;
}

/* delete event timer */
int evtimer_delete(EVTIMER *evtimer, int evid)
{
    EVTNODE *nodes = NULL, *node = NULL, *tmp = NULL;
    int ret = -1, i = 0, x = 0;

    if(evtimer && evid >= 0 && evid < ((evtimer->nevlist+1) * EVTNODE_LINE_NUM))
    {
        MUTEX_LOCK(evtimer->mutex);
        x = evid / EVTNODE_LINE_NUM;
        i = evid % EVTNODE_LINE_NUM;
        if((nodes = evtimer->evlist[x]) && (node = nodes[i]) && node->ison)
        {
            if(node->prev == NULL) evtimer->head = node->next;
            else node->prev->next = node->next;
            if(node->next == NULL) evtimer->tail = node->prev;
            else node->next->prev = node->prev;
            memset(node, 0, sizeof(EVTNODE));
            node->id = evid;
        }
        MUTEX_UNLOCK(evtimer->mutex);
    }
    return ret;
}

/* check timeout */
void evtimer_check(EVTIMER *evtimer)
{
    off_t now = 0;
}

/* reset evtimer */
void evtimer_reset(EVTIMER *evtimer)
{
    EVTNODE *tmp = NULL;
    off_t time = 0;

    if(evtimer && evtimer->tree)
    {
        if(PQS(evtimer->tree)->count > 0)
        {
            do
            {
                tmp = NULL;
                QSMAP_POP_MAX(evtimer->tree, time, tmp);
                if(tmp) 
                {
                    tmp->next = evtimer->left;
                    evtimer->left = tmp;
                }
            }while(QS_ROOT(evtimer->tree));
        }
    }
    return ;
}

/* clean evtimer */
void evtimer_clean(EVTIMER *evtimer)
{
    int i = 0;

    if(evtimer)
    {
        MUTEX_DESTROY(evtimer->mutex);
        for(i = 0; i < evtimer->nevlist; i++)
        {
            xmm_free(evtimer->evlist[i], sizeof(EVTNODE) * EVTNODE_LINE_NUM);
        }
        xmm_free(evtimer, sizeof(EVTIMER));
    }

    return ;
}

/* intialize evtimer */
EVTIMER *evtimer_init()
{
    EVTIMER *evtimer = NULL;

    if((evtimer = (EVTIMER *)xmm_new(sizeof(EVTIMER))))
    {
        MUTEX_INIT(evtimer->mutex);
    }
    return evtimer;
}

#ifdef _DEBUG_EVTIMER
int main()
{

}
#endif
