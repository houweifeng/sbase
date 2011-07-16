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
#include <openssl/crypto.h>
#endif
#include "evbase.h"
#include "logger.h"
#include "log.h"
#ifdef HAVE_EVKQUEUE
#define CONN_MAX 1024
#else
#define CONN_MAX 65536
#endif
#define EV_BUF_SIZE 1024
static int max_connections = 0;
static int connections = 0;
static int lfd = 0;
static struct sockaddr_in sa = {0};	
static socklen_t sa_len = sizeof(struct sockaddr_in);
static EVBASE *evbase = NULL;
static int ev_sock_type = 0;
static int ev_sock_list[] = {SOCK_STREAM, SOCK_DGRAM};
static int ev_sock_count = 2;
static int is_use_ssl = 1;
static in_addr_t multicast_addr = INADDR_NONE;
#ifdef USE_SSL
static SSL_CTX *ctx = NULL;
static const char *cert_file = "cacert.pem";
static const char *priv_file = "privkey.pem";
#endif
typedef struct _CONN
{
    int fd;
    EVENT event;
    char buffer[EV_BUF_SIZE];
    int n;
#ifdef USE_SSL
    SSL *ssl;
#endif
}CONN;
static CONN *conns = NULL;

/* set rlimit */
int setrlimiter(char *name, int rlimit, int nset)
{
    int ret = -1;
    struct rlimit rlim;
    if(name)
    {
        if(getrlimit(rlimit, &rlim) == -1)
            return -1;
        else
        {
            fprintf(stdout, "getrlimit %s cur[%ld] max[%ld]\n", 
                    name, (long)rlim.rlim_cur, (long)rlim.rlim_max);
        }
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

void ev_udp_handler(int fd, int ev_flags, void *arg)
{
    int rfd = 0 ;
    struct 	sockaddr_in  rsa;
    socklen_t rsa_len ;
    int n = 0, opt = 1;
    SHOW_LOG("fd[%d] ev[%d] arg[%p]", fd, ev_flags, arg);
    if(fd == lfd )
    {
        if((ev_flags & E_READ))
        {
            if((rfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0 
                || setsockopt(rfd, SOL_SOCKET, SO_REUSEADDR, 
                    (char *)&opt, (socklen_t) sizeof(int)) != 0
#ifdef SO_REUSEPORT
                || setsockopt(rfd, SOL_SOCKET, SO_REUSEPORT, 
                    (char *)&opt, (socklen_t) sizeof(int)) != 0
#endif
                || bind(rfd, (struct sockaddr *)&sa, sa_len) != 0) 
            {
                FATAL_LOG("Connect %d to %s:%d failed, %s",
                    rfd, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), strerror(errno));
                close(rfd);
                _exit(-1);
                return ;
            }
            rsa_len = sizeof(struct sockaddr_in);
            memset(&rsa, 0 , rsa_len);
            if((n = conns[rfd].n = recvfrom(fd, conns[rfd].buffer, EV_BUF_SIZE - 1, 0, 
                            (struct sockaddr *)&rsa, &rsa_len)) > 0 )
            {
                //sendto(fd, conns[rfd].buffer, n, 0, (struct sockaddr *)&rsa, rsa_len);
                SHOW_LOG("Received %d bytes from %s:%d via %d, %s", conns[rfd].n, 
                        inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port), rfd, conns[rfd].buffer);
                //return ;
                /*
                if(multicast_addr != INADDR_NONE)
                {
                    struct ip_mreq mreq;
                    memset(&mreq, 0, sizeof(struct ip_mreq));
                    mreq.imr_multiaddr.s_addr = multicast_addr;
                    mreq.imr_interface.s_addr = INADDR_ANY;
                    if(setsockopt(rfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&mreq, sizeof(mreq)) != 0)
                    {
                        SHOW_LOG("Setsockopt(MULTICAST) failed, %s", strerror(errno));
                        return ;
                    }
                }
                */
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
                /* set FD NON-BLOCK */
                fcntl(rfd, F_SETFL, O_NONBLOCK);
                event_set(&(conns[rfd].event), rfd, E_READ|E_WRITE|E_PERSIST,
                        (void *)&(conns[rfd].event), &ev_udp_handler);
                evbase->add(evbase, &(conns[rfd].event));
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
            //if( ( n = recvfrom(fd, conns[fd].buffer, EV_BUF_SIZE, 0, (struct sockaddr *)&rsa, &rsa_len)) > 0 )
            if((n = conns[fd].n = read(fd, conns[fd].buffer, EV_BUF_SIZE - 1)) > 0)
            {
                SHOW_LOG("Read %d bytes from %d, %s", n, fd, conns[fd].buffer);
                SHOW_LOG("Updating event[%p] on %d ", &conns[fd].event, fd);
                event_add(&conns[fd].event, E_WRITE);	
                SHOW_LOG("Updated event[%p] on %d ", &conns[fd].event, fd);
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
            if((n = write(fd, conns[fd].buffer, conns[fd].n)) > 0 )
            {
                SHOW_LOG("Echo %d bytes to %d", n, fd);
            }
            else
            {
                if(n < 0)
                    FATAL_LOG("Echo data to %d failed, %s", fd, strerror(errno));	
                goto err;
            }
            event_del(&conns[fd].event, E_WRITE);
        }
        DEBUG_LOG("EV_OVER on %d", fd);
        return ;
err:
        {
            event_destroy(&conns[fd].event);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            SHOW_LOG("Connection %d closed", fd);
        }
        return ;
    }
}

void ev_handler(int fd, int ev_flags, void *arg)
{
    int rfd = 0 ;
    struct 	sockaddr_in rsa;
    socklen_t rsa_len = sizeof(struct sockaddr_in);
    int n = 0;
    if(fd == lfd )
    {
        if((ev_flags & E_READ))
        {
            if((rfd = accept(fd, (struct sockaddr *)&rsa, &rsa_len)) > 0)
            {
                SHOW_LOG("Accept new connection %s:%d via %d total %d ",
                        inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port),
                        rfd, ++connections);
                conns[rfd].fd = rfd;
            }
            else
            {
                FATAL_LOG("Accept new connection failed, %s", strerror(errno));
                return ;
            }
            if(is_use_ssl)
            {
#ifdef USE_SSL
                if((conns[rfd].ssl = SSL_new(ctx)) == NULL)
                {
                    FATAL_LOG("SSL_new() failed, %s", ERR_reason_error_string(ERR_get_error()));
                    shutdown(rfd, SHUT_RDWR);
                    close(rfd);
                    return ;
                }
                //set fd
                if(SSL_set_fd(conns[rfd].ssl, rfd) == 0) 
                {
                    ERR_print_errors_fp(stdout);
                    shutdown(rfd, SHUT_RDWR);
                    close(rfd);
                    return ;
                }
                //ssl accept 
                if((SSL_accept(conns[rfd].ssl)) <= 0)
                {
                    FATAL_LOG("SSL_Accept connection %s:%d via %d failed, %s",
                            inet_ntoa(rsa.sin_addr), ntohs(rsa.sin_port),
                            rfd,  ERR_reason_error_string(ERR_get_error()));
                    shutdown(rfd, SHUT_RDWR);
                    close(rfd);
                    return ;
                }
#endif
            }
            /* set FD NON-BLOCK */
            fcntl(rfd, F_SETFL, O_NONBLOCK);
                event_set(&conns[rfd].event, rfd, E_READ|E_PERSIST,
                        (void *)&conns[rfd].event, &ev_handler);
                evbase->add(evbase, &conns[rfd].event);
            return ;
        }
    }
    else
    {
        SHOW_LOG("EVENT %d on %d", ev_flags, fd);
        if(ev_flags & E_READ)
        {
            if(is_use_ssl)
            {
#ifdef USE_SSL
                n = SSL_read(conns[fd].ssl, conns[fd].buffer, EV_BUF_SIZE - 1);
#else
                n = read(fd, conns[fd].buffer, EV_BUF_SIZE - 1);
#endif
            }
            else
            {
                n = read(fd, conns[fd].buffer, EV_BUF_SIZE - 1);
            }
            if(n > 0)
            {
                conns[fd].n = n;
                SHOW_LOG("Read %d bytes from %d", n, fd);
                conns[fd].buffer[n] = 0;
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
                n = SSL_write(conns[fd].ssl, conns[fd].buffer, conns[fd].n);
#else
                char *s = "daffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm";
                conns[fd].n = sprintf(conns[fd].buffer, 
                        "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n%s", (int)strlen(s), s);
                n = write(fd, conns[fd].buffer, conns[fd].n);
#endif
            }
            else
            {
                n = read(fd, conns[fd].buffer, conns[fd].n);
            }

            if(n > 0 )
            {
                SHOW_LOG("Echo %d bytes to %d", n, fd);
            }
            else
            {
                if(n < 0)
                    FATAL_LOG("Echo data to %d failed, %s", fd, strerror(errno));	
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
            {   SSL_shutdown(conns[fd].ssl);
                SSL_free(conns[fd].ssl);
                conns[fd].ssl = NULL;
            }
#endif

            SHOW_LOG("Connection %d closed", fd);
        }
        return ;
    }
}

int main(int argc, char **argv)
{
    int port = 0, connection_limit = 0, fd = 0, opt = 1, i = 0, nprocess = 0;
    char *multicast_ip = NULL;

    if(argc < 5)
    {
        fprintf(stderr, "Usage:%s sock_type(0/TCP|1/UDP) port "
                "connection_limit process_limit multicast_ip(only for UDP)\n", argv[0]);	
        _exit(-1);
    }	
    ev_sock_type = atoi(argv[1]);
    if(ev_sock_type < 0 || ev_sock_type > ev_sock_count)
    {
        fprintf(stderr, "sock_type must be 0/TCP OR 1/UDP\n");
        _exit(-1);
    }
    port = atoi(argv[2]);
    connection_limit = atoi(argv[3]);
    nprocess = atoi(argv[4]);
    if(argc > 5) multicast_ip = argv[5];
    max_connections = (connection_limit > 0) ? connection_limit : CONN_MAX;
    /* Set resource limit */
    setrlimiter("RLIMIT_NOFILE", RLIMIT_NOFILE, CONN_MAX);	
    /* Initialize global vars */
    if((conns = (CONN *)calloc(CONN_MAX, sizeof(CONN))))
    {
#ifdef USE_SSL
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        if((ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        /*load certificate */
        if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        /*load private key file */
        if (SSL_CTX_use_PrivateKey_file(ctx, priv_file, SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        /*check private key file */
        if (!SSL_CTX_check_private_key(ctx))
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
#endif
        memset(&sa, 0, sizeof(struct sockaddr_in));	
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = INADDR_ANY;
        sa.sin_port = htons(port);
        sa_len = sizeof(struct sockaddr_in );
        /* Initialize inet */ 
        lfd = socket(AF_INET, ev_sock_list[ev_sock_type], 0);
#ifdef USE_SSL
        /*
           if(lfd > 0 && is_use_ssl && ev_sock_list[ev_sock_type] == SOCK_STREAM)
           {
           conns[lfd].ssl = SSL_new(ctx);
           if(conns[lfd].ssl == NULL )
           {
           FATAL_LOG("new SSL with created CTX failed:%s",
           ERR_reason_error_string(ERR_get_error()));
           _exit(-1);
           }
           if((ret = SSL_set_fd(conns[lfd].ssl, lfd)) == 0)
           {
           FATAL_LOG("add SSL to tcp socket failed:%s",
           ERR_reason_error_string(ERR_get_error()));
           _exit(-1);
           }

           }
           */
#endif
        if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&opt, (socklen_t) sizeof(opt)) != 0
#ifdef SO_REUSEPORT
                || setsockopt(lfd, SOL_SOCKET, SO_REUSEPORT,
                    (char *)&opt, (socklen_t) sizeof(opt)) != 0
#endif
          )
        {
            fprintf(stderr, "setsockopt[SO_REUSEADDR] on fd[%d] failed, %s", fd, strerror(errno));
            _exit(-1);
        }
        /* Bind */
        if(bind(lfd, (struct sockaddr *)&sa, sa_len) != 0 )
        {
            SHOW_LOG("Binding failed, %s", strerror(errno));
            return -1;
        }
        /* set FD NON-BLOCK */
        /*
           if(fcntl(lfd, F_SETFL, O_NONBLOCK) != 0 )
           {
           SHOW_LOG("Setting NON-BLOCK failed, %s", strerror(errno));
           return ;
           }
           */
        /* Listen */
        if(ev_sock_list[ev_sock_type] == SOCK_STREAM)
        {
            if(listen(lfd, CONN_MAX) != 0 )
            {
                SHOW_LOG("Listening  failed, %s", strerror(errno));
                return -1;
            }
        }
        /* set multicast */
        if(ev_sock_list[ev_sock_type] == SOCK_DGRAM && multicast_ip)
        {
            struct ip_mreq mreq;
            memset(&mreq, 0, sizeof(struct ip_mreq));
            mreq.imr_multiaddr.s_addr = multicast_addr = inet_addr(multicast_ip);
            mreq.imr_interface.s_addr = INADDR_ANY;
            if(setsockopt(lfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&mreq, sizeof(mreq)) != 0)
            {
                SHOW_LOG("Setsockopt(MULTICAST) failed, %s", strerror(errno));
                return -1;
            }
        }
        SHOW_LOG("Initialize evbase ");
        /*
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
        */
        /* set evbase */
        if((evbase = evbase_init(0)))
        {
            evbase->set_logfile(evbase, "/tmp/ev_server.log");
            //evbase->set_evops(evbase, EOP_POLL);
            SHOW_LOG("Initialized event ");
            if(ev_sock_list[ev_sock_type] == SOCK_STREAM)
                event_set(&conns[lfd].event, lfd, E_READ|E_PERSIST, 
                        (void *)&conns[lfd].event, &ev_handler);
            else 
                event_set(&conns[lfd].event, lfd, E_READ|E_PERSIST, 
                        (void *)&conns[lfd].event, &ev_udp_handler);
            evbase->add(evbase, &conns[lfd].event);
            struct timeval tv = {0};
            tv.tv_usec = 1000;
            while(1)
            {
                evbase->loop(evbase, 0, &tv);
                //usleep(1000);
            }
        }
        for(i = 0; i < CONN_MAX; i++)
        {
#ifdef USE_SSL
            if(conns[i].ssl)
            {
                SSL_shutdown(conns[i].ssl);
                SSL_free(conns[i].ssl);
                conns[i].ssl = NULL;
            }
#endif
            shutdown(conns[i].fd, SHUT_RDWR);
            close(conns[i].fd);
            event_destroy(&conns[i].event);
        }
#ifdef USE_SSL
        ERR_free_strings();
        SSL_CTX_free(ctx);
#endif
        free(conns);
    }
    return 0;
}
