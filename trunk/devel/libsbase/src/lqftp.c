#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/stat.h>
#include <sbase.h>
#include "iniparser.h"
#include "task.h"
#include "basedef.h"
char *cmdlist[] = {"put", "status"};
#define     OP_NUM          2
#define     OP_PUT          0
#define     OP_STATUS       1
#ifndef     BUF_SIZE
#define     BUF_SIZE      65536
#endif
/**** lqftpd response ID *****/
int  resplist[] = {200, 201, 203, 205, 207, 209};
#define RESP_NUM                    6
#define RESP_OK_ID                  0
#define RESP_NOT_IMPLEMENT_ID       1
#define RESP_BAD_REQ_ID             2
#define RESP_SERVER_ERROR_ID        3
#define RESP_FILE_NOT_EXISTS_ID     4
#define RESOP_INVALID_MD5_ID        5
/**** lqftp response ID *****/
#define     RESP_OK_CODE                300
#define     RESP_FILE_NOT_EXIST_CODE    301
#define     RESP_INVALID_TASK_CODE      303
#define     RESP_BAD_REQ                "307 bad requestment\r\n\r\n"
#define     SBASE_LOG                   "/tmp/sbase_access_log"
#define     LQFTP_LOG                   "/tmp/lqftp_access_log"
#define     LQFTP_EVLOG                 "/tmp/lqftp_evbase_log"
static SBASE *sbase = NULL;
static SERVICE *transport = NULL;
static SERVICE *serv = NULL;
static dictionary *dict = NULL;
static TASKTABLE *tasktable = NULL;
#define GET_RESPID(p, end, n, respid)                                               \
{                                                                                   \
    respid = -1;                                                                    \
    while(p < end && (*p == ' ' || *p == '\t'))++p;                                 \
    respid = atoi(p);                                                               \
    n = 0;                                                                          \
    while(n < RESP_NUM)                                                             \
    {                                                                               \
        if(resplist[n] == respid)                                                   \
        {                                                                           \
            respid = n;                                                             \
            break;                                                                  \
        }                                                                           \
        ++n;                                                                        \
    }                                                                               \
}

//functions
//error handler for cstate
void cb_transport_error_handler(const CONN *conn)
{
    int blockid = -1;
    if(conn)
    {
        blockid = conn->c_id;
        tasktable->update_status(tasktable, blockid, BLOCK_STATUS_ERROR);
        conn->over_cstate((CONN *)conn);
    }
}

//
int cb_transport_packet_reader(const CONN *conn, const BUFFER *buffer)
{
}

void cb_transport_packet_handler(const CONN *conn, const BUFFER *packet)
{        
    char *p = NULL, *end = NULL;
    int respid = -1, n = 0;
    int blockid = -1;
    int status = BLOCK_STATUS_ERROR;

    if(conn)
    {
        p = (char *)packet->data;
        end = (char *)packet->end;
        GET_RESPID(p, end, n, respid);
        blockid = conn->c_id;
        switch(respid)
        {
            case RESP_OK_ID:
                status = BLOCK_STATUS_OVER;
                break;
            default:
                status = BLOCK_STATUS_ERROR; 
                break;
        }
        tasktable->update_status(tasktable, blockid, status);
        conn->over_cstate((CONN *)conn);
    }
}

void cb_transport_data_handler(const CONN *conn, const BUFFER *packet, 
        const CHUNK *chunk, const BUFFER *cache)
{
}

void cb_transport_oob_handler(const CONN *conn, const BUFFER *oob)
{
}

