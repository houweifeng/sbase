#include <stdarg.h>
#include <evbase.h>
#include <netinet/tcp.h>
#include "timer.h"
#include "queue.h"
#include "buffer.h"
#include "message.h"
#include "conn.h"
#include "chunk.h"
#include "logger.h"
#define CONN_CHECK_RET(conn, ret)                                                       \
{                                                                                       \
        if(conn == NULL ) return ret;                                                   \
        if(conn->s_state == S_STATE_CLOSE) return ret;                                  \
}
#define CONN_CHECK(conn)                                                                \
{                                                                                       \
        if(conn == NULL) return ;                                                       \
        if(conn->s_state == S_STATE_CLOSE) return ;                                     \
}
#define CONN_TERMINATE(conn)                                                            \
{                                                                                       \
	if(conn)                                                                            \
	{                                                                                   \
		conn->s_state = S_STATE_CLOSE;                                                  \
		conn->push_message(conn, MESSAGE_QUIT);                                         \
	}                                                                                   \
}
/* Initialize CONN */
CONN *conn_init(char *ip, int port)
{
	CONN *conn = (CONN *)calloc(1, sizeof(CONN));		
	if(port <= 0 ) goto ERROR;
	if(conn)
	{
        TIMER_INIT(conn->timer);
		conn->buffer	            = buffer_init();
		conn->oob		            = buffer_init();
		conn->cache		            = buffer_init();
		conn->packet		        = buffer_init();
		conn->chunk		            = chunk_init();
		conn->send_queue 	        = queue_init();
		conn->event		            = ev_init();
		conn->set		            = conn_set;
		conn->event_handler 	    = conn_event_handler;
		conn->state_handler 	    = conn_state_handler;
		conn->read_handler	        = conn_read_handler;
		conn->write_handler	        = conn_write_handler;
		conn->packet_reader	        = conn_packet_reader;
		conn->packet_handler	    = conn_packet_handler;
		conn->chunk_reader	        = conn_chunk_reader;
		conn->recv_chunk	        = conn_recv_chunk;
		conn->recv_file		        = conn_recv_file;
		conn->push_chunk	        = conn_push_chunk;
		conn->push_file	            = conn_push_file;
		conn->data_handler	        = conn_data_handler;
		conn->transaction_handler	= conn_transaction_handler;
		conn->push_message	        = conn_push_message;
        conn->set_timeout           = conn_set_timeout;
        conn->start_cstate          = conn_start_cstate;
        conn->over_cstate           = conn_over_cstate;
		conn->over 	    	        = conn_over;
		conn->close 	    	    = conn_close;
		conn->terminate 	        = conn_terminate;
		conn->clean 		        = conn_clean;
		strcpy(conn->ip, ip);
		conn->port  = port; 
		conn->sa.sin_family = AF_INET;
		if(conn->ip == NULL)
			conn->sa.sin_addr.s_addr     = INADDR_ANY;
		else 
			conn->sa.sin_addr.s_addr     = inet_addr(ip);
		    conn->sa.sin_port            = htons(conn->port);
	}
	return conn;
ERROR:
	{
		if(conn) free(conn);
		conn = NULL;
		return NULL;	
	}
}

/* Initialize setting  */
int conn_set(CONN *conn)
{
	/*
	int keep_alive = 1;//设定KeepAlive
        int keep_idle = 1;//开始首次KeepAlive探测前的TCP空闭时间
        int keep_interval = 1;//两次KeepAlive探测间的时间间隔
        int keep_count = 3;//判定断开前的KeepAlive探测次数
	*/
	if(conn && conn->fd > 0 )
	{
		fcntl(conn->fd, F_SETFL, O_NONBLOCK);
		/* set keepalive */
		/*
		setsockopt(conn->fd, SOL_SOCKET, SO_KEEPALIVE,
				(void*)&keep_alive, sizeof(keep_alive));
		setsockopt(conn->fd, SOL_TCP, TCP_KEEPIDLE,
				(void *)&keep_idle,sizeof(keep_idle));
		setsockopt(conn->fd,SOL_TCP,TCP_KEEPINTVL,
				(void *)&keep_interval, sizeof(keep_interval));
		setsockopt(conn->fd,SOL_TCP,TCP_KEEPCNT,
				(void *)&keep_count,sizeof(keep_count));
		*/
		if(conn->evbase && conn->event)
		{
			conn->event->set(conn->event, conn->fd, E_READ|E_PERSIST, (void *)conn, conn->event_handler);
			conn->evbase->add(conn->evbase, conn->event);
			return 0;
		}
		else
		{
			FATAL_LOGGER(conn->logger, "Connection[%08x] fd[%d] EVBASE or Initialize event failed, %s",
					conn, conn->fd, strerror(errno));	
			/* Terminate connection */
			CONN_TERMINATE(conn);
		}
	}	
	return -1;	
}

