#include "message.h"
#include "sbase.h"
#include "logger.h"
#include "mutex.h"
#include "xmm.h"
int get_msg_no(int message_id)
{
    int msg_no = 0;
    int n = message_id;
    while((n /= 2))++msg_no;
    return msg_no;
}

/* initialize */
void *qmessage_init()
{
    QMESSAGE *q = NULL;

    if((q = (QMESSAGE *)xmm_mnew(sizeof(QMESSAGE))))
    {
        MUTEX_RESET(q->mutex);
    }
    return q;
}

/* qmessage */
void qmessage_push(void *qmsg, int id, int index, int fd, int tid, 
        void *parent, void *handler, void *arg)
{
    MESSAGE *msg = NULL, *tmp = NULL;
    QMESSAGE *q = (QMESSAGE *)qmsg;
    int i = 0;

    if(q)
    {
        MUTEX_LOCK(q->mutex);
        if((msg = q->left))
        {
            q->left = msg->next;
            q->nleft--;
        }
        else
        {
            if((i = q->nlist) < QMSG_LINE_MAX)
            {
                if((msg = (MESSAGE *)xmm_mnew(QMSG_LINE_NUM * sizeof(MESSAGE))))
                {
                    q->list[i] = msg;
                    q->nlist++;
                    i = 1;
                    while(i < QMSG_LINE_NUM)
                    {
                        tmp = &(msg[i]);
                        tmp->next = q->left;
                        q->left = tmp;
                        ++i;
                        q->nleft++;
                    }
                    q->qtotal += QMSG_LINE_NUM;
                }
            }
        }
        MUTEX_UNLOCK(q->mutex);
        if(msg)
        {
            msg->msg_id = id;
            msg->index = index;
            msg->fd = fd;
            msg->tid = tid;
            msg->handler = handler;
            msg->parent = parent;
            msg->arg = arg;
            msg->next = NULL;
            MUTEX_LOCK(q->mutex);
            if(q->last)
            {
                q->last->next = msg;
                q->last = msg;
            }
            else
            {
                q->first = q->last = msg;
            }
            ++(q->total);
            MUTEX_UNLOCK(q->mutex);
        }
    }
    return ;
}

MESSAGE *qmessage_pop(void *qmsg)
{
    QMESSAGE *q = (QMESSAGE *)qmsg;
    MESSAGE *msg = NULL;

    if(q)
    {
        MUTEX_LOCK(q->mutex);
        if((msg = q->first))
        {
            if((q->first = q->first->next) == NULL)
            {
                q->last = NULL;
            }
            --(q->total);
            msg->next = NULL;
        }
        MUTEX_UNLOCK(q->mutex);
    }
    return msg;
}

/* clean qmessage */
void qmessage_clean(void *qmsg)
{
    QMESSAGE *q = (QMESSAGE *)qmsg;
    int i = 0;

    if(q)
    {
        for(i = 0; i < q->nlist; i++)
        {
            xmm_free(q->list[i], QMSG_LINE_NUM * sizeof(MESSAGE));
        }
        MUTEX_DESTROY(q->mutex);
        xmm_free(q, sizeof(QMESSAGE));
    }
    return ;
}

/* to qleft */
void qmessage_left(void *qmsg, MESSAGE *msg)
{
    QMESSAGE *q = (QMESSAGE *)qmsg;

    if(q && msg)
    {
        MUTEX_LOCK(q->mutex);
        msg->next = q->left;
        q->left = msg;
        q->nleft++;
        MUTEX_UNLOCK(q->mutex);
    }
    return ;
}

