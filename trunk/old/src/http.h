#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#ifndef _HTTP_H
#define _HTTP_H
#define HTTP_IP_MAX 		    16
#define HTTP_VHOST_MAX     	    256
#define HTTP_INDEX_MAX     	    32
#define HTTP_INDEXES_MAX   	    1048576
#define HTTP_URL_PATH_MAX  	    1024
#define HTTP_PATH_MAX      	    1024
#define HTTP_BUF_SIZE      	    65536
#define HTTP_ARGV_LINE_MAX 	    4096
#define HTTP_HEAD_MAX 	        4096
#define HTTP_COOKIES_MAX   	    32
#define HTTP_ARGVS_MAX     	    1024
#define HTTP_BUFFER_SIZE   	    65536
#define HTTP_HEADER_MAX    	    65536
#define HTTP_BYTE_K        	    1024
#define HTTP_BYTE_M        	    1048576
#define HTTP_BYTE_G        	    1073741824
#define HTTP_MMAP_MAX           157810688
#define HTTP_ENCODING_DEFLATE  	0x01
#define HTTP_ENCODING_GZIP 	0x02
#define HTTP_ENCODING_BZIP2  	0x04
#define HTTP_ENCODING_COMPRESS  0x08
#define HTTP_ENCODING_NUM       4
static const char *http_encodings[] = {"deflate", "gzip", "bzip2", "compress"}; 
typedef struct _HTTP_VHOST
{
    char *name;
    char *home;
}HTTP_VHOST;
typedef struct _HTTP_ELEMENT
{
	int id;
	char *e;
	int  elen;
	char *s;
}HTTP_ELEMENT;

/* HTTP/1.1 RESPONSE STATUS FROM RFC2616
   Status-Code    =
   "100"  ; Section 10.1.1: Continue
   | "101"  ; Section 10.1.2: Switching Protocols
   | "200"  ; Section 10.2.1: OK
   | "201"  ; Section 10.2.2: Created
   | "202"  ; Section 10.2.3: Accepted
   | "203"  ; Section 10.2.4: Non-Authoritative Information
   | "204"  ; Section 10.2.5: No Content
   | "205"  ; Section 10.2.6: Reset Content
   | "206"  ; Section 10.2.7: Partial Content
   | "300"  ; Section 10.3.1: Multiple Choices
   | "301"  ; Section 10.3.2: Moved Permanently
   | "302"  ; Section 10.3.3: Found
   | "303"  ; Section 10.3.4: See Other
   | "304"  ; Section 10.3.5: Not Modified
   | "305"  ; Section 10.3.6: Use Proxy
   | "307"  ; Section 10.3.8: Temporary Redirect
   | "400"  ; Section 10.4.1: Bad Request
   | "401"  ; Section 10.4.2: Unauthorized
   | "402"  ; Section 10.4.3: Payment Required
   | "403"  ; Section 10.4.4: Forbidden
   | "404"  ; Section 10.4.5: Not Found
   | "405"  ; Section 10.4.6: Method Not Allowed
   | "406"  ; Section 10.4.7: Not Acceptable

   | "407"  ; Section 10.4.8: Proxy Authentication Required
   | "408"  ; Section 10.4.9: Request Time-out
   | "409"  ; Section 10.4.10: Conflict
   | "410"  ; Section 10.4.11: Gone
   | "411"  ; Section 10.4.12: Length Required
   | "412"  ; Section 10.4.13: Precondition Failed
   | "413"  ; Section 10.4.14: Request Entity Too Large
   | "414"  ; Section 10.4.15: Request-URI Too Large
   | "415"  ; Section 10.4.16: Unsupported Media Type
   | "416"  ; Section 10.4.17: Requested range not satisfiable
   | "417"  ; Section 10.4.18: Expectation Failed
   | "500"  ; Section 10.5.1: Internal Server Error
   | "501"  ; Section 10.5.2: Not Implemented
   | "502"  ; Section 10.5.3: Bad Gateway
   | "503"  ; Section 10.5.4: Service Unavailable
   | "504"  ; Section 10.5.5: Gateway Time-out
   | "505"  ; Section 10.5.6: HTTP Version not supported
   | extension-code

   extension-code = 3DIGIT
   Reason-Phrase  = *<TEXT, excluding CR, LF>
 */