/* Event handler */
void conn_event_handler(int event_fd, short event, void *arg)
{
    CONN *conn = (CONN *)arg;
    if(conn)
    {
        if(event_fd == conn->fd)
        {
            if(event & E_CLOSE)
            {
                DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d START ", E_CLOSE, event_fd);
                conn->push_message(conn, MESSAGE_QUIT);
                DEBUG_LOGGER(conn->logger, "E_CLOSE:%d on %d OVER ", E_CLOSE, event_fd);
            }
            if(event & E_READ)
            {
                DEBUG_LOGGER(conn->logger, "E_READ:%d on %d START", E_READ, event_fd);
                conn->read_handler(conn);
                DEBUG_LOGGER(conn->logger, "E_READ:%d on %d OVER ", E_READ, event_fd);
            }
            if(event & E_WRITE)
            {
                DEBUG_LOGGER(conn->logger, "E_WRITE:%d on %d START", E_WRITE, event_fd);
                conn->write_handler(conn);
                DEBUG_LOGGER(conn->logger, "E_WRITE:%d on %d OVER", E_WRITE, event_fd);
            } 
        }	
    }
}

/* Check connection state  TIMEOUT */
void conn_state_handler(CONN *conn)
{
    /* Check connection and transaction state  */
    CONN_CHECK(conn);
    int check = -1;
    void *ptr = NULL;
    long long usec = 0ll;
    if(conn)
    {
        if(conn->timeout > 0 && conn->timer && TIMER_CHECK(conn->timer, conn->timeout) == 0)
        {
                WARN_LOGGER(conn->logger, "Connection[%d] from %s:%d TIMEOUT",
                        conn->fd, conn->ip, conn->port);
                CONN_TERMINATE(conn);
        }
    }
}

/* Read handler */
void conn_read_handler(CONN *conn)
{
	int n = 0;
	BUFFER *buf = NULL ;
	char *tmp = NULL;
	int nbuf = SB_BUF_SIZE;
		
	/* Check connection and transaction state  */
	CONN_CHECK(conn);
	if(conn)
	{
		if(conn->buffer_size)
			nbuf = conn->buffer_size;
		if((buf = buffer_init()) == NULL 
			|| ((tmp = (char *)buf->calloc(buf, nbuf)) == NULL) ) goto end;
		/* Receive OOB */
		if((n = recv(conn->fd, tmp, nbuf, MSG_OOB)) > 0 )	
		{
			conn->recv_oob_total += n;
			DEBUG_LOGGER(conn->logger, "Received %d bytes OOB total %lld from %s:%d via %d",
				n, conn->recv_oob_total, conn->ip, conn->port, conn->fd);
			conn->oob->push(conn->oob, buf->data, n);	
			conn->oob_handler(conn);
			/* CONN TIMER sample */
			TIMER_SAMPLE(conn->timer);
			goto end;
		}
		/* Receive normal data */
		if( (n = read(conn->fd, tmp, nbuf)) <= 0 )
		{
			FATAL_LOGGER(conn->logger, "Reading %d bytes data from %s:%d via %d failed, %s",
				n, conn->ip, conn->port, conn->fd, strerror(errno));
			/* Terminate connection */
                        CONN_TERMINATE(conn);
			goto end;
		}
		/* CONN TIMER sample */
		TIMER_SAMPLE(conn->timer);
		/* Push to buffer */
		conn->buffer->push(conn->buffer, tmp, n);
		conn->recv_data_total += n;	
		DEBUG_LOGGER(conn->logger, "Received %d bytes data total %lld from %s:%d via %d",
                                n, conn->recv_data_total, conn->ip, conn->port, conn->fd);
		/* Handle buffer with packet_reader OR chunk_reader (with high priority) */
		if(conn->s_state == S_STATE_READ_CHUNK)
			conn->chunk_reader(conn);
		else
			conn->packet_reader(conn);			
	}
	end:
	{
		if(buf) buf->clean(&buf);
	}
	return ;
}

