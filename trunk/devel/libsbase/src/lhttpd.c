#define _GNU_SOURCE
#include <signal.h>
#include <locale.h>
#include <sys/resource.h>
#include <sbase.h>
#include <time.h>
static int is_detail = 0;
static SBASE *sbase = NULL;
static SERVICE *lhttpd = NULL;
void *logger = NULL;
static char *httpd_home = "/tmp";
#define HTTP_RESP_OK            "HTTP/1.0 200 OK"
#define HTTP_BAD_REQUEST        "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n"
#define HTTP_FORBIDDEN          "HTTP/1.0 403 Forbidden\r\nContent-Length: 0\r\n\r\n" 
#define HTTP_NOT_FOUND          "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n" 
#define HTTP_NOT_MODIFIED       "HTTP/1.0 304 Not Modified\r\nContent-Length: 0\r\n\r\n"
#define HTTP_NO_CONTENT         "HTTP/1.0 206 No Content\r\nContent-Length: 0\r\n\r\n"
#define HTTP_VIEW_SIZE 65536
#define HTTP_PATH_MAX  1024
#define HTTP_LINE_MAX  4096
#define LHTTPD_VERSION "1.0.0"
#define LL(xxx) ((long long int)xxx)
#define http_default_charset "utf-8"
#define HEX2CH(c, x) ( ((x = (c - '0')) >= 0 && x < 10) \
                || ((x = (c - 'a')) >= 0 && (x += 10) < 16) \
                || ((x = (c - 'A')) >= 0 && (x += 10) < 16) )
