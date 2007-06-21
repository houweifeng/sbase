#include <signal.h>
#include <locale.h>
#include <time.h>
#include <stdint.h>
#include <sbase.h>
#include "lhttpd.h"
#include "mystring.h"
#include "iniparser.h"
#define SBASE_LOG	"/tmp/sbase_access_log"
#define LHTTPD_LOG	"/tmp/lhttpd_access_log"
/* global vars*/
SBASE *sbase = NULL;
dictionary *dict = NULL;
char  *default_version = "1.1";
char  *default_server_root = "/var/www/html";
char  *server_root = NULL;
char *html_head  = "<html><head><title>lhttpd 0.1 Index View </title>\n\
	<meta http-equiv='content-type' content='text/html; charset=UTF-8'>\n</head>\
	<body>\n<h2>SounOS lhttpd </h2>\n<hr noshade>\n<ul>";
char *html_tail  = "<hr noshade><em>\n\
	<a href='http://libsbase.googlecode.com/svn/trunk/0.1/'>lhttpd 0.1</a>\n\
	Powered By<a href='http://code.google.com/p/libsbase/'>\
	SounOS::sbase</a></body></html>\n";
char *html_dir_format  = "<li><a href='%s/%s'>%s/</a></li>\n";
char *html_index_title = "<table width=100% bgcolor=#c0c0c0 ><tr><td align=left width=500 ><B>File Name</B></td><td width=200 align=left ><B>File Size</B></td></tr></table>\n";
char *html_file_format = "<table width=100% ><tr><td align=left width=500 ><li><a href='%s/%s'>%s</a></li></td><td width=200 align=left>%llu</td></tr></table>\n";
char *html_link_format  = "<li><a href='%s'><b>%s</b></a></li>\n";

/* HTTP header initialize */
HTTP_HEADER *http_header()
{
	HTTP_HEADER *http_header = (HTTP_HEADER *)calloc(1, sizeof(HTTP_HEADER));
	if(http_header != NULL )
	{
		http_header->buf	= (char *)calloc(1, HTTP_HEADER_MAX);
		http_header->add 	= http_header_add;
		http_header->send 	= http_header_send;
		http_header->clear 	= http_header_clear;
	}
	return http_header;
}

/* HTTP header add */
void http_header_add(HTTP_HEADER *http_header, const char *format, ...)
{
	va_list ap;
	int n = 0;
	if(http_header && http_header->buf && http_header->len < HTTP_HEADER_MAX)	
	{
		va_start(ap, format);
		n = vsprintf((http_header->buf + http_header->len), format,  ap);	
		if(n > 0 ) http_header->len += n;
		va_end(ap);
	}
}

/* HTTP header send */
void http_header_send(HTTP_HEADER *http_header, const CONN *conn)
{
	if(http_header && http_header->buf && http_header->len > 0 )
	{
		//printf("header:%s\n", http_header->buf);
		if(conn)conn->push_chunk((CONN *)conn, (void *)(http_header->buf), http_header->len * 1llu);
	}	
}

/* HTTP header clear */
void http_header_clear(HTTP_HEADER *http_header)
{
	if(http_header)	
	{
		if(http_header->buf)free(http_header->buf);
		free(http_header);
	}
}


/* send General Headers withou relation of File  */
#define HTTP_GENERAL_RESP(_resp_header){\
        time_t _timep;\
        struct tm *_p;\
        time(&_timep);\
        _p = gmtime(&_timep);\
        _resp_header->add(_resp_header, "Date: %s, %d %s %d %02d:%02d:%02d GMT\r\n",\
                wdays[_p->tm_wday], _p->tm_mday, ymonths[_p->tm_mon],\
                1900+_p->tm_year, _p->tm_hour, _p->tm_min, _p->tm_sec);\
        _resp_header->add(_resp_header, "%s Binary\r\n", http_headers[HEAD_GEN_TRANSFER_ENCODING].e);\
        _resp_header->add(_resp_header, "%s Close \r\n", http_headers[HEAD_GEN_CONNECTION].e);\
}

