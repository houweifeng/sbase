#include "message.h"
#include "queue.h"
#include "sbase.h"
#include "logger.h"

int get_msg_no(int message_id)
{
    int msg_no = 0;
    int n = message_id;
    while((n /= 2))++msg_no;
    return msg_no;
}

void message_handler(void *message_queue, void *logger)
{
    PROCTHREAD *pth = NULL;
    CONN *conn = NULL;
    MESSAGE msg = {0};

    if(message_queue && QTOTAL(message_queue) > 0 
            && QUEUE_POP(message_queue, MESSAGE, &msg) == 0)
    {
        DEBUG_LOGGER(logger, "Got message[%d] fd[%d] handler[%08x] parent[%08x]", msg.msg_id, msg.fd, msg.handler, msg.parent);
        if(!(msg.msg_id & MESSAGE_ALL)) 
        {
            FATAL_LOGGER(logger, "Invalid message[%d] handler[%08x] parent[%08x] fd[%d]",
                    msg.msg_id, msg.handler, msg.parent, msg.fd);
            goto next;
        }
        conn = (CONN *)(msg.handler);
        pth = (PROCTHREAD *)(msg.parent);
        if(msg.msg_id == MESSAGE_STOP && pth)
        {
            pth->stop(pth);
            goto next;
        }
        //task and heartbeat
        if(msg.msg_id == MESSAGE_TASK || msg.msg_id == MESSAGE_HEARTBEAT)
        {
            if(msg.handler)
            {
                ((CALLBACK)(msg.handler))(msg.arg);
            }
            goto next;
        }
        if(conn == NULL || pth == NULL || msg.fd != conn->fd || pth->service == NULL)
        {
            ERROR_LOGGER(logger, "Invalid MESSAGE[%d] fd[%d] handler[%08x] "
                    "parent[%08x]", msg.msg_id, msg.fd, conn, pth);
            goto next;
        }
        DEBUG_LOGGER(logger, "Got message[%s] On service[%s] procthread[%08x] "
                "connection[%d] %s:%d", MESSAGE_DESC(msg.msg_id), pth->service->service_name,
                pth, conn->fd, conn->ip, conn->port);
        //message  on connection 
        switch(msg.msg_id)
        {
            case MESSAGE_NEW_SESSION :
                pth->add_connection(pth, conn);
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
            case MESSAGE_PACKET :
                conn->packet_handler(conn);
                break;
            case MESSAGE_DATA :
                conn->data_handler(conn);
                break;
            case MESSAGE_TRANSACTION :
                //conn->transaction_handler(conn, msg.tid);
                break;
            case MESSAGE_STATE :
                //conn->state_handler(conn);
                break;
        }
next: 
        usleep(1);
    }
    return ;
}
