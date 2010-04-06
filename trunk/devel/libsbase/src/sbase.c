#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include "sbase.h"
#include "logger.h"
#include "queue.h"
#include "message.h"
#include "evtimer.h"
#include "xssl.h"

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
            fprintf(stdout, "getrlimit %s cur[%ld] max[%ld]\n",
                    name, (long)rlim.rlim_cur, (long)rlim.rlim_max);
        }
        if(rlim.rlim_cur > nset && rlim.rlim_max > nset)
            return 0;
        rlim.rlim_cur = nset;
        rlim.rlim_max = nset;
        if((ret = setrlimit(rlimit, &rlim)) == 0)
        {
            fprintf(stdout, "setrlimit %s cur[%ld] max[%ld]\n",
                    name, (long)rlim.rlim_cur, (long)rlim.rlim_max);
            return 0;
        }
        else
        {
            fprintf(stderr, "setrlimit %s cur[%ld] max[%ld] failed, errno:%d\n",
                    name, (long)rlim.rlim_cur, (long)rlim.rlim_max, errno);
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
void sbase_evtimer_handler(void *arg)
{
	SBASE *sbase = NULL;
	if((sbase = (SBASE *)arg))
	{
                fprintf(stdout, "%s::%d::Ready for stop sbase\n", __FILE__, __LINE__);
		sbase->running_status = 0;
	}
	return ;
}

/* running all service */
int sbase_running(SBASE *sbase, int useconds)
{
    struct timeval tv = {0};
    SERVICE *service = NULL;
    int ret = -1, i = -1;
    pid_t pid = 0;

    if(sbase)
    {
        if(useconds > 0 )
        {
            EVTIMER_ADD(sbase->evtimer, useconds,
                    &sbase_evtimer_handler, (void *)sbase, sbase->evid);
        }
        if(sbase->nchilds > SB_THREADS_MAX) sbase->nchilds = SB_THREADS_MAX;
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
        tv.tv_usec = SB_USEC_SLEEP;
        if(sbase->usec_sleep > 0) tv.tv_usec = sbase->usec_sleep;
        do
        {
            //running evbase 
            sbase->evbase->loop(sbase->evbase, 0, &tv);
            //sbase->nheartbeat++;
            //check evtimer for heartbeat and timeout
            EVTIMER_CHECK(sbase->evtimer);
            //EVTIMER_LIST(sbase->evtimer, stdout);
            //running message queue
            if(QMTOTAL(sbase->message_queue) > 0)
                qmessage_handler(sbase->message_queue, sbase->logger);
            //else usleep(1);
        }while(sbase->running_status);
        /* handler left message */
        if(QMTOTAL(sbase->message_queue) > 0)
            qmessage_handler(sbase->message_queue, sbase->logger);
        /* stop service */
        for(i = 0; i < sbase->running_services; i++)
        {
            if(sbase->services[i])
            {
                sbase->services[i]->stop(sbase->services[i]);
            }
        }
        ret = 0;
    }
    return ret;
}

void sbase_stop(SBASE *sbase)
{
    if(sbase)
    {
        sbase->running_status = 0;
        return ;
    }
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
        if((*psbase)->evtimer){EVTIMER_CLEAN((*psbase)->evtimer);}
        if((*psbase)->logger){LOGGER_CLEAN((*psbase)->logger);}
        if((*psbase)->evbase){(*psbase)->evbase->clean(&((*psbase)->evbase));}
        if((*psbase)->message_queue){qmessage_clean((*psbase)->message_queue);}
#ifdef HAVE_SSL
        ERR_free_strings();
#endif
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
        EVTIMER_INIT(sbase->evtimer);
        sbase->message_queue    = qmessage_init();
        //QUEUE_INIT(sbase->message_queue);
        sbase->set_log		    = sbase_set_log;
        sbase->set_evlog	    = sbase_set_evlog;
        sbase->add_service	    = sbase_add_service;
        sbase->running 		    = sbase_running;
        sbase->stop 		    = sbase_stop;
        sbase->clean 		    = sbase_clean;
#ifdef HAVE_SSL
        sbase->ssl_id           = SSL_library_init();
#endif
        //sbase->evbase->set_evops(sbase->evbase, EOP_POLL);
    }
    return sbase;
}
