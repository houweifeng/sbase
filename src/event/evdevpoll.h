#include "event.h"
#ifdef HAVE_DEVPOLL
#ifndef _EVDEVPOLL_H
#define _EVDEVPOLL_H
/* Initialize evdevpoll  */
int evdevpoll_init(EVBASE *evbase);
/* Add new event to evbase */
int evdevpoll_add(EVBASE *evbase, EVENT *event);
/* Update event in evbase */
int evdevpoll_update(EVBASE *evbase, EVENT *event);
/* Delete event from evbase */
int evdevpoll_del(EVBASE *evbase, EVENT *event);
/* Loop evbase */
void evdevpoll_loop(EVBASE *evbase, short, timeval *tv);
/* Clean evbase */
void evdevpoll_clean(EVBASE **evbase);
#endif
#endif

