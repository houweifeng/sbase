#include "sbase.h"
#include "conn.h"
#include "queue.h"
#include "memb.h"
#include "chunk.h"
#include "logger.h"
#include "message.h"
#include "timer.h"
#include "evtimer.h"
#ifndef PPL
#define PPL(_x_) ((void *)(_x_))
#endif
#define CONN_CHECK_RET(conn, _state_, ret)                                                  \
{                                                                                           \
    if(conn == NULL ) return ret;                                                           \
    if(conn->d_state & (_state_)) return ret;                                               \
}
#define CONN_CHECK(conn, _state_)                                                           \
{                                                                                           \
    if(conn == NULL) return ;                                                               \
    if(conn->d_state & (_state_)) return ;                                                  \
}
#define CONN_TERMINATE(conn, _state_)                                                       \
{                                                                                           \
    if(conn)                                                                                \
    {                                                                                       \
        if(!(conn->d_state & (_state_)))                                                    \
        {                                                                                   \
        DEBUG_LOGGER(conn->logger, "Ready for closing connection[%s:%d] local[%s:%d] via %d", conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);\
            conn->push_message(conn, MESSAGE_QUIT);                                         \
            conn->d_state |= _state_;                                                       \
            if(conn->event)conn->event->del(conn->event, E_WRITE|E_READ);                   \
        }                                                                                   \
    }                                                                                       \
}
#define CONN_STATE_RESET(conn)                                                              \
{                                                                                           \
    if(conn && (conn->d_state == 0))                                                        \
    {                                                                                       \
        conn->s_state = 0;                                                                  \
    }                                                                                       \
}
#ifdef HAVE_SSL
#define CHUNK_READING(conn) ((conn->ssl)?CHUNK_READ_SSL(conn->chunk, conn->ssl)              \
    :CHUNK_READ(conn->chunk, conn->fd))
#else
#define CHUNK_READING(conn) CHUNK_READ(conn->chunk, conn->fd)
#endif
#define CONN_CHUNK_READ(conn, n)                                                            \
{                                                                                           \
    /* read to chunk */                                                                     \
    if(conn->s_state == S_STATE_READ_CHUNK)                                                 \
    {                                                                                       \
        if((n = CHUNK_READING(conn)) <= 0 && errno != EAGAIN)                               \
        {                                                                                   \
            FATAL_LOGGER(conn->logger, "Reading %d bytes data from %s:%d "                  \
                    "on %s:%d via %d failed, %s", n, conn->remote_ip, conn->remote_port,    \
                    conn->local_ip, conn->local_port, conn->fd, strerror(errno));           \
            CONN_TERMINATE(conn, D_STATE_CLOSE);                                            \
            return -1;                                                                      \
        }                                                                                   \
        DEBUG_LOGGER(conn->logger, "Read %d bytes ndata:%d left:%lld to chunk from %s:%d"   \
                " on %s:%d via %d", n, CK_NDATA(conn->chunk), CK_LEFT(conn->chunk),         \
                conn->remote_ip, conn->remote_port, conn->local_ip,                         \
                conn->local_port, conn->fd);                                                \
        if(CHUNK_STATUS(conn->chunk) == CHUNK_STATUS_OVER )                                 \
        {                                                                                   \
            conn->s_state = S_STATE_DATA_HANDLING;                                          \
            conn->push_message(conn, MESSAGE_DATA);                                         \
            DEBUG_LOGGER(conn->logger, "Chunk completed %lld bytes from %s:%d "             \
                    "on %s:%d via %d", CK_SIZE(conn->chunk), conn->remote_ip,               \
                    conn->remote_port, conn->local_ip, conn->local_port, conn->fd);         \
        }                                                                                   \
        return 0;                                                                           \
    }                                                                                       \
}
/* evtimer setting */
#define CONN_EVTIMER_SET(conn)                                                              \
do{                                                                                         \
    if(conn && conn->evtimer && conn->timeout > 0)                                          \
    {                                                                                       \
        if(conn->evid >= 0 )                                                                \
        {                                                                                   \
            EVTIMER_UPDATE(conn->evtimer, conn->evid, conn->timeout,                        \
                    &conn_evtimer_handler, (void *)conn);                                   \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            EVTIMER_ADD(conn->evtimer, conn->timeout,                                       \
                    &conn_evtimer_handler, (void *)conn, conn->evid);                       \
        }                                                                                   \
    }                                                                                       \
}while(0)
#define SESSION_RESET(conn)                                                             \
do                                                                                      \
{                                                                                       \
    CONN_STATE_RESET(conn);                                                             \
    MB_RESET(conn->packet);                                                             \
    MB_RESET(conn->cache);                                                              \
    CK_RESET(conn->chunk);                                                              \
    if(PCB(conn->buffer)->ndata > 0) conn->packet_reader(conn);                         \
}while(0)
/* chunk pop/push */
#define PPARENT(conn) ((PROCTHREAD *)(conn->parent))
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
                    ERROR_LOGGER(conn->logger, "socket %d to remote[%s:%d] local[%s:%d] "
                            "connectting failed,  %s", conn->fd, conn->remote_ip, conn->remote_port,
                            conn->local_ip, conn->local_port, strerror(errno));
                    conn->status = CONN_STATUS_CLOSED;
                    CONN_TERMINATE(conn, D_STATE_CLOSE);          
                    return ;
                }
                DEBUG_LOGGER(conn->logger, "Connection[%s:%d] local[%s:%d] via %d is OK event[%d]",
                        conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, 
                        conn->fd, event);
                conn->status = CONN_STATUS_FREE;
                return ;
            }
            if(event & E_CLOSE)
            {
                //DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d START ", E_CLOSE, event_fd);
                CONN_TERMINATE(conn, D_STATE_CLOSE);          
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
            /*
            fprintf(stdout, "%s::%d event:%d on remote[%s:%d] local[%s:%d] via %d\n",
                    __FILE__, __LINE__, event, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd);
            */
            CONN_EVTIMER_SET(conn);
        }
    }
}