#define HTTP_RESPONSE_NUM 40
static const HTTP_ELEMENT response_status[] = 
{
#define RESP_CONTINUE		0
	{0, "100", 3, "Continue"},
#define RESP_SWITCH		1
	{1, "101", 3, "Switching Protocols"},
#define RESP_OK			2
	{2, "200", 3, "OK"},
#define RESP_CREATED		3
	{3, "201", 3, "Created"},
#define RESP_ACCEPTED		4
	{4, "202", 3, "Accepted"},
#define RESP_NONAUTHINFO	5
	{5, "203", 3, "Non-Authoritative Information"},
#define RESP_NOCONTENT		6
	{6, "204", 3, "No Content"},
#define RESP_RESETCONTENT	7
	{7, "205", 3, "Reset Content"},
#define RESP_PARTIALCONTENT	8
	{8, "206", 3, "Partial Content"},
#define RESP_MULTCHOICES	9
	{9, "300", 3, "Multiple Choices"},
#define RESP_MOVEDPERMANENTLY	10
	{10, "301", 3, "Moved Permanently"},
#define RESP_FOUND		11
	{11, "302", 3, "Found"},
#define RESP_SEEOTHER		12
	{12, "303", 3, "See Other"},
#define RESP_NOTMODIFIED	13
	{13, "304", 3, "Not Modified"},
#define RESP_USEPROXY		14
	{14, "305", 3, "Use Proxy"},
#define RESP_TEMPREDIRECORDT	15
	{15, "307", 3, "Temporary Redirect"},
#define RESP_BADREQUEST		16
	{16, "400", 3, "Bad Request"},
#define RESP_UNAUTHORIZED	17
	{17, "401", 3, "Unauthorized"},
#define RESP_PAYMENTREQUIRED	18
	{18, "402", 3, "Payment Required"},
#define RESP_FORBIDDEN		19
	{19, "403", 3, "Forbidden"},
#define RESP_NOTFOUND		20
	{20, "404", 3, "Not Found"},
#define RESP_METHODNOTALLOWED	21
	{21, "405", 3, "Method Not Allowed"},
#define RESP_NOTACCEPTABLE	22
	{22, "406", 3, "Not Acceptable"},
#define RESP_PROXYAUTHREQ	23
	{23, "407", 3, "Proxy Authentication Required"},
#define RESP_REQUESTTIMEOUT	24
	{24, "408", 3, "Request Time-out"},
#define RESP_CONFLICT		25
	{25, "409", 3, "Conflict"},
#define RESP_GONE		26
	{26, "410", 3, "Gone"},
#define RESP_LENGTHREQUIRED	27
	{27, "411", 3, "Length Required"},
#define RESP_PREFAILED		28
	{28, "412", 3, "Precondition Failed"},
#define RESP_REQENTTOOLARGE	29
	{29, "413", 3, "Request Entity Too Large"},
#define RESP_REQURLTOOLARGE	30
	{30, "414", 3, "Request-URI Too Large"},
#define RESP_UNSUPPORTEDMEDIA	31
	{31, "415", 3, "Unsupported Media Type"},
#define RESP_REQRANGENOTSAT	32
	{32, "416", 3, "Requested range not satisfiable"},
#define RESP_EXPECTFAILED	33
	{33, "417", 3, "Expectation Failed"},
#define RESP_INTERNALSERVERERROR	34
	{34, "500", 3, "Internal Server Error"},
#define RESP_NOTIMPLEMENT	35
	{35, "501", 3, "Not Implemented"},
#define RESP_BADGATEWAY		36
	{36, "502", 3, "Bad Gateway"},
#define RESP_SERVICEUNAVAILABLE	37
	{37, "503", 3, "Service Unavailable"},
#define RESP_GATEWAYTIMEOUT	38
	{38, "504", 3, "Gateway Time-out"},
#define RESP_HTTPVERNOTSUPPORT	39
	{39, "505", 3, "HTTP Version not supported"}
};