/* SEND HTTP ERROR MESSAGE */
#define HTTP_ERROR_RESP(_conn, _req, _resp)\
{\
	char *_version = _req->version;\
	HTTP_HEADER *_resp_header = NULL;\
	if(_req->version == NULL)_version = default_version;\
	if((_resp_header = http_header()) != NULL )\
	{\
		_resp_header->add(_resp_header, "HTTP/%s %s %s\r\n", \
	 		_req->version, response_status[_resp].e,response_status[_resp].s);\
		HTTP_GENERAL_RESP(_resp_header);\
        	_resp_header->add(_resp_header, "%s text/html\r\n", \
			http_headers[HEAD_ENT_CONTENT_TYPE].e);\
		_resp_header->add(_resp_header, "%s %lu\r\n",\
			http_headers[HEAD_ENT_CONTENT_LENGTH].e ,\
			strlen(response_status[_resp].s));\
		_resp_header->add(_resp_header, "\r\n");\
		_resp_header->add(_resp_header, "<CENTER><B ><font size=36 >%s</B></CENTER>",\
			 response_status[_resp].s);\
		_resp_header->send(_resp_header, _conn);\
		_resp_header->clear(_resp_header);\
	}\
}

time_t convert2time(char *time_str)
{
}

/* Convert String to unsigned long long int */
uint64_t str2llu(char *str)
{
        char *s = str;
        uint64_t llu  = 0ll;
        uint64_t n = 1ll;
        while( *s >= '0' && *s <= '9'){s++;}
        while(s > str)
        {
                llu += ((*--s) - 48) * n;
                n *= 10ll;
        }
        return llu;
}

/* converts hex char (0-9, A-Z, a-z) to decimal */
char hex2int(unsigned char hex)
{
        hex = hex - '0';
        if (hex > 9) {
                hex = (hex + '0' - 1) | 0x20;
                hex = hex - 'a' + 11;
        }
        if (hex > 15)
                hex = 0xFF;

        return hex;
}


/* http URL decode */
int http_urldecode(char *url)
{
	unsigned char high, low;
	char *src = url;
	char *dst = url;
	int query = 0;
	while (*src != '\0')
	{
		if(*src == '?')
		{
			query = 1;	
		}
		if (*src == '+' && query)
		{
			*dst = 0x20;
		} 
		else if (*src == '%') 
		{
			*dst = '%';
			high = hex2int(*(src + 1));
			if (high != 0xFF) 
			{
				low = hex2int(*(src + 2));
				if (low != 0xFF) 
				{
					high = (high << 4) | low;
					if (high < 32 || high == 127) high = '_';
					*dst = high;
					src += 2;
				}
			}
		} 
		else 
		{
			*dst = *src;
		}
		dst++;
		src++;
	}
        *dst = '\0';
}