/* Write hanler */
void conn_write_handler(CONN *conn)
{
	int n = 0;
	CHUNK *cp = NULL;

	/* Check connection and transaction state */
	CONN_CHECK(conn);

	if(conn && conn->send_queue)
	{
		cp = (CHUNK *)HEAD_QUEUE(conn->send_queue);
		if(cp)
		{
			if((n = cp->send(cp, conn->fd, conn->buffer_size)) > 0)
			{
				conn->sent_data_total += n * 1ll;
				DEBUG_LOGGER(conn->logger, "Sent %d byte(s) (total sent %lld) "
						"to %s:%d via %d leave %lld",
						n, conn->sent_data_total, conn->ip, 
						conn->port, conn->fd, cp->len);
				/* CONN TIMER sample */
		        TIMER_SAMPLE(conn->timer);
				if(cp->len <= 0ll )
				{
					cp = (CHUNK *)POP_QUEUE(conn->send_queue);	
					DEBUG_LOGGER(conn->logger, "Completed chunk[%08x] and clean it leave %d", 
                            cp, TOTAL_QUEUE(conn->send_queue));
					if(cp) cp->clean(&cp);
				}
			}
			else
			{
				FATAL_LOGGER(conn->logger, "Sending data to %s:%d via %d failed, %s",
						conn->ip, conn->port, conn->fd, strerror(errno));
				/* Terminate connection */
				CONN_TERMINATE(conn);
			}
		}
		if(TOTAL_QUEUE(conn->send_queue) <= 0)
		{
			conn->event->del(conn->event, E_WRITE);	
		}
	}		
}

/* Packet reader */
int conn_packet_reader(CONN *conn)
{
	int len = 0, i = 0;
	char *p = NULL, *e = NULL;

        /* Check connection and transaction state */
        CONN_CHECK_RET(conn, -1);
	if(conn)
	{
		DEBUG_LOGGER(conn->logger, "Reading packet type[%d]", conn->packet_type);
		/* Remove invalid packet type */
		if(!(conn->packet_type & PACKET_ALL))
		{
			FATAL_LOGGER(conn->logger, "Unkown packet_type[%d] on %s:%d via %d",
					conn->packet_type, conn->ip, conn->port, conn->fd);
			/* Terminate connection */
			CONN_TERMINATE(conn);
		}
		/* Read packet with customized function from user */
		if(conn->packet_type == PACKET_CUSTOMIZED && conn->cb_packet_reader)
		{
			len = conn->cb_packet_reader(conn,  conn->buffer);	
			DEBUG_LOGGER(conn->logger, 
					"Reading packet with customized function[%08x] length[%d] on %s:%d via %d",
					conn->cb_packet_reader, len, conn->ip, conn->port, conn->fd);
			goto end;
		}
		/* Read packet with certain length */
		if(conn->packet_type == PACKET_CERTAIN_LENGTH && conn->buffer->size >= conn->packet_length)
		{
			len = conn->packet_length;
			DEBUG_LOGGER(conn->logger, "Reading packet with certain length[%d] on %s:%d via %d",
					len, conn->ip, conn->port, conn->fd);
			goto end;
		}
		/* Read packet with delimiter */
		if(conn->packet_type == PACKET_DELIMITER && conn->packet_delimiter && conn->packet_delimiter_length > 0)
		{
			p = (char *) conn->buffer->data;
			e = (char *) conn->buffer->end;
			i = 0;
			while(p < e && i < conn->packet_delimiter_length )
			{
				if(((char *)conn->packet_delimiter)[i++] != *p++) i = 0;
				if(i == conn->packet_delimiter_length)
				{
					len = p - (char *) conn->buffer->data;
					break;
				}
			}
			DEBUG_LOGGER(conn->logger, "Reading packet with delimiter[%d] length[%d] on %s:%d via %d",
					conn->packet_delimiter_length, len, conn->ip, conn->port, conn->fd);

			goto end;
		}
		return ;
		end:
		/* Copy data to packet from buffer */
		if(len > 0)
		{
			conn->packet->reset(conn->packet);
			conn->packet->push(conn->packet, conn->buffer->data, len);
			conn->buffer->del(conn->buffer, len);
			/* For packet handling */
			conn->s_state = S_STATE_PACKET_HANDLING;
			conn->push_message(conn, MESSAGE_PACKET);
		}
	}	
}

/* Packet Handler */
void conn_packet_handler(CONN *conn)
{
	/* Check connection and transaction state */
        CONN_CHECK(conn);

	if(conn && conn->cb_packet_handler )
	{
		DEBUG_LOGGER(conn->logger, "Handling packet with customized function[%08x] on %s:%d via %d",
			conn->cb_packet_handler,  conn->ip, conn->port, conn->fd);
		return conn->cb_packet_handler(conn, conn->packet);
	}
}

