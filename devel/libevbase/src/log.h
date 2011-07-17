#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifndef _LOG_H
#define _LOG_H
static char *_ymonths[]= {
	"Jan", "Feb", "Mar",
	"Apr", "May", "Jun",
	"Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"};
#ifdef HAVE_PTHREAD
#define LOG_HEADER(out)									\
{											\
	struct timeval tv;								\
	time_t timep;  									\
	struct tm *p = NULL;								\
	gettimeofday(&tv, NULL);							\
	time(&timep); 									\
	p = localtime(&timep);								\
	fprintf(out, "[%02d/%s/%04d:%02d:%02d:%02d +%06u] #%u/%p# %s::%d ",		\
			p->tm_mday, _ymonths[p->tm_mon], (1900+p->tm_year), p->tm_hour,	\
			p->tm_min, p->tm_sec, (unsigned int)tv.tv_usec, 			\
			(unsigned int)getpid(), (void *)pthread_self(), __FILE__, __LINE__);\
}
#else
#define LOG_HEADER(out)									\
{											\
	struct timeval tv;								\
	time_t timep;  									\
	struct tm *p = NULL;								\
	gettimeofday(&tv, NULL);							\
	time(&timep); 									\
	p = localtime(&timep);								\
	fprintf(out, "[%02d/%s/%04d:%02d:%02d:%02d +%06u] #%u# %s::%d ",		\
			p->tm_mday, _ymonths[p->tm_mon], (1900+p->tm_year), p->tm_hour,	\
			p->tm_min, p->tm_sec, (size_t)tv.tv_usec, 			\
			(size_t)getpid(), __FILE__, __LINE__);				\
}
#endif
#ifndef DEBUG_LOG
#ifdef _DEBUG										
#define DEBUG_LOG(format...)								\
{											\
	LOG_HEADER(stdout);								\
	fprintf(stdout, "\"DEBUG:");							\
	fprintf(stdout, format);							\
	fprintf(stdout, "\"\n");							\
	fflush(stdout);									\
}											
#else											
#define  DEBUG_LOG(format...)
#endif											
#endif

#define FATAL_LOG(format...)								\
{											\
	LOG_HEADER(stderr);								\
	fprintf(stderr, "\"FATAL:");							\
	fprintf(stderr, format);							\
	fprintf(stderr, "\"\n");							\
	fflush(stdout);									\
}
#define SHOW_LOG(format...)								
/*
#define SHOW_LOG(format...)								\
{											\
	LOG_HEADER(stdout);                                                             \
        fprintf(stdout, "\"DEBUG:");                                                    \
        fprintf(stdout, format);                                                        \
        fprintf(stdout, "\"\n");                                                        \
	fflush(stdout);									\
}
*/
#endif
