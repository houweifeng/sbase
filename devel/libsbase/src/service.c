#include <evbase.h>
#include "sbase.h"
#include "service.h"
#include "procthread.h"
#include "logger.h"
#include "conn.h"

/* Initialize service */
SERVICE *service_init()
{
	SERVICE *service = (SERVICE *)calloc(1, sizeof(SERVICE));
	if(service)
	{
		service->event_handler	= service_event_handler;
		service->set		    = service_set;
		service->run		    = service_run;
		service->addconn	    = service_addconn;
        service->state_conns    = service_state_conns;
        service->getconn        = service_getconn;
		service->terminate	    = service_terminate;
		service->clean		    = service_clean;
		service->event 		    = ev_init();
		service->timer		    = timer_init();
	}
	return service;
}

/* Set service  */
int service_set(SERVICE *service)
{
	int opt = 1;
	if(service)
	{
		//service->running_status = 1;	
		/* Setting logger */
		if(service->logfile)
		{
			service->logger = logger_init(service->logfile);
			DEBUG_LOGGER(service->logger, "Setting service[%d] log to %s", service->name, service->logfile);
		}
        if(service->evlogfile)
        {
			service->evlogger = logger_init(service->evlogfile);
			DEBUG_LOGGER(service->logger, "Setting service[%d] evlog to %s", service->name, service->evlogfile);
        }
        /* Initialize conns array */
        if(service->max_connections > 0)
            service->connections = (CONN **)calloc(service->max_connections, sizeof(CONN *));
        /* setting as service type */
        if(service->service_type == S_SERVICE)
            goto server_setting;
        else
            goto client_setting;
server_setting:
        DEBUG_LOGGER(service->logger, "Setting server[%s]\n", service->name);
		/* INET setting  */
		if((service->fd = socket(service->family, service->socket_type, 0)) <= 0 )
		{
			FATAL_LOGGER(service->logger, "Initialize socket failed, %s", strerror(errno));
			return -1;
		}
		service->sa.sin_addr.s_addr = (service->ip)?inet_addr(service->ip):INADDR_ANY;
		service->sa.sin_port = htons(service->port); 
		//setsockopt 
		setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR,
				(char *)&opt, (socklen_t)sizeof(opt) );
		//Bind 
		if(bind(service->fd, (struct sockaddr *)&(service->sa), 
                    (socklen_t)sizeof(struct sockaddr)) != 0 )
		{
			FATAL_LOGGER(service->logger, "Bind fd[%d] to %s:%d failed, %s",
					service->fd, service->ip, service->port, strerror(errno));
			return -1;
		}
		//Listen
		if(listen(service->fd, service->backlog) != 0)
		{
			FATAL_LOGGER(service->logger, "Listen fd[%d] On %s:%d failed, %s",
					service->fd, service->ip, service->port, strerror(errno));	
			return -1;
		}
		/* Event base */
		//service->ev_eventbase = ev_init();
		if(service->evbase == NULL)
		{
			 FATAL_LOGGER(service->logger, "Eventbase on fd[%d]  %s:%d is NULL",
                                        service->fd, service->ip, service->port);
			return -1;
		}
		if(service->event)
		{
			service->event->set(service->event, service->fd,
					E_READ | E_PERSIST, (void *)service, service->event_handler);
			service->evbase->add(service->evbase, service->event);
		}
		return 0;
client_setting:
        DEBUG_LOGGER(service->logger, "Setting client[%s]\n", service->name);
        service->sa.sin_addr.s_addr = (service->ip)?inet_addr(service->ip):INADDR_ANY;
        service->sa.sin_port = htons(service->port);
        return 0;
	}
	return -1;
}

