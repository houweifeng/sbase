#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <netdb.h>
#include <sbase.h>
#include "timer.h"
#include <sys/resource.h>
#define HTTP_BUF_SIZE    65536
#define HTTP_IP_MAX      16
#define HTTP_TIMEOUT     2000000
static SBASE *sbase = NULL;
static SERVICE *service = NULL;
static int concurrency = 1;
static int ncurrent = 0;
static int ntasks = 1024;
static int nrequests = 0;
static int ntimeout = 0;
static int nerrors = 0;
static int ncompleted = 0;
static int is_keepalive = 0;
static int is_post = 0;
static int is_verbosity = 0;
static char *server_host = NULL;
static char server_ip[HTTP_IP_MAX];
static int server_port = 80;
static char *server_url = "";
static char *server_argv = "";
static int server_is_ssl = 0;
static char request[HTTP_BUF_SIZE];
static int request_len = 0; 
static void *timer = NULL;
static int running_status = 0;

CONN *http_newconn(char *ip, int port, int is_ssl)
{
    CONN *conn = NULL;
    int id = 0;

    if(running_status && ncurrent <  concurrency && ip && port > 0)
    {
        if(is_ssl) service->session.is_use_SSL = 1;
        if((conn = service->newconn(service, -1, -1, ip, port, NULL)))
        {
            id = ++ncurrent;
            conn->start_cstate(conn);
            conn->c_id = id;
            conn->set_timeout(conn, HTTP_TIMEOUT);
            service->newtransaction(service, conn, id);
        }
    }
    return conn;
}

/* http request */
int http_request(CONN *conn)
{
    if(running_status && conn && request_len > 0)
    {
        //fprintf(stdout, "%s::%d conn[%d]->status:%d\n", __FILE__, __LINE__, conn->fd, conn->status);
        if(nrequests >= ntasks)
        {
            return -1;
        }
        if(nrequests == 0)
        {
            TIMER_INIT(timer);
        }
        ++nrequests;
        conn->start_cstate(conn);
        conn->set_timeout(conn, HTTP_TIMEOUT);
        //fprintf(stdout, "%s::%d conn[%d]->status:%d\n", __FILE__, __LINE__, conn->fd, conn->status);
        return conn->push_chunk(conn, request, request_len);
    }
    return -1;
}

/* http over */
int http_over(CONN *conn, int respcode)
{
    if(conn)
    {
        ncompleted++;
        if(ncompleted > 0 && (ncompleted%1000) == 0)
        {
            fprintf(stdout, "completed %d\n", ncompleted);
        }
        if(ncompleted >= ntasks)
        {
            TIMER_SAMPLE(timer);
            fprintf(stdout, "timeouts:%d\nerrors:%d\n", ntimeout, nerrors);
            if(PT_LU_USEC(timer) > 0)
                fprintf(stdout, "time used:%lld\nrequest per sec:%lld\n", PT_LU_USEC(timer), 
                        ((long long int)ncompleted * 1000000ll /PT_LU_USEC(timer)));
            _exit(-1);
        }
        if(respcode < 200 || respcode >= 300)
        {
            //fprintf(stdout, "%s::%d ERROR:%d\n", __FILE__, __LINE__, respcode);
            nerrors++;
            conn->over(conn);
            --nrequests;
            if((conn = http_newconn(server_ip, server_port, server_is_ssl)))
                return 0;
        }
        else
        {
            return http_request(conn);
        }
    }
    return -1;
}

int benchmark_packet_reader(CONN *conn, CB_DATA *buffer)
{
    return 0;
}

int benchmark_packet_handler(CONN *conn, CB_DATA *packet)
{
    char *p = NULL, *end = NULL, *s = NULL;
    int respcode = -1;

	if(conn)
    {
        p = packet->data;
        end = packet->data + packet->ndata;
        //check response code 
        if((s = strstr(p, "HTTP/")))
        {
            s += 5;
            while(*s != 0x20 && s < end)++s;
            while(*s == 0x20)++s;
            if(*s >= '0' && *s <= '9') respcode = atoi(s);
        }
        conn->s_id = respcode;
        if(respcode < 200 || respcode > 300)
        {
            //fprintf(stdout, "HTTP:%s\n", p);
            return http_over(conn, respcode);
        }
        //check Content-Length
        if((s = strstr(p, "Content-Length")) || (s = strstr(p, "content-length")))
        {
            s += 14;
            while(s < end)
            {
                if(*s >= '0' && *s <= '9')break;
                else++s;
            }
            if(*s >= '0' && *s <= '9') 
                conn->recv_chunk(conn, atoll(s));
            else 
                return http_over(conn, respcode);
            
        }
    }
    return -1;
}

