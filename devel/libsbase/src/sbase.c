#include <evbase.h>
#include <sys/resource.h>
#include "sbase.h"
#include "timer.h"
#include "logger.h"
#include "service.h"
#include "procthread.h"
#include "message.h"


int sbase_add_service(struct _SBASE *, struct _SERVICE *);
/* SBASE Initialize setting logger  */
int sbase_set_log(struct _SBASE *sb, char *logfile);
/* SBASE Initialize setting evbase logger  */
int sbase_set_evlog(struct _SBASE *sb, char *evlogfile);
/* SBASE set max_connection */
int sbase_setrlimit(struct _SBASE *sb, char *name, int rlimit, int nset);
/* Running as timer limit */
int sbase_running(SBASE *sb, uint32_t seconds);
/* Running once */
void sbase_running_once(SBASE *sb);
int sbase_stop(struct _SBASE *);
void sbase_clean(struct _SBASE **);

/* Initialize struct sbase */
SBASE *sbase_init()
{
    int n = 0;
    SBASE *sb = (SBASE *)calloc(1, sizeof(SBASE));	
    if(sb)
    {
        sb->set_log		    = sbase_set_log;
        sb->set_evlog		= sbase_set_evlog;
        sb->setrlimit       = sbase_setrlimit;
        sb->add_service     = sbase_add_service;
        sb->running       	= sbase_running;
        sb->running_once	= sbase_running_once;
        sb->stop        	= sbase_stop;
        sb->clean		    = sbase_clean;
        sb->evbase		    = evbase_init();
        sb->message_queue	= QUEUE_INIT();
        TIMER_INIT(sb->timer);
	sb->setrlimit(sb, "RLIMIT_NOFILE", RLIMIT_NOFILE, CONN_MAX);
        //sb->evbase->set_evops(sb->evbase, EOP_POLL);
    }
    return sb;
}

/* SBASE Initialize setting logger  */
int sbase_set_log(struct _SBASE *sb, char *logfile)
{
    if(sb)
    {
        if(sb->logger == NULL)
            LOGGER_INIT(sb->logger, logfile);
    }		
}

/* SBASE Initialize setting evlogger  */
int sbase_set_evlog(struct _SBASE *sb, char *evlogfile)
{
    if(sb)
    {
        if(sb->evbase) sb->evbase->set_logfile(sb->evbase, evlogfile);
    }
}

/* SBASE set max_connection */
int sbase_setrlimit(struct _SBASE *sb, char *name, int rlimit, int nset)
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

/* Add service */
int sbase_add_service(struct _SBASE *sb, struct _SERVICE *service)
{
    if(sb && service)
    {
        if(sb->services)
            sb->services = (SERVICE **)realloc(sb->services, 
                    sizeof(SERVICE *) * (sb->running_services + 1));
        else
            sb->services = (SERVICE **)calloc(1, sizeof(SERVICE *));
        if(sb->services)
        {
            sb->services[sb->running_services++] = service;
            service->evbase = sb->evbase;
            service->message_queue = sb->message_queue;
            service->working_mode = sb->working_mode;
            if(service->set(service) != 0)
            {
                FATAL_LOGGER(sb->logger, "Setting service[%08x] failed, %s",
                        service, strerror(errno));	
                return -1;
            }
            if(service->logger == NULL)
                service->logger = sb->logger;
            DEBUG_LOGGER(sb->logger, "Added service[%08x][%s] to sbase[%08x]",
                    service, service->name, sb);
            return 0;
        }
    }
    return -1;
}
/* heartbeat */
int sbase_state(struct _SBASE *sb)
{
    int i = 0;
    SERVICE *service = NULL;

    if(sb)
    {
        for(i = 0; i < sb->running_services; i++)
        {
            if((service = sb->services[i]))
            {
                service->active_heartbeat(service); 
            }
        }

    }
    return ;
}

/* Running SBASE */
int sbase_running(struct _SBASE *sb, uint32_t seconds)
{
    pid_t pid;
    int i = 0, j = 0;
    MESSAGE *msg = NULL;
    PROCTHREAD *pth = NULL;
    CONN *conn = NULL;
    TIMER *timer = NULL;
    if(sb)
    {
#ifdef HAVE_PTHREAD
        if(sb->working_mode == WORKING_PROC) 
            goto procthread_init;
        else 
            goto running;
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
        return 0;
running:
        /* service procthread running */
        for(j = 0; j < sb->running_services; j++)
        {
            if(sb->services[j])
            {
                sb->services[j]->run(sb->services[j]);	
            }
        }	
        TIMER_INIT(timer);
        sb->running_status = 1;
        if(sb->working_mode != WORKING_PROC)
        {
            if(seconds > 0) 
            {
                TIMER_SAMPLE(timer);
                while(sb->running_status)
                {
                    if(TIMER_CHECK(timer, seconds) == 0) sb->stop(sb);
                    sb->evbase->loop(sb->evbase, 0, NULL);
                    sbase_state(sb);
                    usleep(sb->sleep_usec);
                }
            }
            else
            {
                while(sb->running_status)
                {
                    sb->evbase->loop(sb->evbase, 0, NULL);
                    sbase_state(sb);
                    usleep(sb->sleep_usec);
                }
            }
            return ;
        }
        else
        {
            /* only for WORKING_PROC */
            if(seconds > 0 )
            {
                TIMER_SAMPLE(timer);
                while(sb->running_status)
                {
                    if(TIMER_CHECK(timer, seconds) == 0) sb->stop(sb);
                    sb->evbase->loop(sb->evbase, 0, NULL);
                    sb->running_once(sb);
                    sbase_state(sb);
                    usleep(sb->sleep_usec);
                }
            }
            else
            {
                while(sb->running_status)
                {
                    sb->evbase->loop(sb->evbase, 0, NULL);
                    sb->running_once(sb);
                    sbase_state(sb);
                    usleep(sb->sleep_usec);
                }
            }
        }
    }
}

/* Running once */
void sbase_running_once(SBASE *sb)
{
    CONN    *conn = NULL;
    PROCTHREAD *pth = NULL;
    MESSAGE msg;
    if(sb)
    {
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
            sb->services[i]->clean(&(sb->services[i]));
        }
        sb->clean(&sb);
    }	
}

/* Clean SBASE */
void sbase_clean(struct _SBASE **sb)
{
    int i = 0;
    MESSAGE msg = {0};
    if((*sb) && (*sb)->running_status == 0 )	
    {
        /* Clean services */
        if((*sb)->services)
        {
            for(i = 0; i < (*sb)->running_services; i++)
            {
                if((*sb)->services[i]) (*sb)->services[i]->clean(&((*sb)->services[i]));
            }
            free((*sb)->services);
            (*sb)->services = NULL;
        }
        /* Clean eventbase */
        if((*sb)->evbase) (*sb)->evbase->clean(&((*sb)->evbase));
        /* Clean timer */
        TIMER_CLEAN((*sb)->timer);
        /* Clean logger */
        if((*sb)->logger) LOGGER_CLEAN((*sb)->logger);
        /* Clean message queue */
        if((*sb)->message_queue) 
	{
		QUEUE_CLEAN((*sb)->message_queue); 
	}
        free((*sb));
        (*sb) = NULL;
    }
}