/* Run service */
void service_run(SERVICE *service)
{
	int i = 0, fd = 0;
#ifdef HAVE_PTHREAD
	pthread_t procthread_id;
#endif
	/* Running */
	if(service)
	{
	DEBUG_LOGGER(service->logger, "service[%s] using WORKING_MODE[%d]", 
	service->name, service->working_mode);
#ifdef	HAVE_PTHREAD
		if(service->working_mode == WORKING_PROC) 
			goto work_proc_init;
		else 
			goto work_thread_init;
		return ;
#endif
work_proc_init:
		/* Initialize procthread(s) */
		if((service->procthread = procthread_init()))
		{
			service->procthread->logger = service->logger;
			if(service->procthread->evbase)
				service->procthread->evbase->clean(&(service->procthread->evbase));
			service->procthread->evbase = service->evbase;
			if(service->message_queue && service->procthread->message_queue)
			{
				service->procthread->message_queue->clean(&(service->procthread->message_queue));
				service->procthread->message_queue = service->message_queue;
				DEBUG_LOGGER(service->logger, 
				 "Replaced procthread[%08x]->message_queue with service[%08x]->message_queue[%08x]",
				 service->procthread, service, service->message_queue);
			}
			service->procthread->service = service;
			service->running_status = 1;
		}
		else
		{
			FATAL_LOGGER(service->logger, "Initialize procthreads failed, %s",
					strerror(errno));
			exit(EXIT_FAILURE);
		}	
        goto end;
work_thread_init:
		/* Initialize Threads */
		service->procthreads = (PROCTHREAD **)calloc(service->max_procthreads,
				sizeof(PROCTHREAD *));
		if(service->procthreads == NULL)
		{
			FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
					strerror(errno));
			exit(EXIT_FAILURE);
		}
		for(i = 0; i < service->max_procthreads; i++)
		{
			if((service->procthreads[i] = procthread_init()))
			{
				service->procthreads[i]->service = service;
				service->procthreads[i]->logger = service->logger;
				service->procthreads[i]->evbase->logger = service->evlogger;
			}
			else
			{
				FATAL_LOGGER(service->logger, "Initialize procthreads pool failed, %s",
						strerror(errno));
				exit(EXIT_FAILURE);
			}
#ifdef HAVE_PTHREAD
			if(pthread_create(&procthread_id, NULL, 
				(void *)(service->procthreads[i]->run),
				(void *)service->procthreads[i]) == 0)
			{
				DEBUG_LOGGER(service->logger, "Created procthreads[%d] ID[%08x]",
						i, procthread_id);	
			}	
			else
			{
				FATAL_LOGGER(service->logger, "Create procthreads[%d] failed, %s",
						i, strerror(errno));
				exit(EXIT_FAILURE);				
			}
#endif
		}	
		goto end ;
    /* client connection initialize */
end:
        if(service->service_type == C_SERVICE)
        {
            for(i = 0; i < service->connections_limit; i++)
            {
                fd = socket(service->family, service->socket_type, 0);
                if(fd > 0 && connect(fd, (struct sockaddr *)&(service->sa), 
                            sizeof(struct sockaddr )) == 0)
                {
                    service->addconn(service, fd, &(service->sa));
                    DEBUG_LOGGER(service->logger, "Connected to %s:%d via %d",
                            service->ip, service->port, fd);
                }
                else
                {
                    ERROR_LOGGER(service->logger, "Connectting to %s:%d via %d failed, %s",
                            service->ip, service->port, fd, strerror(errno));
                    close(fd);
                }
            }
        }
        return ; 
	}
}

/* Event handler */
void service_event_handler(int event_fd, short event, void *arg)
{
	struct sockaddr_in rsa;
	int fd = 0;
	socklen_t rsa_len ;
	SERVICE *service = (SERVICE *)arg;
	if(service)
	{
		if(event_fd != service->fd) 
		{
			FATAL_LOGGER(service->logger, "Invalid event_fd[%d] not match daemon fd[%d]",
				event_fd, service->fd);
			return ;
		}
		if(event & E_READ)
		{
			if((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0 )	
			{
				DEBUG_LOGGER(service->logger, "Accept new connection[%d] from %s:%d",
					fd, inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port));			
				return service->addconn(service, fd, &rsa);
			}
			else
			{
				FATAL_LOGGER(service->logger, "Accept new connection[%d] failed, %s",
					strerror(errno), fd);
			}
		}
	}
	return ;
}

