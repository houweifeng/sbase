#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/stat.h>
#include <sbase.h>
#include "iniparser.h"
#include "md5.h"
#include "basedef.h"
#include "logger.h"
#include "common.h"

#ifndef LQFTPD_DEF
#define LQFTPD_DEF
static char *cmdlist[] = {"TRUNCATE", "PUT", "DEL", "MD5SUM"};
#define CMD_NUM         4
#define CMD_MAX_LEN     8
static char *truncate_plist[] = {"size"};
static char *put_plist[] = {"offset", "size"};
static char *md5sum_plist[] = {"md5"};
#define TRUNCATE_PNUM   1
#define PUT_PNUM        2
#define DEL_PNUM        0
#define MD5SUM_PNUM     1
#define PNUM_MAX        2
#define RESP_OK                 "200 OK\r\n\r\n"
#define RESP_NOT_IMPLEMENT      "201 not implement\r\n\r\n"
#define RESP_BAD_REQ            "203 bad requestment\r\n\r\n"
#define RESP_SERVER_ERROR       "205 service error\r\n\r\n"
#define RESP_FILE_NOT_EXISTS    "207 file not exists\r\n\r\n"
#define RESP_INVALID_MD5        "209 invalid md5\r\n\r\n"
#define RESPONSE(conn, msg) conn->push_chunk((CONN *)conn, (void *)msg, strlen(msg));
typedef struct _kitem
{
    char *key;
    char *data;
}kitem;
#define SBASE_LOG       "/tmp/sbase_access_log"
#define LQFTPD_LOG      "/tmp/lqftpd_access_log"
#define LQFTPD_EVLOG      "/tmp/lqftpd_evbase_log"
#endif
LOGGER *lqftpd_logger = NULL;
char *document_root = NULL;
SBASE *sbase = NULL;
dictionary *dict = NULL;
SERVICE *lqftpd = NULL;
char *histlist = "/tmp/hist.list";
char *qlist = "/tmp/q.list";

int cb_packet_reader(CONN *conn, CB_DATA *buffer)
{
}

#define GET_CMDID(p, n, cmdid)                                                  \
{                                                                               \
    n = 0;                                                                      \
    while(n < CMD_NUM)                                                          \
    {                                                                           \
        if(strncasecmp(p, cmdlist[n], strlen(cmdlist[n])) == 0)                 \
        {                                                                       \
            cmdid = n;                                                          \
            break;                                                              \
        }                                                                       \
        ++n;                                                                    \
    }                                                                           \
}
#define GET_PROPERTY(p, end, n, plist)                                          \
{                                                                               \
    n = 0;                                                                      \
    while(p < end)                                                              \
    {                                                                           \
        while(p < end && *p == ' ') ++p;                                        \
        plist[n].key = p;                                                       \
        while(p < end && *p != ':') ++p;                                        \
        while(p < end && *p == ' ') ++p;                                        \
        ++p;                                                                    \
        plist[n].data = p;                                                      \
        while(p < end && *p != '\n') ++p;                                       \
		++p;																	\
        n++;                                                                    \
        if(*p == '\r' || *p == '\n')break;                                      \
    }                                                                           \
}

int pmkdir(char *path, int mode)
{
   char *p = NULL, fullpath[PATH_MAX_SIZE];
   int n = 0, ret = 0, level = -1;
   struct stat st;

   if(path)
   {
       strcpy(fullpath, path);    
       p = fullpath;
       while(*p != '\0')
       {
           if(*p == '/' ) 
           {       
               level++;
               while(*p != '\0' && *p == '/' && *(p+1) == '/')++p;
               if(level > 0)
               {       
                   *p = '\0'; 
                   memset(&st, 0, sizeof(struct stat)); 
                   ret = stat(fullpath, &st);
                   if(ret == 0 && !S_ISDIR(st.st_mode)) return -1;
                   if(ret != 0 && mkdir(fullpath, mode) != 0) return -1;
                   *p = '/';
               }       
           }       
           ++p;
       }
       return 0;
   }
   return -1;
}

