#include <evbase.h>
#include <sys/resource.h>
#include "sbase.h"
#include "timer.h"
#include "logger.h"
#include "service.h"
#include "procthread.h"
#include "buffer.h"
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
		sb->set_log		= sbase_set_log;
		sb->set_evlog		= sbase_set_evlog;
        sb->setrlimit  = sbase_setrlimit;
		sb->add_service        	= sbase_add_service;
		sb->running       	= sbase_running;
		sb->running_once	= sbase_running_once;
		sb->stop        	= sbase_stop;
		sb->clean		= sbase_clean;
		sb->evbase		= evbase_init();
		sb->timer		= timer_init();
		sb->message_queue	= queue_init();
	}
	return sb;
}

/* SBASE Initialize setting logger  */
int sbase_set_log(struct _SBASE *sb, char *logfile)
{
	if(sb)
	{
		if(sb->logger == NULL)
			sb->logger = logger_init((char *)logfile);		
	}		
}

/* SBASE Initialize setting evlogger  */
int sbase_set_evlog(struct _SBASE *sb, char *evlogfile)
{
    if(sb)
    {
        if(sb->evlogger == NULL)
            sb->evlogger = logger_init((char *)evlogfile);
        if(sb->evbase)
            sb->evbase->logger = sb->evlogger;
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
		sb->running_status = 1;
		if(sb->working_mode != WORKING_PROC)
		{
			if(seconds > 0) 
			{
				timer = timer_init();
				while(sb->running_status)
				{
					if(timer)
					{
						timer->sample(timer);
						if(timer->sec_used >= seconds)
							sb->stop(sb);
					}
					sb->evbase->loop(sb->evbase, 0, NULL);
					usleep(sb->sleep_usec);
				}
			}
			else
			{
				while(sb->running_status)
				{
					sb->evbase->loop(sb->evbase, 0, NULL);
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
				timer = timer_init();
				while(sb->running_status)
				{
					if(timer)
					{
						timer->sample(timer);
						if(timer->sec_used >= seconds)
							sb->stop(sb);
					}
					sb->evbase->loop(sb->evbase, 0, NULL);
					sb->running_once(sb);
					usleep(sb->sleep_usec);
				}
			}
			else
			{
				while(sb->running_status)
				{
					sb->evbase->loop(sb->evbase, 0, NULL);
					sb->running_once(sb);
					usleep(sb->sleep_usec);
				}
			}
		}
	}
}

/* Running once */
void sbase_running_once(SBASE *sb)
{
	MESSAGE *msg = NULL;
	CONN    *conn = NULL;
	PROCTHREAD *pth = NULL;
	if(sb)
	{
		msg = sb->message_queue->pop(sb->message_queue);
		if(msg)
		{
			DEBUG_LOGGER(sb->logger, "Got message[%08x] id[%d] handler[%08x] parent[%08x]",
                                        msg, msg->msg_id, msg->handler, msg->parent);
			if(!(msg->msg_id & MESSAGE_ALL))
			{
				WARN_LOGGER(sb->logger, "Unkown message[%d]", msg->msg_id);
				goto next;
			}
			conn = (CONN *)msg->handler;
			pth = (PROCTHREAD *)msg->parent;
			if(conn == NULL || pth == NULL || msg->fd != conn->fd || pth->service == NULL )
			{
				WARN_LOGGER(sb->logger, "Invalid MESSAGE[%08x] fd[%d] handler[%08x] parent[%08x]",
					msg, msg->fd, conn, pth);
				goto next;
			}
			DEBUG_LOGGER(sb->logger, 
                    "Got message[%s] On service[%s] procthread[%08x] connection[%d] %s:%d",
					messagelist[msg->msg_id], pth->service->name, 
                    pth, conn->fd, conn->ip, conn->port);
			switch(msg->msg_id)
			{
				/* NEW connection */
				case MESSAGE_NEW_SESSION :
					pth->add_connection(pth, conn);
					break;
				/* Close connection */
				case MESSAGE_QUIT :
					if(pth->connections[msg->fd])
						pth->terminate_connection(pth, conn);
					break;
				case MESSAGE_INPUT :
					break;
				case MESSAGE_OUTPUT :
					conn->write_handler(conn);
					break;
				case MESSAGE_PACKET :
					conn->packet_handler(conn);
					break;
				case MESSAGE_DATA :
					conn->data_handler(conn);
					break;
				default:
					break;
			}
next:
			msg->clean(&msg);
		}
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
	MESSAGE *msg = NULL;
	if((*sb) && (*sb)->running_status == 0 )	
	{
		/* Clean services */
		if((*sb)->services)
		{
			for(i = 0; i < (*sb)->running_services; i++)
			{
				if((*sb)->services[i])
					(*sb)->services[i]->clean(&((*sb)->services[i]));
			}
			free((*sb)->services);
			(*sb)->services = NULL;
		}
		/* Clean eventbase */
		if((*sb)->evbase) (*sb)->evbase->clean(&((*sb)->evbase));
		/* Clean timer */
		if((*sb)->timer) (*sb)->timer->clean(&((*sb)->timer));
		/* Clean logger */
		if((*sb)->logger) (*sb)->logger->close(&((*sb)->logger));
		/* Clean message queue */
		if((*sb)->message_queue) 
		{
			while((*sb)->message_queue->total > 0 )
                        {
                                msg = (*sb)->message_queue->pop((*sb)->message_queue);
                                if(msg) msg->clean(&msg);
                        }
                        (*sb)->message_queue->clean(&((*sb)->message_queue));
		}
		free((*sb));
		(*sb) = NULL;
	}
}
