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
char *document_root = NULL;
SBASE *sbase = NULL;
dictionary *dict = NULL;
SERVICE *lqftpd = NULL;

/* Convert String to unsigned long long int */
unsigned long long str2llu(char *str)
{
    char *s = str;
    unsigned long long  llu  = 0ll;
    unsigned long long  n = 1ll;
    while( *s >= '0' && *s <= '9')s++;
    while(s > str)
    {
        llu += ((*--s) - 48) * n;
        n *= 10ll;
    }
    return llu;
}

int cb_packet_reader(const CONN *conn, const BUFFER *buffer)
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

void cb_packet_handler(const CONN *conn, const BUFFER *packet)
{
    char *p = NULL, *end = NULL, *np = NULL, path[PATH_MAX_SIZE], fullpath[PATH_MAX_SIZE];
    int i = 0, n = 0, cmdid = -1, is_path_ok = 0, nplist = 0, is_valid_offset = 0;
    kitem plist[PNUM_MAX];
    unsigned long long  offset = 0llu, size = 0llu;
    unsigned char md5[_MD5_N];
    char md5sum[MD5SUM_SIZE + 1], *pmd5sum = NULL;
    int fd = -1;
    struct stat st;

    if(conn)
    {
        p = (char *)packet->data;
        end = (char *)packet->end;
        //cmd
        if(packet->size > CMD_MAX_LEN)
        {
            GET_CMDID(p, n, cmdid);
        }
        np = NULL;
        //path
        while(p < end && *p != '\r' && *p != '\n' && *p != '/') ++p;
        memset(path, '\0', PATH_MAX_SIZE);
        if(*p == '/')
        {
            np = path;
            while(p < end && *p != ' ' && *p != '\r' && *p != '\n') *np++ = *p++;
        }
        while(p < end && *p != '\n')++p;
        ++p;
        //plist
        memset(&plist, 0, sizeof(kitem) * PNUM_MAX);
        GET_PROPERTY(p, end, nplist, plist);
        if((is_path_ok = strlen(path)) == 0) goto bad_req;
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
            RESPONSE(conn, RESP_OK);
        }
        else
        {
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
                RESPONSE(conn, RESP_OK);
            }
            else 
            {
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
            RESPONSE(conn, RESP_NOT_IMPLEMENT);
            return ;
        }
        pmd5sum = NULL;
        for(i = 0; i < nplist; i++)
        {
            if(strncasecmp(plist[i].key, md5sum_plist[0],
                        strlen(md5sum_plist[0])) == 0)
            {
                pmd5sum = plist[i].data;
            }
        }
        if(pmd5sum == NULL)
        {
            RESPONSE(conn, RESP_BAD_REQ);
            return ;
        }
        if(access(fullpath, F_OK) == 0 )
        {
            if(md5_file(fullpath, md5) == 0)
            {
                memset(md5sum, 0, MD5SUM_SIZE);
                p = md5sum;
                for(i = 0; i < _MD5_N; i++)
                    p += sprintf(p, "%02x", md5[i]);
                if(strncasecmp(md5sum, pmd5sum, (p - md5sum)) == 0)
                {
                    RESPONSE(conn, RESP_OK);
                }
                else
                {
                    RESPONSE(conn, RESP_INVALID_MD5);
                }
            }
            else
            {
                RESPONSE(conn, RESP_SERVER_ERROR);
            }
        }
        else
        {
            RESPONSE(conn, RESP_FILE_NOT_EXISTS);
        }
        return;
    }
}

void cb_data_handler(const CONN *conn, const BUFFER *packet, const CHUNK *chunk, const BUFFER *cache)
{
    if(conn)
    {
        RESPONSE(conn, RESP_OK);
    }
}

void cb_oob_handler(const CONN *conn, const BUFFER *oob)
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
	/* LQFTPD */
	fprintf(stdout, "Parsing LQFTPD...\n");
	if((service = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize service failed, %s", strerror(errno));
		_exit(-1);
	}
	/* INET protocol family */
	n = iniparser_getint(dict, "LQFTPD:inet_family", 0);
	/* INET protocol family */
	n = iniparser_getint(dict, "LQFTPD:inet_family", 0);
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
	n = iniparser_getint(dict, "LQFTPD:socket_type", 0);
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
	service->name = iniparser_getstr(dict, "LQFTPD:service_name");
	service->ip = iniparser_getstr(dict, "LQFTPD:service_ip");
	if(service->ip && service->ip[0] == 0 )
		service->ip = NULL;
	service->port = iniparser_getint(dict, "LQFTPD:service_port", 80);
	service->working_mode = iniparser_getint(dict, "LQFTPD:working_mode", WORKING_PROC);
	service->max_procthreads = iniparser_getint(dict, "LQFTPD:max_procthreads", 1);
	service->sleep_usec = iniparser_getint(dict, "LQFTPD:sleep_usec", 100);
	logfile = iniparser_getstr(dict, "LQFTPD:logfile");
	if(logfile == NULL)
		logfile = LQFTPD_LOG;
	service->logfile = logfile;
	logfile = iniparser_getstr(dict, "LQFTPD:evlogfile");
	service->evlogfile = logfile;
	document_root = iniparser_getstr(dict, "LQFTPD:server_root");
	service->max_connections = iniparser_getint(dict, "LQFTPD:max_connections", MAX_CONNECTIONS);
	service->packet_type = PACKET_DELIMITER;
	service->packet_delimiter = iniparser_getstr(dict, "LQFTPD:packet_delimiter");
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
	service->buffer_size = iniparser_getint(dict, "LQFTPD:buffer_size", BUF_SIZE);
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