/* set connection */
int conn_set(CONN *conn)
{
    short flag = 0;
    if(conn && conn->fd > 0 )
    {
        //reset buffer
        if(conn->session.buffer_size > MB_BLOCK_SIZE)
        {
            MB_SET_BLOCK_SIZE(conn->buffer, conn->session.buffer_size);
            //MB_RESET(conn->buffer);
        }
        //timeout
        if(conn->parent && conn->session.timeout > 0) 
            conn->set_timeout(conn, conn->session.timeout);
        conn->evid = -1;
        fcntl(conn->fd, F_SETFL, O_NONBLOCK);
        if(conn->evbase && conn->event)
        {
            flag = E_READ|E_PERSIST;
            if(conn->status == CONN_STATUS_READY) flag |= E_WRITE;
            conn->event->set(conn->event, conn->fd, flag, (void *)conn, &conn_event_handler);
            conn->evbase->add(conn->evbase, conn->event);
            DEBUG_LOGGER(conn->logger, "setting connection[%s:%d] local[%s:%d] via %d", 
                    conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
            return 0;
        }
        else
        {
            FATAL_LOGGER(conn->logger, "Connection[%p] fd[%d] evbase or"
                    "initialize event failed, %s", PPL(conn), conn->fd, strerror(errno));	
            /* Terminate connection */
            CONN_TERMINATE(conn, D_STATE_CLOSE);
        }
    }	
    return -1;	
}

/* close connection */
int conn_close(CONN *conn)
{
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn)
    {
        CONN_TERMINATE(conn, D_STATE_CLOSE);
        return 0;
    }
    return -1;
}


/* over connection */
int conn_over(CONN *conn)
{
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn)
    {
        conn->i_state = C_STATE_OVER;
        if(QTOTAL(conn->send_queue) <= 0)
        {
            CONN_TERMINATE(conn, D_STATE_CLOSE);
        }
        return 0;
    }
    return -1;
}

/* terminate connection */
int conn_terminate(CONN *conn)
{
    int ret = -1;

    if(conn)
    {
        conn->d_state = D_STATE_CLOSE;
        if(conn->c_state == C_STATE_USING && conn->session.error_handler)
        {
            DEBUG_LOGGER(conn->logger, "error handler session[%s:%d] local[%s:%d] via %d cid:%d %d", 
                    conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, 
                    conn->fd, conn->c_id, PCB(conn->packet)->ndata);
            conn->session.error_handler(conn, PCB(conn->packet), PCB(conn->cache), PCB(conn->chunk));
            MB_RESET(conn->buffer); 
            MB_RESET(conn->packet); 
            MB_RESET(conn->cache); 
            CK_RESET(conn->chunk); 
        }
        conn->close_proxy(conn);
        EVTIMER_DEL(conn->evtimer, conn->evid);
        conn->event->destroy(conn->event);
        DEBUG_LOGGER(conn->logger, "terminateing session[%s:%d] local[%s:%d] via %d",
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* SSL */
        #ifdef HAVE_SSL
        if(conn->ssl)
        {
            SSL_shutdown(XSSL(conn->ssl));
            SSL_free(XSSL(conn->ssl));
            conn->ssl = NULL;
        }
        #endif
        shutdown(conn->fd, SHUT_RDWR);
        close(conn->fd);
        conn->fd = -1;
        ret = 0;
    }
    return ret;
}

void conn_evtimer_handler(void *arg)
{
    CONN *conn = (CONN *)arg;

    CONN_CHECK(conn, D_STATE_CLOSE);

    if(conn)
    {
        DEBUG_LOGGER(conn->logger, "evtimer_handler[%d](%p) "
                "on remote[%s:%d] local[%s:%d] via %d", conn->evid, 
                PPL(conn->evtimer), conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        conn->push_message(conn, MESSAGE_TIMEOUT);
    }
    return ;
}

/* set timeout */
int conn_set_timeout(CONN *conn, int timeout_usec)
{
    int ret = -1;

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn && timeout_usec > 0)
    {
        conn->timeout = timeout_usec;
        CONN_EVTIMER_SET(conn);
        DEBUG_LOGGER(conn->logger, "set evtimer[%p] timeout[%d] evid:%d "
                "on %s:%d local[%s:%d] via %d", PPL(conn->evtimer), conn->timeout, conn->evid,
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
    }
    return ret;
}

/* set evstate as wait*/
int conn_wait_evstate(CONN *conn)
{
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn)
    {
        conn->evstate = EVSTATE_WAIT;
        return 0;
    }
    return -1;
}

/* over evstate */
int conn_over_evstate(CONN *conn)
{
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn)
    {
        conn->evstate = EVSTATE_INIT;
        return 0;
    }
    return -1;
}