/* HTTP COMMAND LINE parser */
int http_command_line_parser(HTTP_REQ *req, char *req_line, char **ptr)
{
	assert(req);
	assert(req_line);
	char *s = req_line;
	char *s1 = NULL;
	int i = 0 ;
	req->method = -1;	
	/* ltrim */
	while(*s == 0x20)s++;
	s1 = s;
	while(*s != 0x20 && *s != '\0')s++;
	*s++ = '\0';
	while(i < HTTP_METHOD_NUM )
	{
		if(strncasecmp(s1, http_methods[i].e, http_methods[i].elen) == 0 )
		{
			req->method = i;
			break;
		}
		i++;
	}
	if(req->method == -1)
	{
		ERROR_LOG("Invalid request METHOD[%s]\n", s1);
		return -1;
	}
	/* ltrim */
	while(*s == 0x20)s++;
	if(strncasecmp(s, "http://", 7) == 0 )
	{
		s += 7;
		s1 = req->host ;
		while(*s != '\0' && *s != '/') *s1++ = *s++;
		*s1++ = '\0';
	}
	s1 = req->url;
	while(*s != 0x20 && *s != '\0')
	{
		/* remove the redundant slash */
		if(*s == '/' && *(s+1) == '/')s++;
		else *s1++ = *s++;
	}
	*s1 = '\0';
	http_urldecode(req->url);
	if(*(req->url) == '\0' )
	{
		ERROR_LOG("NULL request PATH\n");
		return -1;	
	}
	while(*s != '\0' && *s++ != '/');
	s1 = req->version;
	while(*s != '\0' && *s != 0x20) *s1++ = *s++;
	*s1 = '\0';
	/* string end */
	*ptr = s;
	/* get PATH  */
	s = req->url;
	s1 = req->path;
        while(*s != '\0' && *s != '?') *s1++ = *s++;
	*s1 = '\0';
	if(*s == '?')s++;
	/* get QUERY string */
	s1 = req->query;
	while(*s != '\0') *s1++ = *s++;
	*s1 = '\0';
	return 0;
}

/* HTTP HEADER parser */
int http_header_parser(HTTP_REQ *req, char *req_line, char *end)
{
        char *s = req_line;
        int i  = 0;

        while(s < end)
        {
                /* ltrim */
                if(*s == 0x20){s++;continue;}
                if(*s == '\0'){s++;continue;}
                for(i = 0; i < HTTP_HEADER_NUM; i++)
                {
                        if( (end - s) >= http_headers[i].elen
                         && strncasecmp(s, http_headers[i].e, http_headers[i].elen) == 0)
                        {
                                s +=  http_headers[i].elen;
                                while(*s == 0x20 && s < end )s++;
                                req->headers[i] = s;
                                break;
                        }
			//printf("%d[%d]:%s\n\n", i, HTTP_HEADER_NUM, s);
                }
                while(*s != '\0' && s < end )s++;
        }
        return 0;
}

/* http REQUEST Virtual Host handler */
int http_host_handler(HTTP_REQ *req)
{
	char *s = NULL;
	char *s1 = NULL;
	/* check Virtual Host */
        if(*(req->host) == '\0' && req->port == 0 )
	{
		if(req->headers[HEAD_REQ_HOST] != NULL)
		{
			s = req->headers[HEAD_REQ_HOST];
			s1 = req->host;
			while(*s != '\0' && *s != ':') *s1++ = *s++;
			if(*s++ == ':')req->port = atoi(s);
		}
        }
	req->server_root = default_server_root;
	/* find Server Root */
	return 0;
}

/* http REQUEST PATH handler */
void http_path_handler(HTTP_REQ *req)
{
	struct stat st;
        char *s = req->abspath;
        char *s1 = req->server_root;
	char *s2 = req->path;
	int i = 0;
	
	/* copy server root */	
	while(*s1 != '\0') *s++ = *s1++;
	/* auto complete slash */
	if(*(s-1) != '/' && *s2 != '/') *s++ = '/';
	/* copy src path */
	while(*s2 != '\0') *s++ = *s2++;
        *s = '\0';
        /* if directory check index.* */
        if(stat(req->abspath, &st) == 0 && S_ISDIR(st.st_mode))
	{
		for(; i< INDEX_HOME_NUM; i++){
			s1 = s;
			s2 = index_exts[i];
			while(*s2 != '\0' ) *s1++ = *s2++;
			*s1 = '\0';
			if(access(req->abspath, F_OK) == 0)
			{
				DEBUG_LOG("Located homepage :%s\n", req->abspath);
				break;
			}
			else
			{
				*s = '\0';
			}
		}	
	}
        DEBUG_LOG("Got URL full abspath:%s\n", req->abspath);
	return ;
}