int benchmark_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        return http_over(conn, conn->s_id);
    }
    return 0;
}

/* transaction handler */
int benchmark_trans_handler(CONN *conn, int tid)
{
    if(conn)
    {
        //fprintf(stdout, "trans on conn[%s:%d] via %d\n", conn->local_ip, conn->local_port, conn->fd);
        //fprintf(stdout, "conn[%d]->status:%d\n", conn->fd, conn->status);
        if(conn->status == 0)
        {
            conn->over_evstate(conn);
            return http_request(conn);
        }
        else
        {
            conn->wait_evstate(conn);
            return conn->set_timeout(conn, HTTP_TIMEOUT);
            //return service->newtransaction(service, conn, tid);
        }
    }
    return 0;
}

/* error handler */
int benchmark_error_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        //fprintf(stdout, "error on conn[%s:%d] via %d\n", conn->local_ip, conn->local_port, conn->fd);
        return http_over(conn, conn->s_id);
    }
    return -1;
}

/* timeout handler*/
int benchmark_timeout_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        if(conn->evstate == EVSTATE_WAIT)
        {
            conn->over_evstate(conn);
            return benchmark_trans_handler(conn, conn->c_id);
        }
        else
        {
            fprintf(stdout, "timeout on conn[%s:%d] via %d status:%d\n", conn->local_ip, conn->local_port, conn->fd, conn->status);
            conn->over_cstate(conn);
            return http_over(conn, 0);
        }
    }
    return -1;
}

int benchmark_oob_handler(CONN *conn, CB_DATA *oob)
{
    if(conn)
    {
        return 0;
    }
    return -1;
}

/* heartbeat */
void benchmark_heartbeat_handler(void *arg)
{
    CONN *conn = NULL;

    while(ncurrent < concurrency)
    {
        if((conn = http_newconn(server_ip, server_port, server_is_ssl)) == NULL)
        {
            break;
        }
    }
    return ;
}