void cb_serv_heartbeat_handler(void *arg)
{
    int taskid = 0;
    TASK *task = NULL;
    TBLOCK *block = NULL;
    struct stat st;
    CONN *c_conn = NULL;
    char buf[BUF_SIZE];
    int n = 0;

    if(serv && transport && tasktable)
    {
        //check and start new task
        taskid = tasktable->running_task_id;
        if(taskid == -1)
            taskid = tasktable->running_task_id = 0;    

        while(taskid < tasktable->ntask)
        {
            if(tasktable->table 
                    && (task = tasktable->table[taskid])
                    && task->status != TASK_STATUS_OVER)
            {
                if(tasktable->status == NULL)
                {
                    tasktable->running_task_id = taskid;
                    tasktable->ready(tasktable, taskid);
                }
                task = tasktable->table[taskid];
                tasktable->table[taskid]->nretry++;

                while((c_conn = transport->getconn(transport)))
                {
                    if((block = tasktable->pop_block(tasktable)))
                    {
                        c_conn->c_id = block->id;
                        if(block->cmdid == CMD_PUT)
                        {
                            if(stat(tasktable->table[taskid]->file, &st) == 0 && st.st_size > 0) 
                            {
                                n = sprintf(buf, "put %s\r\noffset:%llu\r\nsize:%llu\r\n\r\n",
                                        tasktable->table[taskid]->destfile, 
                                        block->offset, block->size); 
                                c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                                c_conn->push_file((CONN *)c_conn, 
                                        tasktable->table[taskid]->file, 
                                        block->offset, block->size);
                            }
                        }
                        if(block->cmdid == CMD_MD5SUM)
                        {
                            tasktable->md5sum(tasktable, taskid);
                            n = sprintf(buf, "md5sum %s\r\nmd5:%s\r\n\r\n", 
                                    tasktable->table[taskid]->destfile,
                                    tasktable->table[taskid]->md5); 
                            c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                        }
                    }
                    else
                    {
                        c_conn->over_cstate((CONN *)c_conn);
						break;
                    }
                }
                break;
            }
            ++taskid;
        }
        //if task is running then log TIMEOUT
    }
    return  ;
}

int cb_serv_packet_reader(const CONN *conn, const BUFFER *buffer)
{
}

void cb_serv_packet_handler(const CONN *conn, const BUFFER *packet)
{
    char *p = NULL, *end = NULL, *np = NULL;
    char file[PATH_MAX_SIZE], destfile[PATH_MAX_SIZE], buf[BUF_SIZE];
    int cmdid = -1;
    int i = 0, n = 0;
    int taskid = -1;

    if(conn)
    {
        p = (char *)packet->data;
        end = (char *)packet->end;
        while(p < end && *p == ' ') ++p;
        for(i = 0; i < OP_NUM; i++)
        {
            if(strncasecmp(p, cmdlist[i], strlen(cmdlist[i])) == 0)
            {
                cmdid = i;
                break;
            }
        }
        if(cmdid == OP_PUT)
        {
            while(p < end && *p != ' ' )++p;
            while(p < end && *p == ' ')++p;
            np = file;
            while(p < end && *p != ' ' && *p != '\r' && *p != '\n')*np++ = *p++;
            while(p < end && *p == ' ')++p;
            *np = '\0';
            n = np - file;

            np = destfile;
            while(p < end && *p != ' ' && *p != '\r' && *p != '\n')*np++ = *p++;
            *np = '\0';
            if(n > 0 && (np - destfile) > 0) 
            {
                if((taskid = tasktable->add(tasktable, file, destfile)) >= 0)
                {
                    n = sprintf(buf, "%d OK\r\ntaskid:%ld\r\n\r\n", RESP_OK_CODE, taskid);
                    conn->push_chunk((CONN *)conn, (void *)buf, n);
                    return;
                }
					//fprintf(stdout, "stat %s failed, %s\n", file, strerror(errno));
            }
            return ;
        }
        if(cmdid == OP_STATUS)
        {
            while(p < end && *p != ' ' )++p;
            while(p < end && *p == ' ')++p;
            taskid = atoi(p);
            if(taskid >= 0 && taskid < tasktable->ntask)
            {
                n = sprintf(buf, "%d OK\r\ntaskid:%d\r\nstatus:%d\r\n"
                                 "timeout:%d(seconds)\r\nretry:%d\r\n\r\n", 
                                 RESP_OK_CODE, taskid, tasktable->table[taskid]->status,
                                 tasktable->table[taskid]->timeout, 
                                 tasktable->table[taskid]->nretry);
                conn->push_chunk((CONN *)conn, (void *)buf, n);
            }
            else
            {
                n = sprintf(buf, "%d invalid taskid %d\r\n\r\n", RESP_INVALID_TASK_CODE, taskid);
                conn->push_chunk((CONN *)conn, (void *)buf, n);
            }
            return;
        }
        conn->push_chunk((CONN *)conn, (void *)RESP_BAD_REQ, strlen(RESP_BAD_REQ));
    }
    return ;
}

