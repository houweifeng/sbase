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
#include <netdb.h>
#include "evbase.h"
#include "log.h"
#include "logger.h"
#define SETRLIMIT(NAME, RLIM, rlim_set)\
{\
        struct rlimit rlim;\
        rlim.rlim_cur = rlim_set;\
        rlim.rlim_max = rlim_set;\
        if(setrlimit(RLIM, (&rlim)) != 0) {\
                FATAL_LOG("setrlimit RLIM[%s] cur[%ld] max[%ld] failed, %s",\
                                NAME, rlim.rlim_cur, rlim.rlim_max, strerror(errno));\
        } else {\
                SHOW_LOG("setrlimit RLIM[%s] cur[%ld] max[%ld]",\
                                NAME, rlim.rlim_cur, rlim.rlim_max);\
        }\
}
#define CONN_MAX 65536
#define BUF_SIZE 128
int max_connections = 0;
int connections = 0;
int lfd = 0;
EVBASE *evbase = NULL;
char buffer[CONN_MAX][BUF_SIZE];
EVENT *events[CONN_MAX];
void ev_handler(int fd, short ev_flags, void *arg)
{
	int rfd = 0 ;
	struct 	sockaddr_in rsa;
	socklen_t rsa_len = 0;
	int n = 0;
	if(fd == lfd )
	{
		if((ev_flags & E_READ))
		{
			if((rfd = accept(fd, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
			{
				SHOW_LOG("Accept new connection %s:%d via %d total %d ",
					inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port),
					rfd, ++connections);
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
			else
			{
				FATAL_LOG("Accept new connection failed, %s", strerror(errno));
			}
		}	
		return ;
	}
	else
	{
		SHOW_LOG("EVENT %d on %d", ev_flags, fd);
		if(ev_flags & E_READ)
		{
			if( ( n = read(fd, buffer[fd], BUF_SIZE)) > 0 )
			{
				SHOW_LOG("Read %d bytes from %d", n, fd);
				buffer[fd][n] = 0;
				SHOW_LOG("Updating event[%x] on %d ", events[fd], fd);
				if(events[fd])
				{
					events[fd]->add(events[fd], E_WRITE);	
				}
			}		
			else
			{
				if(n < 0 )
					FATAL_LOG("Reading from %d failed, %s", fd, strerror(errno));
				goto err;
			}
		}
		if(ev_flags & E_WRITE)
		{
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
    struct sockaddr_in sa;	
    socklen_t sa_len;
    int opt = 1;
    int i = 0;
    int nprocess = 0;
    pid_t pid ;
    EVENT  *event = NULL;
    if(argc < 4)
    {
        fprintf(stderr, "Usage:%s port connection_limit process_limit\n", argv[0]);	
        _exit(-1);
    }	
    port = atoi(argv[1]);
    connection_limit = atoi(argv[2]);
    nprocess = atoi(argv[3]);
    max_connections = (connection_limit > 0) ? connection_limit : CONN_MAX;
    /* Set resource limit */
    SETRLIMIT("RLIMIT_NOFILE", RLIMIT_NOFILE, CONN_MAX);	
    /* Initialize global vars */
    memset(events, 0, sizeof(EVENT *) * CONN_MAX);
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
    if(listen(lfd, CONN_MAX) != 0 )
    {
        SHOW_LOG("Listening  failed, %s", strerror(errno));
        return ;
    }
    SHOW_LOG("Initialize evbase ");
    for(i = 0; i < nprocess; i++)
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
    /* set evbase */
    if((evbase = evbase_init()))
    {
        evbase->set_logfile(evbase, "/tmp/ev_server.log");
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
