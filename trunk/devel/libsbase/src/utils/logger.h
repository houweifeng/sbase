#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "mutex.h"
#ifndef _LOGGER_H
#define _LOGGER_H
#ifdef __cplusplus
extern "C" {
#endif
static char *_logger_level_s[] = {"DEBUG", "WARN", "ERROR", "FATAL", ""};
#ifndef _STATIS_YMON
#define _STATIS_YMON
static char *ymonths[]= {
        "Jan", "Feb", "Mar",
        "Apr", "May", "Jun",
        "Jul", "Aug", "Sep",
        "Oct", "Nov", "Dec"};
#endif
#ifndef _TYPEDEF_LOGGER
#define _TYPEDEF_LOGGER
#define LOG_ROTATE_HOUR             0x01
#define LOG_ROTATE_DAY              0x02
#define LOG_ROTATE_WEEK             0x04
#define LOG_ROTATE_MONTH            0x08
#define LOG_ROTATE_SIZE             0x10
#define ROTATE_LOG_SIZE             268435456
#define LOGGER_FILENAME_LIMIT  	1024
#define LOGGER_LINE_LIMIT  	    1048576
#define __DEBUG__		0
#define	__WARN__ 		1
#define	__ERROR__ 		2
#define	__FATAL__ 		3
#define	__ACCESS__ 		4
typedef struct _LOGGER
{
	char file[LOGGER_FILENAME_LIMIT];
    char buf[LOGGER_LINE_LIMIT];
    struct timeval tv;
    struct stat st;
    time_t timep;
    time_t uptime;
    time_t x;
    struct tm *p;
    char *ps;
    void *mutex;
    int rflag;
    int n;
	int fd ;
    int total;
}LOGGER;
#endif
#ifdef HAVE_PTHREAD
#define THREADID() (size_t)pthread_self()
#else
#define THREADID() (0)
#endif
#define PL(ptr) ((LOGGER *)ptr)
#define PLF(ptr) (PL(ptr)->file)
#define PLP(ptr) (PL(ptr)->p)
#define PLTV(ptr) (PL(ptr)->tv)
#define PLTP(ptr) (PL(ptr)->timep)
#define PLN(ptr)  (PL(ptr)->n)
#define PLFD(ptr) (PL(ptr)->fd)
#define PLB(ptr) (PL(ptr)->buf)
#define PLPS(ptr) (PL(ptr)->ps)
#define PLST(ptr) (PL(ptr)->st)
#define PLX(ptr)  (PL(ptr)->x)
#define PMKDIR(ptr, lp)                                                             \
do                                                                                  \
{                                                                                   \
    strcpy(PLB(ptr), lp);                                                           \
    PLPS(ptr) = PLB(ptr);                                                           \
    PLN(ptr) = 0;                                                                   \
    while(*(PLPS(ptr)) != '\0')                                                     \
    {                                                                               \
        if(*(PLPS(ptr)) == '/' )                                                    \
        {                                                                           \
            while(*(PLPS(ptr)) != '\0' && *(PLPS(ptr)) == '/'                       \
                    && *((PLPS(ptr))+1) == '/')++(PLPS(ptr));                       \
            if(PLN(ptr) > 0)                                                        \
            {                                                                       \
                *(PLPS(ptr)) = '\0';                                                \
                PLX(ptr) = stat(PLB(ptr), &(PL(ptr)->st));                          \
                if(PLX(ptr) == 0 && !S_ISDIR(PL(ptr)->st.st_mode)) break;           \
                if(PLX(ptr) != 0 && mkdir(PLB(ptr), 0755) != 0) break;              \
                *(PLPS(ptr)) = '/';                                                 \
            }                                                                       \
            ++PLN(ptr);                                                             \
        }                                                                           \
        ++(PLPS(ptr));                                                              \
    }                                                                               \
}while(0)

/* log rotate */
#define LOGGER_ROTATE_CHECK(ptr)                                                    \
do                                                                                  \
{                                                                                   \
    gettimeofday(&(PLTV(ptr)), NULL); time(&(PLTP(ptr)));                           \
    PLP(ptr) = localtime(&(PLTP(ptr)));                                             \
    if(PL(ptr)->rflag == LOG_ROTATE_HOUR)                                           \
    {                                                                               \
        PLX(ptr) = ((1900+PLP(ptr)->tm_year) * 1000000)                             \
        + ((PLP(ptr)->tm_mon+1) * 10000)                                            \
        + (PLP(ptr)->tm_mday * 100)                                                 \
        + (PLP(ptr)->tm_hour);                                                      \
        if(PLX(ptr) > PL(ptr)->uptime) PL(ptr)->uptime = PLX(ptr);                  \
        else PLX(ptr) = 0;                                                          \
    }                                                                               \
    else if(PL(ptr)->rflag == LOG_ROTATE_DAY)                                       \
    {                                                                               \
        PLX(ptr) = ((1900+PLP(ptr)->tm_year) * 10000)                               \
        + ((PLP(ptr)->tm_mon+1) * 100)                                              \
        + PLP(ptr)->tm_mday;                                                        \
        if(PLX(ptr) > PL(ptr)->uptime) PL(ptr)->uptime = PLX(ptr);                  \
        else PLX(ptr) = 0;                                                          \
    }                                                                               \
    else if(PL(ptr)->rflag == LOG_ROTATE_WEEK)                                      \
    {                                                                               \
        PLX(ptr) = ((1900+PLP(ptr)->tm_year) * 100)                                 \
        + ((PLP(ptr)->tm_yday+1)/7);                                                \
        if(PLX(ptr) > PL(ptr)->uptime) PL(ptr)->uptime = PLX(ptr);                  \
        else PLX(ptr) = 0;                                                          \
    }                                                                               \
    else if(PL(ptr)->rflag == LOG_ROTATE_MONTH)                                     \
    {                                                                               \
        PLX(ptr) = ((1900+PLP(ptr)->tm_year) * 100)                                 \
        + PLP(ptr)->tm_mon;                                                         \
        if(PLX(ptr) > PL(ptr)->uptime) PL(ptr)->uptime = PLX(ptr);                  \
        else PLX(ptr) = 0;                                                          \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        while(sprintf(PL(ptr)->buf, "%s.%u", PL(ptr)->file, PL(ptr)->total)>0       \
                && lstat(PL(ptr)->buf, &(PL(ptr)->st)) == 0                         \
                && PL(ptr)->st.st_size > ROTATE_LOG_SIZE)                           \
            ++(PL(ptr)->total);                                                     \
        if(PL(ptr)->total == 0) ++(PL(ptr)->total);                                 \
        PLX(ptr) =  (PL(ptr)->total);                                               \
    }                                                                               \
    if(PLX(ptr) > 0)                                                                \
    {                                                                               \
        if(PL(ptr)->fd > 0){close(PL(ptr)->fd);}                                    \
        sprintf(PL(ptr)->buf, "%s.%u", PL(ptr)->file, (unsigned int)PLX(ptr));      \
        PL(ptr)->fd = open(PL(ptr)->buf, O_CREAT|O_WRONLY|O_APPEND, 0644);          \
    }                                                                               \
}while(0)

/* logger init */
#define LOGGER_INIT(ptr, lp)                                                        \
do                                                                                  \
{                                                                                   \
    if((ptr = (LOGGER *)calloc(1, sizeof(LOGGER))))                                 \
    {                                                                               \
        MUTEX_INIT(PL(ptr)->mutex);                                                 \
        strcpy(PLF(ptr), lp);                                                       \
        PMKDIR(ptr, lp);                                                            \
        LOGGER_ROTATE_CHECK(ptr);                                                   \
    }                                                                               \
}while(0)

/* logger-rotate init  */
#define LOGGER_ROTATE_INIT(ptr, lp, rotate_flag)                                    \
do                                                                                  \
{                                                                                   \
    if((ptr = (LOGGER *)calloc(1, sizeof(LOGGER))))                                 \
    {                                                                               \
        MUTEX_INIT(PL(ptr)->mutex);                                                 \
        PMKDIR(ptr, lp);                                                            \
        strcpy(PLF(ptr), lp);                                                       \
        PL(ptr)->rflag = rotate_flag;                                               \
        LOGGER_ROTATE_CHECK(ptr);                                                   \
    }                                                                               \
}while(0)

#define LOGGER_ADD(ptr, __level__, format...)                                       \
do{                                                                                 \
    if(ptr)                                                                         \
    {                                                                               \
    MUTEX_LOCK(PL(ptr)->mutex);                                                     \
    LOGGER_ROTATE_CHECK(ptr);                                                       \
    PLPS(ptr) = PLB(ptr);                                                           \
    PLPS(ptr) += sprintf(PLPS(ptr), "[%02d/%s/%04d:%02d:%02d:%02d +%06u] "          \
            "[%u/%p] #%s::%d# %s:", PLP(ptr)->tm_mday, ymonths[PLP(ptr)->tm_mon],   \
            (1900+PLP(ptr)->tm_year), PLP(ptr)->tm_hour, PLP(ptr)->tm_min,          \
        PLP(ptr)->tm_sec, (unsigned int)(PLTV(ptr).tv_usec),(unsigned int)getpid(), \
        ((char *)THREADID()), __FILE__, __LINE__, _logger_level_s[__level__]);        \
    PLPS(ptr) += sprintf(PLPS(ptr), format);                                        \
    *PLPS(ptr)++ = '\n';                                                            \
    PLN(ptr) = write(PLFD(ptr), PLB(ptr), (PLPS(ptr) - PLB(ptr)));                  \
    MUTEX_UNLOCK(PL(ptr)->mutex);                                                   \
    }                                                                               \
}while(0)
#define REALLOG(ptr, format...)                                                     \
do{                                                                                 \
    if(ptr)                                                                         \
    {                                                                               \
    MUTEX_LOCK(PL(ptr)->mutex);                                                     \
    LOGGER_ROTATE_CHECK(ptr);                                                       \
    PLPS(ptr) = PLB(ptr);                                                           \
    PLPS(ptr) = PLB(ptr);                                                           \
    PLPS(ptr) += sprintf(PLPS(ptr), "[%02d/%s/%04d:%02d:%02d:%02d +%06u] ",         \
            PLP(ptr)->tm_mday, ymonths[PLP(ptr)->tm_mon],                           \
            (1900+PLP(ptr)->tm_year), PLP(ptr)->tm_hour, PLP(ptr)->tm_min,          \
        PLP(ptr)->tm_sec, (unsigned int)(PLTV(ptr).tv_usec));                       \
    PLPS(ptr) += sprintf(PLPS(ptr), format);                                        \
    *PLPS(ptr)++ = '\n';                                                            \
    PLN(ptr) = write(PLFD(ptr), PLB(ptr), (PLPS(ptr) - PLB(ptr)));                  \
    MUTEX_UNLOCK(PL(ptr)->mutex);                                                   \
    }                                                                               \
}while(0)
#ifdef _DEBUG
#define DEBUG_LOGGER(ptr, format...) {LOGGER_ADD(ptr, __DEBUG__, format);}
#else
#define DEBUG_LOGGER(ptr, format...)
#endif
#define WARN_LOGGER(ptr, format...) {LOGGER_ADD(ptr, __WARN__, format);}
#define ERROR_LOGGER(ptr, format...) {LOGGER_ADD(ptr, __ERROR__, format);}
#define FATAL_LOGGER(ptr, format...) {LOGGER_ADD(ptr, __FATAL__, format);}
#define ACCESS_LOGGER(ptr, format...) {LOGGER_ADD(ptr, __ACCESS__, format);}
#define LOGGER_CLEAN(ptr)                                                           \
do{                                                                                 \
    if(ptr)                                                                         \
    {                                                                               \
        close(PLFD(ptr));                                                           \
        if(PL(ptr)->mutex){MUTEX_DESTROY(PL(ptr)->mutex);}                          \
        free(ptr);                                                                  \
        ptr = NULL;                                                                 \
    }                                                                               \
}while(0)
#ifdef __cplusplus
 }
#endif
#endif
