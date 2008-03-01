#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sbase.h>
#include "iniparser.h"

#define SBASE_LOG       "/tmp/sbase_access_log"
#define LECHOD_LOG      "/tmp/lechod_access_log"
#define LECHOD_EVLOG      "/tmp/lechod_evbase_log"
SBASE *sbase = NULL;
dictionary *dict = NULL;

int cb_packet_reader(CONN *conn, BUFFER *buffer)
{
}

void cb_packet_handler(CONN *conn, BUFFER *packet)
{
	if(conn && conn->push_chunk)
		conn->push_chunk((CONN *)conn, ((BUFFER *)packet)->data, packet->size);
}

void cb_data_handler(CONN *conn, BUFFER *packet, CHUNK *chunk, BUFFER *cache)
{
}

void cb_oob_handler(CONN *conn, BUFFER *oob)
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
	fprintf(stdout, "Parsing SBASE...\n");
	sbase->working_mode = iniparser_getint(dict, "SBASE:working_mode", WORKING_PROC);
	sbase->max_procthreads = iniparser_getint(dict, "SBASE:max_procthreads", 1);
	if(sbase->max_procthreads > MAX_PROCTHREADS)
		sbase->max_procthreads = MAX_PROCTHREADS;
	sbase->sleep_usec = iniparser_getint(dict, "SBASE:sleep_usec", MIN_SLEEP_USEC);
	if((logfile = iniparser_getstr(dict, "SBASE:logfile")) == NULL)
		logfile = SBASE_LOG;
	fprintf(stdout, "Parsing LOG[%s]...\n", logfile);
	fprintf(stdout, "SBASE[%08x] sbase->evbase:%08x ...\n", sbase, sbase->evbase);
	sbase->set_log(sbase, logfile);
	if((logfile = iniparser_getstr(dict, "SBASE:evlogfile")))
	    sbase->set_evlog(sbase, logfile);
	/* LECHOD */
	fprintf(stdout, "Parsing LECHOD...\n");
	if((service = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize service failed, %s", strerror(errno));
		_exit(-1);
	}
	/* INET protocol family */
	n = iniparser_getint(dict, "LECHOD:inet_family", 0);
	/* INET protocol family */
	n = iniparser_getint(dict, "LECHOD:inet_family", 0);
	switch(n)
	{
		case 0:
			service->family = AF_INET;
			break;
		case 1:
			service->family = AF_INET6;
			break;
		default:
			fprintf(stderr, "Illegal INET family :%d \n", n);
			_exit(-1);
	}
	/* socket type */
	n = iniparser_getint(dict, "LECHOD:socket_type", 0);
	switch(n)
	{
		case 0:
			service->socket_type = SOCK_STREAM;
			break;
		case 1:
			service->socket_type = SOCK_DGRAM;
			break;
		default:
			fprintf(stderr, "Illegal socket type :%d \n", n);
			_exit(-1);
	}
	/* service name and ip and port */
	service->name = iniparser_getstr(dict, "LECHOD:service_name");
	service->ip = iniparser_getstr(dict, "LECHOD:service_ip");
	if(service->ip && service->ip[0] == 0 )
		service->ip = NULL;
	service->port = iniparser_getint(dict, "LECHOD:service_port", 80);
	service->working_mode = iniparser_getint(dict, "LECHOD:working_mode", WORKING_PROC);
	service->max_procthreads = iniparser_getint(dict, "LECHOD:max_procthreads", 1);
	service->sleep_usec = iniparser_getint(dict, "LECHOD:sleep_usec", 100);
	logfile = iniparser_getstr(dict, "LECHOD:logfile");
	if(logfile == NULL)
		logfile = LECHOD_LOG;
	service->logfile = logfile;
	logfile = iniparser_getstr(dict, "LECHOD:evlogfile");
	service->evlogfile = logfile;
	service->max_connections = iniparser_getint(dict, "LECHOD:max_connections", MAX_CONNECTIONS);
	service->packet_type = PACKET_DELIMITER;
	service->packet_delimiter = iniparser_getstr(dict, "LECHOD:packet_delimiter");
	p = s = service->packet_delimiter;
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
	service->packet_delimiter_length = strlen(service->packet_delimiter);
	service->buffer_size = iniparser_getint(dict, "LECHOD:buffer_size", SB_BUF_SIZE);
	service->cb_packet_reader = &cb_packet_reader;
	service->cb_packet_handler = &cb_packet_handler;
	service->cb_data_handler = &cb_data_handler;
	service->cb_oob_handler = &cb_oob_handler;
	/* server */
	fprintf(stdout, "Parsing for server...\n");
	return sbase->add_service(sbase, service);
}

static void cb_stop(int sig){
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
	// locale
	setlocale(LC_ALL, "C");
	// signal
	signal(SIGTERM, &cb_stop);
	signal(SIGINT,  &cb_stop);
	signal(SIGHUP,  &cb_stop);
	signal(SIGPIPE, SIG_IGN);
	pid = fork();
	switch (pid) {
		case -1:
			perror("fork()");
			exit(EXIT_FAILURE);
			break;
		case 0: //child process
			if(setsid() == -1)
				exit(EXIT_FAILURE);
			break;
		default://parent
			_exit(EXIT_SUCCESS);
			break;
	}


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
        //sbase->running(sbase, 3600);
        sbase->running(sbase, 0);
}
