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
            return ;                                                                        \
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
        return ;                                                                            \
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
                    //CONN_TERMINATE(conn);          
                    return ;
                }
                conn->status = CONN_STATUS_CONNECTED;
            }
            if(event & E_CLOSE)
            {
                //DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d START ", E_CLOSE, event_fd);
                conn->push_message(conn, MESSAGE_QUIT);
                //DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d OVER ", E_CLOSE, event_fd);
            }
            if(event & E_READ)
            {
                //DEBUG_LOGGER(conn->logger, "E_READ:%d on %d START", E_READ, event_fd);
                //conn->read_handler(conn);
                conn->push_message(conn, MESSAGE_INPUT);
                //DEBUG_LOGGER(conn->logger, "E_READ:%d on %d OVER ", E_READ, event_fd);
            }
            if(event & E_WRITE)
            {
                conn->push_message(conn, MESSAGE_OUTPUT);
                // DEBUG_LOGGER(conn->logger, "E_WRITE:%d on %d START", E_WRITE, event_fd);
                // conn->write_handler(conn);
                // DEBUG_LOGGER(conn->logger, "E_WRITE:%d on %d OVER", E_WRITE, event_fd);
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
            //TIMER_SAMPLE(conn->timer);
            return ;
        }

    }
}

/* write handler */
int conn_write_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
    }
    return ret;
}

/* packet reader */
int conn_packet_reader(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
    }
    return ret;
}

/* packet handler */
int conn_packet_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
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
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
    }
    return ret;
}

/* receive chunk */
int conn_recv_chunk(CONN *conn, int size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
    }
    return ret;
}

/* push chunk */
int conn_push_chunk(CONN *conn, void *chunk_data, int size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
    }
    return ret;
}

/* receive chunk file */
int conn_recv_file(CONN *conn, char *file, long long offset, long long size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
    }
    return ret;
}

/* push chunk file */
int conn_push_file(CONN *conn, char *file, long long offset, long long size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, ret);

    if(conn)
    {
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

/* clean connection */
void conn_clean(CONN **pconn)
{
    CHUNK *cp = NULL;

    if(*pconn)
    {
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