/* HTTP Dir Index view */
void http_index_dir(const CONN *conn, HTTP_REQ *req)
{
	struct _BUFFER *html = NULL;
	char 	buf[BUFFER_SIZE];
	char 	*path = req->abspath;
	struct 	dirent *file = NULL;
	int 	n = 0;
	char 	*format = NULL;
	DIR 	*dp = NULL;
	char 	*curdir = req->path;
	int 	ret = -1;
	struct	stat st;

	HTTP_HEADER *resp_header = NULL;
	if(*curdir == '/' && *(curdir+1) == '\0') curdir++;
	//DEBUG_LOG("path:%s\ncurdir:%s\n", path, curdir);
	dp = opendir(path);
	if(dp != NULL && (html = buffer_init()) != NULL)
	{
		/* HTML head */
		//n = sprintf(buf, "%s", html_head);
		html->push(html, (void *)html_head, strlen(html_head));
		html->push(html, (void *)html_index_title, strlen(html_index_title));
		if(*curdir != '\0')
		{
			n = sprintf(buf, html_link_format, "../", "../");
			html->push(html, buf, n);
		}
		while((file = readdir(dp)) != NULL)
		{
			if(file->d_name[0] == '.') continue;
			format = (file->d_type == DT_DIR) ? html_dir_format : html_file_format;
			if(file->d_type != DT_DIR)
			{
				sprintf(buf, "%s/%s", path, file->d_name);
				stat(buf, &st);		
				n = sprintf(buf, format, curdir, file->d_name, file->d_name, st.st_size);
			}
			else
			{
				n = sprintf(buf, format, curdir, file->d_name, file->d_name);
			}
			html->push(html, buf, n);
		}
		/* HTML tail */
		n = sprintf(buf, "%s\n", html_tail); 
		html->push(html, buf, n);
	}
	else
	{
		HTTP_ERROR_RESP(conn, req, RESP_INTERNALSERVERERROR);
	}
	finish :
	{	
		if(dp)closedir(dp);
		if(html && html->size > 0 )
		{
			if((resp_header = http_header()) != NULL )
			{
				resp_header->add(resp_header, "HTTP/%s %s %s\r\n", req->version,
                			response_status[RESP_OK].e, response_status[RESP_OK].s);
				HTTP_GENERAL_RESP(resp_header);
        			resp_header->add(resp_header,"%s text/html\r\n",
					http_headers[HEAD_ENT_CONTENT_TYPE].e  );
        			resp_header->add(resp_header,"%s %llu\r\n\r\n",
					http_headers[HEAD_ENT_CONTENT_LENGTH].e, html->size * 1llu);
				resp_header->send(resp_header, conn);
				resp_header->clear(resp_header);
				/* push chunk */
				conn->push_chunk((CONN *)conn, (void*)html->data, html->size * 1llu);
			}
			/* clear BUFFER data */
			html->clean(&html);
		}
	}
	return ;
}

/* get file type */
int http_file_type(HTTP_REQ *req)
{
	char *s = req->path;
	char *p = NULL;
	int  i = 0;
	while(*s != '\0')s++;
	s++;
	//while(*s != '.' && s >= req->path)s--;
	//if(s >= req->path)
	//{
	for(i = 0; i< HTTP_EXT_NUM; i++) 
	{
		p = s - http_exts[i].elen;
		if(p > req->path 
				&& strncasecmp(p, http_exts[i].e, http_exts[i].elen) == 0 ) {
			DEBUG_LOG("FileType:%s of %s\n", http_exts[i].s, req->path);
			return i;
			break;
		}
	}
	//}
	return -1;
}