/* timeout handler */
int conn_timeout_handler(CONN *conn)
{
    int ret = -1;

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        if(conn->session.timeout_handler)
        {
            DEBUG_LOGGER(conn->logger, "timeout_handler(%d) on remote[%s:%d] local[%s:%d] via %d", 
                    conn->timeout, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd);
            ret = conn->session.timeout_handler(conn, PCB(conn->packet), 
                    PCB(conn->cache), PCB(conn->chunk));
            DEBUG_LOGGER(conn->logger, "over timeout_handler(%d) on remote[%s:%d] "
                    "local[%s:%d] via %d", conn->timeout, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd);
            CONN_STATE_RESET(conn);
        }
        else
        {
            DEBUG_LOGGER(conn->logger, "TIMEOUT[%d]-close connection on remote[%s:%d] "
                    "local[%s:%d] via %d", conn->timeout, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd);
            CONN_TERMINATE(conn, D_STATE_CLOSE);
        }
    }
    return -1;
}

/* start client transaction state */
int conn_start_cstate(CONN *conn)
{
    CHUNK *cp = NULL;
    int ret = -1;
    /* Check connection and transaction state */
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        if(conn->c_state == C_STATE_FREE)
        {
            DEBUG_LOGGER(conn->logger, "Start cstate on conn[%s:%d] local[%s:%d] via %d",
                    conn->remote_ip, conn->remote_port, conn->local_ip, 
                    conn->local_port, conn->fd);
            conn->c_state = C_STATE_USING;
            while(QUEUE_POP(conn->send_queue, PCHUNK, &cp) == 0)
            {
                if(cp && PPARENT(conn) && PPARENT(conn)->service)
                {
                    PPARENT(conn)->service->pushchunk(PPARENT(conn)->service, cp);
                }
                cp  = NULL;
            }
            MB_RESET(conn->packet);
            MB_RESET(conn->cache);
            MB_RESET(conn->buffer);
            MB_RESET(conn->oob);
            MB_RESET(conn->exchange);
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

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn)
    {
        conn->c_state = C_STATE_FREE;
        EVTIMER_DEL(conn->evtimer, conn->evid);
        ret = 0;
    }
    return ret;
}

/* push message to message queue */
int conn_push_message(CONN *conn, int message_id)
{
    MESSAGE msg = {0};
    int ret = -1;

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn && (message_id & MESSAGE_ALL) )
    {
        msg.msg_id = message_id;
        msg.fd = conn->fd;
        msg.index = conn->index;
        msg.handler  = conn;
        msg.parent   = conn->parent;
        QUEUE_PUSH(conn->message_queue, MESSAGE, &msg);
        DEBUG_LOGGER(conn->logger, "Pushed message[%s] to message_queue[%p] "
                "on conn[%s:%d] local[%s:%d] via %d total %d handler[%p] parent[%p]",
                MESSAGE_DESC(message_id), PPL(conn->message_queue), conn->remote_ip, 
                conn->remote_port, conn->local_ip, conn->local_port, 
                conn->fd, QTOTAL(conn->message_queue),
                PPL(conn), PPL(conn->parent));
        ret = 0;
    }
    return ret;
}