/* HTTP/1.1 REQUEST HEADERS FROM RFC2616 
   request-header = 
   | Accept                   ; Section 14.1
   | Accept-Charset           ; Section 14.2
   | Accept-Encoding          ; Section 14.3
   | Accept-Language          ; Section 14.4
   | Authorization            ; Section 14.8
   | Expect                   ; Section 14.20
   | From                     ; Section 14.22
   | Host                     ; Section 14.23
   | If-Match                 ; Section 14.24

   | If-Modified-Since        ; Section 14.25
   | If-None-Match            ; Section 14.26
   | If-Range                 ; Section 14.27
   | If-Unmodified-Since      ; Section 14.28
   | Max-Forwards             ; Section 14.31
   | Proxy-Authorization      ; Section 14.34
   | Range                    ; Section 14.35
   | Referer                  ; Section 14.36
   | TE                       ; Section 14.39
   | User-Agent               ; Section 14.43

  HTTP/1.1  RESPONSE HEADERS FROM RFC2616 
   response-header = 
   | Accept-Ranges           ; Section 14.5
   | Age                     ; Section 14.6
   | ETag                    ; Section 14.19
   | Location                ; Section 14.30
   | Proxy-Authenticate      ; Section 14.33

   | Retry-After             ; Section 14.37
   | Server                  ; Section 14.38
   | Vary                    ; Section 14.44
   | WWW-Authenticate        ; Section 14.47

  HTTP/1.1 GENERAL HEADERS FROM RFC2616
   general-header = 
   | Cache-Control            ; Section 14.9
   | Connection               ; Section 14.10
   | Date                     ; Section 14.18
   | Pragma                   ; Section 14.32
   | Trailer                  ; Section 14.40
   | Transfer-Encoding        ; Section 14.41
   | Upgrade                  ; Section 14.42
   | Via                      ; Section 14.45
   | Warning                  ; Section 14.46

   HTTP/1.1 ENTITY  HEADERS FROM RFC2616
   entity-header  = 
   | Allow                    ; Section 14.7
   | Content-Encoding         ; Section 14.11
   | Content-Language         ; Section 14.12
   | Content-Length           ; Section 14.13
   | Content-Location         ; Section 14.14
   | Content-MD5              ; Section 14.15
   | Content-Range            ; Section 14.16
   | Content-Type             ; Section 14.17
   | Expires                  ; Section 14.21
   | Last-Modified            ; Section 14.29
   | extension-header
    
   Additional headers
   | Cookies 
   | Set-Cookie 
 */
