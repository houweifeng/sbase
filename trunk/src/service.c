#include "service.h"

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
		if((service->fd = socket(service->family, service->sock_type, NULL)) <= 0 )
		{
			FATAL_LOG(service->logger, "Initialize socket failed, %s", strerror(errno));
			return -1;
		}
		service->sa.sin_addr.s_addr = if(service->ip)?inet_addr(service->ip):INADDR_ANY;
		service->sa.sin_port = htons(service->port); 
		//setsockopt 
		setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR,
				(char *)&opt, (socklen_t)sizeof(opt) );
		//Bind 
		if(bind(service->fd, (struct sockaddr *)&(service->sa), (socklen_t)sizeof(sa)) != 0 )
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
		if(service->ev_eventbase == NULL)
		{
			 FATAL_LOG(service->logger, "Eventbase on fd[%d]  %s:%d is NULL",
                                        service->fd, service->ip, service->port);
			return -1;
		}
		event_set(&(service->ev_event), service->fd, EV_READ | EV_PERSIST,
				service->event_handler, (void *)service );
		event_base_set(service->ev_eventbase, &(service->ev_event));
		event_add(&(service->ev_event), NULL);
	}
	return -1;
}

/* Run service */
void service_run(SERVICE *service)
{
	int i = 0;
#ifdef HAVE_PTHREAD_H
	pthread_t procthread_id;
#endif
	/* Running */
	if(service)
	{
#ifdef	HAVE_PTHREAD_H
		if(service->work == WORK_PROC) goto work_proc_init;
		else goto work_thread_init;
#endif
work_proc_init:
		/* Initialize procthread(s) */
		if((servuce->procthread = procthread_init()))
		{
			servuce->procthread->service = service;
			service->procthread->logger = service->logger;
			servuce->procthread->set(servuce->procthread);
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
		service->procthreads = (PROCTHREAD **)calloc(service->max_threads,
				sizeof(PROCTHREAD *));
		if(service->procthreads == NULL)
		{
			FATAL_LOG(service->logger, "Initialize procthreads pool failed, %s",
					strerror(errno));
			exit(EXIT_FAILURE);
		}
		for(i = 0; i < service->max_threads; i++)
		{
			if((service->procthreads[i] = procthread_init()))
			{
				service->service = service;
				service->procthreads[i]->logger = service->logger;
				service->procthreads[i]->set(service->procthreads[i]);	
			}
			else
			{
				FATAL_LOG(service->logger, "Initialize procthreads pool failed, %s",
						strerror(errno));
				exit(EXIT_FAILURE);
			}
#ifdef HAVE_PTHREAD_H
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
				service->addconn(service, fd, rsa);
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
void service_addconn(SERVICE *service, int fd,  struct sockaddr_in sa)
{
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
		/* Add connection for procthread */
		if(service->work == WORK_PROC && service->procthread)
		{
			service->procthread->add_connections(service->procthread, fd, sa);
			return ;
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
		if(service->work == WORK_PROC)
		{
			service->procthread->terminate(service->procthread);
			shutdown(service->fd, SHUT_RDWR)
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
		if((*service)->work == WORK_PROC)
                {
                        (*service)->procthread->clean(&(*service)->procthread);
			free((*service));
			(*service) = NULL;
			return ;
                }	
	}		
}