/* read handler */
int conn_read_handler(CONN *conn)
{
    int ret = -1, n = -1;

    CONN_CHECK_RET(conn, (D_STATE_RCLOSE|D_STATE_CLOSE), ret);

    if(conn)
    {
        DEBUG_LOGGER(conn->logger, "Ready for read data from %s:%d on %s:%d via %d ",
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* Receive OOB */
        if((n = MB_RECV(conn->oob, conn->fd, MSG_OOB)) > 0)
        {
            conn->recv_oob_total += n;
            DEBUG_LOGGER(conn->logger, "Received %d bytes OOB total %lld "
                    "from %s:%d on %s:%d via %d", n, conn->recv_oob_total, 
                    conn->remote_ip, conn->remote_port, conn->local_ip, 
                    conn->local_port, conn->fd);
            if((n = conn->oob_handler(conn)) > 0)
            {
                MB_DEL(conn->oob, n);
            }
            /* CONN TIMER sample */
            return (ret = 0);
        }
        /* Receive to chunk with chunk_read_state before reading to buffer */
        if(conn->s_state == S_STATE_READ_CHUNK  && conn->session.packet_type != PACKET_PROXY
            && CK_LEFT(conn->chunk) > 0)
        {
            if(PCB(conn->buffer)->ndata > 0) ret = conn_chunk_reader(conn);
            if(PCB(conn->buffer)->ndata <= 0){CONN_CHUNK_READ(conn, n);}
            return ret;
        }
        /* Receive normal data */
#ifdef HAVE_SSL
        if(conn->ssl) n = MB_READ_SSL(conn->buffer, conn->ssl);
        else n = MB_READ(conn->buffer, conn->fd);
#else
        n = MB_READ(conn->buffer, conn->fd);
#endif
        if(n <= 0)
        {
            FATAL_LOGGER(conn->logger, "Reading data %d bytes ptr:%p left:%d "
                    "from %s:%d on %s:%d via %d failed, %s",
                    n, MB_END(conn->buffer), MB_LEFT(conn->buffer), conn->remote_ip, 
                    conn->remote_port, conn->local_ip, conn->local_port, 
                    conn->fd, strerror(errno));
            /* Terminate connection */
            CONN_TERMINATE(conn, (D_STATE_CLOSE|D_STATE_RCLOSE));
            return (ret = 0);
        }
        conn->recv_data_total += n;
        DEBUG_LOGGER(conn->logger, "Received %d bytes data total %lld "
                "from %s:%d on %s:%d via %d", n, conn->recv_data_total, 
                conn->remote_ip, conn->remote_port, conn->local_ip, 
                conn->local_port, conn->fd);
        //for proxy
        if(conn->session.packet_type & PACKET_PROXY)
        {
            ret = conn->proxy_handler(conn);
            if(conn->session.packet_type == PACKET_PROXY) return ret;
        }
        if(conn->s_state == 0 && PCB(conn->packet)->ndata == 0)
            conn->packet_reader(conn);
        ret = 0;
    }
    return ret;
}

/* write handler */
int conn_write_handler(CONN *conn)
{
    int ret = -1, n = 0;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE), ret);

    if(conn && conn->send_queue)
    {
        DEBUG_LOGGER(conn->logger, "Ready for send data to %s:%d on %s:%d via %d "
                "qtotal:%d qhead:%d qcount:%d", conn->remote_ip, conn->remote_port,
                conn->local_ip, conn->local_port, conn->fd, QTOTAL(conn->send_queue),
                QHEAD(conn->send_queue), QCOUNT(conn->send_queue));   
        if(QTOTAL(conn->send_queue) > 0 && QUEUE_HEAD(conn->send_queue, PCHUNK, &cp) == 0
            && CK_LEFT(cp) > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready for send data to %s:%d ssl:%p "
                    "on %s:%d via %d qtotal:%d pcp:%p", conn->remote_ip, conn->remote_port,
                    conn->ssl, conn->local_ip, conn->local_port, conn->fd, 
                    QTOTAL(conn->send_queue), PPL(cp));   
#ifdef HAVE_SSL
            if(conn->ssl) 
            {
                if((n = CHUNK_WRITE_SSL(cp, conn->ssl)) < 0)
                {
                    ERR_print_errors_fp(stdout);
                    FATAL_LOGGER(conn->logger, "Sending data to %s:%d on %s:%d via %d failed, %s",
                            conn->remote_ip, conn->remote_port, conn->local_ip, 
                            conn->local_port, conn->fd, strerror(errno));
                }
            }
            else 
            {
                if((n = CHUNK_WRITE(cp, conn->fd)) < 0)
                {
                    FATAL_LOGGER(conn->logger, "Sending data to %s:%d on %s:%d via %d failed, %s",
                            conn->remote_ip, conn->remote_port, conn->local_ip, 
                            conn->local_port, conn->fd, strerror(errno));
                }

            }
#else
            if((n = CHUNK_WRITE(cp, conn->fd)) < 0)
            {
                FATAL_LOGGER(conn->logger, "Sending data to %s:%d on %s:%d via %d failed, %s",
                        conn->remote_ip, conn->remote_port, conn->local_ip, 
                        conn->local_port, conn->fd, strerror(errno));

            }
#endif
            if(n > 0)
            {
                conn->sent_data_total += n;
                DEBUG_LOGGER(conn->logger, "Sent %d byte(s) (total sent %lld) "
                        "to %s:%d on %s:%d via %d leave %lld", n, conn->sent_data_total, 
                        conn->remote_ip, conn->remote_port, conn->local_ip, 
                        conn->local_port, conn->fd, CK_LEFT(cp));
                /* CONN TIMER sample */
                if(CHUNK_STATUS(cp) == CHUNK_STATUS_OVER )
                {
                    if(QUEUE_POP(conn->send_queue, PCHUNK, &cp) == 0)
                    {
                        DEBUG_LOGGER(conn->logger, "Completed chunk[%p] to %s:%d "
                                "on %s:%d via %d clean it leave %d", PPL(cp), 
                                conn->remote_ip, conn->remote_port, conn->local_ip,
                                conn->local_port, conn->fd, QTOTAL(conn->send_queue));
                        if(cp && PPARENT(conn) && PPARENT(conn)->service)
                        {
                            PPARENT(conn)->service->pushchunk(PPARENT(conn)->service, cp);
                        }
                        cp  = NULL;
                    }
                }
                ret = 0;
            }
            else
            {
                FATAL_LOGGER(conn->logger, "Sending data to %s:%d on %s:%d via %d failed, %s",
                        conn->remote_ip, conn->remote_port, conn->local_ip, 
                        conn->local_port, conn->fd, strerror(errno));
                /* Terminate connection */
                CONN_TERMINATE(conn, D_STATE_CLOSE);
            }
        }
        if(QTOTAL(conn->send_queue) <= 0)
        {
            conn->event->del(conn->event, E_WRITE);
            if(conn->i_state == C_STATE_OVER)
            {
                CONN_TERMINATE(conn, D_STATE_CLOSE);
            }
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

    CONN_CHECK_RET(conn, (D_STATE_CLOSE), -1);

    if(conn)
    {
        data = PCB(conn->buffer);
        packet_type = conn->session.packet_type;
        DEBUG_LOGGER(conn->logger, "Reading packet type[%d] buffer[%p][%d]", 
                packet_type, conn->buffer, MB_NDATA(conn->buffer));
        /* Remove invalid packet type */
        if(!(packet_type & PACKET_ALL))
        {
            FATAL_LOGGER(conn->logger, "Unkown packet_type[%d] from %s:%d on %s:%d via %d",
                    packet_type, conn->remote_ip, conn->remote_port, conn->local_ip,
                    conn->local_port, conn->fd);
            /* Terminate connection */
            CONN_TERMINATE(conn, D_STATE_CLOSE);
        }
        /* Read packet with customized function from user */
        else if(packet_type & PACKET_CUSTOMIZED && conn->session.packet_reader)
        {
            len = conn->session.packet_reader(conn, data);
            DEBUG_LOGGER(conn->logger,
                    "Reading packet with customized function[%p] length[%d]-[%d] "
                    "from %s:%d on %s:%d via %d", PPL(conn->session.packet_reader), len, 
                    data->ndata, conn->remote_ip, conn->remote_port, conn->local_ip, 
                    conn->local_port, conn->fd);
            goto end;
        }
        /* Read packet with certain length */
        else if(packet_type & PACKET_CERTAIN_LENGTH
                && MB_NDATA(conn->buffer) >= conn->session.packet_length)
        {
            len = conn->session.packet_length;
            DEBUG_LOGGER(conn->logger, "Reading packet with certain length[%d] "
                    "from %s:%d on %s:%d via %d", len, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd);
            goto end;
        }
        /* Read packet with delimiter */
        else if(packet_type & PACKET_DELIMITER && conn->session.packet_delimiter
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
                    DEBUG_LOGGER(conn->logger, "Reading packet with delimiter[%d] "
                    "length[%d] from %s:%d on %s:%d via %d", conn->session.packet_delimiter_length, 
                    len, conn->remote_ip, conn->remote_port, conn->local_ip, 
                    conn->local_port, conn->fd);
                    break;
                }
            }
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

    if(conn && conn->session.packet_handler)
    {

        DEBUG_LOGGER(conn->logger, "packet_handler(%p) on %s:%d local[%s:%d] via %d", conn->session.packet_handler, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        ret = conn->session.packet_handler(conn, PCB(conn->packet));
        DEBUG_LOGGER(conn->logger, "over packet_handler(%p) on %s:%d local[%s:%d] via %d", conn->session.packet_handler, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        if(conn->s_state == S_STATE_PACKET_HANDLING)
        {
            DEBUG_LOGGER(conn->logger, "Reset packet_handler(%p) on %s:%d via %d", conn->session.packet_handler, conn->remote_ip, conn->remote_port, conn->fd);
            SESSION_RESET(conn);
        }
    }
    return ret;
}

/* oob data handler */
int conn_oob_handler(CONN *conn)
{
    int ret = -1;

    if(conn && conn->session.oob_handler)
    {
        DEBUG_LOGGER(conn->logger, "oob_handler(%p) on %s:%d via %d", conn->session.oob_handler, conn->remote_ip, conn->remote_port, conn->fd);
        ret = conn->session.oob_handler(conn, PCB(conn->oob));
        DEBUG_LOGGER(conn->logger, "over oob_handler(%p) on %s:%d via %d", conn->session.oob_handler, conn->remote_ip, conn->remote_port, conn->fd);
    }
    return ret;
}

/* chunk data  handler */
int conn_data_handler(CONN *conn)
{
    int ret = -1;

    if(conn)
    {
        if(conn->session.packet_type == PACKET_PROXY)
        {
            return conn->proxy_handler(conn);
        }
        else if(conn->session.data_handler)
        {
            DEBUG_LOGGER(conn->logger, "data_handler(%p) on %s:%d via %d", 
                    conn->session.data_handler, conn->remote_ip, conn->remote_port, conn->fd);
            ret = conn->session.data_handler(conn, PCB(conn->packet), 
                    PCB(conn->cache), PCB(conn->chunk));
            DEBUG_LOGGER(conn->logger, "over data_handler(%p) on %s:%d via %d", 
                    conn->session.data_handler, conn->remote_ip, conn->remote_port, conn->fd);
        }
        //reset session
        SESSION_RESET(conn);
    }
    return ret;
}

/* bind proxy */
int conn_bind_proxy(CONN *conn, CONN *child)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, ret);

    if(conn && child)
    {
        conn->session.packet_type |= PACKET_PROXY;
        conn->session.childid = child->index;
        conn->session.child = child;
        ret = conn->push_message(conn, MESSAGE_PROXY);
        DEBUG_LOGGER(conn->logger, "Bind proxy connection[%s:%d] to connection[%s:%d]",
                conn->remote_ip, conn->remote_port, child->remote_ip, child->remote_port);
    }
    return ret;
}