/* parser HTTP RANGE */
int http_send_range_file(const CONN *conn, HTTP_REQ *req, struct stat st)
{
	char *s = NULL;
	uint64_t l = 0llu;
	uint64_t len = 0llu;
	uint64_t offset = 0llu;
	uint64_t offto = 0llu;
	int file_ext = -1;
	HTTP_HEADER *resp_header = NULL;
	struct tm *p;
	p = gmtime(&(st.st_mtime));

	s = req->headers[HEAD_REQ_RANGE];
	while(*s == 0x20)s++;
	if(strncasecmp(s, "bytes=", 6) == 0){s+=6;}
	else{goto invalid_range;}
	while(*s == 0x20)s++;
	if(*s == '-'){
		l = str2llu(++s);	
		if(l > st.st_size )goto invalid_range;
		offset = st.st_size - l;
		offto = st.st_size;
	}else if(*s >= '0' && *s <= '9'){
		offset = str2llu(s);		
		while(*++s != '-');
		++s;
		offto = str2llu(s);
		if(offto == 0llu){offto = st.st_size;}
	}else{goto invalid_range;}
	if(offto <= offset) {goto invalid_range;}
	len = (offto - offset) * 1llu;
	//printf("offset:%llu, offto:%llu, len:%llu\n", offset, offto, len);
	//return 0;

	if((resp_header = http_header()) != NULL)
	{
		resp_header->add(resp_header, "HTTP/%s %s %s\r\n", req->version,
			response_status[RESP_PARTIALCONTENT].e,
			response_status[RESP_PARTIALCONTENT].s);
		HTTP_GENERAL_RESP(resp_header);
		resp_header->add(resp_header, "%s bytes\r\n",
			 http_headers[HEAD_RESP_ACCEPT_RANGE].e);
		if((file_ext = http_file_type(req) ) != -1)
		{
			resp_header->add(resp_header, "%s %s\r\n",
			http_headers[HEAD_ENT_CONTENT_TYPE].e, http_exts[file_ext].s);
		}
		resp_header->add(resp_header, "%s %llu-%llu/%llu\r\n",
			http_headers[HEAD_ENT_CONTENT_RANGE].e, offset * 1llu,
			(offto - 1) * 1llu, offto * 1llu);
		resp_header->add(resp_header, "%s %llu\r\n",
			 http_headers[HEAD_ENT_CONTENT_LENGTH].e, len);
		resp_header->add(resp_header, "%s %s, %d %s %d %02d:%02d:%02d GMT\r\n",
			http_headers[HEAD_ENT_LAST_MODIFIED].e,
			wdays[p->tm_wday], p->tm_mday, ymonths[p->tm_mon],
			1900+p->tm_year, p->tm_hour, p->tm_min, p->tm_sec);
		resp_header->add(resp_header, "\r\n");
		resp_header->send(resp_header, conn);
		resp_header->clear(resp_header);
		if(len > 0llu)
			conn->push_file((CONN *)conn, req->abspath, offset, len);
	}
	return 0;
	invalid_range:
	{
		HTTP_ERROR_RESP(conn, req, RESP_REQRANGENOTSAT);
	}
	return -1;
}

/* send file */
int http_send_file(const CONN *conn, HTTP_REQ *req, struct stat st)
{
	int file_ext = -1;
	HTTP_HEADER *resp_header = NULL;
	struct tm *p;

	p = gmtime(&(st.st_mtime));

	if((resp_header = http_header()) != NULL)
	{
		resp_header->add(resp_header, "HTTP/%s %s %s\r\n", req->version,
				response_status[RESP_OK].e,response_status[RESP_OK].s);
		HTTP_GENERAL_RESP(resp_header);
		resp_header->add(resp_header, "%s bytes\r\n",
				http_headers[HEAD_RESP_ACCEPT_RANGE].e);
		if((file_ext = http_file_type(req) ) != -1)
                {
                        resp_header->add(resp_header, "%s %s\r\n",
                        http_headers[HEAD_ENT_CONTENT_TYPE].e, http_exts[file_ext].s);
                }
		resp_header->add(resp_header, "%s %llu\r\n",
			http_headers[HEAD_ENT_CONTENT_LENGTH].e,  st.st_size * 1llu);
		resp_header->add(resp_header, "%s %s, %d %s %d %02d:%02d:%02d GMT\r\n",
			http_headers[HEAD_ENT_LAST_MODIFIED].e,
			wdays[p->tm_wday], p->tm_mday, ymonths[p->tm_mon],
			1900+p->tm_year, p->tm_hour, p->tm_min, p->tm_sec);
		resp_header->add(resp_header, "\r\n");
		resp_header->send(resp_header, conn);
		resp_header->clear(resp_header);
		if(st.st_size > 0llu)
			conn->push_file((CONN *)conn, req->abspath, 0, st.st_size);
	}
	return 0;
}


