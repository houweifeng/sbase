#include "sbase.h"
#include "xssl.h"
#include "conn.h"
#include "mmblock.h"
#include "chunk.h"
#include "logger.h"
#include "xqueue.h"
#include "message.h"
#include "service.h"
#include "timer.h"
#include "evtimer.h"
#include "xmm.h"
#ifndef PPL
#define PPL(_x_) ((void *)(_x_))
#endif
#ifndef LL
#define LL(_x_) ((long long int)(_x_))
#endif
int conn__push__message(CONN *conn, int message_id);
int conn_shut(CONN *conn, int d_state, int e_state);
int conn_read_chunk(CONN *conn)
{
	if(conn->ssl) return CHUNK_READ_SSL(&conn->chunk, conn->ssl);
	else return CHUNK_READ(&conn->chunk, conn->fd);
}
int conn_write_chunk(CONN *conn, CHUNK *cp)
{
	if(conn->ssl) 
    {
        return CHUNK_WRITE_SSL(cp, conn->ssl);
    }
	else 
    {
        return CHUNK_WRITE(cp, conn->fd);
    }
    return -1;
}
int conn_read_buffer(CONN *conn)
{
	if(conn->ssl) return MMB_READ_SSL(conn->buffer, conn->ssl);
	else return MMB_READ(conn->buffer, conn->fd);
}
#define PPARENT(conn) ((PROCTHREAD *)(conn->parent))
#define IODAEMON(conn) ((PROCTHREAD *)(conn->iodaemon))
#define IOWAKEUP(conn) {if(IODAEMON(conn))IODAEMON(conn)->wakeup(IODAEMON(conn));}            
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
#define CONN_EVENT_ADD(conn, flag)                                                          \
{                                                                                           \
        event_add(&(conn->event), flag);                                                    \
}
#define CONN_READY_WRITE(conn)                                                              \
{                                                                                           \
    event_add(&(conn->event), E_WRITE);                                                     \
}

#define CONN_STATE_RESET(conn)                                                              \
{                                                                                           \
    if(conn)                                                                                \
    {                                                                                       \
        conn->s_state = 0;                                                                  \
    }                                                                                       \
}
#define CONN_PUSH_MESSAGE(conn, msgid)                                                      \
do                                                                                          \
{                                                                                           \
    if(conn && conn->d_state == 0)                                                          \
    {                                                                                       \
            qmessage_push(conn->message_queue, msgid,                                       \
                conn->index, conn->fd, -1, conn->parent, conn, NULL);                       \
        if(conn->parent&&PPARENT(conn)->use_cond_wait){MUTEX_SIGNAL(PPARENT(conn)->mutex);} \
    }                                                                                       \
}while(0)
#define CONN__PUSH__MESSAGE(conn, msgid)                                                    \
do                                                                                          \
{                                                                                           \
    if(conn)                                                                                \
    {                                                                                       \
            qmessage_push(conn->message_queue, msgid,                                       \
                conn->index, conn->fd, -1, conn->parent, conn, NULL);                       \
        if(conn->parent&&PPARENT(conn)->use_cond_wait){MUTEX_SIGNAL(PPARENT(conn)->mutex);} \
    }                                                                                       \
}while(0)

#define CONN_CHUNK_READ(conn, n)                                                            \
{                                                                                           \
    /* read to chunk */                                                                     \
    if(conn->s_state == S_STATE_READ_CHUNK)                                                 \
    {                                                                                       \
        if((n = conn_read_chunk(conn)) <= 0 && errno != EAGAIN)                             \
        {                                                                                   \
            FATAL_LOGGER(conn->logger, "Reading %d bytes data from conn[%p][%s:%d] ssl:%p " \
                    "on %s:%d via %d failed, %s", n, conn, conn->remote_ip, conn->remote_port,  \
                    conn->ssl, conn->local_ip, conn->local_port, conn->fd, strerror(errno));    \
            conn_shut(conn, D_STATE_CLOSE, E_STATE_ON);                                     \
            return -1;                                                                      \
        }                                                                                   \
        DEBUG_LOGGER(conn->logger, "Read %d bytes ndata:%d left:%lld to chunk from %s:%d"   \
                " on %s:%d via %d", n, CHK_NDATA(conn->chunk), LL(CHK_LEFT(conn->chunk)),   \
                conn->remote_ip, conn->remote_port, conn->local_ip,                         \
                conn->local_port, conn->fd);                                                \
        if(CHUNK_STATUS(&conn->chunk) == CHUNK_STATUS_OVER )                                \
        {                                                                                   \
            DEBUG_LOGGER(conn->logger, "Chunk completed %lld bytes from %s:%d "             \
                    "on %s:%d via %d", LL(CHK_SIZE(conn->chunk)), conn->remote_ip,          \
                    conn->remote_port, conn->local_ip, conn->local_port, conn->fd);         \
            conn->s_state = S_STATE_DATA_HANDLING;                                          \
            CONN_PUSH_MESSAGE(conn, MESSAGE_DATA);                                          \
        }                                                                                   \
        return 0;                                                                           \
    }                                                                                       \
}
/* evtimer setting */
#define CONN_EVTIMER_SET(conn)                                                              \
    do{                                                                                         \
        if(conn && conn->evtimer && conn->timeout > 0)                                          \
        {                                                                                       \
            if(conn->evid >= 0)                                                                 \
            {                                                                                   \
                EVTIMER_UPDATE(conn->evtimer, conn->evid, conn->timeout,                        \
                        &conn_evtimer_handler, (void *)conn);                                   \
            }                                                                                   \
            else                                                                                \
            {                                                                                   \
                conn->evid = EVTIMER_ADD(conn->evtimer, conn->timeout,                          \
                        &conn_evtimer_handler, (void *)conn);                                   \
            }                                                                                   \
        }                                                                                       \
    }while(0)

/* update evtimer */
#define CONN_UPDATE_EVTIMER(conn , _evtimer_, _evid_)                                       \
    do                                                                                      \
{                                                                                           \
    if(conn && (_evtimer_ = conn->evtimer) && conn->d_state == 0 && conn->timeout > 0       \
            && (_evid_ = conn->evid) >= 0)                                                  \
    {                                                                                       \
        EVTIMER_UPDATE(_evtimer_, _evid_, conn->timeout,                                    \
                &conn_evtimer_handler, (void *)conn);                                       \
    }                                                                                       \
}while(0)

