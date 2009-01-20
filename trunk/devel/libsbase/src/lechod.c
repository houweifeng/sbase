#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/resource.h>
#include <sbase.h>
#include "iniparser.h"

SBASE *sbase = NULL;
dictionary *dict = NULL;

int lechod_packet_reader(CONN *conn, CB_DATA *buffer)
{
}

int lechod_packet_handler(CONN *conn, CB_DATA *packet)
{
	if(conn && conn->push_chunk)
		conn->push_chunk((CONN *)conn, ((CB_DATA *)packet)->data, packet->ndata);
}

int lechod_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
}

int lechod_oob_handler(CONN *conn, CB_DATA *oob)
{
}

/* Initialize from ini file */
int sbase_initialize(SBASE *sbase, char *conf)
{
	char *logfile = NULL, *s = NULL, *p = NULL;
	int n = 0;
	SERVICE *service = NULL;
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
	/* LECHOD */
	if((service = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize service failed, %s", strerror(errno));
		_exit(-1);
	}
	service->family = iniparser_getint(dict, "LECHOD:inet_family", AF_INET);
	service->sock_type = iniparser_getint(dict, "LECHOD:socket_type", SOCK_STREAM);
	service->ip = iniparser_getstr(dict, "LECHOD:service_ip");
	service->port = iniparser_getint(dict, "LECHOD:service_port", 80);
	service->working_mode = iniparser_getint(dict, "LECHOD:working_mode", WORKING_PROC);
	service->service_type = iniparser_getint(dict, "LECHOD:service_type", C_SERVICE);
	service->service_name = iniparser_getstr(dict, "LECHOD:service_name");
	service->nprocthreads = iniparser_getint(dict, "LECHOD:nprocthreads", 1);
	service->ndaemons = iniparser_getint(dict, "LECHOD:ndaemons", 0);
	service->set_log(service, iniparser_getstr(dict, "LECHOD:logfile"));
    	service->session.packet_type = iniparser_getint(dict, "LECHOD:packet_type",PACKET_DELIMITER);
	service->session.packet_delimiter = iniparser_getstr(dict, "LECHOD:packet_delimiter");
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
	service->session.buffer_size = iniparser_getint(dict, "LECHOD:buffer_size", SB_BUF_SIZE);
	service->session.packet_reader = &lechod_packet_reader;
	service->session.packet_handler = &lechod_packet_handler;
	service->session.data_handler = &lechod_data_handler;
	service->session.oob_handler = &lechod_oob_handler;
	/* server */
	fprintf(stdout, "Parsing for server...\n");
	return sbase->add_service(sbase, service);
}

static void lechod_stop(int sig){
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            fprintf(stderr, "lhttpd server is interrupted by user.\n");
            if(sbase)sbase->stop(sbase);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    pid_t pid;
    char *conf = NULL;

    /* get configure file */
    if(getopt(argc, argv, "c:") != 'c')
    {
        fprintf(stderr, "Usage:%s -c config_file\n", argv[0]);
        _exit(-1);
    }       
    conf = optarg;
    /* locale */
    setlocale(LC_ALL, "C");
    /* signal */
    signal(SIGTERM, &lechod_stop);
    signal(SIGINT,  &lechod_stop);
    signal(SIGHUP,  &lechod_stop);
    signal(SIGPIPE, SIG_IGN);
/*
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
*/
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
        return -1;
    }
    fprintf(stdout, "Initialized successed\n");
    //sbase->running(sbase, 0);
    //sbase->running(sbase, 3600);
    sbase->running(sbase, 60000000);
    sbase->stop(sbase);
    sbase->clean(&sbase);
    if(dict)iniparser_free(dict);
}
