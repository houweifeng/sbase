#include "sbase.h"
#include "procthread.h"
#include "buffer.h"
#include "queue.h"
#include "timer.h"

/* Initialize procthread */
PROCTHREAD *procthread_init()
{
	PROCTHREAD *pth = (PROCTHREAD *) calloc(1, sizeof(PROCTHREAD));
	if(pth)
	{
		pth->ev_eventbase 		= event_base_init();		
		pth->message_queue		= queue_init();
		pth->timer			= timer_init();
		//pth->logger			= logger_init();
		pth->run			= procthread_run;
		pth->addconn			= procthread_addconn;
		pth->add_connection		= procthread_add_connection;
		pth->terminate_connection	= procthread_terminate_connection;
		pth->terminate			= procthread_terminate;
		pth->clean			= procthread_clean;
		pth->running_status		= 1;
	}
	return pth;
}

/* Running procthread */
void procthread_run(void *arg)
{
	MESSAGE *msg = NULL;
	CONN	*conn = NULL;
	struct sockaddr_in sa;
	PROCTHREAD *pth = (PROCTHREAD *)arg;

	while(pth->running_status)
	{
		event_loop(pth->ev_eventbase, EV_ONCE|EV_NONBLOCK);			
		usleep(pth->sleep_usec)
			msg = pth->message_queue->pop(pth->message_queue);
		if(msg)
		{
			DEBUG_LOG(pth->log, "Handling message[%08x] ID[%d]", msg, msg->msg_id);
			if(msg->handler) conn = (SESSION *)msg->handler;
			else conn = NULL;
			switch(msg->msg_id)
			{
				/* NEW connection */
				case MESSAGE_NEW_SESSION :
					if(msg->handler)
					{
						pth->add_connection(pth, msg->fd,
								*((struct sockaddr_in *)msg->handler));
						free(msg->handler);
					}
					break;
					/* Close connection */
				case MESSAGE_QUIT :
					if(pth->connions[msg->fd])
						pth->terminate_connection(pth, conn);
					break;
				case MESSAGE_INPUT :
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
				default:
					break;
			}
			msg->clean(msg);
		}
		usleep(pth->sleep_usec)
	}
}

/* Add connection msg */
void procthread_addconn(PROCTHREAD *pth, int fd, struct sockaddr_in sa)
{
	MESSAGE *msg = NULL;
	if(pth && pth->message_queue)
	{
		if((msg = message_init()))
		{
			msg->msg_id = MESSAGE_NEW_SESSION;
			msg->fd = fd;
			msg->handler = calloc(1, sizeof(struct sockaddr_in ));
			if(msg->handler) memcpy(msg->handler, &sa, sizeof(struct sockaddr_in));
			DEBUG_LOG(pth->logger, "message for NEW_SESSION[%d] %s:%d",
				fd, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
		}
	}	
}

/* Add new connection */
void procthread_add_connection(PROCTHREAD *pth, int fd, struct sockaddr_in sa)
{
	CONN *conn = NULL;
	char *ip = NULL;
	int  port = 0;

	if(pth && pth->connections 
		&& pth->running_connections < max_connections && fd > 0 )
	{
		ip = inet_ntoa(sa.sin_addr);
		port = ntohs(sa.sin_port);
		if(conn = conn_init(ip, port))
		{
			conn->eventbase = pth->ev_eventbase;
			conn->fd = fd;
			conn->event_init(conn);
			pth->connections[fd] = conn;
			DEBUG_LOG(pth->logger,
				"Procthread[%d] ID[%d] added new connection[%d] from %s:%d",
				 fd, ip, port);
		}
	}
	else
	{
		if(fd > 0 )
		{
			ERROR_LOG(pth->logger, "Procthread[%d] ID[%d] connection full, closing fd[%d]",
			pth->index, pth->id, fd);
			shutdown(fd, SHUT_RDWR);
			close(fd);
		}
	}
}

/* Terminate connection */
void procthread_terminate_connection(PROCTHREAD *pth, CONN *conn)
{
	if(pth && pth->connections && conn && conn->fd < pth->max_connection  )
	{
		pth->connections[conn->fd] = NULL;
		conn->close(conn);	
		conn->clean(conn);
	}
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
	int i = 0;
	CONN *conn = NULL;

	if(pth && pth->connections)
	{
		for(i = 0; i < pth->max_connection; i++)
		{
			conn = pth->connections[i];
			pth->connections[i] = NULL;
			conn->close(conn);	
		}	
	}
}

/* Clean procthread */
void procthread_clean(PROCTHREAD **pth)
{
	int i = 0;
        MESSAGE *msg = NULL;

	if((*pth))
	{
		/* Clean connections */
		if((*pth)->connections)
		{	
			for(i = 0; i < (*pth)->max_connection; i++)
			{
				(*pth)->connections[i]->clean(&((*pth)->connections[i]));
			}
			free(pth->connections);
			pth->connections = NULL;
		}
		/* Clean message queue */
		if((*pth)->message_queue)
		{
			while((*pth)->message_queue->total > 0 )
			{
				msg = (*pth)->message_queue->pop((*pth)->message_queue);	
				if(msg) msg->clean(&msg);
			}	
			(*pth)->message_queue->clean(&((*pth)->message_queue));	
		}
		/* Clean Timer */
		if((*pth)->timer) (*pth)->timer->clean(&((*pth)->timer));
		(*pth) = NULL;
	}	
}