#define PUSH_IOQMESSAGE(conn, msgid)                                                        \
do                                                                                          \
{                                                                                           \
    if(conn && conn->d_state == 0)                                                          \
    {                                                                                       \
        qmessage_push(conn->ioqmessage, msgid,                                              \
                conn->index, conn->fd, -1, conn->iodaemon, conn, NULL);                     \
        event_add(&(conn->event), E_WRITE);                                                 \
    }                                                                                       \
}while(0)
#define SESSION_RESET(conn)                                                                 \
do                                                                                          \
{                                                                                           \
    MMB_RESET(conn->packet);                                                                \
    MMB_RESET(conn->cache);                                                                 \
    chunk_reset(&conn->chunk);                                                              \
    CONN_STATE_RESET(conn);                                                                 \
    if(MMB_NDATA(conn->buffer) > 0){PUSH_IOQMESSAGE(conn, MESSAGE_BUFFER);}                 \
}while(0)
    //else{CONN_PUSH_MESSAGE(conn, MESSAGE_END);}                                             
/* chunk pop/push */
/* read */
void conn_buffer_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK(conn, D_STATE_CLOSE);

    if(conn)
    {
        if(xqueue_total(conn->queue, conn->qid) < 1)
            event_del(&(conn->event), E_WRITE);
        ret = conn->packet_reader(conn);
    }
    return ;
}

/* write */
void conn_chunk_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK(conn, D_STATE_CLOSE);

    if(conn)
    {
        if(xqueue_total(conn->queue, conn->qid) < 1)
            event_del(&(conn->event), E_WRITE);
        ret = conn__read__chunk(conn);
    }
    return ;
}

/* over */
void conn_shut_handler(CONN *conn)
{
    if(conn)
    {
        if(conn->iodaemon)
        {
            DEBUG_LOGGER(conn->logger, "Ready for shut-connection[%s:%d] local[%s:%d] via %d ",
                        conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, 
                        conn->fd);
            event_add(&(conn->event), E_WRITE);
            qmessage_push(conn->ioqmessage, MESSAGE_OVER,
                    conn->index, conn->fd, -1, conn->iodaemon, conn, NULL);
        }
        else
        {
            DEBUG_LOGGER(conn->logger, "Ready for quit-connection[%s:%d] local[%s:%d] via %d ",
                        conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, 
                        conn->fd)
            qmessage_push(conn->message_queue, MESSAGE_QUIT, 
                    conn->index, conn->fd, -1, conn->parent, conn, NULL);
        }
        /*
        event_destroy(&conn->event);
        qmessage_push(conn->message_queue, MESSAGE_OVER, 
            conn->index, conn->fd, -1, conn->parent, conn, NULL);
        */
    }
    return ;
}

/* end handler  */
void conn_end_handler(CONN *conn)
{
    CONN_CHECK(conn, D_STATE_CLOSE);

    if(conn)
    {
        if(xqueue_total(conn->queue, conn->qid) > 0)
            event_add(&(conn->event), E_WRITE);
        else
            event_del(&(conn->event), E_WRITE);
        if(conn->s_state == 0 && MMB_NDATA(conn->buffer) > 0){PUSH_IOQMESSAGE(conn, MESSAGE_BUFFER);}
    }
    return ;
}

/* connection event handler */
void conn_event_handler(int event_fd, short event, void *arg)
{
    int len = sizeof(int), error = 0, evid = -1, ret = -1;
    CONN *conn = (CONN *)arg;
    void *evtimer = NULL;

    if(conn)
    {
        //CONN_CHECK(conn, D_STATE_CLOSE|D_STATE_RCLOSE|D_STATE_WCLOSE);
        //fprintf(stdout, "%s::%d event[%d] on fd[%d]\n", __FILE__, __LINE__, event, event_fd);
        if(event_fd == conn->fd)
        {
            //fprintf(stdout, "%s::%d event[%d] on fd[%d]\n", __FILE__, __LINE__, event, event_fd);
            if(conn->status == CONN_STATUS_READY)
            {
                if(getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0 
                        || error != 0)
                {
                    WARN_LOGGER(conn->logger, "socket %d to conn[%p] remote[%s:%d] local[%s:%d] "
                            "connectting failed, error:%d %s", conn->fd, conn, conn->remote_ip, 
                            conn->remote_port, conn->local_ip, conn->local_port, error, strerror(errno));
                    conn_shut(conn, D_STATE_CLOSE, E_STATE_ON);          
                    return ;
                }
                DEBUG_LOGGER(conn->logger, "Connection[%s:%d] local[%s:%d] via %d is OK event[%d]",
                        conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, 
                        conn->fd, event);
                //set conn->status
                if(PPARENT(conn) && PPARENT(conn)->service)
                    PPARENT(conn)->service->okconn(PPARENT(conn)->service, conn);
                if(xqueue_total(conn->queue, conn->qid) < 1) 
                {
                    event_del(&conn->event, E_WRITE);
                }
                if(conn->session.ok_handler) conn->session.ok_handler(conn);
                return ;
            }
            int flag = fcntl(conn->fd, F_GETFL, 0);
            if(conn->ssl) 
            {

                if(flag & O_NONBLOCK)
                {
                    flag &= ~O_NONBLOCK;
                    fcntl(conn->fd, F_SETFL, flag);
                }
            }
            else 
            {
                if(!(flag & O_NONBLOCK))
                {
                    flag |= O_NONBLOCK;
                    fcntl(conn->fd, F_SETFL, flag);
                }
            }
            CONN_UPDATE_EVTIMER(conn, evtimer, evid);
            if(event & E_READ)
            {
                //DEBUG_LOGGER(conn->logger, "E_READ:%d on conn[%p]->d_state:%d via %d START", E_READ, conn, conn->d_state, event_fd);
                ret = conn->read_handler(conn);
                //DEBUG_LOGGER(conn->logger, "E_READ:%d on conn[%p]->d_state:%d via %d END", E_READ, conn, conn->d_state, event_fd);
                if(ret < 0)return ;
            }
            if(event & E_WRITE)
            {
                //DEBUG_LOGGER(conn->logger, "E_WRITE:%d on conn[%p]->d_state:%d via %d START", E_WRITE, conn, conn->d_state, event_fd);
                ret = conn->write_handler(conn);
                //DEBUG_LOGGER(conn->logger, "E_WRITE:%d on conn[%p]->d_state:%d via %d END", E_WRITE, conn, conn->d_state, event_fd);
                if(ret < 0)return ;
            } 
            /*
               fprintf(stdout, "%s::%d event:%d on remote[%s:%d] local[%s:%d] via %d\n",
               __FILE__, __LINE__, event, conn->remote_ip, conn->remote_port, 
               conn->local_ip, conn->local_port, conn->fd);
               */
        }
        else
        {
            FATAL_LOGGER(conn->logger, "Invalid fd[%d:%d] event:%d", event_fd, conn->fd, event);
        }
    }
    return ;
}

