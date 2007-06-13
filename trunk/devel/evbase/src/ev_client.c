#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "evbase.h"
#define SETRLIMIT(NAME, RLIM, rlim_set)\
{\
        struct rlimit rlim;\
        rlim.rlim_cur = rlim_set;\
        rlim.rlim_max = rlim_set;\
        if(setrlimit(RLIM, (&rlim)) != 0) {\
                fprintf(stderr, "setrlimit RLIM[%s] cur[%ld] max[%ld] failed, %s\n",\
                                NAME, rlim.rlim_cur, rlim.rlim_max, strerror(errno));\
                 _exit(-1);\
        } else {\
                fprintf(stdout, "setrlimit RLIM[%s] cur[%ld] max[%ld]\n",\
                                NAME, rlim.rlim_cur, rlim.rlim_max);\
        }\
}
#define CONN_MAX 65535
#define BUF_SIZE 8192
EVBASE *evbase = NULL;
char buffer[CONN_MAX][BUF_SIZE];
EVENT *events[CONN_MAX];
void ev_handler(int fd, short ev_flags, void *arg)
{
	int n = 0;
	if(ev_flags & EV_READ)
	{
		if( ( n = read(fd, buffer[fd], BUF_SIZE)) > 0 )
		{
			fprintf(stdout, "Read %d bytes from %d\n", n, fd);
			buffer[fd][n] = 0;
			fprintf(stdout, "Updating event[%x] on %d \n", events[fd], fd);
			if(events[fd])
			{
				events[fd]->add(events[fd], EV_WRITE);	
			}
		}		
		else
		{
			if(n < 0 )
				fprintf(stderr, "Reading from %d failed, %s\n", fd, strerror(errno));
			goto err;
		}
	}
	if(ev_flags & EV_WRITE)
	{
		if(  (n = write(fd, buffer[fd], strlen(buffer[fd])) ) > 0 )
		{
			fprintf(stdout, "Wrote %d bytes via %d\n", n, fd);
		}
		else
		{
			if(n < 0)
				fprintf(stderr, "Wrote data via %d failed, %s\n", fd, strerror(errno));	
			goto err;
		}
		if(events[fd]) events[fd]->del(events[fd], EV_WRITE);
	}
	return ;
	err:
	{
		if(events[fd])
		{
			events[fd]->destroy(events[fd]);
			events[fd] = NULL;
			shutdown(fd, SHUT_RDWR);
			close(fd);
			fprintf(stdout, "Connection %d closed\n", fd);
		}

	}
}

int main(int argc, char **argv)
{
	char *ip = NULL;
	int port = 0;
	int fd = 0;
	struct sockaddr_in sa;	
	socklen_t sa_len;
	int opt = 1;
	int i = 0;
	int conn_num = 0;
	EVENT  *event = NULL;
	if(argc < 4)
	{
		fprintf(stderr, "Usage:%s ip port connection_number\n", argv[0]);	
		_exit(-1);
	}	
	ip = argv[1];
	port = atoi(argv[2]);
	conn_num = atoi(argv[3]);
	/* Set resource limit */
	SETRLIMIT("RLIMIT_NOFILE", RLIMIT_NOFILE, CONN_MAX);	
	/* Initialize global vars */
	memset(events, 0, sizeof(EVENT *) * CONN_MAX);
	/* Initialize inet */ 
	memset(&sa, 0, sizeof(struct sockaddr_in));	
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(ip);
	sa.sin_port = htons(port);
	sa_len = sizeof(struct sockaddr);
        /* set evbase */
        if((evbase = evbase_init()))
	{
		while((fd = socket(AF_INET, SOCK_STREAM, 0)) > 0 && fd < conn_num)
		{
			/* Connect */
			if(connect(fd, (struct sockaddr *)&sa, sa_len) == 0 )
			{
				/* set FD NON-BLOCK */
                                fcntl(fd, F_SETFL, O_NONBLOCK);
				if((events[fd] = event_init()))
				{
					events[fd]->set(events[fd], fd, EV_READ|EV_WRITE|EV_PERSIST, 
						(void *)events[fd], &ev_handler);
					evbase->add(evbase, events[fd]);
					sprintf(buffer[fd], "%d:client message\n", fd);
				}
			}
			else
			{
				fprintf(stderr, "Connect to %s:%d failed, %s\n", ip, port, strerror(errno));
				break;
			}
		}
		while(1)
		{
			evbase->loop(evbase, 0, NULL);
			usleep(10);
		}
	}
}