#define URLDECODE(s, end, high, low, pp)                                            \
do                                                                                  \
{                                                                                   \
    if(*s == '%' && s < (end - 2) && HEX2CH(*(s+1), high)  && HEX2CH(*(s+2), low))  \
    {                                                                               \
        *pp++ = (high << 4) | low;                                                  \
        s += 3;                                                                     \
    }                                                                               \
    else *pp++ = *s++;                                                              \
}while(0)
int GMTstrdate(time_t times, char *date)
{
    static char *_wdays_[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static char *_ymonths_[]= {"Jan", "Feb", "Mar","Apr", "May", "Jun",
        "Jul", "Aug", "Sep","Oct", "Nov", "Dec"};
    struct tm *tp = NULL;
    time_t timep = 0;
    int n = 0;

    if(date)
    {
        if(times > 0)
        {
            tp = gmtime(&times);
        }
        else
        {
            time(&timep);
            tp = gmtime(&timep);
        }
        if(tp)
        {
            n = sprintf(date, "%s, %02d %s %d %02d:%02d:%02d GMT", _wdays_[tp->tm_wday],
                    tp->tm_mday, _ymonths_[tp->tm_mon], 1900+tp->tm_year, tp->tm_hour,
                    tp->tm_min, tp->tm_sec);
        }
    }
    return n;
}
int lhttpd_packet_handler(CONN *conn, CB_DATA *packet)
{
    char *s = NULL, *end = NULL, *p = NULL, path[HTTP_PATH_MAX], 
         line[HTTP_LINE_MAX];
    int high = 0, low = 0, n = 0;
    struct stat st;

	if(conn)
    {
        if(packet && (s = packet->data) 
                && (n = packet->ndata) > 0)
        {
            end = s + packet->ndata;
            while(*s == 0x20 || *s == '\t')s++;
            if(s < end && strncasecmp(s, "get", 3) == 0)
            {
                s += 3;        
                while(*s == 0x20 || *s == '\t')s++;
                if(*s != '/') goto err;
                p = path;
                p += sprintf(path, "%s", httpd_home);
                while(s < end && *s != 0x20 && *s != '\t' 
                        && *s != '?' && *s != '#' 
                        && p < (path + HTTP_PATH_MAX))
                {
                    URLDECODE(s, end, high, low, p);
                }
                *p = '\0';
                if(lstat(path, &st) != 0) goto not_found;
                if(S_ISDIR(st.st_mode))
                {
                    if(*(p-1) == '/')
                        p += sprintf(p, "index.html");
                    else
                        p += sprintf(p, "/index.html");
                    if(lstat(path, &st) != 0) goto not_found;
                }
                if(S_ISREG(st.st_mode) && st.st_size > 0)
                {
                    p = line;
                    p += sprintf(p, "HTTP/1.0 200 OK\r\nConnection: close\r\n"
                            "Content-Length: %lld\r\nLast-Modified:", LL(st.st_size));
                    p += GMTstrdate(st.st_mtime, p);
                    p += sprintf(p, "%s", "\r\n");
                    p += sprintf(p, "Date: ");
                    p += GMTstrdate(time(NULL), p);
                    p += sprintf(p, "\r\n");
                    p += sprintf(p, "Server: lhttpd/%s\r\n\r\n", LHTTPD_VERSION);
                    conn->push_chunk(conn, line, (p - line));
                    conn->push_file(conn, path, 0, st.st_size);
                    return conn->over(conn);
                }
                else
                {
                    conn->push_chunk(conn, HTTP_FORBIDDEN, strlen(HTTP_FORBIDDEN));
                    return conn->over(conn);
                }
            }
        }
err:
        conn->push_chunk(conn, HTTP_BAD_REQUEST, strlen(HTTP_BAD_REQUEST));
        return conn->over(conn);
not_found:
        conn->push_chunk(conn, HTTP_NOT_FOUND, strlen(HTTP_NOT_FOUND));
        return conn->over(conn);
    }
    return -1;
}
int lhttpd_timeout_handler(CONN *conn)
{
    if(conn)
    {
        conn->over(conn);
        return 0;
    }
    return -1;
}
static void lhttpd_stop(int sig)
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
    int is_daemon = 0;
    pid_t pid;

    /* locale */
    setlocale(LC_ALL, "C");
    /* signal */
    signal(SIGTERM, &lhttpd_stop);
    signal(SIGINT,  &lhttpd_stop);
    signal(SIGHUP,  SIG_IGN);
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
    setrlimiter("RLIMIT_NOFILE", RLIMIT_NOFILE, 10240);
    if((sbase = sbase_init()) == NULL)
    {
        exit(EXIT_FAILURE);
        return -1;
    }
    sbase->usec_sleep = SB_USEC_SLEEP;
    sbase->connections_limit = 10240;
    if((lhttpd = service_init()) == NULL)
    {
        fprintf(stderr, "Initialize service failed, %s", strerror(errno));
        _exit(-1);
    }
    lhttpd->family = AF_INET;
    lhttpd->sock_type = SOCK_STREAM;
    lhttpd->ip = "0.0.0.0";
    lhttpd->port = 2080;
    lhttpd->flag = SB_USE_COND|SB_USE_OUTDAEMON|SB_EVENT_LOCK;
    lhttpd->use_cond_wait = 1;
    lhttpd->working_mode = WORKING_THREAD;
    lhttpd->service_type = S_SERVICE;
    lhttpd->service_name = "lhttpd";
    lhttpd->nprocthreads = 8;
    lhttpd->niodaemons = 2;
    lhttpd->session.packet_type= PACKET_DELIMITER;
    lhttpd->session.packet_delimiter = "\r\n\r\n";
    lhttpd->session.packet_delimiter_length = strlen(lhttpd->session.packet_delimiter);
    lhttpd->session.buffer_size = SB_BUF_SIZE;
    lhttpd->session.packet_handler = &lhttpd_packet_handler;
    lhttpd->session.timeout_handler = &lhttpd_timeout_handler;
    lhttpd->set_log(lhttpd, "/tmp/lhttpd.log");
    lhttpd->set_log_level(lhttpd, 2);
    httpd_home = "/data/www";
    if(sbase->add_service(sbase, lhttpd) != 0)
    {
        fprintf(stderr, "add service lhttpd failed, %s\r\n", strerror(errno));
        _exit(-1);
    }
    fprintf(stdout, "Initialized successed\n");
    sbase->running(sbase, 0);
    //sbase->running(sbase, 60000000); sbase->stop(sbase);
    sbase->clean(sbase);
    return 0;
}
