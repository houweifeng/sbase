#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include "sbase.h"
#include "logger.h"
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
}

/* set evbase log */
int sbase_set_evlog(SBASE *sbase, char *evlogfile)
{
}

/* add service to sbase */
int sbase_add_service(SBASE *sbase, SERVICE  *service)
{
	if(sbase)
	{
        if(service)
        {
            service->evbase = sbase->evbase;
            return service->set(service);
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
		sbase->setrlimit     	= sbase_setrlimit;
		sbase->set_log		    = sbase_set_log;
		sbase->set_evlog	    = sbase_set_evlog;
		sbase->add_service	    = sbase_add_service;
		sbase->running 		    = sbase_running;
		sbase->stop 		    = sbase_stop;
	}
	return sbase;
}
