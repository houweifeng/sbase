#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "stime.h"
static char *_wdays_[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static char *_ymonths_[]= {"Jan", "Feb", "Mar","Apr", "May", "Jun",
    "Jul", "Aug", "Sep","Oct", "Nov", "Dec"};

//convert str datetime to time
time_t str2time(char *datestr)
{
    struct tm tp = {0};
    char month[16], wday[16];
    int i = 0, day = -1, mon = -1;
    time_t time = 0;
    //Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
    //      Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
    //            Sun Nov  6 08:49:37 1994 
    if(sscanf(datestr, "%3s, %2d %3s %4d %2d:%2d:%2d GMT", wday, &(tp.tm_mday), 
                month, &(tp.tm_year), &(tp.tm_hour), &(tp.tm_min), &(tp.tm_sec)) == 7
            || sscanf(datestr, "%[A-Z a-z], %2d-%3s-%2d %2d:%2d:%2d GMT", wday, &(tp.tm_mday), 
                month, &(tp.tm_year), &(tp.tm_hour), &(tp.tm_min), &(tp.tm_sec)) == 7 
            || sscanf(datestr, "%3s %3s  %1d %2d:%2d:%2d %4d", wday, month, &(tp.tm_mday), 
                &(tp.tm_hour), &(tp.tm_min), &(tp.tm_sec), &(tp.tm_year)) == 7)
    {
        i = 0;
        while(i < 7)
        {
            if(strncasecmp(_wdays_[i], wday, 3) == 0)
            {
                day = i;  
                break;
            }
            ++i;
        }
        i = 0;
        while(i < 12)
        {
            if(strncasecmp(_ymonths_[i], month, 3) == 0)
            {
                mon = i; 
                break;
            }
            ++i;
        }
        if(day >= 0 && mon >= 0)
        {
            tp.tm_mon = mon;
            tp.tm_wday = day;
            if(tp.tm_year > 1900) tp.tm_year -= 1900;
            else if(tp.tm_year < 10) tp.tm_year += 100;
            time = mktime(&tp);
        }
    }
    return time;
}

/* time to GMT */
int GMTstrdate(time_t times, char *date)
{
    struct tm *tp = NULL;
    time_t timep;
    int n = 0;

    if(date)
    {
        if(times > 0) tp = gmtime(&times);
        else
        {
            time(&timep);
            tp = gmtime(&timep);
        }
        n = sprintf(date, "%s, %02d %s %d %02d:%02d:%02d GMT", _wdays_[tp->tm_wday],
                tp->tm_mday, _ymonths_[tp->tm_mon], 1900+tp->tm_year, tp->tm_hour,
                tp->tm_min, tp->tm_sec);
    }
    return n;
}

/* time to str */
int strdate(time_t times, char *date)
{
    struct tm *tp = NULL;
    time_t timep;
    int n = 0;

    if(date)
    {
        if(times > 0) tp = gmtime(&times);
        else
        {
            time(&timep);
            tp = gmtime(&timep);
        }
        n = sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", 1900+tp->tm_year, 
		tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    }
    return n;
}

/* timetospec */
void timetospec(struct timespec *ts, int usecs)
{
    struct timeval tv = {0};

    if(ts)
    {
        gettimeofday(&tv, NULL);
        ts->tv_sec = tv.tv_sec;
        ts->tv_nsec = tv.tv_usec + usecs;
        if(ts->tv_nsec > 1000000)
        {
            ts->tv_sec += ts->tv_nsec/1000000;
            ts->tv_nsec %= 1000000;
        }
        ts->tv_nsec *= 1000000;
    }
    return ;
}

#ifdef _DEBUG_TM
int main(int argc, char **argv)
{
    time_t time = 0;
    char buf[1024];

    if((time = str2time("Mon, 15 Jun 2009 02:43:12 GMT")) != 0 
            && GMTstrdate(time, buf) == 0)
    {
        fprintf(stdout, "|%ld|%s|\n", time, buf);
    }
    if((time = str2time("Sunday, 06-Nov-06 08:49:37 GMT")) != 0 
            && GMTstrdate(time, buf) == 0)
    {
        fprintf(stdout, "|%ld|%s|\n", time, buf);
    }

    if((time = str2time("Sun Nov  6 08:49:37 1994")) != 0 
            && GMTstrdate(time, buf) == 0)
    {
        fprintf(stdout, "|%ld|%s|\n", time, buf);
    }
}
//gcc -o tm tm.c -D_DEBUG_TM && ./tm
#endif