/* CHUNK reader */
void conn_chunk_reader(CONN *conn)
{
	int n = 0;

	/* Check connection and transaction state */
    CONN_CHECK(conn);

	if(conn && conn->chunk && conn->buffer)
	{
		if(conn->buffer->size > 0 )
		{
			if((n = conn->chunk->fill(conn->chunk,
			 	conn->buffer->data, conn->buffer->size) ) > 0 )
			{
				conn->buffer->del(conn->buffer, n);
				DEBUG_LOGGER(conn->logger, "Filled  %d byte(s) to CHUNK on conn[%s:%d] via %d",
					n, conn->ip, conn->port, conn->fd);
				if(conn->chunk->len <= 0 )
				{
					conn->s_state = S_STATE_DATA_HANDLING;
					conn->push_message(conn, MESSAGE_DATA);
				}
			}
		}	
	}	
}

/* Receive CHUNK */
void conn_recv_chunk(CONN *conn, size_t size)
{
        /* Check connection and transaction state */
        CONN_CHECK(conn);

	if(conn && conn->chunk)
	{
		conn->chunk->set(conn->chunk, conn->s_id, MEM_CHUNK, NULL, 0, size);
		conn->s_state = S_STATE_READ_CHUNK;
		conn->chunk_reader(conn);
	}			
}

/* Receive FILE CHUNK */
void conn_recv_file(CONN *conn, char *filename,
         long long  offset, long long  size)
{
        /* Check connection and transaction state */
        CONN_CHECK(conn);

        if(conn && conn->chunk)
        {
            conn->chunk->set(conn->chunk, conn->s_id, FILE_CHUNK, filename, offset, size);
            conn->s_state = S_STATE_READ_CHUNK;
            conn->chunk_reader(conn);
        } 
}

/* Push Chunk */
int conn_push_chunk(CONN *conn, void *data, size_t size)
{
	CHUNK *cp = NULL;
    /* Check connection and transaction state */
    CONN_CHECK_RET(conn, -1);

	if(conn && conn->send_queue && data)
	{
		if((cp = (CHUNK *)TAIL_QUEUE(conn->send_queue)) && cp->type == MEM_CHUNK)
		{
			cp->append(cp, data, size);
		}
		else
		{
			cp = chunk_init();
			cp->set(cp, conn->s_id, MEM_CHUNK, NULL, 0ll, 0ll);
			cp->append(cp, data, size);
			PUSH_QUEUE(conn->send_queue, (void *)cp);
		}
		if(TOTAL_QUEUE(conn->send_queue) > 0 ) 
			conn->event->add(conn->event, E_WRITE);	
	}	
}

/* Push File */
int conn_push_file(CONN *conn, char *filename,
         long long  offset, long long  size)
{
	CHUNK *cp = NULL;
        /* Check connection and transaction state */
        CONN_CHECK_RET(conn, -1);

	if(conn && conn->send_queue && filename && offset >= 0 && size > 0 )
	{
		cp = chunk_init();
		cp->set(cp, conn->s_id, FILE_CHUNK, filename, offset, size);
		PUSH_QUEUE(conn->send_queue, (void *)cp);
		if((TOTAL_QUEUE(conn->send_queue)) > 0 ) 
			conn->event->add(conn->event, E_WRITE);	
		DEBUG_LOGGER(conn->logger, 
			"Pushed file\"%s\" [%lld][%lld] to send_queue (total %d) on %s:%d via %d\n",
			filename, offset, size, TOTAL_QUEUE(conn->send_queue), 
            conn->ip, conn->port, conn->fd);
	}
}

/* Data handler */
void conn_data_handler(CONN *conn)
{
    /* Check connection and transaction state */
    CONN_CHECK(conn);

	if(conn && conn->cb_data_handler )
	{
		DEBUG_LOGGER(conn->logger, "Handling data with customized function[%08x] on %s:%d via %d",
                        conn->cb_data_handler,  conn->ip, conn->port, conn->fd);
		conn->cb_data_handler(conn, conn->packet, conn->chunk, conn->cache);
	}	
}

/* Transaction handler */
void conn_transaction_handler(struct _CONN *conn, int tid)
{
    /* Check connection and transaction state */
    CONN_CHECK(conn);

    if(conn && conn->cb_transaction_handler )
    {
        DEBUG_LOGGER(conn->logger, "Handling transaction with customized "
                "function[%08x] on %s:%d via %d",
                conn->cb_transaction_handler,  
                conn->ip, conn->port, conn->fd);
        conn->cb_transaction_handler(conn, tid);
    }
    return ;
}

