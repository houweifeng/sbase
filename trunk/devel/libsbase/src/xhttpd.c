#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/resource.h>
#include <sbase.h>
#include "iniparser.h"
#include "http.h"
#include "mime.h"

static SBASE *sbase = NULL;
static SERVICE *service = NULL;
static dictionary *dict = NULL;
/* xhttpd packet reader */
int xhttpd_packet_reader(CONN *conn, CB_DATA *buffer)
{
    return buffer->ndata;
}
/* packet handler */
int xhttpd_packet_handler(CONN *conn, CB_DATA *packet)
{
	if(conn && conn->push_chunk)
    {
		return conn->push_chunk((CONN *)conn, ((CB_DATA *)packet)->data, packet->ndata);
    }
    return -1;
}
/* data handler */
int xhttpd_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
}
/* OOB handler */
int xhttpd_oob_handler(CONN *conn, CB_DATA *oob)
{
    if(conn && conn->push_chunk)
    {
        conn->push_chunk((CONN *)conn, ((CB_DATA *)oob)->data, oob->ndata);
        return oob->ndata;
    }
    return -1;
}
/* signal */
static void xhttpd_stop(int sig)
{
    switch (sig) 
    {
        case SIGINT:
        case SIGTERM:
            fprintf(stderr, "xhttpd server is interrupted by user.\n");
            if(sbase)sbase->stop(sbase);
            break;
        default:
            break;
    }
}

/* Initialize from ini file */
int sbase_initialize(SBASE *sbase, char *conf)
{
	char *logfile = NULL, *s = NULL, *p = NULL, *cacert_file = NULL, *privkey_file = NULL;
	int n = 0, ret = -1;
	if((dict = iniparser_new(conf)) == NULL)
	{
		fprintf(stderr, "Initializing conf:%s failed, %s\n", conf, strerror(errno));
		_exit(-1);
	}
	/* SBASE */
	sbase->nchilds = iniparser_getint(dict, "SBASE:nchilds", 0);
	sbase->connections_limit = iniparser_getint(dict, "SBASE:connections_limit", SB_CONN_MAX);
	sbase->usec_sleep = iniparser_getint(dict, "SBASE:usec_sleep", SB_USEC_SLEEP);
	sbase->set_log(sbase, iniparser_getstr(dict, "SBASE:logfile"));
	sbase->set_evlog(sbase, iniparser_getstr(dict, "SBASE:evlogfile"));
	/* XHTTPD */
	if((service = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize service failed, %s", strerror(errno));
		_exit(-1);
	}
	service->family = iniparser_getint(dict, "XHTTPD:inet_family", AF_INET);
	service->sock_type = iniparser_getint(dict, "XHTTPD:socket_type", SOCK_STREAM);
	service->ip = iniparser_getstr(dict, "XHTTPD:service_ip");
	service->port = iniparser_getint(dict, "XHTTPD:service_port", 80);
	service->working_mode = iniparser_getint(dict, "XHTTPD:working_mode", WORKING_PROC);
	service->service_type = iniparser_getint(dict, "XHTTPD:service_type", C_SERVICE);
	service->service_name = iniparser_getstr(dict, "XHTTPD:service_name");
	service->nprocthreads = iniparser_getint(dict, "XHTTPD:nprocthreads", 1);
	service->ndaemons = iniparser_getint(dict, "XHTTPD:ndaemons", 0);
    service->session.packet_type=iniparser_getint(dict, "XHTTPD:packet_type",PACKET_DELIMITER);
    if((service->session.packet_delimiter = iniparser_getstr(dict, "XHTTPD:packet_delimiter")))
    {
        p = s = service->session.packet_delimiter;
        while(*p != 0 )
        {
            if(*p == '\\' && *(p+1) == 'n')
            {
                *s++ = '\n';
                p += 2;
            }
            else if (*p == '\\' && *(p+1) == 'r')
            {
                *s++ = '\r';
                p += 2;
            }
            else
                *s++ = *p++;
        }
        *s++ = 0;
        service->session.packet_delimiter_length = strlen(service->session.packet_delimiter);
    }
	service->session.buffer_size = iniparser_getint(dict, "XHTTPD:buffer_size", SB_BUF_SIZE);
	service->session.packet_reader = &xhttpd_packet_reader;
	service->session.packet_handler = &xhttpd_packet_handler;
	service->session.data_handler = &xhttpd_data_handler;
    service->session.oob_handler = &xhttpd_oob_handler;
    cacert_file = iniparser_getstr(dict, "XHTTPD:cacert_file");
    privkey_file = iniparser_getstr(dict, "XHTTPD:privkey_file");
    if(cacert_file && privkey_file && iniparser_getint(dict, "XHTTPD:is_use_SSL", 0))
    {
        service->is_use_SSL = 1;
        service->cacert_file = cacert_file;
        service->privkey_file = privkey_file;
    }
    if((p = iniparser_getstr(dict, "XHTTPD:logfile")))
    {
        service->set_log(service, p);
    }
	/* server */
	fprintf(stdout, "Parsing for server...\n");
	return sbase->add_service(sbase, service);
    /*
    if(service->sock_type == SOCK_DGRAM 
            && (p = iniparser_getstr(dict, "XHTTPD:multicast")) && ret == 0)
    {
        ret = service->add_multicast(service, p);
    }
    return ret;
    */
}

int main(int argc, char **argv)
{
    pid_t pid;
    char *conf = NULL, *p = NULL, ch = 0;
    int is_daemon = 0;

    /* get configure file */
    while((ch = getopt(argc, argv, "c:d")) != -1)
    {
        if(ch == 'c') conf = optarg;
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
    signal(SIGTERM, &xhttpd_stop);
    signal(SIGINT,  &xhttpd_stop);
    signal(SIGHUP,  &xhttpd_stop);
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
    fprintf(stdout, "Initializing from configure file:%s\n", conf);
    /* Initialize sbase */
    if(sbase_initialize(sbase, conf) != 0 )
    {
        fprintf(stderr, "Initialize from configure file failed\n");
        exit(EXIT_FAILURE);
        return -1;
    }
    fprintf(stdout, "Initialized successed\n");
    if(service->sock_type == SOCK_DGRAM 
            && (p = iniparser_getstr(dict, "XHTTPD:multicast")))
    {
        if(service->add_multicast(service, p) != 0)
        {
            fprintf(stderr, "add multicast:%s failed, %s", p, strerror(errno));
            exit(EXIT_FAILURE);
            return -1;
        }
        p = "224.1.1.168";
        if(service->add_multicast(service, p) != 0)
        {
            fprintf(stderr, "add multicast:%s failed, %s", p, strerror(errno));
            exit(EXIT_FAILURE);
            return -1;
        }

    }
    sbase->running(sbase, 0);
    //sbase->running(sbase, 3600);
    //sbase->running(sbase, 90000000);sbase->stop(sbase);
    sbase->clean(&sbase);
    if(dict)iniparser_free(dict);
}
