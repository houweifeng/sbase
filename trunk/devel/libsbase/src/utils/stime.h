#ifndef _STIME_H
#define _STIME_H
//convert str datetime to time
time_t str2time(char *datestr);
/* time to GMT */
int GMTstrdate(time_t time, char *date);
/* strdate */
int strdate(time_t time, char *date);
#endif
