#include "sbase.h"
#ifndef _SERVICE_H
#define _SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif
/* set service */
int service_set(SERVICE *service);
/* run service */
int service_run(SERVICE *service);
/* stop service */
void service_stop(SERVICE *service);
/* new connection */
CONN *service_newconn(SERVICE *service, int inet_family, int socket_type, 
        char *ip, int port, SESSION *session);
/* add new connection */
CONN *service_addconn(SERVICE *service, int fd, char *ip, int port, SESSION *session);
/* push connection to connections pool */
int service_pushconn(SERVICE *service, CONN *conn);
/* pop connection from connections pool */
int service_popconn(SERVICE *service, CONN *conn);
/* get free connection */
CONN *service_getconn(SERVICE *service);
/* set service session */
int service_set_session(SERVICE *service, SESSION *session);
/* new task */
int service_newtask(SERVICE *service, CALLBACK *callback, void *arg);
/* add new transaction */
int service_newtransaction(SERVICE *service, CONN *conn, int tid);
/* set log */
int service_set_log(SERVICE *service, char *logfile);
void service_event_handler(int, short, void *);
void service_clean(SERVICE **pservice);
#ifdef __cplusplus
 }
#endif
#endif