/* HTTP GET HANDLER */
void http_get_handler(const CONN *conn, HTTP_REQ *req)
{
	struct stat st;	
	time_t timep;
	/* check is exists */
	//DEBUG_LOG("check is exists\n");
	if(access(req->abspath, F_OK) != 0)
	{
		HTTP_ERROR_RESP(conn, req, RESP_NOTFOUND)
		goto close;
	}
	stat(req->abspath, &st);
	///DEBUG_LOG("check is index\n");
	/* if directory check index.* */
	if(S_ISDIR(st.st_mode))
	{
		http_index_dir(conn, req);
		goto close;
	}
	/* check is readable*/
	if(access(req->abspath, R_OK) != 0)
	{
		HTTP_ERROR_RESP(conn, req, RESP_FORBIDDEN);
		goto close;
	}
	/* HTTP cache */	
	if(req->headers[HEAD_REQ_IF_MODIFIED_SINCE] != NULL)
	{
				
		goto close;
	}
	/* SEND  Range  FILE */
	if(req->headers[HEAD_REQ_RANGE] != NULL )
	{
		http_send_range_file(conn, req, st);
	}
	/* SEND file */
	else
	{
		http_send_file(conn, req, st);
	}
	close :
	{
		return ;
	}
	return ;
}

/* HTTP PUT HANDLER */
void http_put_handler(const CONN *conn, HTTP_REQ *req)
{
	char tmp[] = "/tmp/temp-XXXXXX";
	if(conn) conn->recv_file((CONN *)conn, tmp, 0llu, 1024llu);	
				
}

/* handle packet */
void cb_packet_handler(const CONN *conn,  const struct _BUFFER *packet)
{
	HTTP_REQ req;
	char *buf = (char *)calloc(1, BUFFER_SIZE);
	char *s = (char *)packet->data;
	size_t packet_len = packet->size; 
	int len = 0;
	
	memset(&req, 0, sizeof(HTTP_REQ));
	N2NULL(s, packet_len, buf, len);
	//N2PRINT(buf, len);
        if(len <= 0) goto close;
	/* parse HTTP REQUEST COMMAND LINE  */
        if(http_command_line_parser(&req, buf, &s) != 0 )
        {
		HTTP_ERROR_RESP(conn, (&req), RESP_BADREQUEST);
		goto close;
        }
	/* check HTTP REQUEST VERSION */
	/* parse HTTP REQUEST HEADER */
        if(http_header_parser(&req, s, (buf+len)) != 0 )
        {
		HTTP_ERROR_RESP(conn, (&req), RESP_BADREQUEST);
		goto close;
        }
	/* check Virtual Host Handler */
	if(http_host_handler(&req) != 0 )
	{
		goto close;
	}
	/* request path handler */
	http_path_handler(&req);
	/* data handler */
	switch(req.method)
	{
		case HTTP_OPTIONS:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
			break;
		case HTTP_HEAD:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
                        break;
		case HTTP_TRACE:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
                        break;
		case HTTP_CONNECT:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
                        break;
		case HTTP_POST:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
                        break;
		case HTTP_GET:
			//DEBUG_LOG("OK\n");
			http_get_handler(conn, &req);
			goto close;
			break;
		case HTTP_PUT:
			http_put_handler(conn, &req);
			//HTTP_ERROR_RESP(handler, (&req), RESP_NOTIMPLEMENT);
                        break;
		case HTTP_DELETE:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
                        break;
		default:
			HTTP_ERROR_RESP(conn, (&req), RESP_NOTIMPLEMENT);
                        break;
	}
	close:
        {
		//printf("----PACKET----\n%s\n", (char*)packet);
		if(buf)free(buf);	
        }
	return ;
}

