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
#include "logger.h"
#include "common.h"

static char *cmdlist[] = {"put", "status"};
#define     OP_NUM          2
#define     OP_PUT          0
#define     OP_STATUS       1
#ifndef     SBUF_SIZE
#define     SBUF_SIZE      65536
#endif
/**** lqftpd response ID *****/
static int  resplist[] = {200, 201, 203, 205, 207, 209};
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
SERVICE *transport = NULL;
SERVICE *serv = NULL;
static dictionary *dict = NULL;
static TASKTABLE *tasktable = NULL;
static LOGGER *daemon_logger = NULL;
static unsigned long long global_timeout_times = 60000000llu;
//static unsigned long long nheartbeat = 0;

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
void cb_transport_error_handler(CONN *conn)
{
    int blockid = -1;
    if(conn)
    {
        blockid = conn->c_id;
		ERROR_LOGGER(daemon_logger, "action failed on %d blockid:%d", conn->fd, conn->c_id);
        tasktable->update_status(tasktable, blockid, BLOCK_STATUS_ERROR, conn->fd);
		ERROR_LOGGER(daemon_logger, "action failed on %d blockid:%d", conn->fd, conn->c_id);
        conn->over_cstate((CONN *)conn);
    }
}

//
int cb_transport_packet_reader(CONN *conn, BUFFER *buffer)
{
}

void cb_transport_packet_handler(CONN *conn, BUFFER *packet)
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
        conn->over_cstate((CONN *)conn);
        switch(respid)
        {
            case RESP_OK_ID:
                status = BLOCK_STATUS_OVER;
                break;
            default:
                status = BLOCK_STATUS_ERROR; 
				conn->close((CONN *)conn);
                break;
        }
        tasktable->update_status(tasktable, blockid, status, conn->fd);
		DEBUG_LOGGER(daemon_logger, "action over on %d blockid:%d status:%d", 
				conn->fd, conn->c_id, status);
    }
}

void cb_transport_data_handler(CONN *conn, BUFFER *packet, 
        CHUNK *chunk, BUFFER *cache)
{
}

void cb_transport_oob_handler(CONN *conn, BUFFER *oob)
{
}

void cb_serv_heartbeat_handler(void *arg)
{
    int taskid = 0;
    TASK *task = NULL;
    TBLOCK *block = NULL;
    struct stat st;
    CONN *c_conn = NULL;
    char buf[SBUF_SIZE];
    int n = 0;

    if(serv && transport && tasktable)
    {
        //DEBUG_LOGGER(daemon_logger, "Heartbeat:%llu", nheartbeat++);
        while((c_conn = transport->getconn(transport)))
        {
                //DEBUG_LOGGER(daemon_logger, "Got connection[%08x][%d][%d]", 
                 //       c_conn, c_conn->fd, c_conn->index);
            if((block = tasktable->pop_block(tasktable, c_conn->fd)))
            {
                //DEBUG_LOGGER(daemon_logger, "NEW BLOCK TASK");
                taskid = tasktable->running_task_id;
                task = tasktable->table[taskid];
                //transaction confirm
                c_conn->c_id = block->id;
                if(block->cmdid == CMD_TRUNCATE)
                {
                    DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_TRNCATE %s", 
                            block->id, task->destfile);
                    n = sprintf(buf, "truncate %s\r\nsize:%llu\r\n\r\n",
                            task->destfile, block->size); 
                    DEBUG_LOGGER(daemon_logger, "truncate %s offset:%llu size:%llu on %d",
                            task->destfile, block->offset, block->size, c_conn->fd); 
                    c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                }
                if(block->cmdid == CMD_PUT)
                {
                    DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_PUT", block->id);
                    n = sprintf(buf, "put %s\r\noffset:%llu\r\nsize:%llu\r\n\r\n",
                            task->destfile, block->offset, block->size); 
                    DEBUG_LOGGER(daemon_logger, "put %s offset:%llu size:%llu on %d",
                            task->destfile, block->offset, block->size, c_conn->fd); 
                    c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                    c_conn->push_file((CONN *)c_conn, tasktable->table[taskid]->file, 
                            block->offset, block->size);
                }
                if(block->cmdid == CMD_MD5SUM)
                {
                    tasktable->md5sum(tasktable, taskid);
                    //DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_MD5", block->id);
                    n = sprintf(buf, "md5sum %s\r\nmd5:%s\r\n\r\n", 
                            tasktable->table[taskid]->destfile,
                            tasktable->table[taskid]->md5); 
                    //DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_MD5", block->id);
                    DEBUG_LOGGER(daemon_logger, "md5sum %s[%s] on %d",
                            task->destfile, tasktable->table[taskid]->md5, c_conn->fd);
                    c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                }
            }
            else
            {
                //DEBUG_LOGGER(daemon_logger, "Over cstate on connection[%d] "
                 //       "with %d tasks running[%d]",
                 //       c_conn->fd, tasktable->ntask, tasktable->running_task_id);
                c_conn->over_cstate((CONN *)c_conn);
                break;
            }
        }
        if((n = tasktable->check_timeout(tasktable, global_timeout_times)) > 0)
        {
            DEBUG_LOGGER(daemon_logger, "%d block is TIMEOUT on task:%d",
                    n, tasktable->running_task_id);
        }
        //if task is running then log TIMEOUT
    }
    return  ;
}