//log completed file 
int log_task(char *file)
{
    int fd = -1;
    char *line_end = "\r\n";

    if(qlist == NULL || histlist == NULL) return -1;
    if(access(qlist, F_OK) != 0) pmkdir(qlist, 0755);
    if(access(histlist, F_OK) != 0) pmkdir(histlist, 0755);
    if((fd = open(qlist, O_CREAT|O_APPEND|O_RDWR, 0644)) > 0) 
    {
        write(fd, file, strlen(file));
        write(fd, line_end, strlen(line_end));
        close(fd);
    }
    if((fd = open(histlist, O_CREAT|O_APPEND|O_RDWR, 0644)) > 0) 
    {
        write(fd, file, strlen(file));
        write(fd, line_end, strlen(line_end));
        close(fd);
    }
    return 0;
}

//check file size and resize it 
int mod_file_size(char *file, unsigned long long size)
{
    struct stat st;
    int fd = -1;
    int ret = -1;
    
    if(access(file, F_OK) != 0)
    {
            if((fd = open(file, O_CREAT|O_WRONLY, 0644)) > 0)
            {
                ret = ftruncate(fd, size);
                close(fd);
            }
            else ret = -1;
    }
    else
    {
        if((ret = stat(file, &st)) == 0)
        {
            if(!S_ISREG(st.st_mode)) ret = -1;
            else if(st.st_size >= size) ret = 0;
            else
            {
                ret = truncate(file, size);
            }
        }
    }
    return ret;
}

//truncate file 
int truncate_file(char *file, unsigned long long size)
{
    int fd = -1;
    int ret = -1;

    if(access(file, F_OK) != 0)
    {
        if((fd = open(file, O_CREAT|O_RDONLY, 0644)) > 0) 
			close(fd);
		else
			fprintf(stderr, "open %s failed, %s\n", file, strerror(errno));
    }
	fprintf(stdout, "truncate %s size %llu\n", file, size);
    return truncate(file, size);
}

