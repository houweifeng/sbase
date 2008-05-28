#include "sbase.h"
#ifndef _SERVICE_H
#define _SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif
int service_set(SERVICE *service);
int service_run(SERVICE *service);
int service_set_log(SERVICE *service, char *logfile);
void service_event_handler(int, short, void *);
void service_clean(SERVICE **pservice);
#ifdef __cplusplus
 }
#endif
#endif