void cb_serv_data_handler(const CONN *conn, const BUFFER *packet, 
        const CHUNK *chunk, const BUFFER *cache)
{
}

void cb_serv_oob_handler(const CONN *conn, const BUFFER *oob)
{
}

/* Initialize from ini file */
int sbase_initialize(SBASE *sbase, char *conf)
{
	char *logfile = NULL, *s = NULL, *p = NULL, *taskfile = NULL, *statusfile = NULL;
	int n = 0;
	int ret = 0;

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
	/* TRANSPORT */
	fprintf(stdout, "Parsing LQFTP...\n");
	if((transport = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize transport failed, %s", strerror(errno));
		_exit(-1);
	}
    /* service type */
    transport->service_type = iniparser_getint(dict, "TRANSPORT:service_type", 1);
	/* INET protocol family */
	n = iniparser_getint(dict, "TRANSPORT:inet_family", 0);
	/* INET protocol family */
	n = iniparser_getint(dict, "TRANSPORT:inet_family", 0);
	switch(n)
	{
		case 0:
			transport->family = AF_INET;
			break;
		case 1:
			transport->family = AF_INET6;
			break;
		default:
			fprintf(stderr, "Illegal INET family :%d \n", n);
			_exit(-1);
	}
	/* socket type */
	n = iniparser_getint(dict, "TRANSPORT:socket_type", 0);
	switch(n)
	{
		case 0:
			transport->socket_type = SOCK_STREAM;
			break;
		case 1:
			transport->socket_type = SOCK_DGRAM;
			break;
		default:
			fprintf(stderr, "Illegal socket type :%d \n", n);
			_exit(-1);
	}
	/* transport name and ip and port */
	transport->name = iniparser_getstr(dict, "TRANSPORT:service_name");
	transport->ip = iniparser_getstr(dict, "TRANSPORT:service_ip");
	if(transport->ip && transport->ip[0] == 0 )
		transport->ip = NULL;
	transport->port = iniparser_getint(dict, "TRANSPORT:service_port", 80);
	transport->max_procthreads = iniparser_getint(dict, "TRANSPORT:max_procthreads", 1);
	transport->sleep_usec = iniparser_getint(dict, "TRANSPORT:sleep_usec", 100);
    /* connections number */
	transport->connections_limit = iniparser_getint(dict, "TRANSPORT:connections_limit", 32);
	logfile = iniparser_getstr(dict, "TRANSPORT:logfile");
	if(logfile == NULL)
		logfile = LQFTP_LOG;
	transport->logfile = logfile;
	logfile = iniparser_getstr(dict, "TRANSPORT:evlogfile");
	transport->evlogfile = logfile;
	transport->max_connections = iniparser_getint(dict, 
            "TRANSPORT:max_connections", MAX_CONNECTIONS);
	transport->packet_type = PACKET_DELIMITER;
	transport->packet_delimiter = iniparser_getstr(dict, "TRANSPORT:packet_delimiter");
	p = s = transport->packet_delimiter;
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
	transport->packet_delimiter_length = strlen(transport->packet_delimiter);
	transport->buffer_size = iniparser_getint(dict, "TRANSPORT:buffer_size", BUF_SIZE);
	transport->cb_packet_reader = &cb_transport_packet_reader;
	transport->cb_packet_handler = &cb_transport_packet_handler;
	transport->cb_data_handler = &cb_transport_data_handler;
	transport->cb_oob_handler = &cb_transport_oob_handler;
	/* server */
	fprintf(stdout, "Parsing for transport...\n");

    /* DAEMON */
    if((serv = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize serv failed, %s", strerror(errno));
		_exit(-1);
	}
    /* service type */
	serv->service_type = iniparser_getint(dict, "DAEMON:service_type", 0);
	/* INET protocol family */
	n = iniparser_getint(dict, "DAEMON:inet_family", 0);
	/* INET protocol family */
	n = iniparser_getint(dict, "DAEMON:inet_family", 0);
	switch(n)
	{
		case 0:
			serv->family = AF_INET;
			break;
		case 1:
			serv->family = AF_INET6;
			break;
		default:
			fprintf(stderr, "Illegal INET family :%d \n", n);
			_exit(-1);
	}
	/* socket type */
	n = iniparser_getint(dict, "DAEMON:socket_type", 0);
	switch(n)
	{
		case 0:
			serv->socket_type = SOCK_STREAM;
			break;
		case 1:
			serv->socket_type = SOCK_DGRAM;
			break;
		default:
			fprintf(stderr, "Illegal socket type :%d \n", n);
			_exit(-1);
	}
	/* serv name and ip and port */
	serv->name = iniparser_getstr(dict, "DAEMON:service_name");
	serv->ip = iniparser_getstr(dict, "DAEMON:service_ip");
	if(serv->ip && serv->ip[0] == 0 )
		serv->ip = NULL;
	serv->port = iniparser_getint(dict, "DAEMON:service_port", 80);
	serv->max_procthreads = iniparser_getint(dict, "DAEMON:max_procthreads", 1);
	serv->sleep_usec = iniparser_getint(dict, "DAEMON:sleep_usec", 100);
	serv->heartbeat_interval = iniparser_getint(dict, "DAEMON:heartbeat_interval", 1000000);
	logfile = iniparser_getstr(dict, "DAEMON:logfile");
	if(logfile == NULL)
		logfile = LQFTP_LOG;
	serv->logfile = logfile;
	logfile = iniparser_getstr(dict, "DAEMON:evlogfile");
	serv->evlogfile = logfile;
	serv->max_connections = iniparser_getint(dict, 
            "DAEMON:max_connections", MAX_CONNECTIONS);
	serv->packet_type = PACKET_DELIMITER;
	serv->packet_delimiter = iniparser_getstr(dict, "DAEMON:packet_delimiter");
	p = s = serv->packet_delimiter;
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
	serv->packet_delimiter_length = strlen(serv->packet_delimiter);
	serv->buffer_size = iniparser_getint(dict, "DAEMON:buffer_size", BUF_SIZE);
	serv->cb_packet_reader = &cb_serv_packet_reader;
	serv->cb_packet_handler = &cb_serv_packet_handler;
	serv->cb_data_handler = &cb_serv_data_handler;
	serv->cb_oob_handler = &cb_serv_oob_handler;
	serv->cb_heartbeat_handler = &cb_serv_heartbeat_handler;
   
    taskfile = iniparser_getstr(dict, "DAEMON:taskfile");
    statusfile = iniparser_getstr(dict, "DAEMON:statusfile");
    if((tasktable = tasktable_init(taskfile, statusfile)) == NULL)
    {
        fprintf(stderr, "Initialize tasktable failed, %s\n", strerror(errno));
        return -1;
    }
	/* server */
	if((ret = sbase->add_service(sbase, serv)) != 0)
	{
		fprintf(stderr, "Initiailize service[%s] failed, %s\n", serv->name, strerror(errno));
		return ret;
	}
	if((ret = sbase->add_service(sbase, transport)) != 0)
	{
		fprintf(stderr, "Initiailize service[%s] failed, %s\n", transport->name, strerror(errno));
		return ret;
	}
	return 0;
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
