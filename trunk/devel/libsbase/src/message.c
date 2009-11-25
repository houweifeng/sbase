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
    int fd = -1, index = 0;

    if(message_queue && QTOTAL(message_queue) > 0 
            && QUEUE_POP(message_queue, MESSAGE, &msg) == 0)
    {
        if((msg.msg_id & MESSAGE_ALL) != msg.msg_id) 
        {
            FATAL_LOGGER(logger, "Invalid message[%04x:%04x:%04x] handler[%p] parent[%p] fd[%d]",
                    msg.msg_id, MESSAGE_ALL, (msg.msg_id & MESSAGE_ALL), msg.handler, 
                    msg.parent, msg.fd);
            goto next;
        }
        conn = (CONN *)(msg.handler);
        pth = (PROCTHREAD *)(msg.parent);
        index = msg.index;
        if(msg.msg_id == MESSAGE_STOP && pth)
        {
            return pth->terminate(pth);
        }
        //task and heartbeat
        if(msg.msg_id == MESSAGE_TASK || msg.msg_id == MESSAGE_HEARTBEAT 
                || msg.msg_id == MESSAGE_STATE)
        {
            if(msg.handler)
            {
                ((CALLBACK *)(msg.handler))(msg.arg);
            }
            goto next;
        }
        if(conn) fd = conn->fd;
        if(conn == NULL || pth == NULL || msg.fd != conn->fd || pth->service == NULL)
        {
            ERROR_LOGGER(logger, "Invalid MESSAGE[%d] fd[%d] conn->fd[%d] handler[%p] "
                    "parent[%p] service[%p]", msg.msg_id, msg.fd, fd, conn, pth, pth->service);
            goto next;
        }
        if(index >= 0 && pth->service->connections[index] != conn) goto next;
        DEBUG_LOGGER(logger, "Got message[%s] On service[%s] procthread[%p] "
                "connection[%s:%d] local[%s:%d] via %d", MESSAGE_DESC(msg.msg_id), 
                pth->service->service_name, pth, conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
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
                conn->packet_reader(conn);
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
                conn->transaction_handler(conn, msg.tid);
                break;
            case MESSAGE_TIMEOUT :
                conn->timeout_handler(conn);
                break;
            case MESSAGE_PROXY :
                conn->proxy_handler(conn);
                break;
        }
next: 
        usleep(1);
    }
    return ;
}
