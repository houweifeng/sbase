#include "sbase.h"
#include "conn.h"
#include "queue.h"
#include "memb.h"
#include "chunk.h"
#include "logger.h"
#include "message.h"
#include "timer.h"

#define CONN_CHECK_RET(conn, ret)                                                           \
{                                                                                           \
    if(conn == NULL ) return ret;                                                           \
    if(conn->s_state == S_STATE_CLOSE) return ret;                                          \
}
#define CONN_CHECK(conn)                                                                    \
{                                                                                           \
    if(conn == NULL) return ;                                                               \
    if(conn->s_state == S_STATE_CLOSE) return ;                                             \
}
#define CONN_TERMINATE(conn)                                                                \
{                                                                                           \
    if(conn)                                                                                \
    {                                                                                       \
        conn->s_state = S_STATE_CLOSE;                                                      \
        conn->push_message(conn, MESSAGE_QUIT);                                             \
    }                                                                                       \
}
#define CONN_CHUNK_READ(conn, n)                                                            \
{                                                                                           \
    /* read to chunk */                                                                     \
    if(conn->s_state == S_STATE_READ_CHUNK)                                                 \
    {                                                                                       \
        if((n = CHUNK_READ(conn->chunk, conn->fd)) <= 0 && errno != EAGAIN)                 \
        {                                                                                   \
            FATAL_LOGGER(conn->logger, "Reading %d bytes data from %s:%d via %d failed, %s",\
                    n, conn->ip, conn->port, conn->fd, strerror(errno));                    \
            CONN_TERMINATE(conn);                                                           \
            return -1;                                                                      \
        }                                                                                   \
        DEBUG_LOGGER(conn->logger, "Read %d bytes left:%lld to chunk  from %s:%d via %d",   \
                n, CK_LEFT(conn->chunk), conn->ip, conn->port, conn->fd);                   \
        if(CHUNK_STATUS(conn->chunk) == CHUNK_STATUS_OVER )                                 \
        {                                                                                   \
            conn->s_state = S_STATE_DATA_HANDLING;                                          \
            conn->push_message(conn, MESSAGE_DATA);                                         \
            DEBUG_LOGGER(conn->logger, "Chunk completed %lld bytes from %s:%d via %d",      \
                    CK_SIZE(conn->chunk), conn->ip, conn->port, conn->fd);                  \
        }                                                                                   \
        TIMER_SAMPLE(conn->timer);                                                          \
        return 0;                                                                           \
    }                                                                                       \
}

/* connection event handler */
void conn_event_handler(int event_fd, short event, void *arg)
{
    int error = 0;
    socklen_t len = sizeof(int);
    CONN *conn = (CONN *)arg;

    if(conn)
    {
        if(event_fd == conn->fd)
        {
            if(conn->status == CONN_STATUS_READY)
            {
                if(getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0)
                {
                    ERROR_LOGGER(conn->logger, "socket %d connecting failed, %s", strerror(errno));
                    conn->status = CONN_STATUS_CLOSED;
                    CONN_TERMINATE(conn);          
                    return ;
                }
                conn->status = CONN_STATUS_CONNECTED;
            }
            if(event & E_CLOSE)
            {
                //DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d START ", E_CLOSE, event_fd);
                CONN_TERMINATE(conn);          
                //conn->push_message(conn, MESSAGE_QUIT);
                //DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d OVER ", E_CLOSE, event_fd);
            }
            if(event & E_READ)
            {
                //DEBUG_LOGGER(conn->logger, "E_READ:%d on %d START", E_READ, event_fd);
                conn->read_handler(conn);
                //conn->push_message(conn, MESSAGE_INPUT);
                //DEBUG_LOGGER(conn->logger, "E_READ:%d on %d OVER ", E_READ, event_fd);
            }
            if(event & E_WRITE)
            {
                //conn->push_message(conn, MESSAGE_OUTPUT);
                //DEBUG_LOGGER(conn->logger, "E_WRITE:%d on %d START", E_WRITE, event_fd);
                 conn->write_handler(conn);
                //DEBUG_LOGGER(conn->logger, "E_WRITE:%d on %d OVER", E_WRITE, event_fd);
            } 
        }
    }
}