static const HTTP_ELEMENT http_headers[] = 
{
#define HEAD_REQ_ACCEPT 0
	{0, "Accept:", 7, NULL},
#define HEAD_REQ_ACCEPT_CHARSET 1
	{1, "Accept-Charset:", 15, NULL},
#define HEAD_REQ_ACCEPT_ENCODING 2
	{2, "Accept-Encoding:", 16, NULL},
#define HEAD_REQ_ACCEPT_LANGUAGE 3
	{3, "Accept-Language:", 16, NULL},
#define HEAD_RESP_ACCEPT_RANGE 4
	{4, "Accept-Ranges:", 14, NULL},
#define HEAD_RESP_AGE 5
	{5, "Age:", 4, NULL},
#define HEAD_ENT_ALLOW 6
	{6, "Allow:", 6, NULL},
#define HEAD_REQ_AUTHORIZATION 7
	{7, "Authorization:", 14, NULL},
#define HEAD_GEN_CACHE_CONTROL 8
	{8, "Cache-Control:", 14, NULL},
#define HEAD_GEN_CONNECTION 9
	{9, "Connection:", 11, NULL},
#define HEAD_ENT_CONTENT_ENCODING 10
	{10, "Content-Encoding:", 17, NULL},
#define HEAD_ENT_CONTENT_LANGUAGE 11
	{11, "Content-Language:", 17, NULL},
#define HEAD_ENT_CONTENT_LENGTH 12
	{12, "Content-Length:", 15, NULL},
#define HEAD_ENT_CONTENT_LOCATION 13
	{13, "Content-Location:", 17, NULL},
#define HEAD_ENT_CONTENT_MD5 14
	{14, "Content-MD5:", 12, NULL},
#define HEAD_ENT_CONTENT_RANGE 15
	{15, "Content-Range:", 14, NULL},
#define HEAD_ENT_CONTENT_TYPE 16
	{16, "Content-Type:", 13, NULL},
#define HEAD_GEN_DATE 17
	{17, "Date:", 5, NULL},
#define HEAD_RESP_ETAG 18
	{18, "ETag:", 5, NULL},
#define HEAD_REQ_EXPECT 19
	{19, "Expect:", 7, NULL},
#define HEAD_ENT_EXPIRES 20
	{20, "Expires:", 8, NULL},
#define HEAD_REQ_FROM 21
	{21, "From:", 5, NULL},
#define HEAD_REQ_HOST 22 
	{22, "Host:", 5, NULL},
#define HEAD_REQ_IF_MATCH 23 
	{23, "If-Match:", 9, NULL},
#define HEAD_REQ_IF_MODIFIED_SINCE 24 
	{24, "If-Modified-Since:", 18, NULL},
#define HEAD_REQ_IF_NONE_MATCH 25
	{25, "If-None-Match:", 14, NULL},
#define HEAD_REQ_IF_RANGE 26 
	{26, "If-Range:", 9, NULL},
#define HEAD_REQ_IF_UNMODIFIED_SINCE 27 
	{27, "If-Unmodified-Since:", 20, NULL},
#define HEAD_ENT_LAST_MODIFIED 28
	{28, "Last-Modified:", 14, NULL},
#define HEAD_RESP_LOCATION 29 
	{29, "Location:", 9, NULL},
#define HEAD_REQ_MAX_FORWARDS 30 
	{30, "Max-Forwards:", 13, NULL},
#define HEAD_GEN_PRAGMA 31 
	{31, "Pragma:", 7, NULL},
#define HEAD_RESP_PROXY_AUTHENTICATE 32 
	{32, "Proxy-Authenticate:", 19, NULL},
#define HEAD_REQ_PROXY_AUTHORIZATION 33
	{33, "Proxy-Authorization:", 20, NULL},
#define HEAD_REQ_RANGE 34 
	{34, "Range:", 6, NULL},
#define HEAD_REQ_REFERER 35
	{35, "Referer:", 8, NULL},
#define HEAD_RESP_RETRY_AFTER 36
	{36, "Retry-After:", 12, NULL},
#define HEAD_RESP_SERVER 37
	{37, "Server:", 7, NULL},
#define HEAD_REQ_TE 38
	{38, "TE:", 3, NULL},
#define HEAD_GEN_TRAILER 39
	{39, "Trailer:", 8, NULL},
#define HEAD_GEN_TRANSFER_ENCODING 40 
	{40, "Transfer-Encoding:", 18, NULL},
#define HEAD_GEN_UPGRADE 41 
	{41, "Upgrade:", 8, NULL},
#define HEAD_REQ_USER_AGENT 42
	{42, "User-Agent:", 11, NULL},
#define HEAD_RESP_VARY 43 
	{43, "Vary:", 5, NULL},
#define HEAD_GEN_VIA 44 
	{44, "Via:", 4, NULL},
#define HEAD_GEN_WARNING 45
	{45, "Warning:", 8, NULL},
#define HEAD_RESP_WWW_AUTHENTICATE 46 
	{46, "WWW-Authenticate:", 17, NULL},
#define HEAD_REQ_COOKIE 47
    {47, "Cookie:", 7, NULL},
#define HEAD_RESP_SET_COOKIE 48
    {48, "Set-Cookie:", 11, NULL},
#define HEAD_GEN_USERID     49
    {49, "UserID:", 7, NULL},
#define HEAD_GEN_UUID     50
    {50, "UUID:", 5, NULL},
#define HEAD_GEN_RAW_LENGTH   51
    {51, "Raw-Length:", 11, NULL},
#define HEAD_GEN_TASK_TYPE   52
    {52, "Task-Type:", 10, NULL},
#define HEAD_GEN_DOWNLOAD_LENGTH   53
    {53, "Download-Length:", 16, NULL}
};
#define HTTP_HEADER_NUM	54

/* HTTP/1.1 METHODS
   Method         = 
   | "OPTIONS"                ; Section 9.2
   | "GET"                    ; Section 9.3
   | "HEAD"                   ; Section 9.4
   | "POST"                   ; Section 9.5
   | "PUT"                    ; Section 9.6
   | "DELETE"                 ; Section 9.7
   | "TRACE"                  ; Section 9.8
   | "CONNECT"                ; Section 9.9
   | extension-method
   extension-method = token
 */
