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
#define BUF_SIZE 8192
EVBASE *evbase = NULL;
char buffer[CONN_MAX][BUF_SIZE];
EVENT *events[CONN_MAX];
void ev_handler(int fd, short ev_flags, void *arg)
{
	int n = 0;
	struct sockaddr_in rsa;
	socklen_t rsa_len ;
	if(ev_flags & EV_READ)
	{
		if( ( n = recvfrom(fd, buffer[fd], BUF_SIZE, 0, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
		{
			SHOW_LOG("Read %d bytes from %d", n, fd);
			buffer[fd][n] = 0;
			SHOW_LOG("Updating event[%x] on %d ", events[fd], fd);
			if(events[fd])
			{
				events[fd]->add(events[fd], EV_WRITE);	
			}
		}		
		else
		{
			if(n < 0 )
				FATAL_LOG("Reading from %d failed, %s", fd, strerror(errno));
			goto err;
		}
	}
	if(ev_flags & EV_WRITE)
	{
		if(  (n = write(fd, buffer[fd], strlen(buffer[fd])) ) > 0 )
		{
			SHOW_LOG("Wrote %d bytes via %d", n, fd);
		}
		else
		{
			if(n < 0)
				FATAL_LOG("Wrote data via %d failed, %s", fd, strerror(errno));	
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
			SHOW_LOG("Connection %d closed", fd);
		}

	}
}

int main(int argc, char **argv)
{
	char *ip = NULL;
	int port = 0;
	int fd = 0;
	struct sockaddr_in sa, lsa;	
	socklen_t sa_len, lsa_len = -1;
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
		while((fd = socket(AF_INET, SOCK_DGRAM, 0)) > 0 && fd < conn_num)
		{
			/* Set REUSE */
			//setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                        //	(char *)&opt, (socklen_t) sizeof(opt) );
			/* Bind */
			//if(lsa_len != -1)
			//	bind(fd, (struct sockaddr *)&lsa, lsa_len);
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
					sprintf(buffer[fd], "%d:client message", fd);
				}
				lsa_len = sizeof(struct sockaddr_in);
				memset(&lsa, 0, lsa_len);
				getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
				SHOW_LOG("Connected to %s:%d via %d port:%d", ip, port, fd, ntohs(lsa.sin_port));
			}
			else
			{
				FATAL_LOG("Connect to %s:%d failed, %s", ip, port, strerror(errno));
				break;
				//_exit(-1);
			}
		}
		while(1)
		{
			evbase->loop(evbase, 0, NULL);
			usleep(10);
		}
	}
}
