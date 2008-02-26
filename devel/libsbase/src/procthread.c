#include <evbase.h>
#include "sbase.h"
#include "procthread.h"
#include "buffer.h"
#include "queue.h"
#include "timer.h"
#include "message.h"
#include "logger.h"
#include "conn.h"

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
		pth->running_once		= procthread_running_once;
		pth->addconn			= procthread_addconn;
		pth->add_connection		= procthread_add_connection;
		pth->terminate_connection	= procthread_terminate_connection;
		pth->terminate			= procthread_terminate;
		pth->clean			= procthread_clean;
		pth->running_status		= 1;
	}
	return pth;
}

/* Running once for message queue */
void procthread_running_once(PROCTHREAD *pth)
{
	MESSAGE *msg = NULL;
	CONN    *conn = NULL;
	PROCTHREAD *parent = NULL;
	if(pth)
	{
		msg = pth->message_queue->pop(pth->message_queue);
		if(msg)
		{
			DEBUG_LOGGER(pth->logger, "Got message[%08x] id[%d] handler[%08x] parent[%08x]",
                                        msg, msg->msg_id, msg->handler, msg->parent);
			if(!(msg->msg_id & MESSAGE_ALL))
			{
				WARN_LOGGER(pth->logger, "Unkown message[%d]", msg->msg_id);
				goto next;
			}
			conn = (CONN *)msg->handler;
			parent = (PROCTHREAD *)msg->parent;
			if(conn == NULL || parent == NULL 
				|| msg->fd != conn->fd || pth->service == NULL )
			{
				WARN_LOGGER(pth->logger, 
				"Invalid MESSAGE[%08x] fd[%d] handler[%08x] parent[%08x]",
					msg, msg->fd, conn, parent);
				goto next;
			}
			DEBUG_LOGGER(pth->logger, "Got message[%s] On service[%s] procthread[%08x] connection[%d] %s:%d",
					messagelist[msg->msg_id], pth->service->name, pth, conn->fd, conn->ip, conn->port);
			switch(msg->msg_id)
			{
				/* NEW connection */
				case MESSAGE_NEW_SESSION :
					pth->add_connection(pth, conn);
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
next:
			msg->clean(&msg);
		}
	}
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
		pth->running_once(pth);
		usleep(pth->service->sleep_usec);
	}
}

/* Add connection msg */
void procthread_addconn(PROCTHREAD *pth, CONN *conn)
{
	MESSAGE *msg = NULL;
	//DEBUG_LOGGER(pth->logger, "Adding NEW CONN[%08x] IN PROCTHREAD[%08x]", conn, pth);
	if(pth && pth->message_queue && conn)
	{
		if((msg = message_init()))
		{
			msg->msg_id = MESSAGE_NEW_SESSION;
			msg->fd = conn->fd;
			msg->handler = conn;
			msg->parent  = (void *)pth;
			pth->message_queue->push(pth->message_queue, msg);
			DEBUG_LOGGER(pth->logger, "Added message for NEW_SESSION[%d] %s:%d total %d",
				conn->fd, conn->ip, conn->port, pth->message_queue->total);
		}
	}	
}

/* Add new connection */
void procthread_add_connection(PROCTHREAD *pth, CONN *conn)
{
	if(pth && conn && pth->service->max_connections > 0)
	{
		if(pth->connections == NULL )
			pth->connections = (CONN **)calloc(pth->service->max_connections, sizeof(CONN *));
		if(pth->connections == NULL && conn->fd <= 0 )
			return ;
		if(conn->fd < pth->service->max_connections)
		{
			/* Add event to evbase */
			conn->evbase = pth->evbase;
			conn->message_queue = pth->message_queue;
			conn->parent = (void *)pth;
			if(conn->set(conn) != 0)
			{
				FATAL_LOGGER(pth->logger, "Setting connections[%d] %s:%d failed, %s",
					conn->fd, conn->ip, conn->port, strerror(errno));
			}
			pth->connections[conn->fd] = conn;
			pth->service->pushconn(pth->service, conn);
			DEBUG_LOGGER(pth->logger, "Added connection[%d] %s:%d to procthread[%d] total %d",
				 conn->fd, conn->ip, conn->port, pth->index, pth->service->running_connections);
		}
		else
		{
			ERROR_LOGGER(pth->logger, "Connections is full in procthread[%d]", pth->index);
			pth->terminate_connection(pth, conn);
		}
	}
}

/* Terminate connection */
void procthread_terminate_connection(PROCTHREAD *pth, CONN *conn)
{
	if(pth && pth->connections && conn && conn->fd < pth->service->max_connections && conn->fd > 0  )
	{
		pth->connections[conn->fd] = NULL;
        pth->service->popconn(pth->service, conn->index);
		conn->terminate(conn);	
		conn->clean(&conn);
	}
}

/* Terminate procthread */
void procthread_terminate(PROCTHREAD *pth)
{
	int i = 0;
	CONN *conn = NULL;
	
	if(pth && pth->connections && pth->service)
	{
		/* Setting  running status */
		pth->running_status = 0;
		for(i = 0; i < pth->service->max_connections; i++)
		{
			if((conn = pth->connections[i]))
			{
				conn->terminate(conn);	
				pth->service->running_connections--;
			}
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
			for(i = 0; i < (*pth)->service->max_connections; i++)
			{
				if((*pth)->connections[i])
					(*pth)->connections[i]->clean(&((*pth)->connections[i]));
			}
			free((*pth)->connections);
			(*pth)->connections = NULL;
		}
		/* Clean message queue */
		if((*pth)->message_queue && (*pth)->message_queue != (*pth)->service->message_queue)
		{
			while((*pth)->message_queue->total > 0 )
			{
				msg = (*pth)->message_queue->pop((*pth)->message_queue);	
				if(msg) msg->clean(&msg);
			}	
			(*pth)->message_queue->clean(&((*pth)->message_queue));	
		}
		/* Clean Event base */
		if( (*pth)->evbase && (*pth)->evbase != (*pth)->service->evbase)
		{
			(*pth)->evbase->clean(&((*pth)->evbase));
		}
		/* Clean Timer */
		if((*pth)->timer) (*pth)->timer->clean(&((*pth)->timer));
		(*pth) = NULL;
	}	
}