/* proxy data handler */
int conn_proxy_handler(CONN *conn)
{
    int ret = -1;
    //CONN_CHECK_RET(conn, ret);
    CONN *parent = NULL, *child = NULL, *oconn = NULL;
    CB_DATA *exchange = NULL, *chunk = NULL, *buffer = NULL;

    if(conn)
    {
        if(conn->session.parent && (parent = PPARENT(conn)->service->findconn(
                        PPARENT(conn)->service, conn->session.parentid))
            && parent == conn->session.parent)
        {
            oconn = parent;
        }
        else if(conn->session.child && (child = PPARENT(conn)->service->findconn(
                        PPARENT(conn)->service, conn->session.childid))
            && child == conn->session.child)
        {
            oconn = child;
        }
        else 
        {
            return -1;
        }
        DEBUG_LOGGER(conn->logger, "Ready exchange connection[%s:%d] to connection[%s:%d]",
                conn->remote_ip, conn->remote_port, oconn->remote_ip, oconn->remote_port);
        if(conn->exchange && (exchange = PCB(conn->exchange)) && exchange->ndata > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready exchange packet[%d] to conn[%s:%d]", 
                    exchange->ndata, oconn->remote_ip, oconn->remote_port);
            oconn->push_chunk(oconn, exchange->data, exchange->ndata);
            MB_RESET(conn->exchange);
        }
        if(conn->chunk && (chunk = PCB(conn->chunk)) && chunk->ndata > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready exchange chunk[%d] to conn[%s:%d]", 
                    chunk->ndata, oconn->remote_ip, oconn->remote_port);
            oconn->push_chunk(oconn, chunk->data, chunk->ndata);
            CK_RESET(conn->chunk);
        }
        if(conn->session.packet_type == PACKET_PROXY 
            && conn->buffer && (buffer = PCB(conn->buffer)) && buffer->ndata > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready exchange buffer[%d] to conn[%s:%d]", 
                    buffer->ndata, oconn->remote_ip, oconn->remote_port);
            oconn->push_chunk(oconn, buffer->data, buffer->ndata);
            MB_DEL(conn->buffer, buffer->ndata);
        }
        return 0;
    }
    return -1;
}

