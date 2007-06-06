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
int max_connections = 0;
int lfd = 0;
EVBASE *evbase = NULL;
void ev_handler(int fd, short ev_flags, void *arg)
{
	int rfd = 0 ;
	struct 	sockaddr_in rsa;
	socklen_t rsa_len = 0;
	char buf[BUF_SIZE];
	int n = 0;
	EVENT *event = NULL;
	if(fd == lfd )
	{
		if((ev_flags & EV_READ))
		{
			if((rfd = accept(fd, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
			{
				fprintf(stdout, "Accept new connection %s:%d via %d\n",
						inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd);	
				if((event = event_init()))
                		{
                        		event->set(event, rfd, EV_READ, (void *)event, &ev_handler);
                        		evbase->add(evbase, event);
                		}
			}
			else
			{
				fprintf(stderr, "Accept new connection failed, %s\n", strerror(errno));
			}
		}	
	}
	else
	{
		if(ev_flags & EV_READ)
		{
			if( ( n = read(fd, buf, BUF_SIZE)) > 0 )
			{
				fprintf(stdout, "Read %d bytes from %d\n", n, fd);
				if(  (n = write(fd, buf, n) )> 0 )
				{
					fprintf(stdout, "Echo %d bytes to %d\n", n, fd);
				}
				else
				{
					fprintf(stderr, "Echo data to %d failed, %s", fd, strerror(errno));	
				}
			}		
			else
                        {
                                if(n == 0 )
                                {
                                        fprintf(stdout, "Connection %d closed\n", fd);
                                }
                                else
                                {
                                        fprintf(stderr, "Reading from %d failed, %s", fd, strerror(errno));
                                }
                        }
                }
        }
}

int main(int argc, char **argv)
{
	int port = 0;
	int connection_limit = 0;
	int fd = 0;
	struct sockaddr_in sa;	
	socklen_t sa_len;
	int opt = 1;
	EVENT  *event = NULL;
	if(argc < 3)
	{
		fprintf(stderr, "Usage:%s port connection_limit\n", argv[0]);	
		_exit(-1);
	}	
	port = atoi(argv[1]);
	connection_limit = atoi(argv[2]);
	max_connections = (connection_limit > 0) ? connection_limit : CONN_MAX;
	/* Set resource limit */
	SETRLIMIT("RLIMIT_NOFILE", RLIMIT_NOFILE, max_connections);	
	/* Initialize inet */ 
	lfd = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
                        (char *)&opt, (socklen_t) sizeof(opt) );
	memset(&sa, 0, sizeof(struct sockaddr_in));	
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(port);
	sa_len = sizeof(struct sockaddr);
	/* Bind */
	if(bind(lfd, (struct sockaddr *)&sa, sa_len ) != 0 )
        {
                fprintf(stdout, "Binding failed, %s\n", strerror(errno));
                return ;
        }
	/* set FD NON-BLOCK */
	if(fcntl(lfd, F_SETFL, O_NONBLOCK) != 0 )
        {
                fprintf(stdout, "Setting NON-BLOCK failed, %s\n", strerror(errno));
                return ;
        }
	/* Listen */
	if(listen(lfd, CONN_MAX) != 0 )
        {
                fprintf(stdout, "Listening  failed, %s\n", strerror(errno));
                return ;
        }
	fprintf(stdout, "Initialize evbase \n");
        /* set evbase */
        if((evbase = evbase_init()))
        {
                if((event = event_init()))
                {
                        event->set(event, lfd, EV_READ, (void *)event, &ev_handler);
                        evbase->add(evbase, event);
                        while(1)
                        {
                                evbase->loop(evbase, 0, NULL);
                                usleep(1000);
                        }
                }
                else
                {
                        evbase->clean(&evbase);
                }
        }
}
