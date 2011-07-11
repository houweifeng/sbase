#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include "sbase.h"
#include "logger.h"
#include "message.h"
#include "evtimer.h"
#include "xssl.h"
#include "xmm.h"
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

/* set sbase log level  */
int sbase_set_log_level(SBASE *sbase, int level)
{
    int ret = -1;
    if(sbase && sbase->logger)
    {
        LOGGER_SET_LEVEL(sbase->logger, level);
        ret = 0;
    }
    return ret;
}


/* set evbase log */
int sbase_set_evlog(SBASE *sbase, char *evlogfile)
{
    int ret = -1;
    if(sbase && evlogfile)
    {
        sbase->evlogfile = evlogfile;
        ret = 0;
    }
    return ret;
}

/* set evbase log level */
int sbase_set_evlog_level(SBASE *sbase, int level)
{
    int ret = -1;
    if(sbase)
    {
        sbase->evlog_level = level;
        ret = 0;
    }
    return ret;
}


/* add service to sbase */
int sbase_add_service(SBASE *sbase, SERVICE  *service)
{
    int i = 0;

    if(sbase)
    {
        if(service && sbase->running_services < SB_SERVICE_MAX)
        {
            service->evbase = sbase->evbase;
            if(service->working_mode == WORKING_PROC) 
                service->evtimer = sbase->evtimer;
            else 
                service->evtimer = service->etimer;
            service->sbase  = sbase;
            service->message_queue = sbase->message_queue;
            service->usec_sleep = sbase->usec_sleep;
            service->connections_limit = sbase->connections_limit;
            if(service->logger == NULL) 
            {
                fprintf(stdout, "replace %s logger with sbase\n", service->service_name);
                service->logger = sbase->logger;
            }
            for(i = 0; i < SB_SERVICE_MAX; i++)
            {
                if(sbase->services[i] == NULL)
                {
                    sbase->services[i] = service;
                    sbase->running_services++;
                    break;
                }
            }
            return service->set(service);
        }
    }
    return -1;
}

/* sbase remove service */
void sbase_remove_service(SBASE *sbase, SERVICE *service)
{
    int i = 0;

    if(sbase && service)
    {
        for(i = 0; i < SB_SERVICE_MAX; i++)    
        {
            if(sbase->services[i] == service)
            {
                sbase->services[i] = NULL;
                sbase->running_services--;
                break;
            }
        }
    }
    return ;
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
            sbase->evid = EVTIMER_ADD(sbase->evtimer, useconds,
                    &sbase_evtimer_handler, (void *)sbase);
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
        sbase->evbase   = evbase_init(1);
        if(sbase->evlogfile && sbase->evlog_level > 0) 
        {
            sbase->evbase->set_logfile(sbase->evbase, sbase->evlogfile);
            sbase->evbase->set_log_level(sbase->evbase, sbase->evlog_level);
        }
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
        if(sbase->usec_sleep > 1000000) tv.tv_sec = sbase->usec_sleep/1000000;
        tv.tv_usec = sbase->usec_sleep % 1000000;
        do
        {
            //running evbase 
            i = sbase->evbase->loop(sbase->evbase, 0, &tv);
            //sbase->evbase->loop(sbase->evbase, 0, &tv);
            //sbase->nheartbeat++;
            //check evtimer for heartbeat and timeout
            EVTIMER_CHECK(sbase->evtimer);
            //EVTIMER_LIST(sbase->evtimer, stdout);
            //running message queue
            if(QMTOTAL(sbase->message_queue) > 0)
            {
                qmessage_handler(sbase->message_queue, sbase->logger);
                i = 1;
            }
            //if(i == 0){usleep(sbase->usec_sleep); k = 0;}
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
    if(sbase && sbase->running_status)
    {
        sbase->running_status = 0;
        return ;
    }
}

/* clean sbase */
void sbase_clean(SBASE *sbase)
{
    int i = 0;

    if(sbase)
    {
        for(i = 0; i < sbase->running_services; i++)
        {
            if(sbase->services[i])
                sbase->services[i]->clean(sbase->services[i]);
        }
        if(sbase->evtimer){EVTIMER_CLEAN(sbase->evtimer);}
        if(sbase->logger){LOGGER_CLEAN(sbase->logger);}
        if(sbase->evbase){sbase->evbase->clean(sbase->evbase);}
        if(sbase->message_queue){qmessage_clean(sbase->message_queue);}
#ifdef HAVE_SSL
        ERR_free_strings();
#endif
        xmm_free(sbase, sizeof(SBASE));
    }
    return ;
}

/* Initialize sbase */
SBASE *sbase_init()
{
    SBASE *sbase = NULL;
    if((sbase = (SBASE *)xmm_new(sizeof(SBASE))))
    {
        sbase->evtimer          = EVTIMER_INIT();
        sbase->message_queue    = qmessage_init();
        sbase->set_log		    = sbase_set_log;
        sbase->set_log_level	= sbase_set_log_level;
        sbase->set_evlog	    = sbase_set_evlog;
        sbase->set_evlog_level	= sbase_set_evlog_level;
        sbase->add_service	    = sbase_add_service;
        sbase->remove_service	= sbase_remove_service;
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