/* close proxy */
int conn_close_proxy(CONN *conn)
{
    int ret = -1;
    CONN *parent = NULL, *child = NULL;

    if(conn && (conn->session.packet_type & PACKET_PROXY))
    {
        conn->proxy_handler(conn);
        if(conn->session.parent && (parent = PPARENT(conn)->service->findconn(
                        PPARENT(conn)->service, conn->session.parentid))
                && parent == conn->session.parent)
        {
            parent->set_timeout(parent, SB_PROXY_TIMEOUT);
            parent->session.childid = 0;
            parent->session.child = NULL;
        }
        else if(conn->session.child && (child = PPARENT(conn)->service->findconn(
                        PPARENT(conn)->service, conn->session.childid))
                && child == conn->session.child)
        {
            child->set_timeout(child, SB_PROXY_TIMEOUT);
            child->session.parent = NULL;
            child->session.parentid = 0;
        }
        ret = 0;
    }
    return ret;
}

/* push to exchange  */
int conn_push_exchange(CONN *conn, void *data, int size)
{
    int ret = -1;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE|D_STATE_RCLOSE), ret);

    if(conn)
    {
        MB_PUSH(conn->exchange, data, size);
        DEBUG_LOGGER(conn->logger, "Push exchange size[%d] remote[%s:%d] local[%s:%d] via %d",
                MB_NDATA(conn->exchange), conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* save cache to connection  */
int conn_save_cache(CONN *conn, void *data, int size)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, ret);

    if(conn)
    {
        MB_RESET(conn->cache);
        MB_PUSH(conn->cache, data, size);
        DEBUG_LOGGER(conn->logger, "Saved cache size[%d] remote[%s:%d] local[%s:%d] via %d",
                MB_NDATA(conn->cache), conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* chunk reader */
int conn_chunk_reader(CONN *conn)
{
    int ret = -1, n = -1;
    CHUNK *cp = NULL;

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
                DEBUG_LOGGER(conn->logger, "Chunk completed %lld bytes from %s:%d on %s:%d via %d",
                        CK_SIZE(conn->chunk), conn->remote_ip, conn->remote_port, 
                        conn->local_ip, conn->local_port, conn->fd);
            }
            if(n > 0)
            {
                DEBUG_LOGGER(conn->logger, "Filled  %d byte(s) left:%lld to chunk from buffer "
                        "to %s:%d on conn[%s:%d] via %d", n, CK_LEFT(conn->chunk),
                        conn->remote_ip, conn->remote_port, conn->local_ip, 
                        conn->local_port, conn->fd);
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

    if(conn && conn->chunk && size > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for recv chunk size:%d from %s:%d on %s:%d via %d",
                size, conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        CK_MEM(conn->chunk, size);
        conn->s_state = S_STATE_READ_CHUNK;
        conn->chunk_reader(conn);
        //CONN_CHUNK_READ(conn, n);
        ret = 0;
    }
    return ret;
}

/* push chunk */
int conn_push_chunk(CONN *conn, void *data, int size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE|D_STATE_RCLOSE), ret);

    if(conn && conn->send_queue && data && size > 0)
    {
        //CHUNK_POP(conn, cp);
        if(PPARENT(conn) && PPARENT(conn)->service 
                && (cp = PPARENT(conn)->service->popchunk(PPARENT(conn)->service)))
        {
            CK_MEM(cp, size);
            CK_MEM_COPY(cp, data, size);
            QUEUE_PUSH(conn->send_queue, PCHUNK, &cp);
        }else return ret;
        if(QTOTAL(conn->send_queue) > 0 ) conn->event->add(conn->event, E_WRITE);
        DEBUG_LOGGER(conn->logger, "Pushed chunk size[%d] to %s:%d send_queue "
                "total %d on %s:%d via %d ", size, conn->remote_ip, conn->remote_port, 
                QTOTAL(conn->send_queue), conn->local_ip, conn->local_port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* receive chunk file */
int conn_recv_file(CONN *conn, char *filename, long long offset, long long size)
{
    int ret = -1, n = -1;
    CHUNK *cp = NULL;

    if(conn && conn->chunk && filename && offset >= 0 && size > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for recv chunk file:%s offset:%lld size:%lld"
                " from %s:%d on %s:%d via %d", filename, offset, size, conn->remote_ip, 
                conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        CK_FILE(conn->chunk, filename, offset, size);
        CK_SET_BSIZE(conn->chunk, conn->session.buffer_size);
        conn->s_state = S_STATE_READ_CHUNK;
        conn->chunk_reader(conn);
        //CONN_CHUNK_READ(conn, n);
        ret = 0;
    }
    return ret;
}

/* push chunk file */
int conn_push_file(CONN *conn, char *filename, long long offset, long long size)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE|D_STATE_RCLOSE), ret);

    if(conn && conn->send_queue && filename && offset >= 0 && size > 0)
    {
        //CHUNK_POP(conn, cp);
        if(PPARENT(conn) && PPARENT(conn)->service 
                && (cp = PPARENT(conn)->service->popchunk(PPARENT(conn)->service)))
        {
            CK_FILE(cp, filename, offset, size);
            QUEUE_PUSH(conn->send_queue, PCHUNK, &cp);
            if((QTOTAL(conn->send_queue)) > 0 ) conn->event->add(conn->event, E_WRITE);
            DEBUG_LOGGER(conn->logger, "Pushed file[%s] [%lld][%lld] to %s:%d "
                    "send_queue total %d on %s:%d via %d ", filename, offset, size, 
                    conn->remote_ip, conn->remote_port, QTOTAL(conn->send_queue), 
                    conn->local_ip, conn->local_port, conn->fd);
            ret = 0;
        }else return ret;
    }
    return ret;
}

/* set session options */
int conn_set_session(CONN *conn, SESSION *session)
{
    int ret = -1;

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn && session)
    {
        memcpy(&(conn->session), session, sizeof(SESSION));
        if(conn->parent && conn->session.timeout > 0) 
            conn->set_timeout(conn, conn->session.timeout);
        ret = 0;
    }
    return ret;
}

