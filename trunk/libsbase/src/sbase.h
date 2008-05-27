#include <unistd.h>
#include <evbase.h>
#ifndef _SBASE_H
#define _SBASE_h
#ifdef __cplusplus
extern "C" {
#endif
#define SB_CONN_MAX    65536
struct _SBASE;
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
typedef struct _SBASE
{
	int fd;
	int usec_sleep;
	EVBASE *evbase;

	/* logger */
	void *logger;

	void (*event_handler)(int , short, void *);
	int (*setrlimit)(struct _SBASE *, char *, int , int);
	
	int(*set_log)(struct _SBASE *, char *);
	int(*set_evlog)(struct _SBASE *, char *);
	
	int(*add_service)(struct _SBASE *, void *);
	int(*running)(struct _SBASE *, int );
	int(*stop)(struct _SBASE *);
}SBASE;
/* Initialize sbase */
SBASE *sbase_init();
#ifdef __cplusplus
 }
#endif
#endif
