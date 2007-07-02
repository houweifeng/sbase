#include <stdio.h>
#include <stdlib.h>
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
#include "evbase.h"
#include "log.h"
#define SETRLIMIT(NAME, RLIM, rlim_set)\
{\
        struct rlimit rlim;\
        rlim.rlim_cur = rlim_set;\
        rlim.rlim_max = rlim_set;\
        if(setrlimit(RLIM, (&rlim)) != 0) {\
                FATAL_LOG("setrlimit RLIM[%s] cur[%ld] max[%ld] failed, %s",\
                                NAME, rlim.rlim_cur, rlim.rlim_max, strerror(errno));\
                 _exit(-1);\
        } else {\
                SHOW_LOG("setrlimit RLIM[%s] cur[%ld] max[%ld]",\
                                NAME, rlim.rlim_cur, rlim.rlim_max);\
        }\
}
#define CONN_MAX 131070
#define BUF_SIZE 128
int max_connections = 0;
int lfd = 0;
struct sockaddr_in sa;	
socklen_t sa_len;
EVBASE *evbase = NULL;
char buffer[CONN_MAX][BUF_SIZE];
//struct sockaddr_in rsalist[CONN_MAX]; 
EVENT *events[CONN_MAX];
void ev_handler(int fd, short ev_flags, void *arg)
{
	int rfd = 0 ;
	struct 	sockaddr_in  rsa;
	socklen_t rsa_len ;
	int n = 0, opt = 1;
	SHOW_LOG("fd[%d] ev[%d] arg[%08x]", fd, ev_flags, arg);
	if(fd == lfd )
	{
		if((ev_flags & E_READ))
		{
			rfd = socket(AF_INET, SOCK_DGRAM, 0);
			setsockopt(rfd, SOL_SOCKET, SO_REUSEADDR,
                        	(char *)&opt, (socklen_t) sizeof(opt) );
			if(bind(rfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == 0)
			{
				SHOW_LOG("Binded %d to %s:%d ",
                                        rfd, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
			}
			else
			{
				FATAL_LOG("Bind %d to %s:%d failed, %s",
                                       	rfd, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), strerror(errno));
                                goto err_end;
			}
			//rfd = dup(fd);
			rsa_len = sizeof(struct sockaddr_in);
			memset(&rsa, 0 , rsa_len);
			if(( n = recvfrom(fd, buffer[rfd], BUF_SIZE, 0, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
			{
				buffer[rfd][n] = 0;
				SHOW_LOG("Received %d bytes from %s:%d via %d, %s",
						n, inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd, buffer[rfd]);
				if(connect(rfd, (struct sockaddr *)&rsa, rsa_len) == 0)
				{
					SHOW_LOG("Connected %s:%d via %d",
							inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd);
				}
				else
				{
					FATAL_LOG("Connectting to %s:%d via %d failed, %s",
						inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd, strerror(errno));
					goto err_end;
				}

				if((n = sendto(rfd, buffer[rfd], strlen(buffer[rfd]), 0, 
					(struct sockaddr *)&rsa, sizeof(struct sockaddr_in))) > 0 )
				//if( n = write(rfd, buffer[rfd], strlen(buffer[rfd])) > 0)
				{
					SHOW_LOG("Sent %d to %s:%d via %d", 
						n, inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd);
				}
				else
				{
					SHOW_LOG("Sending data to %s:%d via %d failed, %s", 
						inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd, strerror(errno));
					goto err_end;
				}
				/* set FD NON-BLOCK */
				fcntl(rfd, F_SETFL, O_NONBLOCK);
				if((events[rfd] = ev_init()))
				{
					events[rfd]->set(events[rfd], rfd, E_READ|E_PERSIST,
							(void *)events[rfd], &ev_handler);
					evbase->add(evbase, events[rfd]);
				}
				return ;
			}
			err_end:
				shutdown(rfd, SHUT_RDWR);
				close(rfd);
		}	
		return ;
	}
	else
	{
		DEBUG_LOG("EVENT %d on %d", ev_flags, fd);
		if(ev_flags & E_READ)
		{
			if( ( n = recvfrom(fd, buffer[fd], BUF_SIZE, 0, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
			{
				buffer[fd][n] = 0;
				SHOW_LOG("Read %d bytes from %d, %s", n, fd, buffer[fd]);
				SHOW_LOG("Updating event[%08x] on %d ", events[fd], fd);
				if(events[fd])
				{
					events[fd]->add(events[fd], E_WRITE);	
					SHOW_LOG("Updated event[%08x] on %d ", events[fd], fd);
				}
			}	
			else
			{
				if(n < 0 )
					FATAL_LOG("Reading from %d failed, %s", fd, strerror(errno));
				goto err;
			}
			DEBUG_LOG("E_READ on %d end", fd);
		}
		if(ev_flags & E_WRITE)
		{
			SHOW_LOG("E_WRITE on %d end", fd);
			if(  (n = write(fd, buffer[fd], strlen(buffer[fd])) ) > 0 )
			{
				SHOW_LOG("Echo %d bytes to %d", n, fd);
			}
			else
			{
				if(n < 0)
					FATAL_LOG("Echo data to %d failed, %s", fd, strerror(errno));	
				goto err;
			}
			if(events[fd]) events[fd]->del(events[fd], E_WRITE);
		}
		DEBUG_LOG("EV_OVER on %d", fd);
		return ;
		err:
		{
			if(events[fd])
			{
				events[fd]->destroy(events[fd]);
				events[fd] = NULL;
				shutdown(fd, SHUT_RDWR);
				close(fd);
				SHOW_LOG("Connection %d closed", fd);
			}
		}
		return ;
	}
}

int main(int argc, char **argv)
{
	int port = 0;
	int connection_limit = 0;
	int fd = 0;
	int opt = 1;
	int i = 0;
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
	SETRLIMIT("RLIMIT_NOFILE", RLIMIT_NOFILE, CONN_MAX);	
	/* Initialize global vars */
	memset(events, 0, sizeof(EVENT *) * CONN_MAX);
	/* Initialize inet */ 
	lfd = socket(AF_INET, SOCK_DGRAM, 0);
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
                SHOW_LOG("Binding failed, %s", strerror(errno));
                return ;
        }
	/* set FD NON-BLOCK */
	if(fcntl(lfd, F_SETFL, O_NONBLOCK) != 0 )
        {
                SHOW_LOG("Setting NON-BLOCK failed, %s", strerror(errno));
                return ;
        }
	/* Listen */
	/*
	if(listen(lfd, CONN_MAX) != 0 )
        {
                SHOW_LOG("Listening  failed, %s", strerror(errno));
                return ;
        }
	*/
	SHOW_LOG("Initialize evbase ");
        /* set evbase */
        if((evbase = evbase_init()))
        {
                if((event = ev_init()))
                {
			SHOW_LOG("Initialized event ");
                        event->set(event, lfd, E_READ|E_PERSIST, (void *)event, &ev_handler);
                        evbase->add(evbase, event);
                        while(1)
                        {
                                evbase->loop(evbase, 0, NULL);
                                usleep(10);
                        }
                }
                else
                {
                        evbase->clean(&evbase);
                }
        }
}