/* set connection */
int conn_set(CONN *conn)
{
    short flag = 0;
    if(conn && conn->fd > 0 )
    {
        fcntl(conn->fd, F_SETFL, O_NONBLOCK);
        if(conn->evbase && conn->event)
        {
            flag = E_READ|E_PERSIST;
            if(conn->status == CONN_STATUS_READY) flag |= E_WRITE;
            conn->event->set(conn->event, conn->fd, flag, (void *)conn, &conn_event_handler);
            conn->evbase->add(conn->evbase, conn->event);
            DEBUG_LOGGER(conn->logger, "setting connection[%s:%d] via %d", 
                    conn->ip, conn->port, conn->fd);
            return 0;
        }
        else
        {
            FATAL_LOGGER(conn->logger, "Connection[%08x] fd[%d] evbase or"
                    "initialize event failed, %s", conn, conn->fd, strerror(errno));	
            /* Terminate connection */
            CONN_TERMINATE(conn);
        }
    }	
    return -1;	
}

/* close connection */
int conn_close(CONN *conn)
{
    if(conn)
    {
        CONN_TERMINATE(conn);
        return 0;
    }
    return -1;
}


/* over connection */
int conn_over(CONN *conn)
{
    if(conn)
    {
        return conn->push_message(conn, MESSAGE_QUIT);
    }
    return -1;
}

/* terminate connection */
int conn_terminate(CONN *conn)
{
    int ret = -1;

    if(conn)
    {
        if(conn->c_state == C_STATE_USING && conn->session.error_handler)
        {
            DEBUG_LOGGER(conn->logger, "error handler session[%s:%d] via %d cid:%d", 
                    conn->ip, conn->port, conn->fd, conn->c_id);
            conn->session.error_handler(conn, PCB(conn->packet), PCB(conn->cache), PCB(conn->chunk));
        }
        conn->event->destroy(conn->event);
        DEBUG_LOGGER(conn->logger, "terminateing session[%s:%d] via %d",
                conn->ip, conn->port, conn->fd);
        shutdown(conn->fd, SHUT_RDWR);
        close(conn->fd);
        conn->fd = -1;
        ret = 0;
    }
    return ret;
}

/* set timeout */
int conn_set_timeout(CONN *conn, int timeout_usec)
{
    int ret = -1;

    if(conn && timeout_usec > 0)
    {
        conn->timeout = timeout_usec;
    }
    return ret;
}

/* start client transaction state */
int conn_start_cstate(CONN *conn)
{
    CHUNK *cp = NULL;
    int ret = -1;
    /* Check connection and transaction state */
    CONN_CHECK_RET(conn, -1);

    if(conn)
    {
        if(conn->c_state == C_STATE_FREE)
        {
            conn->c_state = C_STATE_USING;
            while(QUEUE_POP(conn->send_queue, PCHUNK, &cp) == 0){CK_CLEAN(cp);}
            MB_RESET(conn->packet);
            MB_RESET(conn->cache);
            MB_RESET(conn->buffer);
            MB_RESET(conn->oob);
            CK_RESET(conn->chunk);
            ret = 0;
        }
    }
    return ret;
}

/* over client transaction state */
int conn_over_cstate(CONN *conn)
{
    int ret = -1;

    if(conn)
    {
        conn->c_state = C_STATE_FREE;
        ret = 0;
    }
    return ret;
}

/* push message to message queue */
int conn_push_message(CONN *conn, int message_id)
{
    MESSAGE msg = {0};
    int ret = -1;

    if(conn && (message_id & MESSAGE_ALL) )
    {
        msg.msg_id = message_id;
        msg.fd = conn->fd;
        msg.handler  = conn;
        msg.parent   = conn->parent;
        QUEUE_PUSH(conn->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(conn->logger, "Pushed message[%s] to message_queue[%08x] "
                "on %s:%d via %d total %d handler[%08x] parent[%08x]",
                MESSAGE_DESC(message_id), conn->message_queue,
                conn->ip, conn->port, conn->fd, QTOTAL(conn->message_queue),
                conn, conn->parent);
        ret = 0;
    }
    return ret;
}

/* read handler */
int conn_read_handler(CONN *conn)
{
    int ret = -1, n = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
        /* Receive OOB */
        if((n = MB_RECV(conn->oob, conn->fd, MSG_OOB)) > 0)
        {
            conn->recv_oob_total += n;
            DEBUG_LOGGER(conn->logger, "Received %d bytes OOB total %lld from %s:%d via %d",
                    n, conn->recv_oob_total, conn->ip, conn->port, conn->fd);
            conn->oob_handler(conn);
            /* CONN TIMER sample */
            TIMER_SAMPLE(conn->timer);
            return (ret = 0);
        }
        /* Receive to chunk with chunk_read_state before reading to buffer */
        CONN_CHUNK_READ(conn, n);
        /* Receive normal data */
        if((n = MB_READ(conn->buffer, conn->fd)) <= 0)
        {
            FATAL_LOGGER(conn->logger, "Reading %d bytes left:%d data from %s:%d via %d failed, %s",
                    n, MB_LEFT(conn->buffer), conn->ip, conn->port, conn->fd, strerror(errno));
            /* Terminate connection */
            CONN_TERMINATE(conn);
            return (ret = 0);
        }
        /* CONN TIMER sample */
        TIMER_SAMPLE(conn->timer);
        conn->recv_data_total += n;
        DEBUG_LOGGER(conn->logger, "Received %d bytes data total %lld from %s:%d via %d",
                n, conn->recv_data_total, conn->ip, conn->port, conn->fd);
        conn->packet_reader(conn);
        ret = 0;
    }
    return ret;
}

