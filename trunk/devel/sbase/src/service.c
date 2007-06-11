#include <evbase.h>
#include "service.h"
#include "procthread.h"

/* Initialize service */
SERVICE *service_init()
{
	SERVICE *service = (SERVICE *)calloc(1, sizeof(SERVICE));
	if(service)
	{
		service->event_handler	= service_event_handler;
		service->set		= service_set;
		service->run		= service_run;
		service->addconn	= service_addconn;
		service->terminate	= service_terminate;
		service->clean		= service_clean;
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
		/* INET setting  */
		if((service->fd = socket(service->family, service->sock_type, 0)) <= 0 )
		{
			FATAL_LOG(service->logger, "Initialize socket failed, %s", strerror(errno));
			return -1;
		}
		service->sa.sin_addr.s_addr = (service->ip)?inet_addr(service->ip):INADDR_ANY;
		service->sa.sin_port = htons(service->port); 
		//setsockopt 
		setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR,
				(char *)&opt, (socklen_t)sizeof(opt) );
		//Bind 
		if(bind(service->fd, (struct sockaddr *)&(service->sa), (socklen_t)sizeof(struct sockaddr)) != 0 )
		{
			FATAL_LOG(service->logger, "Bind fd[%d] to %s:%d failed, %s",
					service->fd, service->ip, service->port, strerror(errno));
			return -1;
		}
		//Listen
		if(Listen(service->fd, service->backlog) != 0)
		{
			FATAL_LOG(service->logger, "Listen fd[%d] On %s:%d failed, %s",
					service->fd, service->ip, service->port, strerror(errno));	
			return -1;
		}
		/* Event base */
		//service->ev_eventbase = event_init();
		if(service->evbase == NULL)
		{
			 FATAL_LOG(service->logger, "Eventbase on fd[%d]  %s:%d is NULL",
                                        service->fd, service->ip, service->port);
			return -1;
		}
		service->event = event_init();
		service->event->set(service->event, service->fd,
 			EV_READ | EV_PERSIST, (void *)service, service->event_handler);
		service->evbase->add(service->evbase, service->event);
	}
	return -1;
}

/* Run service */
void service_run(SERVICE *service)
{
	int i = 0;
#ifdef HAVE_PTHREAD
	pthread_t procthread_id;
#endif
	/* Running */
	if(service)
	{
#ifdef	HAVE_PTHREAD
		if(service->working_mode == WORK_PROC) goto work_proc_init;
		else goto work_thread_init;
#endif
work_proc_init:
		/* Initialize procthread(s) */
		if((service->procthread = procthread_init()))
		{
			service->procthread->logger = service->logger;
			service->running_status = 1;
		}
		else
		{
			FATAL_LOG(service->logger, "Initialize procthreads failed, %s",
					strerror(errno));
			exit(EXIT_FAILURE);
		}	
		return ;
work_thread_init:
		/* Initialize Threads */
		service->procthreads = (PROCTHREAD **)calloc(service->max_procthreads,
				sizeof(PROCTHREAD *));
		if(service->procthreads == NULL)
		{
			FATAL_LOG(service->logger, "Initialize procthreads pool failed, %s",
					strerror(errno));
			exit(EXIT_FAILURE);
		}
		for(i = 0; i < service->max_procthreads; i++)
		{
			if((service->procthreads[i] = procthread_init()))
			{
				service->procthreads[i]->logger = service->logger;
			}
			else
			{
				FATAL_LOG(service->logger, "Initialize procthreads pool failed, %s",
						strerror(errno));
				exit(EXIT_FAILURE);
			}
#ifdef HAVE_PTHREAD
			if(pthread_create(&procthread_id, NULL, service->procthreads[i]->run,
						(void *)service->procthreads[i]) == 0)
			{
				DEBUG_LOG(service->logger, "Created procthreads[%d] ID[%08x]",
						i, procthread_id);	
			}	
			else
			{
				FATAL_LOG(service->logger, "Create procthreads[%d] failed, %s",
						i, strerror(errno));
				exit(EXIT_FAILURE);				
			}

#endif
		}	
		return ;
	}
}

/* Event handler */
void service_event_handler(int event_fd, short event, void *arg)
{
	struct sockaddr_in rsa;
	socklen_t rsa_len = 0;
	int fd = 0;
	SERVICE *service = (SERVICE *)arg;
	if(service)
	{
		if(event_fd != service->fd) 
		{
			FATAL_LOG(service->logger, "Invalid event_fd[%d] not match daemon fd[%d]",
				event_fd, service->fd);
			return ;
		}
		if(event & EV_READ)
		{
			if((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0 )	
			{
				DEBUG_LOG(service->logger, "Accept new connection[%d] from %s:%d",
					fd, inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port));			
				service->addconn(service, fd, &rsa);
			}
			else
			{
				FATAL_LOG(service->logger, "Accept new connection failed, %s",
					strerror(errno));
			}
		}
	}
}

/* Add new conn */
int service_addconn(SERVICE *service, int fd,  struct sockaddr_in *sa)
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
			ERROR_LOG(service->logger, "Connections count[%d] reach max[%d]",
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
		else return -1;
		/* Add connection for procthread */
		if(service->working_mode == WORK_PROC && service->procthread)
		{
			service->procthread->add_connection(service->procthread, conn);
			return 0;
		}
		/* Add connection to procthread pool */
		if(service->working_mode == WORK_THREAD && service->procthreads)
		{
			index = fd % service->max_procthreads;
			service->procthreads[index]->add_connection(service->procthreads[index], conn);
			return 0;
                }
	}
	return ;
}

/* Terminate service */
void service_terminate(SERVICE *service)
{
	if(service)
	{
		service->running_status = 0;	
		if(service->working_mode == WORK_PROC)
		{
			service->procthread->terminate(service->procthread);
			shutdown(service->fd, SHUT_RDWR);
			close(service->fd);
			service->fd = -1;
			return ;
		}	
	}	
}

/* Clean service */
void service_clean(SERVICE **service)
{
	if((*service))
	{
		if((*service)->working_mode == WORK_PROC)
                {
                        (*service)->procthread->clean(&(*service)->procthread);
			free((*service));
			(*service) = NULL;
			return ;
                }	
	}		
}