void qmessage_handler(void *qmsg, void *logger)
{
    QMESSAGE *q = (QMESSAGE *)qmsg;
    int fd = -1, index = 0;
    PROCTHREAD *pth = NULL;
    MESSAGE *msg = NULL;
    CONN *conn = NULL;

    if(q)
    {
        while((msg = qmessage_pop(qmsg)))
        {
            if((msg->msg_id & MESSAGE_ALL) != msg->msg_id) 
            {
                FATAL_LOGGER(logger, "Invalid message[%04x:%04x:%04x] handler[%p] parent[%p] fd[%d]",
                        msg->msg_id, MESSAGE_ALL, (msg->msg_id & MESSAGE_ALL), msg->handler, 
                        msg->parent, msg->fd);
                goto next;
            }
            pth = (PROCTHREAD *)(msg->parent);
            if(msg->msg_id == MESSAGE_NEW_CONN)
            {
                DEBUG_LOGGER(logger, "Got message[%s] fd:%d total[%d/%d] left:%d On service[%s] procthread[%p] ", MESSAGE_DESC(msg->msg_id), msg->fd, q->total, q->qtotal, q->nleft, pth->service->service_name, pth); 
                pth->newconn(pth, msg->fd, msg->handler);
                goto next;
            }
            conn = (CONN *)(msg->handler);
            index = msg->index;
            if(msg->msg_id == MESSAGE_STOP && pth)
            {
                 pth->terminate(pth);
                 goto next;
            }
            //task and heartbeat
            if(msg->msg_id == MESSAGE_TASK || msg->msg_id == MESSAGE_HEARTBEAT 
                    || msg->msg_id == MESSAGE_STATE)
            {
                if(msg->handler)
                {
                    ((CALLBACK *)(msg->handler))(msg->arg);
                }
                goto next;
            }
            if(conn) fd = conn->fd;
            if(conn == NULL || pth == NULL || msg->fd != conn->fd || pth->service == NULL)
            {
                ERROR_LOGGER(logger, "Invalid MESSAGE[%d] msg->fd[%d] conn->fd[%d] handler[%p] "
                        "parent[%p] service[%p]", msg->msg_id, msg->fd, fd, conn, pth, pth->service);
                goto next;
            }
            if(index >= 0 && pth->service->connections[index] != conn) goto next;
            DEBUG_LOGGER(logger, "Got message[%s] total[%d/%d] left:%d On service[%s] procthread[%p] "
                    "connection[%p][%s:%d] d_state:%d local[%s:%d] via %d", 
                    MESSAGE_DESC(msg->msg_id), q->total, q->qtotal, q->nleft, pth->service->service_name, pth, 
                    conn, conn->remote_ip, conn->remote_port, conn->d_state,
                    conn->local_ip, conn->local_port, conn->fd);
            //message  on connection 
            switch(msg->msg_id)
            {
                case MESSAGE_NEW_SESSION :
                    pth->add_connection(pth, conn);
                    break;
                case MESSAGE_SHUT :
                    conn->shut_handler(conn);
                    break;
                case MESSAGE_OVER :
                    pth->over_connection(pth, conn);
                    break;
                case MESSAGE_QUIT :
                    pth->terminate_connection(pth, conn);
                    break;
                case MESSAGE_INPUT :
                    conn->read_handler(conn);
                    break;
                case MESSAGE_OUTPUT :
                    conn->write_handler(conn);
                    break;
                case MESSAGE_BUFFER:
                    conn->buffer_handler(conn);
                    break;
                case MESSAGE_PACKET :
                    conn->packet_handler(conn);
                    break;
                case MESSAGE_CHUNK :
                    conn->chunk_handler(conn);
                    break;
                case MESSAGE_DATA :
                    conn->data_handler(conn);
                    break;
                case MESSAGE_END :
                    conn->end_handler(conn);
                    break;
                case MESSAGE_TRANSACTION :
                    conn->transaction_handler(conn, msg->tid);
                    break;
                case MESSAGE_TIMEOUT :
                    conn->timeout_handler(conn);
                    break;
                case MESSAGE_PROXY :
                    conn->proxy_handler(conn);
                    break;
            }
next:
            qmessage_left(qmsg, msg);
        }
    }
    return ;
}
