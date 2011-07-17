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
#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/crypto.h>
#endif
#include "evbase.h"
#include "log.h"
#ifdef HAVE_EVKQUEUE
#define CONN_MAX 10240
#else
#define CONN_MAX 65536
#endif
#define EV_BUF_SIZE 1024
static EVBASE *evbase = NULL;
static int ev_sock_type = 0;
static int ev_sock_list[] = {SOCK_STREAM, SOCK_DGRAM};
static int ev_sock_count  = 2;
static int is_use_ssl = 1;
#ifdef USE_SSL
static SSL_CTX *ctx = NULL;
#endif
typedef struct _CONN
{
    int fd;
    EVENT event;
    char request[EV_BUF_SIZE];
    char response[EV_BUF_SIZE];
#ifdef USE_SSL
    SSL *ssl;
#endif
}CONN;
static CONN *conns = NULL;

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
                    name, (long)rlim.rlim_cur, (long)rlim.rlim_max);
            return 0;
        }
        else
        {
            fprintf(stderr, "setrlimit %s cur[%ld] max[%ld] failed, %s\n",
                    name, (long)rlim.rlim_cur, (long)rlim.rlim_max, strerror(errno));
        }
    }
    return ret;
}
/* sock_dgram /UCP handler */
void ev_udp_handler(int fd, int ev_flags, void *arg)
{
    int n = 0;
    struct sockaddr_in rsa;
    socklen_t rsa_len = sizeof(struct sockaddr);
    if(ev_flags & E_READ)
    {
        if( ( n = recvfrom(fd, conns[fd].response, EV_BUF_SIZE - 1, 
                        0, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
        {
            SHOW_LOG("Read %d bytes from %d", n, fd);
            conns[fd].response[n] = 0;
            SHOW_LOG("Updating event[%p] on %d ", &conns[fd].event, fd);
            event_add(&conns[fd].event, E_WRITE);	
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
        if(  (n = write(fd, conns[fd].request, strlen(conns[fd].request)) ) > 0 )
        {
            SHOW_LOG("Wrote %d bytes via %d", n, fd);
        }
        else
        {
            if(n < 0)
                FATAL_LOG("Wrote data via %d failed, %s", fd, strerror(errno));	
            goto err;
        }
        event_del(&conns[fd].event, E_WRITE);
    }
    return ;
err:
    {
        event_destroy(&conns[fd].event);
        shutdown(fd, SHUT_RDWR);
        close(fd);
        SHOW_LOG("Connection %d closed", fd);
    }
}

/* sock stream/TCP handler */
void ev_handler(int fd, int ev_flags, void *arg)
{
    int n = 0;
    if(ev_flags & E_READ)
    {
        if(is_use_ssl)
        {
#ifdef USE_SSL
            n = SSL_read(conns[fd].ssl, conns[fd].response, EV_BUF_SIZE - 1);
#else
            n = read(fd, conns[fd].response, EV_BUF_SIZE - 1);
#endif
        }
        else
        {
            n = read(fd, conns[fd].response, EV_BUF_SIZE - 1);
        }
        if(n > 0 )
        {
            SHOW_LOG("Read %d bytes from %d", n, fd);
            conns[fd].response[n] = 0;
            SHOW_LOG("Updating event[%p] on %d ", &conns[fd].event, fd);
            event_add(&conns[fd].event, E_WRITE);	
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
        if(is_use_ssl)
        {
#ifdef USE_SSL
            n = SSL_write(conns[fd].ssl, conns[fd].request, strlen(conns[fd].request));
#else
            n = write(fd, conns[fd].request, strlen(conns[fd].request));
#endif
        }
        else
        {
            n = write(fd, conns[fd].request, strlen(conns[fd].request));
        }

        if(n > 0 )
        {
            SHOW_LOG("Wrote %d bytes via %d", n, fd);
        }
        else
        {
            if(n < 0)
                FATAL_LOG("Wrote data via %d failed, %s", fd, strerror(errno));	
            goto err;
        }
        event_del(&conns[fd].event, E_WRITE);
    }
    return ;
err:
    {
        event_destroy(&conns[fd].event);
        shutdown(fd, SHUT_RDWR);
        close(fd);
#ifdef USE_SSL
        if(conns[fd].ssl)
        {
            SSL_shutdown(conns[fd].ssl);
            SSL_free(conns[fd].ssl);
            conns[fd].ssl = NULL;
        }
#endif

        SHOW_LOG("Connection %d closed", fd);
    }
}

int main(int argc, char **argv)
{
    char *ip = NULL;
    int port = 0;
    int fd = 0;
    struct sockaddr_in sa, lsa;
    socklen_t sa_len, lsa_len = -1;
    int i = 0;
    int conn_num = 0;
    int sock_type = 0;
    int flag = 0;

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
    if((conns = (CONN *)calloc(CONN_MAX, sizeof(CONN))))
    {
        //memset(events, 0, sizeof(EVENT *) * CONN_MAX);
        /* Initialize inet */ 
        memset(&sa, 0, sizeof(struct sockaddr_in));	
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr(ip);
        sa.sin_port = htons(port);
        sa_len = sizeof(struct sockaddr);
        /* set evbase */
        if((evbase = evbase_init(0)))
        {
#ifdef USE_SSL
            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
            if((ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
            {
                ERR_print_errors_fp(stdout);
                _exit(-1);
            }
#endif
            //evbase->set_evops(evbase, EOP_POLL);
            while((fd = socket(AF_INET, sock_type, 0)) > 0 && i < conn_num)
            {
                conns[fd].fd = fd;
                /* Connect */
                if(connect(fd, (struct sockaddr *)&sa, sa_len) != 0)
                {
                    FATAL_LOG("Connect to %s:%d failed, %s", ip, port, strerror(errno));
                    break;
                }
                if(is_use_ssl && sock_type == SOCK_STREAM)
                {
#ifdef USE_SSL
                    conns[fd].ssl = SSL_new(ctx);
                    if(conns[fd].ssl == NULL )
                    {
                        FATAL_LOG("new SSL with created CTX failed:%s\n",
                                ERR_reason_error_string(ERR_get_error()));
                        break;
                    }
                    if((ret = SSL_set_fd(conns[fd].ssl, fd)) == 0)
                    {
                        FATAL_LOG("add SSL to tcp socket failed:%s\n",
                                ERR_reason_error_string(ERR_get_error()));
                        break;
                    }
                    /* SSL Connect */
                    if(SSL_connect(conns[fd].ssl) < 0)
                    {
                        FATAL_LOG("SSL connection failed:%s\n",
                                ERR_reason_error_string(ERR_get_error()));
                        break;
                    }
#endif
                }
                /* set FD NON-BLOCK */
                flag = fcntl(fd, F_GETFL, 0);
                flag |= O_NONBLOCK;
                fcntl(fd, F_SETFL, flag);
                    if(sock_type == SOCK_STREAM)
                    {
                        event_set(&conns[fd].event, fd, E_READ|E_WRITE|E_PERSIST, 
                                (void *)&conns[fd].event, &ev_handler);
                    }
                    else
                    {
                        event_set(&conns[fd].event, fd, E_READ|E_WRITE|E_PERSIST, 
                                (void *)&conns[fd].event, &ev_udp_handler);
                    }
                    evbase->add(evbase, &conns[fd].event);
                    sprintf(conns[fd].request, "GET / HTTP/1.0\r\n\r\n");
                lsa_len = sizeof(struct sockaddr);
                memset(&lsa, 0, lsa_len);
                getsockname(fd, (struct sockaddr *)&lsa, &lsa_len);
                SHOW_LOG("%d:Connected to %s:%d via %d port:%d", i, ip, port, fd, ntohs(lsa.sin_port));
                i++;
            }
            while(1)
            {
                evbase->loop(evbase, 0, NULL);
                usleep(1000);
            }
            for(i = 0; i < CONN_MAX; i++)
            {
                    shutdown(conns[i].fd, SHUT_RDWR);
                    close(conns[i].fd);
                    event_destroy(&conns[i].event);
#ifdef USE_SSL
                if(conns[i].ssl)
                {
                    SSL_shutdown(conns[i].ssl);
                    SSL_free(conns[i].ssl); 
                }
#endif
            }
#ifdef USE_SSL
            ERR_free_strings();
            SSL_CTX_free(ctx);
#endif
        }
        free(conns);
    }
    return -1;
}
