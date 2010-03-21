#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/resource.h>
#include <sbase.h>
#define HTTP_BUF_SIZE  65536
static SBASE *sbase = NULL;
static SERVICE *service = NULL;
static int concurrency = 1;
static int ntasks = 1024;
static int nrequests = 0;
static int nfailed = 0;
static int nerrors = 0;
static int ncompleted = 0;
static int is_keepalive = 0;
static char *server_ip = NULL;
static int server_port = 0;
static int server_is_ssl = 0;
static char request[HTTP_BUF_SIZE];
static int request_len = 0; 

CONN *http_newconn(char *ip, int port, int is_ssl)
{
    CONN *conn = NULL;
    int id = 0;

    if(nrequests < ntasks && ip && port > 0)
    {
        if(is_ssl) service->session.is_use_SSL = 1;
        if((conn = service->newconn(service, -1, -1, ip, port, NULL)))
        {
            id = ++nrequests;
            conn->start_cstate(conn);
            service->newtransaction(service, conn, id);
        }
    }
    return conn;
}

/* http request */
int http_request(CONN *conn)
{
    if(conn && request_len > 0)
    {
        return conn->push_chunk(conn, request, request_len);
    }
    return -1;
}

/* http over */
int http_over(CONN *conn, int respcode)
{
    if(conn)
    {
        if(respcode < 200 || respcode >= 300)
        {
            nerrors++;
            conn->over_cstate(conn);
            conn->over(conn);
            if((conn = http_newconn(server_ip, server_port, server_is_ssl)))
                return 0;
        }
        else
        {
            ncompleted++;
            http_request(conn);
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
                return http_request(conn);
            
        }
    }
    return -1;
}

int benchmark_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        if(is_keepalive)
            return http_request(conn);
        else
            return http_over(conn, conn->s_id);
    }
}

int benchmark_error_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        return http_over(conn, conn->s_id);
    }
}

int benchmark_timeout_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        return http_over(conn, conn->s_id);
    }
}

int benchmark_oob_handler(CONN *conn, CB_DATA *oob)
{
    if(conn)
    {
        return 0;
    }
    return -1;
}

static void benchmark_stop(int sig)
{
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            fprintf(stderr, "benchmark  is interrupted by user.\n");
            if(sbase)sbase->stop(sbase);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    pid_t pid;
    char *url = NULL, *p = NULL, ch = 0;
    int is_daemon = 0;

    /* get configure file */
    while((ch = getopt(argc, argv, "kdc:n:")) != -1)
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
                "\t-k is_keepalive\n\t-d is_daemon\n ", argv[0]);
        _exit(-1);
    }
    fprintf(stdout, "url:%s\n", url);
    _exit(-1);
    //parser url 

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
    /*setrlimiter("RLIMIT_NOFILE", RLIMIT_NOFILE, 65536)*/
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
        service->nprocthreads = 1;
        service->ndaemons = 0;
        service->service_type = C_SERVICE;
        service->family = AF_INET;
        service->sock_type = SOCK_STREAM;
        service->service_name = "benchmark";
        service->session.packet_type = PACKET_DELIMITER;
        service->session.packet_delimiter = "\r\n\r\n";
        service->session.packet_delimiter_length = 4;
        service->session.packet_handler = &benchmark_packet_handler;
        service->session.data_handler = &benchmark_data_handler;
        service->session.error_handler = &benchmark_error_handler;
        service->session.timeout_handler = &benchmark_timeout_handler;
        service->session.buffer_size = 2097152;
        //service->set_session(service, &session);
        service->set_log(service, "/tmp/benchmark_access.log");
    }
    if(sbase->add_service(sbase, service) == 0)
    {
        sbase->running(sbase, 0);
        //sbase->running(sbase, 3600);
        //sbase->running(sbase, 90000000);sbase->stop(sbase);
    }
    else fprintf(stderr, "add service failed, %s", strerror(errno));
    sbase->clean(&sbase);
    return 0;
}