/* set connection */
int conn_set(CONN *conn)
{
    short flag = 0;
    if(conn && conn->fd > 0 )
    {
        //timeout
        conn->evid = -1;
        if(conn->parent && conn->session.timeout > 0) 
            conn->set_timeout(conn, conn->session.timeout);
        conn->qid = xqueue_new(conn->queue);
        if(conn->evbase)
        {
            flag = E_READ|E_PERSIST;
            if(conn->status == CONN_STATUS_READY) flag |= E_WRITE;
            event_set(&conn->event, conn->fd, flag, (void *)conn, &conn_event_handler);
            conn->evbase->add(conn->evbase, &conn->event);
            IOWAKEUP(conn);
            DEBUG_LOGGER(conn->logger, "setting conn[%p]->evbase->nfd[%d][%p] remote[%s:%d]"
                    " d_state:%d local[%s:%d] via %d", conn, conn->evbase->nfd, &conn->event, 
                    conn->remote_ip, conn->remote_port, conn->d_state,
                    conn->local_ip, conn->local_port, conn->fd);
            return 0;
        }
        else
        {
            FATAL_LOGGER(conn->logger, "Connection[%p] fd[%d] evbase or "
                    " initialize event failed, %s", conn, conn->fd, strerror(errno));	
            /* Terminate connection */
            conn_shut(conn, D_STATE_CLOSE, E_STATE_ON);
        }
    }	
    return -1;	
}

/* get service id */
int conn_get_service_id(CONN *conn)
{
    if(conn && conn->parent)
    {
        return PPARENT(conn)->service->id;
    }
    return -1;
}