/* Push message */
void conn_push_message(CONN *conn, int message_id)
{
	MESSAGE *msg = NULL;
	if(conn)
	{
		if((message_id & MESSAGE_ALL) && conn->message_queue && (msg = MESSAGE_INIT()))
		{
			msg->msg_id = message_id;
			msg->fd	= conn->fd;
			msg->handler = (void *)conn;
			msg->parent  = (void *)conn->parent;
			PUSH_QUEUE(conn->message_queue, (void *)msg);
			DEBUG_LOGGER(conn->logger, "Pushed message[%s] to message_queue[%08x] "
                    "on %s:%d via %d", MESSAGE_DESC(message_id), conn->message_queue, 
                    conn->ip, conn->port, conn->fd);
		}	
		else
		{
			FATAL_LOGGER(conn->logger, "Initialize MESSAGE failed, %s", strerror(errno));
		}
	}
}

/* Set timeout */
void conn_set_timeout(CONN *conn, long long timeout)
{
    if(conn)
    {
        conn->timeout = timeout;
    }
}

/* Start cstate */
int conn_start_cstate(CONN *conn)
{
	CHUNK *cp = NULL;
    /* Check connection and transaction state */
    CONN_CHECK_RET(conn, -1);
    if(conn)
    {
        if(conn->c_state == C_STATE_FREE) 
        {
            conn->c_state = C_STATE_USING;
			while(TOTAL_QUEUE(conn->send_queue) > 0 )
			{
				cp = (CHUNK *)POP_QUEUE(conn->send_queue);
				if(cp) cp->clean(&(cp));
			}
			conn->packet->reset(conn->packet);	
			conn->cache->reset(conn->cache);	
			conn->buffer->reset(conn->buffer);	
			conn->oob->reset(conn->oob);	
			conn->chunk->reset(conn->chunk);	
            return 0;
        }
    }
    return -1;
}

/* Over cstate */
void conn_over_cstate(CONN *conn)
{
    if(conn)
    {
        conn->c_state = C_STATE_FREE;
        conn->callback_conn = NULL;
    }
}

/* Close connection */
void conn_close(CONN *conn)
{
	CONN_TERMINATE(conn);
}

/* Over connection */
void conn_over(CONN *conn)
{
    if(conn)
    {
        conn->push_message(conn, MESSAGE_QUIT);
    }
}

/* Terminate connection  */
void conn_terminate(CONN *conn)
{
    if(conn)
    {
        if(conn->c_state == C_STATE_USING && conn->cb_error_handler)
        {
            DEBUG_LOGGER(conn->logger, "error handler on %d cid:%d", conn->fd, conn->c_id);
            conn->cb_error_handler(conn); 
        }
        conn->event->destroy(conn->event);
        DEBUG_LOGGER(conn->logger, "terminateing connecion[%d] %s:%d", 
                conn->fd, conn->ip, conn->port);
        shutdown(conn->fd, SHUT_RDWR);
        close(conn->fd);
        conn->fd = -1;
    }
    return ;
}

/* Clean Connection */
void conn_clean(CONN **conn)
{
	CHUNK *cp = NULL;
	if((*conn))
	{
		/* Clean event */
		if((*conn)->event) (*conn)->event->clean(&((*conn)->event));
		/* Clean BUFFER */
		if((*conn)->buffer) (*conn)->buffer->clean(&((*conn)->buffer));
		/* Clean OOB */
		if((*conn)->oob) (*conn)->oob->clean(&((*conn)->oob));
		/* Clean cache */
		if((*conn)->cache) (*conn)->cache->clean(&((*conn)->cache));
		/* Clean packet */
		if((*conn)->packet) (*conn)->packet->clean(&((*conn)->packet));
		/* Clean chunk */
		if((*conn)->chunk) (*conn)->chunk->clean(&((*conn)->chunk));
        /* Clean timer */
        TIMER_CLEAN((*conn)->timer);
		/* Clean send queue */
		if((*conn)->send_queue)
        {
            while((cp = (CHUNK *)POP_QUEUE((*conn)->send_queue)))
            {
                if(cp) cp->clean(&(cp));
            }
            CLEAN_QUEUE((*conn)->send_queue);
        }
		free((*conn));
		(*conn) = NULL;
	}	
}
