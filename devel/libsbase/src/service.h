#include "sbase.h"
#include "chunk.h"
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
CONN *service_addconn(SERVICE *service, int sock_type, int fd, 
        char *remote_ip, int remote_port, char *local_ip, int local_port, SESSION *session);
/* push connection to connections pool */
int service_pushconn(SERVICE *service, CONN *conn);
/* pop connection from connections pool */
int service_popconn(SERVICE *service, CONN *conn);
/* get free connection */
CONN *service_getconn(SERVICE *service);
/* pop chunk from service  */
CHUNK *service_popchunk(SERVICE *service);
/* push chunk to service  */
int service_pushchunk(SERVICE *service, CHUNK *cp);
/* set service session */
int service_set_session(SERVICE *service, SESSION *session);
/* add multicast */
int service_add_multicast(SERVICE *service, char *multicast_ip);
/* drop multicast */
int service_drop_multicast(SERVICE *service, char *multicast_ip);
/* new task */
int service_newtask(SERVICE *service, CALLBACK *callback, void *arg);
/* add new transaction */
int service_newtransaction(SERVICE *service, CONN *conn, int tid);
/* set log */
int service_set_log(SERVICE *service, char *logfile);
/* event handler */
void service_event_handler(int, short, void *);
/* heartbeat handler */
void service_set_heartbeat(SERVICE *service, int interval, CALLBACK *handler, void *arg);
/* state check */
void service_state(void *arg);
/* active heartbeat */
void service_active_heartbeat(void *arg);
/* active evtimer heartbeat */
void service_evtimer_handler(void *arg);
/* clean service */
void service_clean(SERVICE **pservice);
#ifdef __cplusplus
 }
#endif
#endif