/* write handler */
int conn_write_handler(CONN *conn)
{
    int ret = -1, n = 0;
    CONN_CHECK_RET(conn, ret);
    CHUNK *cp = NULL;

    if(conn && conn->send_queue && QTOTAL(conn->send_queue) > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for send data to %s:%d via %d "
                "qtotal:%d qhead:%d qcount:%d",
                conn->ip, conn->port, conn->fd, QTOTAL(conn->send_queue),
                QHEAD(conn->send_queue), QCOUNT(conn->send_queue));   
        if(QUEUE_HEAD(conn->send_queue, PCHUNK, &cp) == 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready for send data to %s:%d via %d qtotal:%d pcp:%08x",
                    conn->ip, conn->port, conn->fd, QTOTAL(conn->send_queue), cp);   
            if((n = CHUNK_WRITE(cp, conn->fd)) > 0)
            {
                conn->sent_data_total += n;
                DEBUG_LOGGER(conn->logger, "Sent %d byte(s) (total sent %lld) "
                        "to %s:%d via %d leave %lld", n, conn->sent_data_total, 
                        conn->ip, conn->port, conn->fd, CK_LEFT(cp));
                /* CONN TIMER sample */
                TIMER_SAMPLE(conn->timer);
                if(CHUNK_STATUS(cp) == CHUNK_STATUS_OVER )
                {
                    if(QUEUE_POP(conn->send_queue, PCHUNK, &cp) == 0)
                    {
                        DEBUG_LOGGER(conn->logger, "Completed chunk[%08x] and clean it leave %d",
                                cp, QTOTAL(conn->send_queue));
                        CK_CLEAN(cp);
                    }
                }
                ret = 0;
            }
            else
            {
                FATAL_LOGGER(conn->logger, "Sending data to %s:%d via %d failed, %s",
                        conn->ip, conn->port, conn->fd, strerror(errno));
                /* Terminate connection */
                CONN_TERMINATE(conn);
            }
        }
        if(QTOTAL(conn->send_queue) <= 0)
        {
            conn->event->del(conn->event, E_WRITE);
        }
    }
    return ret;
}

/* packet reader */
int conn_packet_reader(CONN *conn)
{
    int len = -1, i = 0;
    CB_DATA *data = NULL;
    char *p = NULL, *e = NULL;
    int packet_type = 0;

    CONN_CHECK_RET(conn, -1);

    if(conn)
    {
        data = PCB(conn->buffer);
        packet_type = conn->session.packet_type;
        DEBUG_LOGGER(conn->logger, "Reading packet type[%d]", packet_type);
        /* Remove invalid packet type */
        if(!(packet_type & PACKET_ALL))
        {
            FATAL_LOGGER(conn->logger, "Unkown packet_type[%d] on %s:%d via %d",
                    packet_type, conn->ip, conn->port, conn->fd);
            /* Terminate connection */
            CONN_TERMINATE(conn);
        }
        /* Read packet with customized function from user */
        if(packet_type == PACKET_CUSTOMIZED && conn->session.packet_reader)
        {
            len = conn->session.packet_reader(conn, data);
            DEBUG_LOGGER(conn->logger,
                    "Reading packet with customized function[%08x] length[%d] on %s:%d via %d",
                    conn->session.packet_reader, len, conn->ip, conn->port, conn->fd);
            goto end;
        }
        /* Read packet with certain length */
        else if(packet_type == PACKET_CERTAIN_LENGTH
                && MB_NDATA(conn->buffer) >= conn->session.packet_length)
        {
            len = conn->session.packet_length;
            DEBUG_LOGGER(conn->logger, "Reading packet with certain length[%d] on %s:%d via %d",
                    len, conn->ip, conn->port, conn->fd);
            goto end;
        }
        /* Read packet with delimiter */
        else if(packet_type == PACKET_DELIMITER && conn->session.packet_delimiter
                && conn->session.packet_delimiter_length > 0)
        {
            p = MB_DATA(conn->buffer);
            e = MB_END(conn->buffer);
            i = 0;
            while(p < e && i < conn->session.packet_delimiter_length )
            {
                if(((char *)conn->session.packet_delimiter)[i++] != *p++) i = 0;
                if(i == conn->session.packet_delimiter_length)
                {
                    len = p - MB_DATA(conn->buffer);
                    break;
                }
            }
            DEBUG_LOGGER(conn->logger, "Reading packet with delimiter[%d] "
                    "length[%d] on %s:%d via %d", conn->session.packet_delimiter_length, 
                    len, conn->ip, conn->port, conn->fd);
            goto end;
        }
        return len;
end:
        /* Copy data to packet from buffer */
        if(len > 0)
        {
            MB_RESET(conn->packet);
            MB_PUSH(conn->packet, MB_DATA(conn->buffer), len);
            MB_DEL(conn->buffer, len);
            /* For packet handling */
            conn->s_state = S_STATE_PACKET_HANDLING;
            conn->push_message(conn, MESSAGE_PACKET);
        }
    }
    return len;
}

