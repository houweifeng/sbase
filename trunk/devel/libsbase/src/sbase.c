#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include "sbase.h"
#include "logger.h"
#include "queue.h"
#include "message.h"
#include "timer.h"
#include "evtimer.h"

/* set rlimit */
int setrlimiter(char *name, int rlimit, int nset)
{
    int ret = -1;
    struct rlimit rlim;
    if(name)
    {
        if(getrlimit(rlimit, &rlim) == -1)
            return -1;
        else
        {
            fprintf(stdout, "getrlimit %s cur[%d] max[%ld]\n",
                    name, rlim.rlim_cur, rlim.rlim_max);
        }
        if(rlim.rlim_cur > nset && rlim.rlim_max > nset)
            return 0;
        rlim.rlim_cur = nset;
        rlim.rlim_max = nset;
        if((ret = setrlimit(rlimit, &rlim)) == 0)
        {
            fprintf(stdout, "setrlimit %s cur[%ld] max[%ld]\n",
                    name, rlim.rlim_cur, rlim.rlim_max);
            return 0;
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
            service->evtimer = sbase->evtimer;
            service->sbase  = sbase;
            service->message_queue = sbase->message_queue;
            service->usec_sleep = sbase->usec_sleep;
            service->connections_limit = sbase->connections_limit;
            if(service->logger == NULL) 
            {
                fprintf(stdout, "replace %s logger with sbase\n", service->service_name);
                service->logger = sbase->logger;
            }
            if((sbase->services = (SERVICE **)realloc(sbase->services, 
                            (sbase->running_services + 1) * sizeof(SERVICE *))))
            {
                sbase->services[sbase->running_services++] = service;
                return service->set(service);
            }
        }
    }
    return -1;
}

/* sbase evtimer  handler */
void sbase_evtimer_handler(void *arg, int id)
{
	SBASE *sbase = NULL;
	if((sbase = (SBASE *)arg) && sbase->evid == id)
	{
		sbase->running_status = 0;
	}
	return ;
}

/* running all service */
int sbase_running(SBASE *sbase, int useconds)
{
    int ret = -1, i = -1;
    pid_t pid = 0;
    SERVICE *service = NULL;

    if(sbase)
    {
	if(useconds > 0 )
	{
		  EVTIMER_ADD(sbase->evtimer, useconds,
                    &sbase_evtimer_handler, (void *)sbase, sbase->evid);
	}
        if(sbase->nchilds > SB_NDAEMONS_MAX) sbase->nchilds = SB_NDAEMONS_MAX;
        //nproc
        if(sbase->nchilds > 0)
        {
            for(i = 0; i < sbase->nchilds; i++)
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
            return 0;
        }
running:
        //evbase 
        sbase->evbase           = evbase_init();
        //running services
        if(sbase->services)
        {
            for(i = 0; i < sbase->running_services; i++)
            {
                if((service = sbase->services[i]))
                {
                    service->evbase = sbase->evbase;
                    service->run(service);
                }
            }
        }
        //running sbase 
        sbase->running_status = 1;
        while(sbase->running_status)
        {
            //running evbase 
            sbase->evbase->loop(sbase->evbase, 0, NULL);
            //sbase->nheartbeat++;
            //check evtimer for heartbeat and timeout
            EVTIMER_CHECK(sbase->evtimer);
            //running message queue
            if(QTOTAL(sbase->message_queue) > 0)
            {
                message_handler(sbase->message_queue, sbase->logger);
            }
            //running and check timeout
            /*
            if(useconds > 0 && TIMER_CHECK(sbase->timer, useconds)  == 0)
            {
                break;
            }*/
            usleep(sbase->usec_sleep);
        }
        ret = 0;
    }
    return ret;
}

/* stop all service */
void sbase_stop(SBASE *sbase)
{
    int i = 0;

    if(sbase)
    {
        sbase->running_status = 0;
        for(i = 0; i < sbase->running_services; i++)
        {
            if(sbase->services[i])
            {
                sbase->services[i]->stop(sbase->services[i]);
            }
        }
    }
    return ;
}

/* clean sbase */
void sbase_clean(SBASE **psbase)
{
    int i = 0;

    if(psbase && *psbase)
    {
        if((*psbase)->services) 
        {
            for(i = 0; i < (*psbase)->running_services; i++)
            {
                if((*psbase)->services[i])
                    (*psbase)->services[i]->clean(&((*psbase)->services[i]));
            }
            free((*psbase)->services);
        }
        if((*psbase)->timer) TIMER_CLEAN((*psbase)->timer);
        if((*psbase)->evtimer) EVTIMER_CLEAN((*psbase)->evtimer);
        if((*psbase)->logger) LOGGER_CLEAN((*psbase)->logger);
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
        TIMER_INIT(sbase->timer);
        EVTIMER_INIT(sbase->evtimer);
        QUEUE_INIT(sbase->message_queue);
        sbase->set_log		    = sbase_set_log;
        sbase->set_evlog	    = sbase_set_evlog;
        sbase->add_service	    = sbase_add_service;
        sbase->running 		    = sbase_running;
        sbase->stop 		    = sbase_stop;
        sbase->clean 		    = sbase_clean;
        //sbase->evbase->set_evops(sbase->evbase, EOP_POLL);
    }
    return sbase;
}
