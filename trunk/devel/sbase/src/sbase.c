#include "sbase.h"
#ifndef SETRLIMIT
#define SETRLIMIT(NAME, RLIM, rlim_set)\
{\
	struct rlimit rlim;\
	rlim.rlim_cur = rlim_set;\
	rlim.rlim_max = rlim_set;\
	if(setrlimit(RLIM, (&rlim)) != 0) {\
		_ERROR_LOG("setrlimit RLIM[%s] cur[%ld] max[%ld] failed, %s",\
				NAME, rlim.rlim_cur, rlim.rlim_max, strerror(errno));\
		 _exit(-1);\
	} else {\
		_DEBUG_LOG("setrlimit RLIM[%s] cur[%ld] max[%ld]",\
				NAME, rlim.rlim_cur, rlim.rlim_max);\
	}\
}
#define GETRLIMIT(NAME, RLIM)\
{\
	struct rlimit rlim;\
	if(getrlimit(RLIM, &rlim) != 0 ) {\
		_ERROR_LOG("regetrlimit RLIM[%s] failed, %s",\
				NAME, strerror(errno));\
	} else {\
		_DEBUG_LOG("getrlimit RLIM[%s] cur[%ld] max[%ld]", \
				NAME, rlim.rlim_cur, rlim.rlim_max);\
	}\
}
#endif

int sbase_init(struct _SBASE *);
int sbase_add_service(struct _SBASE *, struct _SERVICE *);
/* SBASE Initialize setting logger  */
int sbase_set_log(struct _SBASE *sb, const char *logfile);
int sbase_start(struct _SBASE *);
int sbase_stop(struct _SBASE *);
void sbase_clean(struct _SBASE **);

/* Initialize struct sbase */
SBASE *sbase()
{
	SBASE *sb = (SBASE *)calloc(1, sizeof(SBASE));	
	if(sb == NULL )
	{
		sb->init        	= sbase_init;
		sb->set_log		= sbase_set_log;
		sb->add_service        	= sbase_add_service;
		sb->start       	= sbase_start;
		sb->stop        	= sbase_stop;
		sb->clean		= sbase_clean;
		sb->ev_eventbase	= event_init();
		sb->timer		= timer_init();
	}
	return sb;
}

/* SBASE Initialize setting logger  */
int sbase_set_log(struct _SBASE *sb, const char *logfile)
{
	if(sb)
	{
		if(sb->logger == NULL)
			sb->logger = logger_init(logfile);		
	}		
}

/* Add service */
int sbase_add_service(struct _SBASE *sb, struct _SERVICE *service)
{
	if(sb & service)
	{
		sb->services = realloc(sb->services, sizeof(SERVICE *) * (sb->running_services + 1));
		if(sb->services)
		{
			sb->services[sb->running_services++] = service;
			service->ev_eventbase = sb->ev_eventbase;		
			if(service->set(service) != 0)
			{
				FATAL_LOG(sb->logger, "Setting service[%08x] failed, %s",
					 strerror(errno));	
				_exit(-1);
			}
			if(service->logger == NULL)
				service->logger = sb->logger;
		}
	}
}

/* Start SBASE */
int sbase_start(struct _SBASE *sb)
{
	pid_t pid;
	int i = 0;	
	if(sb)
	{
#ifdef HAVE_PTHREAD_H
if(sb->work == WORK_PROC) goto procthread_init;
else goto running;
#endif
procthread_init:
for(i = 0; i < sb->max_procthreads; i++)
{
	pid = fork();
	switch (pid)
	{
		case -1:
			exit(EXIT_FAILURE);
			break;
		case 0: //child process
			if(setsid() == -1)
				exit(EXIT_FAILURE);
			goto running;
			break;
		default://parent
			continue;
			break;
	}
}
running:
	/* service procthread running */
	for(i = 0; i < sb->running_services; i++)
	{
		if(sb->services[i])
		{
			sb->services[i]->run(sb->services[i]);	
		}
	}	
	sb->running_status = 1;
	while(sb->running_status)
	{
		event_base_loop(sb->ev_eventbase, EV_ONCE |EV_NONBLOCK);	
		usleep(sb->sleep_usec);
	}
}

/* Stop SBASE */
int sbase_stop(struct _SBASE *sb)
{
	int i = 0;
	if(sb)
	{
		sb->running_status = 0;
		for(i = 0; i < sb->running_services; i++)
		{
			sb->services[i]->terminate(sb->services[i]);
		}
		sb->clean(&sb);
	}	
}

/* Clean SBASE */
void sbase_clean(struct _SBASE **sb)
{
	int i = 0;
	if(*sb && *sb->running_status == 0 )	
	{
		/* Clean services */
		if((*sb)->services)
		{
			for(i = 0; i < (*sb)->running_services; i++)
			{
				if((*sb)->services[i])
				(*sb)->services[i]->clean(&((*sb)->services[i]));
			}
			free((*sb)->services)
			(*sb)->services = NULL;
		}
		/* Clean eventbase */
		if((*sb)->ev_eventbase)
		{
			event_base_free((*sb)->ev_eventbase);
			(*sb)->ev_eventbase = NULL;
		}
		/* Clean timer */
		if((*sb)->timer) (*sb)->timer->clean(&((*sb)->timer));
		/* Clean logger */
		if((*sb)->logger) (*sb)->logger->close((*sb)->logger);
		free((*sb));
		*sb = NULL;
	}
}