static void benchmark_stop(int sig)
{
    switch (sig) 
    {
        case SIGINT:
        case SIGTERM:
            fprintf(stderr, "benchmark  is interrupted by user.\n");
            running_status = 0;
            if(sbase)sbase->stop(sbase);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    pid_t pid;
    char *url = NULL, line[HTTP_BUF_SIZE], *s = NULL, *p = NULL, ch = 0;
    struct hostent *hent = NULL;
    int is_daemon = 0;

    /* get configure file */
    while((ch = getopt(argc, argv, "vpkdc:n:")) != -1)
    {
        switch(ch)
        {
            case 'c': 
                concurrency = atoi(optarg);
                break;
            case 'n':
                ntasks = atoi(optarg);
                break;
            case 'k':
                is_keepalive = 1;
                break;
            case 'd':
                is_daemon = 1;
                break;
            case 'p':
                is_post = 1;
                break;
            case 'v':
                is_verbosity = 1;
                break;
            case '?':
                url = argv[optind];
                break;
            default:
                break;
        }
    }
    if(url == NULL && optind < argc)
    {
        //fprintf(stdout, "opt:%c optind:%d arg:%s\n", ch, optind, argv[optind]);
        url = argv[optind];
    }
    //fprintf(stdout, "concurrency:%d nrequests:%d is_keepalive:%d is_daemon:%d\n",
    //       concurrency, ntasks, is_keepalive, is_daemon);
    if(url == NULL)
    {
        fprintf(stderr, "Usage:%s [options] http(s)://host:port/path\n"
                "Options:\n\t-c concurrency\n\t-n requests\n"
                "\t-p is_POST\n\t-v is_verbosity\n"
                "\t-k is_keepalive\n\t-d is_daemon\n ", argv[0]);
        _exit(-1);
    }
    p = url;
    s = line;
    while(*p != '\0')
    {
        if(*p >= 'A' && *p <= 'Z')
        {
            *s++ = *p++ + 'a' - 'A';
        }
        else if(*((unsigned char *)p) > 127 || *p == 0x20)
        {
            s += sprintf(s, "%%%02x", *((unsigned char *)p));
            ++p;
        }
        else *s++ = *p++;
    }
    *s = '\0';
    s = line;
    if(strncmp(s, "http://", 7) == 0)
    {
        s += 7;
        server_host = s;
    }
    else if(strncmp(s, "https://", 8) == 0)
    {
        s += 8;
        server_host = s;
        server_is_ssl = 1;
    }
    else goto invalid_url;
    while(*s != '\0' && *s != ':' && *s != '/')s++;
    if(*s == ':')
    {
        *s = '\0';
        ++s;
        server_port = atoi(s);          
        while(*s != '\0' && *s != '/')++s;
    }
    if(*s == '/')
    {
        *s = '\0';
        ++s;
        server_url = s;
    }
    while(*s != '\0' && *s != '?')++s;
    if(*s == '?')
    {
        *s = '\0';
        ++s;
        server_argv = s;
    }
invalid_url:
    if(server_host == NULL || server_port <= 0)
    {
        fprintf(stderr, "Invalid url:%s, url must be http://host:port/path?argv "
                " or https://host:port/path?argv\n", url);
        _exit(-1);
    }
    if(is_post)
    {
        p = request;
        p += sprintf(p, "POST /%s HTTP/1.1\r\n", server_url);
        p += sprintf(p, "Host: %s:%d\r\n", server_host, server_port);
        if(is_keepalive) p += sprintf(p, "Connection: KeepAlive\r\n");
        p += sprintf(p, "Content-Length: %d\r\n\r\n", (int)strlen(server_argv));
        p += sprintf(p, "%s", server_argv);
        request_len = p - request;
    }
    else
    {
        p = request;
        p += sprintf(p, "GET /%s?%s HTTP/1.1\r\n", server_url, server_argv);
        p += sprintf(p, "Host: %s:%d\r\n", server_host, server_port);
        if(is_keepalive) p += sprintf(p, "Connection: KeepAlive\r\n");
        request_len = p - request;
    }
    if((hent = gethostbyname(server_host)) == NULL)
    {
        fprintf(stderr, "resolve hostname:%s failed, %s\n", server_host, strerror(h_errno));
        _exit(-1);
    }
    else
    {
        //memcpy(&ip, &(hent->h_addr), sizeof(int));
        sprintf(server_ip, "%s", inet_ntoa(*((struct in_addr *)(hent->h_addr))));
        if(is_verbosity)
        {
            fprintf(stdout, "ip:%s request:%s\n", server_ip, request);
        }
    }
    //_exit(-1);
    /* locale */
    setlocale(LC_ALL, "C");
    /* signal */
    signal(SIGTERM, &benchmark_stop);
    signal(SIGINT,  &benchmark_stop);
    signal(SIGHUP,  &benchmark_stop);
    signal(SIGPIPE, SIG_IGN);
    //daemon
    if(is_daemon)
    {
        pid = fork();
        switch (pid) {
            case -1:
                perror("fork()");
                exit(EXIT_FAILURE);
                break;
            case 0: //child
                if(setsid() == -1)
                    exit(EXIT_FAILURE);
                break;
            default://parent
                _exit(EXIT_SUCCESS);
                break;
        }
    }
    setrlimiter("RLIMIT_NOFILE", RLIMIT_NOFILE, 10240);
    if((sbase = sbase_init()) == NULL)
    {
        exit(EXIT_FAILURE);
        return -1;
    }
    sbase->nchilds = 0;
    sbase->usec_sleep = 1000;
    sbase->connections_limit = 65536;
    //sbase->set_log(sbase, "/tmp/sd.log");
    //sbase->set_evlog(sbase, "/tmp/evsd.log");
    if((service = service_init()))
    {
        service->working_mode = 0;
        service->nprocthreads = 8;
        service->ndaemons = 0;
        service->use_iodaemon = 1;
        service->service_type = C_SERVICE;
        service->family = AF_INET;
        service->sock_type = SOCK_STREAM;
        service->service_name = "benchmark";
        service->session.packet_type = PACKET_DELIMITER;
        service->session.packet_delimiter = "\r\n\r\n";
        service->session.packet_delimiter_length = 4;
        service->session.packet_handler = &benchmark_packet_handler;
        service->session.data_handler = &benchmark_data_handler;
        service->session.transaction_handler = &benchmark_trans_handler;
        service->session.error_handler = &benchmark_error_handler;
        service->session.timeout_handler = &benchmark_timeout_handler;
        service->session.buffer_size = 65536;
        service->set_heartbeat(service, 1000000, &benchmark_heartbeat_handler, NULL);
        //service->set_session(service, &session);
        service->set_log(service, "/tmp/benchmark_access.log");
    }
    if(sbase->add_service(sbase, service) == 0)
    {
        running_status = 1;
        sbase->running(sbase, 0);
        //sbase->running(sbase, 3600);
        //sbase->running(sbase, 90000000);sbase->stop(sbase);
    }
    else fprintf(stderr, "add service failed, %s", strerror(errno));
    sbase->clean(&sbase);
    return 0;
}
