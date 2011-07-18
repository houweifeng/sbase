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
#include <pthread.h>
#include "logger.h"
static char *_logger_level_s[] = {"DEBUG", "WARN", "ERROR", "FATAL", ""};
#ifndef _STATIS_YMON
#define _STATIS_YMON
static char *ymonths[]= {"Jan", "Feb", "Mar", "Apr", "May", "Jun","Jul", "Aug", "Sep","Oct", "Nov", "Dec"};
#endif
int logger_mkdir(char *path)
{
    char fullpath[LOGGER_PATH_MAX];
    int level = -1, ret = -1;
    struct stat st = {0};
    char *p = NULL;

    if(path)
    {
        strcpy(fullpath, path);
        p = fullpath;
        while(*p != '0')
        {
            if(*p == '/' )
            {
                while(*p != '0' && *p == '/' && *(p+1) == '/')++p;
                if(level > 0)
                {
                    *p = '0';
                    memset(&st, 0, sizeof(struct stat));
                    ret = stat(fullpath, &st);
                    if(ret == 0 && !S_ISDIR(st.st_mode)) return -1;
                    if(ret != 0 && mkdir(fullpath, 0755) != 0) return -1;
                    *p = '/';
                }
                level++;
            }
            ++p;
        }
        return 0;
    }
    return -1;
}

void logger_rotate_check(LOGGER *logger)
{
    char line[LOGGER_LINE_SIZE];
    time_t timep = 0 , x = 0;
    struct tm *ptm = NULL;
    struct stat st = {0};


    if(logger)
    {
        gettimeofday(&(logger->tv), NULL);
        time(&timep);
        ptm = logger->ptm = localtime(&timep);
        if(logger->rflag == LOG_ROTATE_HOUR)
        {
            x = ((1900+ptm->tm_year) * 1000000)
                + ((ptm->tm_mon+1) * 10000)
                + (ptm->tm_mday * 100)
                + (ptm->tm_hour);
            if(x > logger->uptime) logger->uptime = x;
            else x = 0;
        }
        else if(logger->rflag == LOG_ROTATE_DAY)
        {
            x = ((1900+ptm->tm_year) * 10000)
                + ((ptm->tm_mon+1) * 100)
                + ptm->tm_mday;
            if(x > logger->uptime) logger->uptime = x;
            else x = 0;
        }
        else if(logger->rflag == LOG_ROTATE_WEEK)
        {
            x = ((1900+ptm->tm_year) * 100)
                + ((ptm->tm_yday+1)/7);
            if(x > logger->uptime) logger->uptime = x;
            else x = 0;
        }
        else if(logger->rflag == LOG_ROTATE_MONTH)
        {
            x = ((1900+ptm->tm_year) * 100) + ptm->tm_mon;
            if(x > logger->uptime) logger->uptime = x;
            else x = 0;
        }
        else
        {
            if(logger->fd > 0)
            {
                if(fstat(logger->fd, &st) == 0 && st.st_size > ROTATE_LOG_SIZE)
                    x = ++(logger->total);
                else 
                    x = 0;
            }
            else
            {
                while(sprintf(line, "%s.%u", logger->file, logger->total) > 0
                        && lstat(line, &st) == 0 && st.st_size > ROTATE_LOG_SIZE)
                    x = ++(logger->total);
                if(logger->total == 0) ++(logger->total);
                x = logger->total;
            }
        }
        if(x > 0)
        {
            if(logger->fd > 0){close(logger->fd);}
            sprintf(line, "%s.%u", logger->file, (unsigned int)x);
            logger->fd = open(line, O_CREAT|O_WRONLY|O_APPEND, 0644);
        }
    }
    return ;
}

LOGGER *logger_init(char *file, int rotate_flag)
{
    LOGGER *logger = NULL;

    if((logger = (LOGGER *)calloc(1, sizeof(LOGGER))))
    {
        MUTEX_INIT(logger->mutex);
        strcpy(logger->file, file);
        logger_mkdir(file);
        logger->rflag = rotate_flag;
        logger_rotate_check(logger);
    }
    return logger;
}

int logger_header(LOGGER *logger, char *buf, int level, char *_file_, int _line_)
{
    char *s = NULL;
    int n = 0;

    if((s = buf))
    {

        s += sprintf(s,"[%02d/%s/%04d:%02d:%02d:%02d +%06u] ", PTM(logger)->tm_mday, 
                ymonths[PTM(logger)->tm_mon], (1900+PTM(logger)->tm_year), 
                PTM(logger)->tm_hour, PTM(logger)->tm_min, PTM(logger)->tm_sec, 
                (unsigned int)(PTV(logger).tv_usec));
        if(level >= 0)                                                          
        {                                                                           
            s += sprintf(s, "[%u/%p] #%s::%d# %s:", (unsigned int)getpid(), 
                    ((char *)THREADID()), _file_, _line_, _logger_level_s[level]); 
        }
        n = s - buf;
    }
    return n;
}

void logger_clean(void *ptr)
{
    LOGGER *logger = (LOGGER *)ptr;
    if(logger)
    {
        if(logger->fd) close(logger->fd);
        MUTEX_DESTROY(logger->mutex);
        free(logger);
    }
    return ;
}
