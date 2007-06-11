#include <stdarg.h>
#include "timer.h"
#include "queue.h"
#include "buffer.h"
#include "message.h"
#include "conn.h"
#include "chunk.h"
/* Initialize CONN */
CONN *conn_init(char *ip, int port)
{
	CONN *conn = (CONN *)calloc(1, sizeof(CONN));		
	if(port <= 0 ) goto ERROR;
	if(conn)
	{
		conn->buffer		= buffer_init();
		conn->oob		= buffer_init();
		conn->cache		= buffer_init();
		conn->packet		= buffer_init();
		conn->chunk		= chunk_init();
		conn->send_queue 	= queue_init();
		conn->sleep_usec 	= CONN_SLEEP_USEC;
		conn->timeout 		= CONN_IO_TIMEOUT;
		conn->set		= conn_set;
		conn->event_handler 	= conn_event_handler;
		conn->read_handler	= conn_read_handler;
		conn->write_handler	= conn_write_handler;
		conn->packet_reader	= conn_packet_reader;
		conn->packet_handler	= conn_packet_handler;
		conn->chunk_reader	= conn_chunk_reader;
		conn->recv_chunk	= conn_recv_chunk;
		conn->recv_file		= conn_recv_file;
		conn->push_chunk	= conn_push_chunk;
		conn->data_handler	= conn_data_handler;
		conn->push_message	= conn_push_message;
		/* client connection connect to server */
		conn->connect 		= conn_connect;
		conn->set_nonblock 	= conn_set_nonblock;
		conn->read 		= conn_read;
		conn->write 		= conn_write;
		conn->close 		= conn_close;
		conn->stats 		= conn_stats;
		conn->clean 		= conn_clean;
		conn->vlog 		= conn_vlog;
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
	if(conn)
	{
		if(conn->evbase && (conn->event = event_init()))
		{
			conn->event->set(conn->event, conn->fd, EV_READ, (void *)conn, conn->event_handler);
			conn->evbase->add(conn->evbase, conn->event);
			return 0;
		}
		else
		{
			FATAL_LOG(conn->logger, "Connection[%08x] fd[%d] EVBASE or Initialize event failed, %s",
				conn, conn->fd, strerror(errno));	
			conn->push_message(conn, MESSAGE_QUIT);
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
			if(event & EV_READ)
			{
				DEBUG_LOG(conn->logger, "EV_READ:%d", EV_READ);
				conn->read_handler(conn);
			}
			if(event & EV_WRITE)
			{
				DEBUG_LOG(conn->logger, "EV_WRITE:%d", EV_WRITE);
				conn->write_handler(conn);
			} 
		}	
	}
}

/* Read handler */
void conn_read_handler(CONN *conn)
{
	int n = 0;
	char tmp[BUF_SIZE];
	if(conn)
	{
		if(read(conn->fd, tmp, BUF_SIZE) < 0 )
		{
			conn->close(conn);
			return ;
		}
		else
		{
			conn->packet_reader(conn);			
		}
				
	}
}

/* Write hanler */
void conn_write_handler(CONN *conn)
{
	int n = 0;
	CHUNK *cp = NULL;
	if(conn && conn->send_queue)
	{
		cp = conn->send_queue->head(conn->send_queue);
		if(cp)
		{
			if( n = cp->send(cp, conn->fd, conn->buffer_size) > 0 )
			{
				DEBUG_LOG(conn->logger, "Sent %d byte(s) to %s:%d via %d leave %llu ",
						n, conn->ip, conn->port, conn->fd, cp->len);
			}
			else
			{
				DEBUG_LOG(conn->logger, "ERROR:sending data to %s:%d failed, %s",
					conn->ip, conn->port, strerror(errno));
				conn->close(conn);
			}
		}
	}		
}

/* Packet reader */
size_t conn_packet_reader(CONN *conn)
{
	if(conn)
	{
		if(conn->buffer->size >= conn->packet_length)
		{
			conn->packet->reset(conn->packet);
			conn->packet->push(conn->packet, conn->buffer->data, conn->packet_length);
			conn->buffer->del(conn->buffer, conn->packet_length);
		}	
	}	
}

/* Packet Handler */
void conn_packet_handler(CONN *conn)
{
	if(conn && conn->cb_packet_handler )
        {
                conn->cb_packet_handler(conn, conn->packet);
        }
}

/* CHUNK reader */
void conn_chunk_reader(CONN *conn)
{
	int n = 0;
	if(conn && conn->chunk && conn->buffer)
	{
		if(conn->buffer->size > 0 )
		{
			if((n = conn->chunk->fill(conn->chunk,
			 	conn->buffer->data, conn->buffer->size) ) > 0 )
			{
				conn->buffer->del(conn->buffer, n);
				DEBUG_LOG(conn->logger, "Filled  %d byte(s) to CHUNK", n);
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
	if(conn && conn->chunk)
	{
		conn->chunk->set(conn->chunk, conn->s_id, MEM_CHUNK, NULL, 0, size);
		conn->chunk_reader(conn);
	}			
}

/* Receive FILE CHUNK */
void conn_recv_file(CONN *conn, char *filename,
         uint64_t offset, uint64_t size)
{
        if(conn && conn->chunk)
        {
                conn->chunk->set(conn->chunk, conn->s_id, MEM_CHUNK, filename, offset, size);
		conn->chunk_reader(conn);
        } 
}
/* Push Chunk */
int conn_push_chunk(CONN *conn, void *data, size_t size)
{
	CHUNK *cp = NULL;
	if(conn && conn->send_queue && data)
	{
		if((cp = conn->send_queue->tail(conn->send_queue)) && cp->type == MEM_CHUNK)
		{
			cp->append(cp, data, size);
		}
		else
		{
			cp = chunk_init();
			cp->set(cp, conn->s_id, MEM_CHUNK, NULL, 0llu, 0llu);
			cp->append(cp, data, size);
			conn->send_queue->push(conn->send_queue, (void *)cp);
		}
		if(conn->send_queue->total == 0 ) conn->event->update(conn->event, EV_READ);			
	}	
}

/* Push File */
int conn_push_file(CONN *conn, char *filename,
         uint64_t offset, uint64_t size)
{
	CHUNK *cp = NULL;
	if(conn && conn->send_queue && filename && offset >= 0 && size > 0 )
	{
		cp = chunk_init();
		cp->set(cp, conn->s_id, FILE_CHUNK, filename, offset, size);
		conn->send_queue->push(conn->send_queue, (void *)cp);
		if(conn->send_queue->total == 0 ) conn->event->update(conn->event, EV_READ);			
	}
}

/* Data handler */
void conn_data_handler(CONN *conn)
{
	if(conn && conn->cb_data_handler )
	{
		conn->cb_data_handler(conn, conn->packet, conn->chunk, conn->cache);
	}	
}

/* Push message */
void conn_push_message(CONN *conn, int message_id)
{
	MESSAGE *msg = NULL;
	if(conn)
	{
		if((msg = message_init()))
		{
			msg->msg_id = message_id;
			msg->fd	= conn->fd;
			msg->handler = (void *)conn;
		}	
		else
		{
			FATAL_LOG(conn->logger, "Initialize MESSAGE failed, %s", strerror(errno));
		}
	}
}

/* Connect to remote Host */
int conn_connect(CONN *conn)
{
	int ret = -1;
	int opt = 0 ;
	if(conn)
	{
                if(conn->fd > 0 )
		{
			shutdown(conn->fd, SHUT_RDWR);
			close(conn->fd);
		}
		if(conn->reconnect_count > RECONNECT_MAX)
		{
			DEBUG_LOG(conn->logger, "RECONNECT Too much time(s) checking network!");
			return -1;
		}
		conn->fd = socket(AF_INET, SOCK_STREAM, 0);
		if(connect(conn->fd, (struct sockaddr *)&(conn->sa),
			 sizeof(conn->sa)) != 0)
		{
			DEBUG_LOG(conn->logger, "connected to %s:%d failed",
			 conn->ip, conn->port, conn->fd);				
			goto ERROR;
		}
		conn->c_state = C_STATE_FREE;
		conn->connect_state = C_STATE_CONNECTED;	
		conn->reconnect_count++;
		DEBUG_LOG(conn->logger, "connected  to %s:%d fd[%d]",
               		conn->ip, conn->port, conn->fd);
	}
	return 0;
ERROR:
	{
		if(conn)
		{
			if(conn->fd > 0 )
			{
				shutdown(conn->fd, SHUT_RDWR);
				close(conn->fd);
			}
			conn->fd = -1;
			conn->c_state = C_STATE_CLOSE;
			conn->connect_state = C_STATE_UNCONNECT;
		}
		return -1;
	}
}

/* Check STATE OR Reconnect */
void conn_stats(CONN *conn)
{
	int buf_size = 1024;
	char buf[buf_size];
	if(conn)
	{
		recv(conn->fd, buf, buf_size,  MSG_OOB);
		if(send(conn->fd, (void *)"\0", 1,  MSG_OOB) < 0 )
		{
			DEBUG_LOG(conn->logger, 
				"connection to %s:%d via fd[%d] is shutdown, trying to reconnect",
				conn->ip,conn->port, conn->fd);
			conn->c_state 		= C_STATE_CLOSE;
			conn->connect_state 	= C_STATE_UNCONNECT;
			conn->connect(conn);
		}
		return ;
	}
}

/* Setting Non-Block */
int conn_set_nonblock(CONN *conn)
{
	if(conn)
	{
		if(fcntl(conn->fd, F_SETFL, O_NONBLOCK) != 0 )
		{
			ERROR_LOG(conn->logger, "Set fd[%d] NONBLOCK failed, %s",
				conn->fd, strerror(errno));
		}
		else
		{
			conn->is_nonblock = CONN_NONBLOCK;
			DEBUG_LOG(conn->logger,  "Set fd[%d] NONBLOCK ", conn->fd);
		}
	}
	return -1;
}

/* Read data  via fd */
int conn_read(CONN *conn, void *buf, size_t len, uint32_t timeout)
{
	int n = 0,  recv =  0;
	size_t total = len ;
	char *p = (char *)buf;
	TIMER *timer = NULL;
	uint32_t sleep_usec = conn->sleep_usec;
	uint32_t io_timeout = (timeout > 0 ) ? timeout : conn->timeout;

	timer = timer_init();
	if(timer == NULL)
	{
		ERROR_LOG(conn->logger, "Initialize timer failed, %s", strerror(errno));
		return -1;
	}
	while(total > 0 )
	{
		if( (n = read(conn->fd, p, total) ) > 0)
		{
			p += n;
			total -= n;
			recv += n;
			DEBUG_LOG(conn->logger, "Received %d bytes data from %s:%d via %d ", 
                                recv, conn->ip, conn->port, conn->fd);
		}
		/*
		else
		{
			ERROR_LOG(conn->logger, "Reading data from %s:%d failed, %s ", 
				conn->ip, conn->port, strerror(errno));
		}
		*/
		timer->sample(timer);
		if(timer->usec_used >= io_timeout)
		{
			ERROR_LOG(conn->logger, "Reading data from %s:%d timeout",
					conn->ip, conn->port);
			goto end;
		}
		usleep(sleep_usec);
	}
end:
	{
		if(timer)timer->clean(&timer);
	}
	return ((recv > 0)? recv : -1);
}

/* Write data  via fd */
int conn_write(CONN *conn, void *buf, size_t len, uint32_t timeout)
{
	int n = 0, sent = 0;
	size_t total =  len ;
	char *p = (char *)buf;
	TIMER *timer = NULL;
	uint32_t sleep_usec = conn->sleep_usec;
	uint32_t io_timeout = (timeout > 0 ) ? timeout : conn->timeout;
	
	timer = timer_init();
	if(timer == NULL)
	{
		ERROR_LOG(conn->logger, "Initialize timer failed, %s", strerror(errno));
		return -1;
	}
	while(total > 0 )
	{
		if( (n = write(conn->fd, p, total) ) > 0)
		{
			p += n;
			total -= n;
			sent += n;
			DEBUG_LOG(conn->logger, "Wrote %d bytes data to %s:%d via %d ",
                                sent, conn->ip, conn->port, conn->fd);
		}
		else
		{
			ERROR_LOG(conn->logger, "Writting data to %s:%d failed, %s ",
                            conn->ip, conn->port, strerror(errno));
		}
		timer->sample(timer);
                if(timer->usec_used >= io_timeout)
		if(timer->usec_used >= io_timeout)
		{
			ERROR_LOG(conn->logger, 
				"Writting data to %s:%d timeout ",
				conn->ip, conn->port);
			goto end;
		}
		usleep(sleep_usec);
	}
end:
        {
                if(timer)timer->clean(&timer);
        }
	return ((sent > 0 )?sent : -1);
}

/* Close Connection to remote Host */
int conn_close(CONN *conn)
{
	if(conn)
	{
		conn->c_state = C_STATE_CLOSE;
		conn->connect_state = C_STATE_UNCONNECT;
		shutdown(conn->fd, SHUT_RDWR);
		close(conn->fd);
		conn->fd = -1;
	}
	return -1;	
}

/* Clean Connection */
void conn_clean(CONN **conn)
{
	CHUNK *cp = NULL;
	if((*conn))
	{
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
		/* Clean send queue */
		if((*conn)->send_queue)
		{
			while((*conn)->send_queue->total > 0 )
			{
				cp = (CHUNK *)(*conn)->send_queue->pop((*conn)->send_queue);
				if(cp) cp->clean(&(cp));
			}
			free((*conn)->send_queue);
		}
		free((*conn));
		(*conn) = NULL;
	}	
}

/* vprintf log */
void conn_vlog(const char *format,...)
{
	va_list ap;

	va_start(ap, format);	
	vfprintf(stdout, format, ap);
	fprintf(stdout, "\n");
	va_end(ap);	
}
