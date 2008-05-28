#include "sbase.h"
#include "logger.h"
#include "service.h"
/* set service */
int service_set(SERVICE *service)
{
    char *p = NULL;
    int ret = -1, opt = 1;

    if(service)
    {
        p = service->ip;
        service->sa.sin_family = service->family;
        service->sa.sin_addr.s_addr = (p)? inet_addr(p):INADDR_ANY;
        service->sa.sin_port = htons(service->port);
        if(service->backlog <= 0) service->backlog = SB_CONN_MAX;
        if(service->service_type == S_SERVICE)
        {
            if((service->fd = socket(service->family, service->sock_type, 0)) > 0 
                && fcntl(service->fd, F_SETFL, O_NONBLOCK) == 0
                && setsockopt(service->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0
                && bind(service->fd, (struct sockaddr *)&(service->sa), sizeof(service->sa)) == 0
                && listen(service->fd, service->backlog) == 0 
                && service->evbase && (service->event = ev_init()))
            {
                service->event->set(service->event, service->fd, E_READ|E_PERSIST,
                        (void *)service, (void *)&service_event_handler);
                ret = service->evbase->add(service->evbase, service->event);
            }
        
        }else if(service->service_type == C_SERVICE)
        {
            ret = 0;
        }
    }
    return ret;
}

/* running */
int service_run(SERVICE *service)
{
}

/* set logfile  */
int service_set_log(SERVICE *service, char *logfile)
{
    if(service && logfile)
    {
        LOGGER_INIT(service->logger, logfile);
	DEBUG_LOGGER(service->logger, "Initialize logger %s", logfile);
        return 0;
    }
    return -1;
}

/* event handler */
void service_event_handler(int event_fd, short flag, void *arg)
{
    struct sockaddr_in rsa;
    socklen_t rsa_len = sizeof(rsa);
    int fd = -1;
    SERVICE *service = (SERVICE *)arg;

    if(service)
    {
        if(event_fd == service->fd)
        {
            if(E_READ & flag)
            {
                if((fd = accept(event_fd, (struct sockaddr *)&rsa, &rsa_len)) > 0)
                {
                    DEBUG_LOGGER(service->logger, "Accepted new connection[%s:%d] via %d\n",
                            inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), fd);
                }
                else
                {
                    FATAL_LOGGER(service->logger, "Accept new connection failed, %s", 
                            strerror(errno));
                }
            }
        }
    }
    return ;
}

/* stop service */
int service_stop(SERVICE *service)
{
    if(service)
    {
    }
    return -1;
}

/* service clean */
void service_clean(SERVICE **pservice)
{
    if(*pservice)
    {
    }
}

/* Initialize service */
SERVICE *service_init()
{
    SERVICE *service = NULL;
    if((service = (SERVICE *)calloc(1, sizeof(SERVICE))))
    {
        service->set        = service_set;
        service->run        = service_run;
        service->stop       = service_stop;
        service->set_log    = service_set_log;
        service->clean      = service_clean;
    }
    return service;
}