/* transaction handler */
int conn_transaction_handler(CONN *conn, int tid)
{

    int ret = -1;

    if(conn)
    {
        if(conn && conn->session.transaction_handler)
        {
            ret = conn->session.transaction_handler(conn, tid);
        }
    }
    return ret;
}

/* reset connection */
void conn_reset(CONN *conn)
{
    CHUNK *cp = NULL;

    if(conn)
    {
        DEBUG_LOGGER(conn->logger, "Reset connection[%s:%d] local[%s:%d] via %d",
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* global */
        conn->index = 0;
        conn->d_state = 0;
        /* connection */
        conn->fd = 0;
        conn->sock_type = 0;
        memset(conn->remote_ip, 0, SB_IP_MAX);
        conn->remote_port = 0;
        memset(conn->local_ip, 0, SB_IP_MAX);
        conn->local_port = 0;
        /* bytes stats */
        conn->recv_oob_total = 0ll;
        conn->sent_oob_total = 0ll;
        conn->recv_data_total = 0ll;
        conn->sent_data_total = 0ll;
        /* event */
        conn->evbase = NULL;

        /* event timer */
        conn->evid = 0;
        conn->evtimer = NULL;

        /* buffer and chunk */
        MB_RESET(conn->buffer);
        MB_RESET(conn->packet);
        MB_RESET(conn->oob);
        MB_RESET(conn->cache);
        CK_RESET(conn->chunk);

        /* timer, logger, message_queue and send_queue */
        conn->message_queue = NULL;
        if(conn->send_queue)
        {
            while(QUEUE_POP(conn->send_queue, PCHUNK, &cp) == 0)
            {
                if(cp && PPARENT(conn) && PPARENT(conn)->service)
                {
                    PPARENT(conn)->service->pushchunk(PPARENT(conn)->service, cp);
                }
                cp  = NULL;
            }
        }
        /* SSL */
#ifdef HAVE_SSL
        if(conn->ssl)
        {
            SSL_shutdown(XSSL(conn->ssl));
            SSL_free(XSSL(conn->ssl));
            conn->ssl = NULL;
        }
#endif
        /* client transaction state */
        conn->parent = NULL;
        conn->status = 0;
        conn->i_state = 0;
        conn->c_state = 0;
        conn->c_id = 0;

        /* transaction */
        conn->s_id = 0;
        conn->s_state = 0;
        /* event state */
        conn->evstate = 0;
        conn->timeout = 0;
        /* session */
        memset(&(conn->session), 0, sizeof(SESSION));
    }
    return ;
}
/* clean connection */
void conn_clean(CONN **pconn)
{
    CHUNK *cp = NULL;

    if(pconn && *pconn)
    {
        DEBUG_LOGGER((*pconn)->logger, "Ready for clean conn[%p]", PPL(*pconn));
        if((*pconn)->event) (*pconn)->event->clean(&((*pconn)->event));
        /* Clean BUFFER */
        MB_CLEAN((*pconn)->buffer);
        /* Clean OOB */
        MB_CLEAN((*pconn)->oob);
        /* Clean cache */
        MB_CLEAN((*pconn)->cache);
        /* Clean packet */
        MB_CLEAN((*pconn)->packet);
        /* Clean exchange */
        MB_CLEAN((*pconn)->exchange);
        /* Clean chunk */
        CK_CLEAN((*pconn)->chunk);
        /* Clean send queue */
        if((*pconn)->send_queue)
        {
            while(QUEUE_POP((*pconn)->send_queue, PCHUNK, &cp) == 0){CK_CLEAN(cp);}
            QUEUE_CLEAN((*pconn)->send_queue);
        }
        DEBUG_LOGGER((*pconn)->logger, "Over for clean conn[%p]", PPL(*pconn));
        free(*pconn);
        (*pconn) = NULL;
    }
}

/* Initialize connection */
CONN *conn_init()
{
    CONN *conn = NULL;

    if((conn = calloc(1, sizeof(CONN))))
    {
        MB_INIT(conn->buffer, MB_BLOCK_SIZE);
        MB_INIT(conn->packet, MB_BLOCK_SIZE);
        MB_INIT(conn->cache, MB_BLOCK_SIZE);
        MB_INIT(conn->oob, MB_BLOCK_SIZE);
        MB_INIT(conn->exchange, MB_BLOCK_SIZE);
        CK_INIT(conn->chunk);
        QUEUE_INIT(conn->send_queue);
        conn->event                 = ev_init();
        conn->set                   = conn_set;
        conn->close                 = conn_close;
        conn->over                  = conn_over;
        conn->terminate             = conn_terminate;
        conn->start_cstate          = conn_start_cstate;
        conn->over_cstate           = conn_over_cstate;
        conn->set_timeout           = conn_set_timeout;
        conn->wait_evstate          = conn_wait_evstate;
        conn->over_evstate          = conn_over_evstate;
        conn->timeout_handler       = conn_timeout_handler;
        conn->set_session           = conn_set_session;
        conn->push_message          = conn_push_message;
        conn->read_handler          = conn_read_handler;
        conn->write_handler         = conn_write_handler;
        conn->packet_reader         = conn_packet_reader;
        conn->packet_handler        = conn_packet_handler;
        conn->oob_handler           = conn_oob_handler;
        conn->data_handler          = conn_data_handler;
        conn->bind_proxy            = conn_bind_proxy;
        conn->proxy_handler         = conn_proxy_handler;
        conn->close_proxy           = conn_close_proxy;
        conn->push_exchange         = conn_push_exchange;
        conn->transaction_handler   = conn_transaction_handler;
        conn->save_cache            = conn_save_cache;
        conn->chunk_reader          = conn_chunk_reader;
        conn->recv_chunk            = conn_recv_chunk;
        conn->push_chunk            = conn_push_chunk;
        conn->recv_file             = conn_recv_file;
        conn->push_file             = conn_push_file;
        conn->set_session           = conn_set_session;
        conn->reset                 = conn_reset;
        conn->clean                 = conn_clean;
    }
    return conn;
}