#define HTTP_METHOD_NUM 11
static const HTTP_ELEMENT http_methods[] = 
{
#define HTTP_OPTIONS	0
	{0, "OPTIONS", 7, NULL},
#define HTTP_GET	1
	{1, "GET", 3, NULL},
#define HTTP_HEAD	2
	{2, "HEAD", 4, NULL},
#define HTTP_POST	3
	{3, "POST", 4, NULL},
#define HTTP_PUT	4
	{4, "PUT", 3, NULL},
#define HTTP_DELETE	5
	{5, "DELETE", 6, NULL},
#define HTTP_TRACE	6
	{6, "TRACE", 5, NULL},
#define HTTP_CONNECT	7
	{7, "CONNECT", 7, NULL},
#define HTTP_TASK   8
    {8, "TASK", 4, NULL},
#define HTTP_DOWNLOAD 9
    {9, "DOWNLOAD", 8, NULL},
#define HTTP_EXTRACT 10
    {9, "EXTRACT", 7, NULL}
};

/* file ext support list */
#define HTTP_MIME_NUM 88
static const HTTP_ELEMENT http_mime_types[]=
{
    {0, "html", 4, "text/html"},
    {1, "htm", 3, "text/html"},
    {2, "shtml", 5, "text/html"},
    {3, "css", 3, "text/css"},
    {4, "xml", 3, "text/xml"},
    {5, "gif", 3, "image/gif"},
    {6, "jpeg", 4, "image/jpeg"},
    {7, "jpg", 3, "image/jpeg"},
    {8, "js", 2, "application/x-javascript"},
    {9, "atom", 4, "application/atom+xml"},
    {10, "rss", 3, "application/rss+xml"},
    {11, "mml", 3, "text/mathml"},
    {12, "txt", 3, "text/plain"},
    {13, "jad", 3, "text/vnd.sun.j2me.app-descriptor"},
    {14, "wml", 3, "text/vnd.wap.wml"},
    {15, "htc", 3, "text/x-component"},
    {16, "png", 3, "image/png"},
    {17, "tif", 3, "image/tiff"},
    {18, "tiff", 4, "image/tiff"},
    {19, "wbmp", 4, "image/vnd.wap.wbmp"},
    {20, "ico", 3, "image/x-icon"},
    {21, "jng", 3, "image/x-jng"},
    {22, "bmp", 3, "image/x-ms-bmp"},
    {23, "svg", 3, "image/svg+xml"},
    {24, "jar", 3, "application/java-archive"},
    {25, "war", 3, "application/java-archive"},
    {26, "ear", 3, "application/java-archive"},
    {27, "hqx", 3, "application/mac-binhex40"},
    {28, "doc", 3, "application/msterm"},
    {29, "pdf", 3, "application/pdf"},
    {30, "ps", 2, "application/postscript"},
    {31, "eps", 3, "application/postscript"},
    {32, "ai", 2, "application/postscript"},
    {33, "rtf", 3, "application/rtf"},
    {34, "xls", 3, "application/vnd.ms-excel"},
    {35, "ppt", 3, "application/vnd.ms-powerpoint"},
    {36, "wmlc", 4, "application/vnd.wap.wmlc"},
    {37, "xhtml", 5, "application/vnd.wap.xhtml+xml"},
    {38, "kml", 3, "application/vnd.google-earth.kml+xml"},
    {39, "kmz", 3, "application/vnd.google-earth.kmz"},
    {40, "cco", 3, "application/x-cocoa"},
    {41, "jardiff", 7, "application/x-java-archive-diff"},
    {42, "jnlp", 4, "application/x-java-jnlp-file"},
    {43, "run", 3, "application/x-makeself"},
    {44, "pl", 2, "application/x-perl"},
    {45, "pm", 2, "application/x-perl"},
    {46, "prc", 3, "application/x-pilot"},
    {47, "pdb", 3, "application/x-pilot"},
    {48, "rar", 3, "application/x-rar-compressed"},
    {49, "rpm", 3, "application/x-redhat-package-manager"},
    {50, "sea", 3, "application/x-sea"},
    {51, "swf", 3, "application/x-shockwave-flash"},
    {52, "sit", 3, "application/x-stuffit"},
    {53, "tcl", 3, "application/x-tcl"},
    {54, "tk", 2, "application/x-tcl"},
    {55, "der", 3, "application/x-x509-ca-cert"},
    {56, "pem", 3, "application/x-x509-ca-cert"},
    {57, "crt", 3, "application/x-x509-ca-cert"},
    {58, "xpi", 3, "application/x-xpinstall"},
    {59, "zip", 3, "application/zip"},
    {60, "bin", 3, "application/octet-stream"},
    {61, "exe", 3, "application/octet-stream"},
    {62, "dll", 3, "application/octet-stream"},
    {63, "deb", 3, "application/octet-stream"},
    {64, "dmg", 3, "application/octet-stream"},
    {65, "eot", 3, "application/octet-stream"},
    {66, "img", 3, "application/octet-stream"},
    {67, "iso", 3, "application/octet-stream"},
    {68, "msi", 3, "application/octet-stream"},
    {69, "msp", 3, "application/octet-stream"},
    {70, "msm", 3, "application/octet-stream"},
    {71, "mid", 3, "audio/midi"},
    {72, "midi", 4, "audio/midi"},
    {73, "kar", 3, "audio/midi"},
    {74, "mp3", 3, "audio/mpeg"},
    {75, "ra", 2, "audio/x-realaudio"},
    {76, "3gpp", 4, "video/3gpp"},
    {77, "3gp", 3, "video/3gpp"},
    {78, "mpeg", 4, "video/mpeg"},
    {79, "mpg", 3, "video/mpeg"},
    {80, "mov", 3, "video/quicktime"},
    {81, "flv", 3, "video/x-flv"},
    {82, "mng", 3, "video/x-mng"},
    {83, "asx", 3, "video/x-ms-asf"},
    {84, "asf", 3, "video/x-ms-asf"},
    {85, "wmv", 3, "video/x-ms-wmv"},
    {86, "avi", 3, "video/x-msvideo"},
    {87, "dmg", 3, "application/octet-stream"}
};
/*
static const char *ftypes[] = {
	"UNKOWN",
	"FIFO",
	"CHR",
	"UNKOWN",
	"DIR",
	"UNKOWN",
	"BLK",
	"UNKOWN",
	"FILE",
	"UNKOWN",
	"LNK",
	"UNKOWN",
	"SOCK",
	"UNKOWN",
	"WHT"
};
*/
typedef struct _HTTP_KV
{
    short nk;
    short nv;
    int k;
    int v;
}HTTP_KV;
typedef struct _HTTP_RESPONSE
{
    short respid;
    short ncookies;
    int header_size;
    int nhline;
    char hlines[HTTP_HEADER_MAX];
    int headers[HTTP_HEADER_NUM];
    HTTP_KV cookies[HTTP_COOKIES_MAX];
}HTTP_RESPONSE;
typedef struct _HTTP_REQ
{
    short reqid;
    short nargvs;
    short nline;
    short ncookies;
    int   argv_off;
    int   argv_len;
    int   header_size;
    int  nhline;
    char hlines[HTTP_HEADER_MAX];
    int  headers[HTTP_HEADER_NUM];
    char path[HTTP_URL_PATH_MAX];
    char line[HTTP_ARGV_LINE_MAX];
    HTTP_KV argvs[HTTP_ARGVS_MAX];
    HTTP_KV cookies[HTTP_COOKIES_MAX];
    HTTP_KV auth;
}HTTP_REQ;
/* HTTP request HEADER parser */
int http_request_parse(char *p, char *end, HTTP_REQ *http_req, void *map);
/* HTTP argvs  parser */
int http_argv_parse(char *p, char *end, HTTP_REQ *http_req);
/* HTTP response HEADER parser */
int http_response_parse(char *p, char *end, HTTP_RESPONSE *resp, void *map);
/* return HTTP key/value */
int http_kv(HTTP_KV *kv, char *line, int nline, char **name, char **key);
/* HTTP charset convert */
int http_charset_convert(char *content_type, char *content_encoding, char *data, int len, 
        char *tocharset, int is_need_compress, char **out);
/* HTTP charset convert data free*/
void http_charset_convert_free(char *data);
int http_base64encode(char *src, int src_len, char *dst);
int http_base64decode(unsigned char *src, int src_len, unsigned char *dst);
unsigned long http_crc32(unsigned char *in, unsigned int inlen);
#endif