void cb_packet_handler(CONN *conn, CB_DATA *packet)
{
    char *p = NULL, *end = NULL, *np = NULL, path[PATH_MAX_SIZE], fullpath[PATH_MAX_SIZE];
    int i = 0, n = 0, cmdid = -1, is_path_ok = 0, nplist = 0, is_valid_offset = 0;
    kitem plist[PNUM_MAX];
    unsigned long long  offset = 0llu, size = 0llu;
    unsigned char md5[MD5_LEN];
    char md5sum[MD5SUM_SIZE + 1], pmd5sum[MD5SUM_SIZE + 1];
    int fd = -1;
    struct stat st;
    int is_md5sum = 0;

    if(conn)
    {
        p = (char *)packet->data;
        end = (char *)packet->data + packet->ndata;
        //cmd
        if(packet->ndata > CMD_MAX_LEN)
        {
            GET_CMDID(p, n, cmdid);
        }
        np = NULL;
        //path
        while(p < end && *p != '\r' && *p != '\n' && *p != ' ') ++p;
        while(p < end && *p != '\r' && *p != '\n' && *p == ' ') ++p;
        memset(path, '\0', PATH_MAX_SIZE);
        np = path;
        while(p < end && *p != ' ' && *p != '\r' && *p != '\n') 
        {
            if((np - path) >= PATH_MAX_SIZE) goto bad_req;
            *np++ = *p++;
        }
        while(p < end && *p != '\n')++p;
        ++p;
        //plist
        memset(&plist, 0, sizeof(kitem) * PNUM_MAX);
        GET_PROPERTY(p, end, nplist, plist);
        if((is_path_ok = strlen(path)) == 0) goto bad_req;
        urldecode(path);
        n = sprintf(fullpath, "%s%s", document_root, path);
        switch(cmdid)
        {
            case -1:
                goto not_implement;
                break;
            case CMD_TRUNCATE :
                goto op_truncate;
                break;
            case CMD_PUT :
                goto op_put;
                break;
            case CMD_DEL :
                goto op_del;
                break;
            case CMD_MD5SUM :
                goto op_md5sum;
                break;
            default:
                goto not_implement;
                break;
        }
bad_req:
        RESPONSE(conn, RESP_BAD_REQ);
        return ;

not_implement:
        RESPONSE(conn, RESP_NOT_IMPLEMENT);
        return ;

op_truncate:
        if(pmkdir(fullpath, 0755) != 0 )
        {
            ERROR_LOGGER(lqftpd_logger, "Pmkdir %s failed, %s", fullpath, strerror(errno));
            RESPONSE(conn, RESP_SERVER_ERROR);
            return ;
        }
        if(nplist == 0)
        {
                RESPONSE(conn, RESP_NOT_IMPLEMENT);
                return ;
        }    
        size = 0llu;
        for(i = 0; i < nplist; i++)
        {
            if(strncasecmp(plist[i].key, truncate_plist[0], 
                        strlen(truncate_plist[0])) == 0)
            {
                size = str2llu(plist[i].data); 
            }
        }
        if(size == 0llu)
        {
            RESPONSE(conn, RESP_BAD_REQ);
            return ;
        }

        if(truncate_file(fullpath, size) == 0)
        {
            DEBUG_LOGGER(lqftpd_logger, "truncate file %s size:%llu", fullpath, size);
            RESPONSE(conn, RESP_OK);
        }
        else
        {
            ERROR_LOGGER(lqftpd_logger, "Truncate %s failed, %s", fullpath, strerror(errno));
            RESPONSE(conn, RESP_SERVER_ERROR);
        }
    return;

op_put:
        if(nplist == 0)
        {
                RESPONSE(conn, RESP_NOT_IMPLEMENT);
                return ;
        }    
        offset = 0llu;
        size = 0llu;
        for(i = 0; i < nplist; i++)
        {
            if(strncasecmp(plist[i].key, put_plist[0], strlen(put_plist[0])) == 0)
            {
                is_valid_offset = 1;
                offset = str2llu(plist[i].data); 
            }
			else if(strncasecmp(plist[i].key, put_plist[1], strlen(put_plist[1])) == 0)
            {
                size = str2llu(plist[i].data); 
            }
			//fprintf(stdout, "%s %s", plist[i].key, plist[i].data);
        }
		//fprintf(stdout, "put %s %ld %ld\n", fullpath, offset, size);
        if(size == 0llu || is_valid_offset == 0llu)
        {
            RESPONSE(conn, RESP_BAD_REQ);
            return ;
        }
		//fprintf(stdout, "put %s %ld %ld\n", fullpath, offset, size);
        //check file size 
        if(pmkdir(fullpath, 0755) != 0 
                || mod_file_size(fullpath, offset + size) != 0)
        {
            ERROR_LOGGER(lqftpd_logger, "Truncate %s failed, %s", fullpath, strerror(errno));
            RESPONSE(conn, RESP_SERVER_ERROR);
        }
        else
        {
            conn->recv_file((CONN *)conn, fullpath, offset, size);
        }
		//fprintf(stdout, "put %s %ld %ld\n", fullpath, offset, size);
        return;
op_del:
        if(access(fullpath, F_OK) == 0 )
        {
            if(unlink(fullpath) == 0)
            {
                DEBUG_LOGGER(lqftpd_logger, "deleted File %s", fullpath);
                RESPONSE(conn, RESP_OK);
            }
            else 
            {
                DEBUG_LOGGER(lqftpd_logger, "delete File %s failed, %s", fullpath, strerror(errno));
                RESPONSE(conn, RESP_SERVER_ERROR);
            }
        }
        else
        {
            RESPONSE(conn, RESP_FILE_NOT_EXISTS);
        }
        return;

op_md5sum:
        if(nplist == 0)
        {
            ERROR_LOGGER(lqftpd_logger, "md5sum %s with bad request", fullpath); 
            RESPONSE(conn, RESP_NOT_IMPLEMENT);
            return ;
        }
        memset(pmd5sum, 0, MD5SUM_SIZE+1);
        for(i = 0; i < nplist; i++)
        {
            if(strncasecmp(plist[i].key, md5sum_plist[0], strlen(md5sum_plist[0])) == 0)
            {
                memcpy(pmd5sum, plist[i].data, MD5SUM_SIZE);
                is_md5sum = 1;
            }
        }
        if(is_md5sum == 0)
        {
            ERROR_LOGGER(lqftpd_logger, "md5sum %s with bad request", fullpath); 
            RESPONSE(conn, RESP_BAD_REQ);
            return ;
        }
        if(access(fullpath, F_OK) == 0 )
        {
            if(md5_file(fullpath, md5) == 0)
            {
                memset(md5sum, 0, MD5SUM_SIZE+1);
                p = md5sum;
                for(i = 0; i < MD5_LEN; i++)
                    p += sprintf(p, "%02x", md5[i]);
                if(strncasecmp(md5sum, pmd5sum, MD5SUM_SIZE) == 0)
                {
                    log_task(fullpath);
                    RESPONSE(conn, RESP_OK);
                }
                else
                {
                    ERROR_LOGGER(lqftpd_logger, "md5sum file[%s][%s] invalid pmd5sum[%s]", 
                            fullpath, md5sum, pmd5sum);
                    RESPONSE(conn, RESP_INVALID_MD5);
                }
            }
            else
            {
                ERROR_LOGGER(lqftpd_logger, "md5sum file %s failed, %s", fullpath, strerror(errno));
                RESPONSE(conn, RESP_SERVER_ERROR);
            }
        }
        else
        {
            ERROR_LOGGER(lqftpd_logger, "File %s is not exists", fullpath);
            RESPONSE(conn, RESP_FILE_NOT_EXISTS);
        }
        return;
    }
}

