#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "sbase.h"
#include "logger.h"
/* sbase event handler */
void sbase_event_handler(int ev_fd, short event, void *arg)
{
	int fd;
	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(sa);
	SBASE *sbase = (SBASE *)arg;

	if(ev_fd == sbase->fd)
	{
		if(event & E_READ)
		{
			if((fd = accept(ev_fd, (struct sockaddr *)&sa, &sa_len)) > 0)
			{
				fprintf(stdout, "Accepted new connection[%s:%d] via %d\n", 
					inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), fd);	
			}
			else
			{
				fprintf(stderr, "Accept new connection failed, %s\n", strerror(errno));
			}
		}	
	}
}

/* set resource limit */
int sbase_setrlimit(SBASE *sb, char *name, int rlimit, int nset)
{
	int ret = -1;
	struct rlimit rlim;
	if(sb)
	{
		if(getrlimit(rlimit, &rlim) == -1)
			return -1;
		if(rlim.rlim_cur > nset && rlim.rlim_max > nset)
			return 0;
		rlim.rlim_cur = nset;
		rlim.rlim_max = nset;
		if((ret = setrlimit(rlimit, &rlim)) == 0)
		{
			DEBUG_LOGGER(sb->logger, "setrlimit %s cur[%ld] max[%ld]",  
					name, rlim.rlim_cur, rlim.rlim_max);
		}
		else
		{
			FATAL_LOGGER(sb->logger, "setrlimit %s cur[%ld] max[%ld] failed, %s",
					name, rlim.rlim_cur, rlim.rlim_max, strerror(errno));
		}
	}
	return ret;
}


/* set sbase log */
int sbase_set_log(SBASE *sbase, char *logfile)
{
}

/* set evbase log */
int sbase_set_evlog(SBASE *sbase, char *evlogfile)
{
}

/* add service to sbase */
int sbase_add_service(SBASE *sbase, void *service)
{
	struct sockaddr_in sa;
	int opt = 1;
	EVENT *event = NULL;
	if(sbase)
	{
		if((sbase->fd = socket(AF_INET, SOCK_STREAM, 0)) > 0 && fcntl(sbase->fd, F_SETFL, O_NONBLOCK) == 0)
		{
			memset(&sa, 0, sizeof(sa));
			sa.sin_family = AF_INET;
			sa.sin_addr.s_addr = INADDR_ANY;
			sa.sin_port = htons(3721);
			if(setsockopt(sbase->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == 0
			&& bind(sbase->fd, (struct sockaddr *)&sa, sizeof(sa)) == 0 && listen(sbase->fd, 65536) == 0)
			{
				if((event = ev_init()))
				{
					event->set(event, sbase->fd, E_READ|E_PERSIST, sbase, (void *)sbase_event_handler);
					sbase->evbase->add(sbase->evbase, event);
					fprintf(stdout, "bind %d to 0.0.0.0:%d successed\n", sbase->fd, 3721);
					return 0;
				}
			}
		}	
	}
	return -1;
}

/* running all service */
int sbase_running(SBASE *sbase, int useconds)
{
	if(sbase)
	{
		while(1)
		{
			sbase->evbase->loop(sbase->evbase, 0, NULL);
			usleep(sbase->usec_sleep);
		}
	}
}

/* stop all service */
int sbase_stop(SBASE *sbase)
{
}

/* Initialize sbase */
SBASE *sbase_init()
{
	SBASE *sbase = NULL;
	if((sbase = (SBASE *)calloc(1, sizeof(SBASE))))
	{
		sbase->evbase        	= evbase_init();
		sbase->event_handler 	= sbase_event_handler;
		sbase->setrlimit     	= sbase_setrlimit;
		sbase->set_log		= sbase_set_log;
		sbase->set_evlog	= sbase_set_evlog;
		sbase->add_service	= sbase_add_service;
		sbase->running 		= sbase_running;
		sbase->stop 		= sbase_stop;
	}
	return sbase;
}