/* close connection */
int conn_close(CONN *conn)
{
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        DEBUG_LOGGER(conn->logger, "Ready for close-conn[%p] remote[%s:%d] local[%s:%d] via %d",
                conn, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        conn->over_cstate(conn);
        conn->over_evstate(conn);
        conn_shut(conn, D_STATE_CLOSE, E_STATE_OFF);
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
        MUTEX_LOCK(conn->mutex);
        DEBUG_LOGGER(conn->logger, "Ready for over-connection[%p] remote[%s:%d] local[%s:%d] via %d", conn, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        if(conn->d_state == D_STATE_FREE) conn_over_chunk(conn);
        MUTEX_UNLOCK(conn->mutex);
        return 0;
    }
    return -1;
}

/* shutdown connection */
int conn_shut(CONN *conn, int d_state, int e_state)
{
    if(conn)
    {
        MUTEX_LOCK(conn->mutex);
        conn->over_timeout(conn);
        DEBUG_LOGGER(conn->logger, "Ready for close-conn[%p] remote[%s:%d] d_state:%d "
                "local[%s:%d] via %d", conn, conn->remote_ip, conn->remote_port,
                conn->d_state, conn->local_ip, conn->local_port, conn->fd);
        if(conn->d_state == D_STATE_FREE && conn->fd > 0)
        {
            conn->d_state |= d_state;
            if(conn->e_state == E_STATE_OFF) conn->e_state = e_state;
            DEBUG_LOGGER(conn->logger, "closed-conn[%p] remote[%s:%d] d_state:%d "
                    "local[%s:%d] via %d", conn, conn->remote_ip, conn->remote_port,
                    conn->d_state, conn->local_ip, conn->local_port, conn->fd);
            CONN__PUSH__MESSAGE(conn, MESSAGE_SHUT);
        }
        MUTEX_UNLOCK(conn->mutex);
    }
    return 0;
}

/* terminate connection */
int conn_terminate(CONN *conn)
{
    PROCTHREAD *parent = NULL;
    CHUNK *cp = NULL;
    int ret = -1;

    if(conn)
    {
        parent = (PROCTHREAD *)conn->parent;
        DEBUG_LOGGER(conn->logger, "Ready for terminate-conn[%p] remote[%s:%d] local[%s:%d] via %d "
                "qtotal:%d d_state:%d i_state:%d c_state:%d s_state:%d e_state:%d", conn, 
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd,
                xqueue_total(conn->queue, conn->qid), conn->d_state, conn->i_state, 
                conn->c_state, conn->s_state, conn->e_state);
        conn->d_state = D_STATE_CLOSE;
        //continue incompleted data handling 
        if(conn->s_state == S_STATE_DATA_HANDLING && CHK_NDATA(conn->chunk) > 0)
        {
            if(conn->session.packet_type == PACKET_PROXY)
            {
                conn->proxy_handler(conn);
            }
        }
        if(conn->e_state == E_STATE_ON && conn->session.error_handler)
        {
            DEBUG_LOGGER(conn->logger, "error handler session[%s:%d] local[%s:%d] via %d cid:%d %d", conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd, conn->c_id, conn->packet.ndata);
            conn->session.error_handler(conn, PCB(conn->packet), PCB(conn->cache), PCB(conn->chunk));
            MMB_RESET(conn->buffer); 
            MMB_RESET(conn->packet); 
            MMB_RESET(conn->cache); 
            MMB_RESET(conn->oob); 
            chunk_reset(&conn->chunk); 
        }
        conn->close_proxy(conn);
        EVTIMER_DEL(conn->evtimer, conn->evid);
        DEBUG_LOGGER(conn->logger, "terminateing conn[%p]->d_state:%d queue:%d session[%s:%d] local[%s:%d] via %d", conn, conn->d_state, xqueue_total(conn->queue, conn->qid), conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* clean send queue */
        while(xqueue_total(conn->queue, conn->qid) > 0)
        {
            if((cp = (CHUNK *)xqueue_pop(conn->queue, conn->qid)))
            {
                if(parent && parent->service)
                    parent->service->pushchunk(parent->service, cp);
                else
                {
                    chunk_clean(cp);
                }
                cp = NULL;
            }
        }
        if(conn->queue) xqueue_close(conn->queue, conn->qid);
        DEBUG_LOGGER(conn->logger, "over-terminateing conn[%p]->d_state:%d queue:%d session[%s:%d] local[%s:%d] via %d", conn, conn->d_state, xqueue_total(conn->queue, conn->qid), conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* SSL */
#ifdef HAVE_SSL
        if(conn->ssl)
        {
            SSL_shutdown(XSSL(conn->ssl));
            SSL_free(XSSL(conn->ssl));
            conn->ssl = NULL;
        }
#endif
        if(conn->fd > 0)
        {
            shutdown(conn->fd, SHUT_RDWR);
            close(conn->fd);
        }
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
        CONN_PUSH_MESSAGE(conn, MESSAGE_TIMEOUT);
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

/* over timeout */
int conn_over_timeout(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        if(conn->evtimer && conn->timeout > 0 && conn->evid >= 0)
        {
            EVTIMER_DEL(conn->evtimer, conn->evid);
        }
        conn->timeout = 0;
        conn->evid = -1;
        ret = 0;
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

    if(conn && conn->evid >= 0)
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
            return 0;
        }
        else
        {
            DEBUG_LOGGER(conn->logger, "TIMEOUT[%d]-close connection[%p] on remote[%s:%d] "
                    "local[%s:%d] via %d", conn->timeout, conn, conn->remote_ip, conn->remote_port, 
                    conn->local_ip, conn->local_port, conn->fd);
            conn_shut(conn, D_STATE_CLOSE, E_STATE_OFF);
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
            while((cp = (CHUNK *)xqueue_pop(conn->queue, conn->qid)))
            {
                if(PPARENT(conn) && PPARENT(conn)->service)
                {
                    PPARENT(conn)->service->pushchunk(PPARENT(conn)->service, cp);
                }
                cp  = NULL;
            }
            MMB_RESET(conn->packet);
            MMB_RESET(conn->cache);
            MMB_RESET(conn->buffer);
            MMB_RESET(conn->oob);
            MMB_RESET(conn->exchange);
            chunk_reset(&conn->chunk);
            ret = 0;
        }
    }
    return ret;
}

/* start error wait state */
int conn_wait_estate(CONN *conn)
{
    int ret = -1;
    /* Check connection and transaction state */
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        if(conn->e_state == E_STATE_OFF)
        {
            conn->e_state = E_STATE_ON;
            ret = 0;
        }
    }
    return ret;
}

/* over error wait state */
int conn_over_estate(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        conn->e_state = E_STATE_OFF;
        ret = 0;
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
int conn__push__message(CONN *conn, int message_id)
{
    PROCTHREAD *parent = NULL;
    int ret = -1;

    if(conn && conn->message_queue && (message_id & MESSAGE_ALL) )
    {
        if((parent = (PROCTHREAD *)conn->parent))
        {
            DEBUG_LOGGER(conn->logger, "Ready for pushing message[%s] to ioqmessage[%p] "
                    "on conn[%s:%d] local[%s:%d] via %d total %d handler[%p] parent[%p]",
                    MESSAGE_DESC(message_id), PPL(conn->message_queue), conn->remote_ip, 
                    conn->remote_port, conn->local_ip, conn->local_port, 
                    conn->fd, QMTOTAL(conn->message_queue),
                    PPL(conn), parent);
            qmessage_push(conn->message_queue, message_id, conn->index, conn->fd, -1, parent, conn, NULL);
            if(parent->use_cond_wait){MUTEX_SIGNAL(parent->mutex);}
        }
        ret = 0;
    }
    return ret;
}

/* push message to message queue */
int conn_push_message(CONN *conn, int message_id)
{
    PROCTHREAD *parent = NULL;
    int ret = -1;

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn && conn->message_queue && (message_id & MESSAGE_ALL) )
    {
        if((parent = (PROCTHREAD *)conn->parent))
        {
            /*
            DEBUG_LOGGER(conn->logger, "Ready for pushing message[%s] to message_queue[%p] "
                    "on conn[%s:%d] local[%s:%d] via %d total %d handler[%p] parent[%p][%p]",
                    MESSAGE_DESC(message_id), PPL(conn->message_queue), conn->remote_ip, 
                    conn->remote_port, conn->local_ip, conn->local_port, 
                    conn->fd, QMTOTAL(conn->message_queue),
                    PPL(conn), parent, (void *)(parent->threadid));
            */
            qmessage_push(conn->message_queue, message_id, conn->index, conn->fd, 
                    -1, parent, conn, NULL);
            if(parent->use_cond_wait){MUTEX_SIGNAL(parent->mutex);}
            /*
            DEBUG_LOGGER(conn->logger, "over for pushing message[%s] to message_queue[%p] "
                    "on conn[%s:%d] local[%s:%d] via %d total %d handler[%p] parent[%p][%p]",
                    MESSAGE_DESC(message_id), PPL(conn->message_queue), conn->remote_ip, 
                    conn->remote_port, conn->local_ip, conn->local_port, conn->fd, 
                    QMTOTAL(conn->message_queue), PPL(conn), parent, (void *)(parent->threadid));
            */
            ret = 0;
        }
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
        DEBUG_LOGGER(conn->logger, "Ready for reading conn[%p] buffer[%d/%d] packet[%d/%d] oob[%d/%d] cache[%d/%d] exchange[%d/%d] chunk[%lld/%d] remote[%s:%d] local[%s:%d] via %d", conn, MMB_LEFT(conn->buffer), MMB_SIZE(conn->buffer), MMB_LEFT(conn->packet), MMB_SIZE(conn->packet), MMB_LEFT(conn->oob), MMB_SIZE(conn->oob), MMB_LEFT(conn->cache), MMB_SIZE(conn->cache), MMB_LEFT(conn->exchange), MMB_SIZE(conn->exchange), LL(CHK_SIZE(conn->chunk)), CHK_BSIZE(conn->chunk), conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* Receive OOB */
        /*
        if((n = MMB_RECV(conn->oob, conn->fd, MSG_OOB)) > 0)
        {
            conn->recv_oob_total += n;
            DEBUG_LOGGER(conn->logger, "Received %d bytes OOB total %lld "
                    "from %s:%d on %s:%d via %d", n, conn->recv_oob_total, 
                    conn->remote_ip, conn->remote_port, conn->local_ip, 
                    conn->local_port, conn->fd);
            if((n = conn->oob_handler(conn)) > 0)
            {
                MMB_DELETE(conn->oob, n);
            }
            // CONN TIMER sample 
            return ret = 0;
        }
        */
        /* Receive to chunk with chunk_read_state before reading to buffer */
        if(conn->s_state == S_STATE_READ_CHUNK
                && conn->session.packet_type != PACKET_PROXY
                && CHK_LEFT(conn->chunk) > 0)
        {
            if(conn->buffer.ndata > 0) ret = conn__read__chunk(conn);
            if(conn->buffer.ndata <= 0){CONN_CHUNK_READ(conn, n);}
            return ret = 0;
            //goto end;
        }
        /* Receive normal data */
        if(conn->ssl) 
        {
            n = MMB_READ_SSL(conn->buffer, conn->ssl);
        }
	    else 
        {
            n = MMB_READ(conn->buffer, conn->fd);
        }
        if(n < 1)
        {
            WARN_LOGGER(conn->logger, "Reading data %d bytes ptr:%p buffer-left:%d qleft:%d "
                    "from %s:%d on %s:%d via %d failed, %s",
                    n, MMB_END(conn->buffer), MMB_LEFT(conn->buffer), 
                    xqueue_total(conn->queue, conn->qid), conn->remote_ip, 
                    conn->remote_port, conn->local_ip, conn->local_port, 
                    conn->fd, strerror(errno));
            // Terminate connection 
            conn_shut(conn, D_STATE_CLOSE|D_STATE_RCLOSE|D_STATE_WCLOSE, E_STATE_ON);
            return ret;
        }
        else
        {
            conn->recv_data_total += n;
        }
        DEBUG_LOGGER(conn->logger, "Received %d bytes nbuffer:%d/%d  left:%d data total %lld "
                "from %s:%d on %s:%d via %d", n, conn->buffer.ndata, MMB_SIZE(conn->buffer),
                MMB_LEFT(conn->buffer), conn->recv_data_total, conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        if(conn->session.packet_type & PACKET_PROXY)
        {
            ret = conn->proxy_handler(conn);
            if(conn->session.packet_type == PACKET_PROXY) return ret;
        }
        if(conn->s_state == 0 && conn->packet.ndata == 0)
            conn->packet_reader(conn);
        ret = 0;
    }
    return ret;
}

/* write handler */
int conn_write_handler(CONN *conn)
{
    int ret = -1, n = 0, chunk_over = 0;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE), ret);

    if(conn && conn->queue && xqueue_total(conn->queue, conn->qid) > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for send-data to %s:%d on %s:%d via %d "
                "qtotal:%d d_state:%d i_state:%d", conn->remote_ip, conn->remote_port,
                conn->local_ip, conn->local_port, conn->fd, xqueue_total(conn->queue, conn->qid),
                conn->d_state, conn->i_state);
        if((cp = (CHUNK *)xqueue_head(conn->queue, conn->qid)))
        {
            if(CHUNK_STATUS(cp) == CHUNK_STATUS_OVER)
            {
                chunk_over = 1; 
            }
            else 
            {
                if((n = conn_write_chunk(conn, cp)) > 0)
                {
                    conn->sent_data_total += n;
                    DEBUG_LOGGER(conn->logger, "Sent %d byte(s) (total sent %lld) "
                            "to %s:%d on %s:%d via %d leave %lld qtotal:%d", 
                            n, conn->sent_data_total, conn->remote_ip, conn->remote_port, 
                            conn->local_ip, conn->local_port, conn->fd, LL(CHK(cp)->left),
                            xqueue_total(conn->queue, conn->qid));
                }
                else
                {
                    WARN_LOGGER(conn->logger, "write %d byte(s) (total sent %lld) "
                            "to %s:%d on %s:%d via %d leave %lld qtotal:%d failed, %s", n, 
                            conn->sent_data_total, conn->remote_ip, conn->remote_port, 
                            conn->local_ip, conn->local_port, conn->fd, LL(CHK(cp)->left), 
                            xqueue_total(conn->queue, conn->qid), strerror(errno));
#ifdef HAVE_SSL
                    if(conn->ssl) ERR_print_errors_fp(stdout);
#endif
                    /* Terminate connection */
                    conn_shut(conn, D_STATE_CLOSE, E_STATE_ON);
                    return ret;
                }
            }
            /* CONN TIMER sample */
            if(CHUNK_STATUS(cp) == CHUNK_STATUS_OVER)
            {
                if((cp = (CHUNK *)xqueue_pop(conn->queue, conn->qid)))
                {
                    DEBUG_LOGGER(conn->logger, "Completed chunk[%p] to %s:%d "
                            "on %s:%d via %d clean it leave %d", PPL(cp), 
                            conn->remote_ip, conn->remote_port, conn->local_ip,
                            conn->local_port, conn->fd, xqueue_total(conn->queue, conn->qid));
                    if(cp && PPARENT(conn) && PPARENT(conn)->service)
                    {
                        PPARENT(conn)->service->pushchunk(PPARENT(conn)->service, cp);
                    }
                    cp  = NULL;
                }
            }
            if(chunk_over)
            {
                conn_shut(conn, D_STATE_CLOSE, E_STATE_OFF);
                ret = -1;
            }
            else
            {
                if(xqueue_total(conn->queue, conn->qid) < 1) 
                {
                    //event_del(&conn->event, E_WRITE);
                    CONN_PUSH_MESSAGE(conn, MESSAGE_END);
                }
                ret = 0;
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

    if(conn && conn->s_state == 0)
    {
        data = PCB(conn->buffer);
        packet_type = conn->session.packet_type;
        DEBUG_LOGGER(conn->logger, "Reading packet type[%d] buffer[%d]", 
                packet_type, MMB_NDATA(conn->buffer));
        //int x = 0, n = 0; char buf[1024], *s = "sdklhafkllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllhflkdfklasdjfkldsakfldsalkfkasdfjksdjfkdasjfklasdjfklsdjfklsjdkfljdssssssssssssssssssssssssssssssssssssssssldkfjsakldjflkajsdfkljadkfjkldajfkljd";x = strlen(s);n = sprintf(buf, "HTTP/1.0 200 OK\r\nContent-Length:%d\r\n\r\n", x);conn->push_chunk(conn, buf, n);conn->push_chunk(conn, s, x);return 0;
        /* Remove invalid packet type */
        if(!(packet_type & PACKET_ALL))
        {
            FATAL_LOGGER(conn->logger, "Unkown packet_type[%d] from %s:%d on conn[%p] %s:%d via %d",
                    packet_type, conn->remote_ip, conn->remote_port, conn, conn->local_ip,
                    conn->local_port, conn->fd);
            /* Terminate connection */
            conn_shut(conn, D_STATE_CLOSE, E_STATE_ON);
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
                && MMB_NDATA(conn->buffer) >= conn->session.packet_length)
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
            p = MMB_DATA(conn->buffer);
            e = MMB_END(conn->buffer);
            while(p < e && i < conn->session.packet_delimiter_length )
            {
                if(((char *)conn->session.packet_delimiter)[i++] != *p++) i = 0;
                if(i == conn->session.packet_delimiter_length)
                {
                    len = p - MMB_DATA(conn->buffer);
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
            MMB_RESET(conn->packet);
            MMB_PUSH(conn->packet, MMB_DATA(conn->buffer), len);
            MMB_DELETE(conn->buffer, len);
            /* For packet handling */
            conn->s_state = S_STATE_PACKET_HANDLING;
            CONN_PUSH_MESSAGE(conn, MESSAGE_PACKET);
            DEBUG_LOGGER(conn->logger, "Got packet length[%d/%d]", len, MMB_SIZE(conn->packet));
        }
    }
    return len;
}

/* packet handler */
int conn_packet_handler(CONN *conn)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn && conn->session.packet_handler)
    {
        DEBUG_LOGGER(conn->logger, "packet_handler(%p) on %s:%d local[%s:%d] via %d", conn->session.packet_handler, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        ret = conn->session.packet_handler(conn, PCB(conn->packet));
        DEBUG_LOGGER(conn->logger, "over packet_handler(%p) on %s:%d local[%s:%d] via %d", conn->session.packet_handler, conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);

        if(conn->s_state == S_STATE_PACKET_HANDLING)
        {
            DEBUG_LOGGER(conn->logger, "Reset packet_handler(%p) buffer:[%d/%d] on %s:%d via %d", conn->session.packet_handler, MMB_LEFT(conn->buffer), MMB_SIZE(conn->buffer), conn->remote_ip, conn->remote_port, conn->fd);
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
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

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
            //fprintf(stdout, "service[%s]->session.data_handler:%p\n", PPARENT(conn)->service->service_name, conn->session.data_handler);
            ret = conn->session.data_handler(conn, PCB(conn->packet), 
                    PCB(conn->cache), PCB(conn->chunk));
            DEBUG_LOGGER(conn->logger, "over data_handler(%p) on %s:%d via %d", 
                    conn->session.data_handler, conn->remote_ip, conn->remote_port, conn->fd);
            if(xqueue_total(conn->queue, conn->qid) > 0){CONN_EVENT_ADD(conn, E_WRITE);}
        }
        //reset session
        if(conn->s_state == S_STATE_DATA_HANDLING)
        {
            SESSION_RESET(conn);
            DEBUG_LOGGER(conn->logger, "Reset data_handler(%p) buffer:%d on %s:%d via %d", conn->session.packet_handler, conn->buffer.ndata, conn->remote_ip, conn->remote_port, conn->fd);
        }
    }
    return ret;
}

/* bind proxy */
int conn_bind_proxy(CONN *conn, CONN *child)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn && child)
    {
        conn->session.packet_type |= PACKET_PROXY;
        conn->session.childid = child->index;
        conn->session.child = child;
        DEBUG_LOGGER(conn->logger, "Bind proxy connection[%s:%d] to connection[%s:%d]",
                conn->remote_ip, conn->remote_port, child->remote_ip, child->remote_port);
        CONN_PUSH_MESSAGE(conn, MESSAGE_PROXY);
        ret = 0;
    }
    return ret;
}

/* proxy data handler */
int conn_proxy_handler(CONN *conn)
{
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
        if((exchange = PCB(conn->exchange)) && exchange->ndata > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready exchange packet[%d] to conn[%s:%d]", 
                    exchange->ndata, oconn->remote_ip, oconn->remote_port);
            oconn->push_chunk(oconn, exchange->data, exchange->ndata);
            MMB_RESET(conn->exchange);
        }
        if((chunk = PCB(conn->chunk)) && chunk->ndata > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready exchange chunk[%d] to conn[%s:%d]", 
                    chunk->ndata, oconn->remote_ip, oconn->remote_port);
            oconn->push_chunk(oconn, chunk->data, chunk->ndata);
            chunk_reset(&conn->chunk);
        }
        if(conn->session.packet_type == PACKET_PROXY 
                && (buffer = PCB(conn->buffer)) && buffer->ndata > 0)
        {
            DEBUG_LOGGER(conn->logger, "Ready exchange buffer[%d] to conn[%s:%d]", 
                    buffer->ndata, oconn->remote_ip, oconn->remote_port);
            oconn->push_chunk(oconn, buffer->data, buffer->ndata);
            MMB_DELETE(conn->buffer, buffer->ndata);
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
        MMB_PUSH(conn->exchange, data, size);
        DEBUG_LOGGER(conn->logger, "Push exchange size[%d] remote[%s:%d] local[%s:%d] via %d",
                MMB_NDATA(conn->exchange), conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* save cache to connection  */
int conn_save_cache(CONN *conn, void *data, int size)
{
    int ret = -1;
    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        MMB_RESET(conn->cache);
        MMB_PUSH(conn->cache, data, size);
        DEBUG_LOGGER(conn->logger, "Saved cache size[%d] remote[%s:%d] local[%s:%d] via %d",
                MMB_NDATA(conn->cache), conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* reading chunk  */
int conn__read__chunk(CONN *conn)
{
    int ret = -1, n = -1;
    //CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        if(conn->s_state == S_STATE_READ_CHUNK
                && conn->session.packet_type != PACKET_PROXY
                && CHK_LEFT(conn->chunk) > 0
                && MMB_NDATA(conn->buffer) > 0)
        {
            if((n = CHUNK_FILL(&conn->chunk, MMB_DATA(conn->buffer), MMB_NDATA(conn->buffer))) > 0)
            {
                MMB_DELETE(conn->buffer, n);
            }
            if(CHUNK_STATUS(&conn->chunk) == CHUNK_STATUS_OVER )
            {
                DEBUG_LOGGER(conn->logger, "Chunk completed %lld bytes from %s:%d on %s:%d via %d",
                        LL(CHK_SIZE(conn->chunk)), conn->remote_ip, conn->remote_port, 
                        conn->local_ip, conn->local_port, conn->fd);
                conn->s_state = S_STATE_DATA_HANDLING;
                CONN_PUSH_MESSAGE(conn, MESSAGE_DATA);
            }
            if(n > 0)
            {
                DEBUG_LOGGER(conn->logger, "Filled  %d byte(s) left:%lld to chunk from buffer "
                        "to %s:%d on conn[%s:%d] via %d", n, LL(CHK_LEFT(conn->chunk)),
                        conn->remote_ip, conn->remote_port, conn->local_ip, 
                        conn->local_port, conn->fd);
                ret = 0;
            }
        }
    }
    return ret;
}


/* chunk reader */
int conn_chunk_reader(CONN *conn)
{
    int ret = -1;
    //CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);

    if(conn)
    {
        ret = conn__read__chunk(conn);
    }
    return ret;
}

/* receive chunk */
int conn_recv_chunk(CONN *conn, int size)
{
    int ret = -1;

    if(conn && size > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for recv-chunk size:%d from %s:%d on %s:%d via %d",
                size, conn->remote_ip, conn->remote_port, 
                conn->local_ip, conn->local_port, conn->fd);
        chunk_mem(&conn->chunk, size);
        conn->s_state = S_STATE_READ_CHUNK;
        if(conn->d_state & D_STATE_CLOSE)
        {
            conn->chunk_reader(conn);
        }
        else
        {
            PUSH_IOQMESSAGE(conn, MESSAGE_CHUNK);
        }
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

    if(conn && conn->status == CONN_STATUS_FREE && conn->queue && data && size > 0)
    {
        //CHUNK_POP(conn, cp);
        if(PPARENT(conn) && PPARENT(conn)->service 
                && (cp = PPARENT(conn)->service->popchunk(PPARENT(conn)->service)))
        {
            chunk_mem(cp, size);
            chunk_mem_copy(cp, data, size);
            xqueue_push(conn->queue,conn->qid, cp);
            CONN_READY_WRITE(conn);
            DEBUG_LOGGER(conn->logger, "Pushed chunk size[%d/%d] to %s:%d queue[%d] "
                    "total:%d on %s:%d via %d", size, cp->bsize,conn->remote_ip, 
                    conn->remote_port, conn->qid, xqueue_total(conn->queue, conn->qid), 
                    conn->local_ip, conn->local_port, conn->fd);
            ret = 0;
        }
    }
    return ret;
}

/* receive chunk file */
int conn_recv_file(CONN *conn, char *filename, long long offset, long long size)
{
    int ret = -1;

    if(conn && filename && offset >= 0 && size > 0)
    {
        DEBUG_LOGGER(conn->logger, "Ready for recv-chunk file:%s offset:%lld size:%lld"
                " from %s:%d on %s:%d via %d", filename, LL(offset), LL(size), conn->remote_ip, 
                conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        chunk_file(&conn->chunk, filename, offset, size);
        chunk_set_bsize(&conn->chunk, conn->session.buffer_size);
        conn->s_state = S_STATE_READ_CHUNK;
        if(conn->d_state & D_STATE_CLOSE)
            conn->chunk_reader(conn);
        else
        {
            PUSH_IOQMESSAGE(conn, MESSAGE_CHUNK);
        }
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

    if(conn && conn->status == CONN_STATUS_FREE && conn->queue 
            && filename && offset >= 0 && size > 0)
    {
        //CHUNK_POP(conn, cp);
        if(PPARENT(conn) && PPARENT(conn)->service 
                && (cp = PPARENT(conn)->service->popchunk(PPARENT(conn)->service)))
        {
            chunk_file(cp, filename, offset, size);
            xqueue_push(conn->queue,conn->qid, cp);
            CONN_READY_WRITE(conn);
            DEBUG_LOGGER(conn->logger, "Pushed file[%s] [%lld][%lld] to %s:%d "
                    "queue total %d on %s:%d via %d ", filename, LL(offset), LL(size), 
                    conn->remote_ip, conn->remote_port, xqueue_total(conn->queue, conn->qid), 
                    conn->local_ip, conn->local_port, conn->fd);
            ret = 0;
        }else return ret;
    }
    return ret;
}

/* send chunk */
int conn_send_chunk(CONN *conn, CB_DATA *chunk, int len)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE|D_STATE_RCLOSE), ret);

    if(conn && (cp = (CHUNK *)chunk))
    {
        CHK(cp)->left = len;
        xqueue_push(conn->queue,conn->qid, cp);
        CONN_READY_WRITE(conn);
        DEBUG_LOGGER(conn->logger, "send chunk len[%d][%d] to %s:%d queue "
                "total %d on %s:%d via %d", len, CHK(cp)->bsize,conn->remote_ip,conn->remote_port, 
                xqueue_total(conn->queue, conn->qid), conn->local_ip, conn->local_port, conn->fd);
        ret = 0;
    }
    return ret;
}

/* over chunk and close connection */
int conn_over_chunk(CONN *conn)
{
    int ret = -1;
    CHUNK *cp = NULL;
    CONN_CHECK_RET(conn, (D_STATE_CLOSE|D_STATE_WCLOSE|D_STATE_RCLOSE), ret);

    if(conn && conn->status == CONN_STATUS_FREE && conn->queue)
    {
        if(PPARENT(conn) && PPARENT(conn)->service 
                && (cp = PPARENT(conn)->service->popchunk(PPARENT(conn)->service)))
        {
            xqueue_push(conn->queue,conn->qid, cp);
            CONN_READY_WRITE(conn);
            ret = 0;
        }
        else 
            return ret;
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

/* over session */
int conn_over_session(CONN *conn)
{
    int ret = -1;

    CONN_CHECK_RET(conn, D_STATE_CLOSE, -1);
    if(conn)
    {
        SESSION_RESET(conn);
        if(PPARENT(conn)->service->service_type == C_SERVICE)
            PPARENT(conn)->service->freeconn(PPARENT(conn)->service, conn);
        ret = 0;
    }
    return ret;
}

/* new task */
int conn_newtask(CONN *conn, CALLBACK *handler)
{
    if(conn && handler)
    {
        conn->s_state = S_STATE_TASKING;
        return PPARENT(conn)->service->newtask(PPARENT(conn)->service, handler, conn);
    }
    return -1;
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

/* reset xids */
void conn_reset_xids(CONN *conn)
{
    if(conn)
    {
        memset(conn->xids, 0, sizeof(int) * SB_XIDS_MAX);
        memset(conn->xids64, 0, sizeof(int64_t) * SB_XIDS_MAX);
    }
    return ;
}

/* reset stat */
void conn_reset_state(CONN *conn)
{
    if(conn)
    {
        conn->groupid = -1;
        conn->index = -1;
        conn->gindex = -1;
        conn->c_state = 0;
        conn->d_state = 0;
        conn->e_state = 0;
    }
    return ;
}

/* reset connection */
void conn_reset(CONN *conn)
{
    CHUNK *cp = NULL;

    if(conn)
    {
        DEBUG_LOGGER(conn->logger, "Reset connection[%p][%s:%d] local[%s:%d] via %d", conn,
                conn->remote_ip, conn->remote_port, conn->local_ip, conn->local_port, conn->fd);
        /* global */
        conn->groupid = -1;
        conn->index = -1;
        conn->gindex = -1;
        conn->qid = 0;
        conn->d_state = 0;
        conn->e_state = 0;
        memset(conn->xids, 0, sizeof(int) * SB_XIDS_MAX);
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
        conn->evid = -1;
        conn->evtimer = NULL;
        /* buffer and chunk */
        MMB_RESET(conn->buffer);
        MMB_RESET(conn->packet);
        MMB_RESET(conn->oob);
        MMB_RESET(conn->cache);
        MMB_RESET(conn->exchange);
        chunk_reset(&conn->chunk);
        /* timer, logger, message_queue and queue */
        conn->message_queue = NULL;
        conn->ioqmessage = NULL;
        if(conn->queue)
        {
            while((cp = (CHUNK *)xqueue_pop(conn->queue, conn->qid)))
            {
                if(PPARENT(conn) && PPARENT(conn)->service)
                {
                    PPARENT(conn)->service->pushchunk(PPARENT(conn)->service, cp);
                }
                else break;
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
void conn_clean(CONN *conn)
{
    if(conn)
    {
        MUTEX_DESTROY(conn->mutex);
        event_clean(&(conn->event));
        DEBUG_LOGGER(conn->logger, "ready-clean conn[%p]", conn);
        /* Clean BUFFER */
        MMB_DESTROY(conn->buffer);
        /* Clean OOB */
        MMB_DESTROY(conn->oob);
        /* Clean cache */
        MMB_DESTROY(conn->cache);
        /* Clean packet */
        MMB_DESTROY(conn->packet);
        /* Clean exchange */
        MMB_DESTROY(conn->exchange);
        /* Clean chunk */
        chunk_destroy(&(conn->chunk));
#ifdef HAVE_SSL
        if(conn->ssl)
        {
            SSL_shutdown(XSSL(conn->ssl));
            SSL_free(XSSL(conn->ssl));
            conn->ssl = NULL;
        }
#endif
        DEBUG_LOGGER(conn->logger, "over-clean conn[%p]", conn);
        xmm_free(conn, sizeof(CONN));
    }
    return ;
}

/* Initialize connection */
CONN *conn_init()
{
    CONN *conn = NULL;

    if((conn = (CONN *)xmm_mnew(sizeof(CONN))))
    {
        conn->groupid = -1;
        conn->index = -1;
        conn->gindex = -1;
        MUTEX_RESET(conn->mutex);
        conn->set                   = conn_set;
        conn->get_service_id        = conn_get_service_id;
        conn->close                 = conn_close;
        conn->over                  = conn_over;
        conn->terminate             = conn_terminate;
        conn->start_cstate          = conn_start_cstate;
        conn->over_cstate           = conn_over_cstate;
        conn->wait_estate          = conn_wait_estate;
        conn->over_estate           = conn_over_estate;
        conn->set_timeout           = conn_set_timeout;
        conn->over_timeout          = conn_over_timeout;
        conn->timeout_handler       = conn_timeout_handler;
        conn->wait_evstate          = conn_wait_evstate;
        conn->over_evstate          = conn_over_evstate;
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
        conn->send_chunk            = conn_send_chunk;
        conn->over_chunk            = conn_over_chunk;
        conn->buffer_handler        = conn_buffer_handler;
        conn->chunk_handler         = conn_chunk_handler;
        conn->end_handler           = conn_end_handler;
        conn->shut_handler          = conn_shut_handler;
        conn->set_session           = conn_set_session;
        conn->over_session          = conn_over_session;
        conn->newtask               = conn_newtask;
        conn->reset_xids            = conn_reset_xids;
        conn->reset_state           = conn_reset_state;
        conn->reset                 = conn_reset;
        conn->clean                 = conn_clean;
    }
    return conn;
}