void cb_data_handler(const CONN *conn,  const struct  _BUFFER *packet, const struct _CHUNK *chunk, const struct _BUFFER *cache)
{
	
}

/* read packet from buffer */
int cb_packet_reader(const CONN *conn, const struct _BUFFER *buf)
{
	char *p   = (char *)(buf->data);
	char *end = (char *)(buf->end);
	int  ret  = 0;
	//((BUFFER *)buf)->push(buf, (void *)"\0", 1);
	//printf("%d:%s %ld\n", buf->size, buf->data, (char *)buf->end - (char *)buf->data);
	while(p < end){
		if((end - p) >= 4)
		{
			MEM_CMP(ret, p, "\r\n\r\n", 4);
			if(ret == 0){return (p + 4 - (char *)buf->data);}
		}
		p++;
	}
	return 0;
}

static void stop(int sig){
	switch (sig) {
		case SIGINT:
		case SIGTERM:
			ERROR_LOG("lhttpd server is interrupted by user.\n");
			if(sbase)sbase->stop(sbase);
			break;
		default:
			break;
	}
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
	/* LHTTPD */
	fprintf(stdout, "Parsing LHTTPD...\n");
	if((service = service_init()) == NULL)
	{
		fprintf(stderr, "Initialize service failed, %s", strerror(errno));	
		_exit(-1);
	}	
	/* INET protocol family */
	n = iniparser_getint(dict, "LHTTPD:inet_family", 0);
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
	n = iniparser_getint(dict, "LHTTPD:socket_type", 0);
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
	service->name = iniparser_getstr(dict, "LHTTPD:service_name");
	service->ip = iniparser_getstr(dict, "LHTTPD:service_ip");
	if(service->ip && service->ip[0] == 0 )
		service->ip = NULL;
	service->port = iniparser_getint(dict, "LHTTPD:service_port", 80);
	service->working_mode = iniparser_getint(dict, "LHTTPD:working_mode", WORKING_PROC);
	service->max_procthreads = iniparser_getint(dict, "LHTTPD:max_procthreads", 1);
	logfile = iniparser_getstr(dict, "LHTTPD:logfile");
	if(logfile == NULL)
		logfile = LHTTPD_LOG;
	service->logfile = logfile;
	service->max_connections = iniparser_getint(dict, "LHTTPD:max_connections", MAX_CONNECTIONS);
	service->packet_type = PACKET_DELIMITER;
	service->packet_delimiter = iniparser_getstr(dict, "LHTTPD:packet_delimiter");	
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
	service->buffer_size = iniparser_getint(dict, "LHTTPD:buffer_size", BUF_SIZE);
	service->cb_packet_reader = &cb_packet_reader;
	service->cb_packet_handler = &cb_packet_handler;
	service->cb_data_handler = &cb_data_handler;
	/* server */
	fprintf(stdout, "Parsing for server...\n");
	server_root = iniparser_getstr(dict, "LHTTPD:server_root");
	return sbase->add_service(sbase, service);
}
int main(int argc, char **argv){
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
	signal(SIGTERM, &stop);
	signal(SIGINT,  &stop);
	signal(SIGHUP,  &stop);
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
	DEBUG_LOG("Initializing fron configure file:%s\n", conf);
	/* Initialize sbase */
	if(sbase_initialize(sbase, conf) != 0 )
	{
		fprintf(stderr, "Initialize from configure file failed\n");
                return -1;
	}
	DEBUG_LOG("Initialized successed\n");
	sbase->running(sbase, 0);
}