/* packet handler */
int conn_packet_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn && conn->session.packet_handler)
    {
        ret = conn->session.packet_handler(conn, PCB(conn->packet));
    }
    return ret;
}

/* oob data handler */
int conn_oob_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
        if(conn && conn->session.oob_handler)
        {
            ret = conn->session.oob_handler(conn, PCB(conn->oob));
        }

    }
    return ret;
}

/* chunk data  handler */
int conn_data_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
        if(conn && conn->session.data_handler)
        {
            ret = conn->session.data_handler(conn, PCB(conn->packet), 
                    PCB(conn->cache), PCB(conn->chunk));
        }
    }
    return ret;
}

/* save cache to connection  */
int conn_save_cache(CONN *conn, void *data, int size)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
        MB_PUSH(conn->cache, data, size);
        ret = 0;
    }
    return ret;
}

/* chunk reader */
int conn_chunk_reader(CONN *conn)
{
    int ret = -1, n = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn && conn->chunk) 
    {
        if(MB_NDATA(conn->buffer) > 0)
        {
            if((n = CHUNK_FILL(conn->chunk, MB_DATA(conn->buffer), MB_NDATA(conn->buffer))) > 0)
            {
                MB_DEL(conn->buffer, n);
            }
            if(CHUNK_STATUS(conn->chunk) == CHUNK_STATUS_OVER )
            {
                conn->s_state = S_STATE_DATA_HANDLING;
                conn->push_message(conn, MESSAGE_DATA);
                DEBUG_LOGGER(conn->logger, "Chunk completed %d bytes from %s:%d via %d",
                        CK_SIZE(conn->chunk), conn->ip, conn->port, conn->fd);
            }
            if(n > 0)
            {
                DEBUG_LOGGER(conn->logger, "Filled  %d byte(s) left:%lld to chunk from buffer "
                        "on conn[%s:%d] via %d", n, CK_LEFT(conn->chunk),
                        conn->ip, conn->port, conn->fd);
                ret = 0;
            }
        }
    }
    return ret;
}

/* receive chunk */
int conn_recv_chunk(CONN *conn, int size)
{
    int ret = -1, n = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn && conn->chunk)
    {
        DEBUG_LOGGER(conn->logger, "Ready for chunk size:%ld on %s:%d via %d",
                size, conn->ip, conn->port, conn->fd);
        CK_MEM(conn->chunk, size);
        conn->s_state = S_STATE_READ_CHUNK;
        conn->chunk_reader(conn);
        CONN_CHUNK_READ(conn, n);
        ret = 0;
    }
    return ret;
}

/* push chunk */
int conn_push_chunk(CONN *conn, void *data, int size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn && conn->send_queue && data && size > 0)
    {
        CK_INIT(cp);
        if(cp)
        {
            CK_MEM(cp, size);
            CK_MEM_COPY(cp, data, size);
            QUEUE_PUSH(conn->send_queue, PCHUNK, &cp);
        }
        if(QTOTAL(conn->send_queue) > 0 ) conn->event->add(conn->event, E_WRITE);
        DEBUG_LOGGER(conn->logger, "Pushed chunk size[%d] to send_queue "
                    "total %d on %s:%d via %d ", size, QTOTAL(conn->send_queue), 
                    conn->ip, conn->port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* receive chunk file */
int conn_recv_file(CONN *conn, char *filename, long long offset, long long size)
{
    int ret = -1, n = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn && conn->chunk && filename && offset >= 0 && size > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for chunk file:%s offset:%lld size:%lld on %s:%d via %d",
                filename, offset, size, conn->ip, conn->port, conn->fd);
        CK_FILE(conn->chunk, filename, offset, size);
        CK_SET_BSIZE(conn->chunk, conn->session.buffer_size);
        conn->s_state = S_STATE_READ_CHUNK;
        conn->chunk_reader(conn);
        CONN_CHUNK_READ(conn, n);
        ret = 0;
    }
    return ret;
}

