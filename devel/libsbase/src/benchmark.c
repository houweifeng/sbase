#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/resource.h>
#include <sbase.h>
static SBASE *sbase = NULL;
static SERVICE *service = NULL;
static int concurrent = 1;
static int nconns = 1024;
static int nfailed = 0;
static int is_keepalive = 1;

int benchmark_packet_reader(CONN *conn, CB_DATA *buffer)
{
    return 0;
}

int benchmark_packet_handler(CONN *conn, CB_DATA *packet)
{
	if(conn && conn->push_chunk)
    {
		return conn->push_chunk((CONN *)conn, ((CB_DATA *)packet)->data, packet->ndata);
    }
    return -1;
}

int benchmark_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {

    }
}

int benchmark_error_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {

    }
}

int benchmark_timeout_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {

    }
}

int benchmark_oob_handler(CONN *conn, CB_DATA *oob)
{
    if(conn && conn->push_chunk)
    {
        conn->push_chunk((CONN *)conn, ((CB_DATA *)oob)->data, oob->ndata);
        return oob->ndata;
    }
    return -1;
}

static void benchmark_stop(int sig){
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
    char *conf = NULL, *p = NULL, ch = 0;
    int is_daemon = 0;

    /* get configure file */
    while((ch = getopt(argc, argv, "c:n:kd")) != -1)
    {
        if(ch == 'c') concurrent = atoi(optarg);
        else if(ch == 'n') nconns = atoi(optarg);
        else if(ch == 'k') is_keepalive = atoi(optarg);
        else if(ch == 'd') is_daemon = 1;
    }
    if(conf == NULL)
    {
        fprintf(stderr, "Usage:%s -d -c config_file\n", argv[0]);
        _exit(-1);
    }
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
        session.packet_type = PACKET_DELIMITER;
        session.packet_delimiter = "\r\n\r\n";
        session.packet_delimiter_length = 4;
        session.packet_handler = &benchmark_packet_handler;
        session.data_handler = &benchmark_data_handler;
        session.error_handler = &benchmark_error_handler;
        session.timeout_handler = &benchmark_timeout_handler;
        session.buffer_size = 2097152;
        service->set_session(service, &session);
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