void cb_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        RESPONSE(conn, RESP_OK);
    }
}

void cb_file_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    if(conn)
    {
        RESPONSE(conn, RESP_OK);
    }
}

void cb_oob_handler(CONN *conn, CB_DATA *oob)
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
    sbase->setrlimit(sbase, "RLIMIT_NOFILE", RLIMIT_NOFILE, sbase->connections_limit);
	sbase->usec_sleep = iniparser_getint(dict, "SBASE:usec_sleep", SB_USEC_SLEEP);
	sbase->set_log(sbase, iniparser_getstr(dict, "SBASE:logfile"));
	sbase->set_evlog(sbase, iniparser_getstr(dict, "SBASE:evlogfile"));
	/* LQFTPD */
	if((service = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize service failed, %s", strerror(errno));
		_exit(-1);
	}
	service->family = iniparser_getint(dict, "LQFTPD:inet_family", AF_INET);
	service->sock_type = iniparser_getint(dict, "LQFTPD:socket_type", SOCK_STREAM);
	service->ip = iniparser_getstr(dict, "LQFTPD:service_ip");
	service->port = iniparser_getint(dict, "LQFTPD:service_port", 80);
	service->working_mode = iniparser_getint(dict, "LQFTPD:working_mode", WORKING_PROC);
	service->service_type = iniparser_getint(dict, "LQFTPD:service_type", C_SERVICE);
	service->service_name = iniparser_getstr(dict, "LQFTPD:service_name");
	service->nprocthreads = iniparser_getint(dict, "LQFTPD:nprocthreads", 1);
	service->ndaemons = iniparser_getint(dict, "LQFTPD:ndaemons", 0);
	service->set_log(service, iniparser_getstr(dict, "LQFTPD:logfile"));
    	service->session.packet_type = iniparser_getint(dict, "LQFTPD:packet_type",PACKET_DELIMITER);
	service->session.packet_delimiter = iniparser_getstr(dict, "LQFTPD:packet_delimiter");
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
	service->session.buffer_size = iniparser_getint(dict, "LQFTPD:buffer_size", SB_BUF_SIZE);
	service->session.packet_reader = &cb_packet_reader;
	service->session.packet_handler = &cb_packet_handler;
	service->session.data_handler = &cb_data_handler;
	service->session.file_handler = &cb_file_handler;
	service->session.oob_handler = &cb_oob_handler;
    if( (p = iniparser_getstr(dict, "LQFTPD:access_log")))
        LOGGER_INIT(lqftpd_logger);
    else
        LOGGER_INIT(LQFTPD_LOG);
    document_root = iniparser_getstr(dict, "LQFTPD:server_root");
    if( (p = iniparser_getstr(dict, "LQFTPD:histlist"))) histlist = p;
    if((p = iniparser_getstr(dict, "LQFTPD:qlist"))) qlist = p;
    return sbase->add_service(sbase, service);
}

static void cb_stop(int sig)
{
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
