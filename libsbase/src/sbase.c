#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include "sbase.h"
#include "logger.h"
#include "queue.h"
#include "message.h"

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
			fprintf(stdout, "setrlimit %s cur[%ld] max[%ld]\n",  
					name, rlim.rlim_cur, rlim.rlim_max);
            ret = 0;
		}
		else
		{
			fprintf(stderr, "setrlimit %s cur[%ld] max[%ld] failed, %s\n",
					name, rlim.rlim_cur, rlim.rlim_max, strerror(errno));
		}
	}
	return ret;
}


/* set sbase log */
int sbase_set_log(SBASE *sbase, char *logfile)
{
    int ret = -1;
    if(sbase && logfile)
    {
        LOGGER_INIT(sbase->logger, logfile);
        ret = 0;
    }
    return ret;
}

/* set evbase log */
int sbase_set_evlog(SBASE *sbase, char *evlogfile)
{
    int ret = -1;
    if(sbase && evlogfile && sbase->evbase)
    {
        sbase->evbase->set_logfile(sbase->evbase, evlogfile);
        ret = 0;
    }
    return ret;
}

/* add service to sbase */
int sbase_add_service(SBASE *sbase, SERVICE  *service)
{
	if(sbase)
	{
        if(service)
        {
            service->evbase = sbase->evbase;
            service->sbase  = sbase;
            service->message_queue = sbase->message_queue;
            service->usec_sleep = sbase->usec_sleep;
            service->connections_limit = sbase->connections_limit;
            if(service->logger == NULL) service->logger = sbase->logger;
            if((sbase->services = (SERVICE **)realloc(sbase->services, 
                            (sbase->running_service + 1) * sizeof(SERVICE *))))
            {
                sbase->services[sbase->running_service++] = service;
                return ((service->set(service) == 0)? service->run(service) : -1);
            }
        }
	}
	return -1;
}

/* running all service */
int sbase_running(SBASE *sbase, int useconds)
{
    int ret = -1;
    int i = 0;
    SERVICE *service = NULL;

	if(sbase)
	{
        //running sbase 
        sbase->running_status = 1;
		while(sbase->running_status)
		{
			sbase->evbase->loop(sbase->evbase, 0, NULL);
            if(QTOTAL(sbase->message_queue) > 0)
            {
                message_handler(sbase->message_queue, sbase->logger);
            }
			usleep(sbase->usec_sleep);
		}
        ret = 0;
	}
    return ret;
}

/* stop all service */
void sbase_stop(SBASE *sbase)
{
    if(sbase)
    {
        sbase->running_status = 0;
    }
    return ;
}

/* clean sbase */
void sbase_clean(SBASE **psbase)
{
    if(*psbase)
    {
        if((*psbase)->services) free((*psbase)->services);
        if((*psbase)->logger) LOGGER_CLEAN((*psbase)->services);
        if((*psbase)->evbase) (*psbase)->evbase->clean(&((*psbase)->evbase));
        if((*psbase)->message_queue) QUEUE_CLEAN((*psbase)->message_queue);
        free(*psbase);
        *psbase = NULL;
    }
}

/* Initialize sbase */
SBASE *sbase_init()
{
	SBASE *sbase = NULL;
	if((sbase = (SBASE *)calloc(1, sizeof(SBASE))))
	{
        sbase->message_queue    = QUEUE_INIT();
		sbase->evbase        	= evbase_init();
		sbase->setrlimit     	= sbase_setrlimit;
		sbase->set_log		    = sbase_set_log;
		sbase->set_evlog	    = sbase_set_evlog;
		sbase->add_service	    = sbase_add_service;
		sbase->running 		    = sbase_running;
		sbase->stop 		    = sbase_stop;
		sbase->clean 		    = sbase_clean;
        sbase->evbase->set_evops(sbase->evbase, EOP_POLL);
	}
	return sbase;
}
