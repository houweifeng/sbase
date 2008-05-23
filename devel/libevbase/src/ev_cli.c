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
#define CONN_MAX 10240
#define EV_BUF_SIZE 1024
static EVBASE *evbase = NULL;
static char buffer[CONN_MAX][EV_BUF_SIZE];
static EVENT *events[CONN_MAX];
static int ev_sock_type = 0;
static int ev_sock_list[] = {SOCK_STREAM, SOCK_DGRAM};
static int ev_sock_count  = 2;

int setrlimiter(char *name, int rlimit, int nset)
{
    int ret = -1;
    struct rlimit rlim;
    if(name)
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
            return 0;
        }
        else
        {
            fprintf(stderr, "setrlimit %s cur[%ld] max[%ld] failed, %s\n",
                    name, rlim.rlim_cur, rlim.rlim_max, strerror(errno));
        }
    }
    return ret;
}
/* sock_dgram /UCP handler */
void ev_udp_handler(int fd, short ev_flags, void *arg)
{
    int n = 0;
    struct sockaddr_in rsa;
    socklen_t rsa_len = sizeof(struct sockaddr);
    if(ev_flags & E_READ)
    {
        if( ( n = recvfrom(fd, buffer[fd], EV_BUF_SIZE, 0, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
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
            SHOW_LOG("Wrote %d bytes via %d", n, fd);
        }
        else
        {
            if(n < 0)
                FATAL_LOG("Wrote data via %d failed, %s", fd, strerror(errno));	
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
}

/* sock stream/TCP handler */
void ev_handler(int fd, short ev_flags, void *arg)
{
    int n = 0;
    if(ev_flags & E_READ)
    {
        if( ( n = read(fd, buffer[fd], EV_BUF_SIZE)) > 0 )
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
            SHOW_LOG("Wrote %d bytes via %d", n, fd);
        }
        else
        {
            if(n < 0)
                FATAL_LOG("Wrote data via %d failed, %s", fd, strerror(errno));	
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
    int sock_type = 0;
    EVENT  *event = NULL;
    if(argc < 5)
    {
        fprintf(stderr, "Usage:%s sock_type(0/TCP|1/UDP) ip port connection_number\n", argv[0]);	
        _exit(-1);
    }	
    ev_sock_type = atoi(argv[1]);
    if(ev_sock_type < 0 || ev_sock_type > ev_sock_count)
    {
        fprintf(stderr, "sock_type must be 0/TCP OR 1/UDP\n");
        _exit(-1);
    }
    sock_type = ev_sock_list[ev_sock_type];
    ip = argv[2];
    port = atoi(argv[3]);
    conn_num = atoi(argv[4]);
    /* Set resource limit */
    setrlimiter("RLIMIT_NOFILE", RLIMIT_NOFILE, CONN_MAX);	
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
        LOGGER_INIT(evbase->logger, "/tmp/ev_client.log");
        evbase->set_evops(evbase, EOP_POLL);
        while((fd = socket(AF_INET, sock_type, 0)) > 0 && fd < conn_num)
        {
            /* Connect */
            if(connect(fd, (struct sockaddr *)&sa, sa_len) == 0 )
            {
                /* set FD NON-BLOCK */
                fcntl(fd, F_SETFL, O_NONBLOCK);
                if((events[fd] = ev_init()))
                {
                    if(sock_type == SOCK_STREAM)
                    {
                        events[fd]->set(events[fd], fd, E_READ|E_WRITE|E_PERSIST, 
                                (void *)events[fd], &ev_handler);
                    }
                    else
                    {
                        events[fd]->set(events[fd], fd, E_READ|E_WRITE|E_PERSIST, 
                                (void *)events[fd], &ev_udp_handler);
                    }
                    evbase->add(evbase, events[fd]);
                    sprintf(buffer[fd], "%d:client message", fd);
                }
                lsa_len = sizeof(struct sockaddr);
                memset(&lsa, 0, lsa_len);
                getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
                SHOW_LOG("Connected to %s:%d via %d port:%d", ip, port, fd, ntohs(lsa.sin_port));
            }
            else
            {
                FATAL_LOG("Connect to %s:%d failed, %s", ip, port, strerror(errno));
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