/* Add new conn */
void  service_addconn(SERVICE *service, int fd,  struct sockaddr_in *sa)
{
	CONN *conn = NULL;
	char *ip = NULL;
	int port = 0;
	int index = 0;
	if(service)
    {
        /*Check Connections Count */
        if(service->running_connections >= service->max_connections)
        {
            ERROR_LOGGER(service->logger, "Connections count[%d] reach max[%d]",
                    service->running_connections, service->max_connections);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            return ;
        }
        /* Initialize connection */
        ip   = inet_ntoa(sa->sin_addr);
        port = ntohs(sa->sin_port);
        if((conn = conn_init(ip, port)))
        {        
            conn->fd = fd;
            conn->logger = service->logger;
            conn->packet_type = service->packet_type;
            conn->packet_length = service->packet_length;
            conn->packet_delimiter = service->packet_delimiter;
            conn->packet_delimiter_length = service->packet_delimiter_length;
            conn->buffer_size = service->buffer_size;
            conn->cb_packet_reader = service->cb_packet_reader;
            conn->cb_packet_handler = service->cb_packet_handler;
            conn->cb_data_handler = service->cb_data_handler;
            conn->cb_oob_handler = service->cb_oob_handler;
        }
        else return ;
        /* handling conns and running_max_fd */
        if(service->connections)
        {
            service->connections[fd] = conn;
            if(fd > service->running_max_fd)
                service->running_max_fd = fd;
        }
        /* Add connection for procthread */
        if(service->working_mode == WORKING_PROC && service->procthread)
        {
            DEBUG_LOGGER(service->logger, "Adding connection[%d] on %s:%d to procthread[%d]",
                    conn->fd, conn->ip, conn->port, getpid());
            return service->procthread->addconn(service->procthread, conn);
        }
        /* Add connection to procthread pool */
        if(service->working_mode == WORKING_THREAD && service->procthreads)
        {
            index = fd % service->max_procthreads;
            DEBUG_LOGGER(service->logger, "Adding connection[%d] on %s:%d to procthreads[%d]",
                    conn->fd, conn->ip, conn->port, index);
            return service->procthreads[index]->addconn(service->procthreads[index], conn);
        }
    }
	return ;
}

/* check conns status */
void service_state_conns(SERVICE *service)
{
    if(service)
    {
    }
}

/* get free connection */
CONN *service_getconn(SERVICE *service)
{
    int i = 0;
    int fd = 0;
    CONN *conn = NULL;
    if(service)
    {
        //select free connection 
        for( i = 0; i < service->running_max_fd; i++)
        {
            conn = service->connections[i];
            if(conn && conn->c_state == C_STATE_FREE)
            {
                conn->c_state = C_STATE_USING;
                break;
            }
            conn = NULL;
        }
        //create new connection 
        if(conn == NULL)
        {
            fd = socket(service->family, service->socket_type, 0);
            if(fd > 0 && connect(fd, (struct sockaddr *)&(service->sa),
                        sizeof(struct sockaddr )))
                service->addconn(service, fd, &(service->sa));
            if(fd < service->running_max_fd)
            {
                conn = service->connections[fd];
                conn->c_state = C_STATE_USING;
            }
        }
    }
    return conn;
}

/* Terminate service */
void service_terminate(SERVICE *service)
{
    int i = 0;
    PROCTHREAD *pth = NULL;
    if(service)
    {
        service->running_status = 0;	
        if(service->working_mode == WORKING_PROC)
        {
            service->procthread->terminate(service->procthread);
        }
        else
        {
            for(i = 0; i < service->max_procthreads; i++)
            {
                pth = service->procthreads[i];
                pth->terminate(pth);
            }
        }
        shutdown(service->fd, SHUT_RDWR);
        close(service->fd);
        service->fd = -1;
        return;
    }
}

/* Clean service */
void service_clean(SERVICE **service)
{
    int i = 0;
    if((*service))
    {
        if((*service)->working_mode == WORKING_PROC)
        {
            (*service)->procthread->clean(&(*service)->procthread);
        }	
        else
        {
             for(i = 0; i < (*service)->max_procthreads; i++)
             {
                 (*service)->procthreads[i]->clean(
                         &((*service)->procthreads[i]));
             }
        }
        if((*service)->connections)
        {
            free((*service)->connections);
            (*service)->connections = NULL;
        }
        if((*service)->logger) 
            (*service)->logger->close(&(*service)->logger);
        if((*service)->evlogger) 
            (*service)->evlogger->close(&(*service)->evlogger);
        if((*service)->event) 
            (*service)->event->clean(&(*service)->event);
        if((*service)->timer) 
            (*service)->timer->clean(&((*service)->timer));
        free((*service));
        (*service) = NULL;
    }		
}
