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
static void *daemon_logger = NULL;
static long long global_timeout_times = 60000000;
static long long nheartbeat = 0;

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
int cb_transport_error_handler(CONN *conn)
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
int cb_transport_packet_reader(CONN *conn, CB_DATA *buffer)
{
}

int cb_transport_packet_handler(CONN *conn, CB_DATA *packet)
{        
    char *p = NULL, *end = NULL;
    int respid = -1, n = 0;
    int blockid = -1;
    int status = BLOCK_STATUS_ERROR;

    if(conn)
    {
        p = (char *)packet->data;
        end = (char *)packet->data + packet->ndata;
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
        return 0;
    }
    return -1;
}

int cb_transport_data_handler(CONN *conn, CB_DATA *packet, 
        CB_DATA *chunk, CB_DATA *cache)
{
    return -1;
}

int cb_transport_oob_handler(CONN *conn, CB_DATA *oob)
{
    return -1;
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
    int i = 0;

    if(serv && transport && tasktable)
    {
        //DEBUG_LOGGER(daemon_logger, "Heartbeat:%ll connections:%d", 
        //        nheartbeat++, transport->running_connections);
        while((c_conn = transport->getconn(transport)))
        {
            //break;
            //DEBUG_LOGGER(daemon_logger, "Got connection[%08x][%d][%d]", 
            //            c_conn, c_conn->fd, c_conn->index);
            if((block = tasktable->pop_block(tasktable, c_conn->fd, (void *)c_conn)))
            {
                //DEBUG_LOGGER(daemon_logger, "running_task_id:%d running_task.id:%d",
                 //tasktable->running_task_id, tasktable->running_task.id);
                c_conn->set_timeout(c_conn, global_timeout_times);
                taskid = tasktable->running_task_id;
                task = &(tasktable->running_task);
                //transaction confirm
                c_conn->c_id = block->id;
                if(block->cmdid == CMD_TRUNCATE)
                {
                    DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_TRNCATE %s", 
                            block->id, task->destfile);
                    n = sprintf(buf, "truncate %s\r\nsize:%lld\r\n\r\n",
                            task->destfile, block->size); 
                    DEBUG_LOGGER(daemon_logger, "truncate %s offset:%lld size:%lld on %d",
                            task->destfile, block->offset, block->size, c_conn->fd); 
                    c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                }
                if(block->cmdid == CMD_PUT)
                {
                    DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_PUT", block->id);
                    n = sprintf(buf, "put %s\r\noffset:%lld\r\nsize:%lld\r\n\r\n",
                            task->destfile, block->offset, block->size); 
                    DEBUG_LOGGER(daemon_logger, "put %s offset:%lld size:%lld on %d",
                            task->destfile, block->offset, block->size, c_conn->fd); 
                    c_conn->push_chunk((CONN *)c_conn, (void *)buf, n);
                    c_conn->push_file((CONN *)c_conn, task->file, 
                            block->offset, block->size);
                }
                if(block->cmdid == CMD_MD5SUM)
                {
                    tasktable->md5sum(tasktable);
                    DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_MD5", block->id);
                    n = sprintf(buf, "md5sum %s\r\nmd5:%s\r\n\r\n", 
                            task->destfile,
                            task->md5); 
                    //DEBUG_LOGGER(daemon_logger, "Got block[%d] CMD_MD5", block->id);
                    DEBUG_LOGGER(daemon_logger, "md5sum %s[%s] on %d",
                            task->destfile, task->md5, c_conn->fd);
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
        /*
        if((n = tasktable->check_timeout(tasktable, global_timeout_times)) > 0)
        {
            if(tasktable->status && tasktable->nblock > 0)
            {
                for(i = 0; i < tasktable->nblock; i++)
                {
                    if(tasktable->status[i].status == BLOCK_STATUS_TIMEOUT 
                            && (c_conn = (CONN *)tasktable->status[i].arg)
                            && (c_conn->fd == tasktable->status[i].sid))
                    {
                        c_conn->close(c_conn);
                    }
                }
            }
            DEBUG_LOGGER(daemon_logger, "%d block is TIMEOUT on task:%d",
                    n, tasktable->running_task_id);
        }*/
        //if task is running then log TIMEOUT
    }
    return  ;
}

int cb_serv_packet_reader(CONN *conn, CB_DATA *buffer)
{
}

int cb_serv_packet_handler(CONN *conn, CB_DATA *packet)
{
    char *p = NULL, *end = NULL, *np = NULL;
    char file[PATH_MAX_SIZE], destfile[PATH_MAX_SIZE], buf[SBUF_SIZE];
    int cmdid = -1;
    int i = 0, n = 0;
    int taskid = -1;
    struct stat st;
    TASK task ;
    double speed = 0.0;

    if(conn)
    {
        p = (char *)packet->data;
        end = (char *)packet->data + packet->ndata;
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
            while(p < end && *p != ' ' && *p != '\r' && *p != '\n')
            {
                if((np - file) >= PATH_MAX_SIZE) goto bad_req_err;
                *np++ = *p++;
            }
            while(p < end && *p == ' ')++p;
            *np = '\0';
            n = np - file;
            np = destfile;
            while(p < end && *p != ' ' && *p != '\r' && *p != '\n')
            {
                if((np - destfile) >= PATH_MAX_SIZE) goto bad_req_err;
                *np++ = *p++;
            }
            *np = '\0';

            if(n > 0 && (np - destfile) > 0) 
            {
                urldecode(file);
                urldecode(destfile);
                if(stat(file, &st) != 0 || S_ISDIR(st.st_mode))
                {
                    goto bad_req_err;
                }
                memset(buf, 0, SBUF_SIZE);
                p = buf;
                np = destfile;
                while(*np != '\0') *p++ = *np++;
                if(*p == '/')
                {
                    end = NULL;
                    np = file; 
                    while(*np != '\0')
                    {
                        if(*np == '/') end = np++;
                        else ++np;
                    }
                    while(end && *end != '\0') *p++ = *end++;
                }
                memset(destfile, 0, PATH_MAX_SIZE);
                DEBUG_LOGGER(daemon_logger, "Adding [put %s %s] tasktable", file, buf); 
                urlencode((unsigned char *)buf, (unsigned char *)destfile);
                if((taskid = tasktable->add(tasktable, file, destfile)) >= 0)
                {
                    n = sprintf(buf, "%d OK\r\ntaskid:%ld\r\n\r\n", RESP_OK_CODE, taskid);
                    //fprintf(stdout, "conn:%08x %08x\n", conn, conn->push_chunk);
                    conn->push_chunk(conn, (void *)buf, n);
                    return 0;
                }
                //fprintf(stdout, "stat %s failed, %s\n", file, strerror(errno));
            }
        }
        if(cmdid == OP_STATUS)
        {
            while(p < end && *p != ' ' )++p;
            while(p < end && *p == ' ')++p;
            taskid = atoi(p);
            if(taskid >= 0 && taskid < tasktable->ntask 
                    && tasktable->statusout(tasktable, taskid, &task) == 0)
            {
                if(task.times >= 0)
                    speed = (double )(task.bytes/task.times);
                n = sprintf(buf, "%d OK\r\nTASKID:%d\r\nSTATUS:%d\r\n SPEED:%8f (Bytes/S)"
                                 "TIMEOUT:%d(times)\r\nRETRY:%d\r\n\r\n", 
                                 RESP_OK_CODE, taskid, task.status, speed,
                                 task.timeout, task.nretry);
                conn->push_chunk((CONN *)conn, (void *)buf, n);
                return 0;
            }
            else
            {
                n = sprintf(buf, "%d invalid taskid %d\r\n\r\n", RESP_INVALID_TASK_CODE, taskid);
                conn->push_chunk((CONN *)conn, (void *)buf, n);
                return 0;
            }
        }
bad_req_err:
        conn->push_chunk((CONN *)conn, (void *)RESP_BAD_REQ, strlen(RESP_BAD_REQ));
    }
    return -1;
}

int cb_serv_data_handler(CONN *conn, CB_DATA *packet, 
        CB_DATA *cache, CB_DATA *chunk)
{
}

int cb_serv_file_handler(CONN *conn, CB_DATA *packet, 
        CB_DATA *cache, CB_DATA *chunk)
{
}

int cb_serv_oob_handler(CONN *conn, CB_DATA *oob)
{
}

/* Initialize from ini file */
int sbase_initialize(SBASE *sbase, char *conf)
{
	char *logfile = NULL, *s = NULL, *p = NULL, 
         *tasklog = NULL, *taskfile = NULL, *statusfile = NULL;
	int n = 0, heartbeat_interval = 0, ret = 0, block_size = 0;

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

	/* TRANSPORT */
	fprintf(stdout, "Parsing LQFTP...\n");
	if((transport = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize transport failed, %s", strerror(errno));
		_exit(-1);
    }
    transport->family = iniparser_getint(dict, "TRANSPORT:inet_family", AF_INET);
    transport->sock_type = iniparser_getint(dict, "TRANSPORT:socket_type", SOCK_STREAM);
    transport->ip = iniparser_getstr(dict, "TRANSPORT:service_ip");
    transport->port = iniparser_getint(dict, "TRANSPORT:service_port", 80);
    transport->working_mode = iniparser_getint(dict, "TRANSPORT:working_mode", WORKING_PROC);
    transport->service_type = iniparser_getint(dict, "TRANSPORT:service_type", C_SERVICE);
    transport->service_name = iniparser_getstr(dict, "TRANSPORT:service_name");
    transport->nprocthreads = iniparser_getint(dict, "TRANSPORT:nprocthreads", 1);
    transport->ndaemons = iniparser_getint(dict, "TRANSPORT:ndaemons", 0);
    transport->set_log(transport, iniparser_getstr(dict, "TRANSPORT:logfile"));
    transport->session.packet_type = iniparser_getint(dict, "TRANSPORT:packet_type",
            PACKET_DELIMITER);
    transport->session.packet_delimiter = iniparser_getstr(dict, "TRANSPORT:packet_delimiter");
    p = s = transport->session.packet_delimiter;
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
    transport->session.packet_delimiter_length = strlen(transport->session.packet_delimiter);
    transport->session.buffer_size = iniparser_getint(dict, "TRANSPORT:buffer_size", SB_BUF_SIZE);
    transport->session.packet_reader = &cb_transport_packet_reader;
    transport->session.packet_handler = &cb_transport_packet_handler;
    transport->session.data_handler = &cb_transport_data_handler;
    transport->session.oob_handler = &cb_transport_oob_handler;
    transport->client_connections_limit = iniparser_getint(dict, 
            "TRANSPORT:client_connections_limit", 8);
    heartbeat_interval = iniparser_getint(dict, "TRANSPORT:heartbeat_interval", 
            SB_HEARTBEAT_INTERVAL);
    transport->set_heartbeat(transport, heartbeat_interval, NULL, NULL);
    /* DAEMON */
    if((serv = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize serv failed, %s", strerror(errno));
		_exit(-1);
    }
    serv->family = iniparser_getint(dict, "DAEMON:inet_family", AF_INET);
    serv->sock_type = iniparser_getint(dict, "DAEMON:socket_type", SOCK_STREAM);
    serv->ip = iniparser_getstr(dict, "DAEMON:service_ip");
    serv->port = iniparser_getint(dict, "DAEMON:service_port", 80);
    serv->working_mode = iniparser_getint(dict, "DAEMON:working_mode", WORKING_PROC);
    serv->service_type = iniparser_getint(dict, "DAEMON:service_type", C_SERVICE);
    serv->service_name = iniparser_getstr(dict, "DAEMON:service_name");
    serv->nprocthreads = iniparser_getint(dict, "DAEMON:nprocthreads", 1);
    serv->ndaemons = iniparser_getint(dict, "DAEMON:ndaemons", 0);
    serv->set_log(serv, iniparser_getstr(dict, "DAEMON:logfile"));
    serv->session.packet_type = iniparser_getint(dict, "DAEMON:packet_type",PACKET_DELIMITER);
    serv->session.packet_delimiter = iniparser_getstr(dict, "DAEMON:packet_delimiter");
    p = s = serv->session.packet_delimiter;
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
    serv->session.packet_delimiter_length = strlen(serv->session.packet_delimiter);
    serv->session.buffer_size = iniparser_getint(dict, "DAEMON:buffer_size", SB_BUF_SIZE);
    serv->session.packet_reader = &cb_serv_packet_reader;
    serv->session.packet_handler = &cb_serv_packet_handler;
    serv->session.data_handler = &cb_serv_data_handler;
    serv->session.oob_handler = &cb_serv_oob_handler;
    heartbeat_interval = iniparser_getint(dict, "DAEMON:heartbeat_interval", SB_HEARTBEAT_INTERVAL);
    serv->set_heartbeat(serv, heartbeat_interval, &cb_serv_heartbeat_handler, NULL);
    /* server */
	if((ret = sbase->add_service(sbase, serv)) != 0)
	{
		fprintf(stderr, "Initiailize service[%s] failed, %s\n", 
                serv->service_name, strerror(errno));
		return ret;
	}
	if((ret = sbase->add_service(sbase, transport)) != 0)
	{
		fprintf(stderr, "Initiailize service[%s] failed, %s\n", 
                transport->service_name, strerror(errno));
		return ret;
	}
    //logger 
	LOGGER_INIT(daemon_logger, iniparser_getstr(dict, "DAEMON:access_log"));
    //task and block status file
	taskfile = iniparser_getstr(dict, "DAEMON:taskfile");
    statusfile = iniparser_getstr(dict, "DAEMON:statusfile");
    //timeout
    if((p = iniparser_getstr(dict, "DAEMON:timeout")))
    {
        global_timeout_times = atoll(p);
    }
    //tasklog
    tasklog = iniparser_getstr(dict, "DAEMON:tasklog");
    if((tasktable = tasktable_init(taskfile, statusfile, tasklog)) == NULL)
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

    setrlimiter("RLIMIT_NOFILE", RLIMIT_NOFILE, 10240);
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
    sbase->stop(sbase);
    sbase->clean(&sbase);

}