int cb_serv_packet_reader(CONN *conn, BUFFER *buffer)
{
}

void cb_serv_packet_handler(CONN *conn, BUFFER *packet)
{
    char *p = NULL, *end = NULL, *np = NULL;
    char file[PATH_MAX_SIZE], destfile[PATH_MAX_SIZE], buf[SBUF_SIZE];
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
                urldecode(file);
                if((taskid = tasktable->add(tasktable, file, destfile)) >= 0)
                {
                    n = sprintf(buf, "%d OK\r\ntaskid:%ld\r\n\r\n", RESP_OK_CODE, taskid);
                    //fprintf(stdout, "conn:%08x %08x\n", conn, conn->push_chunk);
                    conn->push_chunk(conn, (void *)buf, n);
                    return;
                }
		//fprintf(stdout, "stat %s failed, %s\n", file, strerror(errno));
            }
        }
        if(cmdid == OP_STATUS)
        {
            while(p < end && *p != ' ' )++p;
            while(p < end && *p == ' ')++p;
            taskid = atoi(p);
            if(taskid >= 0 && taskid < tasktable->ntask)
            {
                n = sprintf(buf, "%d OK\r\ntaskid:%d\r\nstatus:%d\r\n"
                                 "timeout:%d(times)\r\nretry:%d\r\n\r\n", 
                                 RESP_OK_CODE, taskid, tasktable->table[taskid]->status,
                                 tasktable->table[taskid]->timeout, 
                                 tasktable->table[taskid]->nretry);
                conn->push_chunk((CONN *)conn, (void *)buf, n);
                return;
            }
            else
            {
                n = sprintf(buf, "%d invalid taskid %d\r\n\r\n", RESP_INVALID_TASK_CODE, taskid);
                conn->push_chunk((CONN *)conn, (void *)buf, n);
                return;
            }
        }
bad_req_err:
        conn->push_chunk((CONN *)conn, (void *)RESP_BAD_REQ, strlen(RESP_BAD_REQ));
    }
    return ;
}

void cb_serv_data_handler(CONN *conn, BUFFER *packet, 
        CHUNK *chunk, BUFFER *cache)
{
}

void cb_serv_oob_handler(CONN *conn, BUFFER *oob)
{
}

/* Initialize from ini file */
int sbase_initialize(SBASE *sbase, char *conf)
{
	char *logfile = NULL, *s = NULL, *p = NULL, *taskfile = NULL, *statusfile = NULL;
	int n = 0;
	int ret = 0;
    int block_size = 0;

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
	transport->heartbeat_interval = iniparser_getint(dict, "TRANSPORT:heartbeat_interval", 10000000);
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
	transport->buffer_size = iniparser_getint(dict, "TRANSPORT:buffer_size", SB_BUF_SIZE);
	transport->cb_packet_reader = &cb_transport_packet_reader;
	transport->cb_packet_handler = &cb_transport_packet_handler;
	transport->cb_data_handler = &cb_transport_data_handler;
	transport->cb_oob_handler = &cb_transport_oob_handler;
	transport->cb_error_handler = &cb_transport_error_handler;
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
	serv->buffer_size = iniparser_getint(dict, "DAEMON:buffer_size", SB_BUF_SIZE);
	serv->cb_packet_reader = &cb_serv_packet_reader;
	serv->cb_packet_handler = &cb_serv_packet_handler;
	serv->cb_data_handler = &cb_serv_data_handler;
	serv->cb_oob_handler = &cb_serv_oob_handler;
	serv->cb_heartbeat_handler = &cb_serv_heartbeat_handler;
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
    //task and block status file
	taskfile = iniparser_getstr(dict, "DAEMON:taskfile");
    statusfile = iniparser_getstr(dict, "DAEMON:statusfile");
    //timeout
    if((p = iniparser_getstr(dict, "DAEMON:timeout")))
    {
        global_timeout_times = str2llu(p);
    }
    if((tasktable = tasktable_init(taskfile, statusfile)) == NULL)
    {
        fprintf(stderr, "Initialize tasktable failed, %s\n", strerror(errno));
        return -1;
    }
    /* task block size declare in basedef.h */
    block_size = iniparser_getint(dict, "DAEMON:block_size", 0); 
    if(block_size > 0)
        tasktable->block_size = block_size;
    else
        tasktable->block_size = TBLOCK_SIZE;
	//logger 
	daemon_logger = logger_init(iniparser_getstr(dict, "DAEMON:access_log"));
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