/* push chunk file */
int conn_push_file(CONN *conn, char *filename, long long offset, long long size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn && conn->send_queue && filename && offset >= 0 && size > 0)
    {
        CK_INIT(cp);
        if(cp)
        {
            CK_FILE(cp, filename, offset, size);
            QUEUE_PUSH(conn->send_queue, PCHUNK, &cp);
            if((QTOTAL(conn->send_queue)) > 0 ) conn->event->add(conn->event, E_WRITE);
            DEBUG_LOGGER(conn->logger, "Pushed file[%s] [%lld][%lld] to "
                    "send_queue total %d on %s:%d via %d ", filename, offset, 
                    size, QTOTAL(conn->send_queue), conn->ip, conn->port, conn->fd);
            ret = 0;
        }
    }
    return ret;
}

/* set session options */
int conn_set_session(CONN *conn, SESSION *session)
{
    int ret = -1;

    if(conn && session)
    {
        memcpy(&(conn->session), session, sizeof(SESSION));
        ret = 0;
    }
    return ret;
}

/* transaction handler */
int conn_transaction_handler(CONN *conn, int tid)
{

    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
        if(conn && conn->session.transaction_handler)
        {
            ret = conn->session.transaction_handler(conn, tid);
        }
    }
    return ret;
}

/* clean connection */
void conn_clean(CONN **pconn)
{
    CHUNK *cp = NULL;

    if(*pconn)
    {
        if((*pconn)->timer) {TIMER_CLEAN((*pconn)->timer);}
        if((*pconn)->event) (*pconn)->event->clean(&((*pconn)->event));
        /* Clean BUFFER */
        MB_CLEAN((*pconn)->buffer);
        /* Clean OOB */
        MB_CLEAN((*pconn)->oob);
        /* Clean cache */
        MB_CLEAN((*pconn)->cache);
        /* Clean packet */
        MB_CLEAN((*pconn)->packet);
        /* Clean chunk */
        CK_CLEAN((*pconn)->chunk);
        /* Clean send queue */
        if((*pconn)->send_queue)
        {
            while(QUEUE_POP((*pconn)->send_queue, PCHUNK, &cp) == 0){CK_CLEAN(cp);}
            QUEUE_CLEAN((*pconn)->send_queue);
        }
        free(*pconn);
        (*pconn) = NULL;
    }
}

/* Initialize connection */
CONN *conn_init(int fd, char *ip, int port)
{
    CONN *conn = NULL;

    if(fd > 0 && ip && port > 0 && (conn = calloc(1, sizeof(CONN))))
    {
        conn->fd = fd;
        strcpy(conn->ip, ip);
        conn->port = port;
        TIMER_INIT(conn->timer);
        MB_INIT(conn->buffer, MB_BLOCK_SIZE);
        MB_INIT(conn->packet, MB_BLOCK_SIZE);
        MB_INIT(conn->cache, MB_BLOCK_SIZE);
        MB_INIT(conn->oob, MB_BLOCK_SIZE);
        CK_INIT(conn->chunk);
        conn->send_queue            = QUEUE_INIT();
        conn->event                 = ev_init();
        conn->set                   = conn_set;
        conn->close                 = conn_close;
        conn->over                  = conn_over;
        conn->terminate             = conn_terminate;
        conn->start_cstate          = conn_start_cstate;
        conn->over_cstate           = conn_over_cstate;
        conn->set_timeout           = conn_set_timeout;
        conn->set_session           = conn_set_session;
        conn->push_message          = conn_push_message;
        conn->read_handler          = conn_read_handler;
        conn->write_handler         = conn_write_handler;
        conn->packet_reader         = conn_packet_reader;
        conn->packet_handler        = conn_packet_handler;
        conn->oob_handler           = conn_oob_handler;
        conn->data_handler          = conn_data_handler;
        conn->transaction_handler   = conn_transaction_handler;
        conn->save_cache            = conn_save_cache;
        conn->chunk_reader          = conn_chunk_reader;
        conn->recv_chunk            = conn_recv_chunk;
        conn->push_chunk            = conn_push_chunk;
        conn->recv_file             = conn_recv_file;
        conn->push_file             = conn_push_file;
        conn->set_session           = conn_set_session;
        conn->clean                 = conn_clean;
    }
    return conn;
}
