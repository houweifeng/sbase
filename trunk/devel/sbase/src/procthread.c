#include <evbase.h>
#include "sbase.h"
#include "procthread.h"
#include "buffer.h"
#include "queue.h"
#include "timer.h"
#include "message.h"

/* Initialize procthread */
PROCTHREAD *procthread_init()
{
	PROCTHREAD *pth = (PROCTHREAD *) calloc(1, sizeof(PROCTHREAD));
	if(pth)
	{
		pth->evbase 			= evbase_init();		
		pth->message_queue		= queue_init();
		pth->timer			= timer_init();
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
		pth->evbase->loop(pth->evbase, 0, NULL);
		msg = pth->message_queue->pop(pth->message_queue);
		if(msg)
		{
			conn = (CONN *)msg->handler;
			switch(msg->msg_id)
			{
				/* NEW connection */
				case MESSAGE_NEW_SESSION :
					if(conn) pth->add_connection(pth, conn);
					break;
					/* Close connection */
				case MESSAGE_QUIT :
					if(pth->connections[msg->fd])
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
			msg->clean(&msg);
		}
		usleep(pth->sleep_usec);
	}
}

/* Add connection msg */
void procthread_addconn(PROCTHREAD *pth, CONN *conn)
{
	MESSAGE *msg = NULL;
	if(pth && pth->message_queue)
	{
		if((msg = message_init()))
		{
			msg->msg_id = MESSAGE_NEW_SESSION;
			msg->fd = conn->fd;
			msg->handler = conn;
			pth->message_queue->push(pth->message_queue, msg);
			DEBUG_LOG(pth->logger, "Message for NEW_SESSION[%d] %s:%d",
				conn->fd, conn->ip, conn->port);
		}
	}	
}

/* Add new connection */
void procthread_add_connection(PROCTHREAD *pth, CONN *conn)
{
	if(pth && conn && pth->connections 
		&& pth->running_connections < pth->max_connections 
		&& conn->fd > 0 && conn->fd < pth->max_connections)
	{
			conn->evbase = pth->evbase;
			conn->message_queue = pth->message_queue;
			pth->connections[conn->fd] = conn;
			DEBUG_LOG(pth->logger,
				"Procthread[%d] ID[%d] added new connection[%d] from %s:%d",
				 pth->index, pth->id, conn->fd, conn->ip, conn->port);
	}
	else
	{
		if(conn && conn->fd > 0 )
		{
			ERROR_LOG(pth->logger, "Procthread[%d] ID[%d] connection full, closing fd[%d]",
			pth->index, pth->id, conn->fd);
			pth->terminate_connection(pth, conn);
		}
	}
}

/* Terminate connection */
void procthread_terminate_connection(PROCTHREAD *pth, CONN *conn)
{
	if(pth && pth->connections && conn && conn->fd < pth->max_connections && conn->fd > 0  )
	{
		pth->connections[conn->fd] = NULL;
		conn->close(conn);	
		conn->clean(&conn);
	}
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
	int i = 0;
	CONN *conn = NULL;

	if(pth && pth->connections)
	{
		for(i = 0; i < pth->max_connections; i++)
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
			for(i = 0; i < (*pth)->max_connections; i++)
			{
				(*pth)->connections[i]->clean(&((*pth)->connections[i]));
			}
			free((*pth)->connections);
			(*pth)->connections = NULL;
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

