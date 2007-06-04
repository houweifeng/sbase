#include "event.h"
#ifdef HAVE_POLL
#ifndef _EVPOLL_H
#define _EVPOLL_H
/* Initialize evpoll  */
int evpoll_init(EVBASE *evbase);
/* Add new event to evbase */
int evpoll_add(EVBASE *evbase, EVENT *event);
/* Update event in evbase */
int evpoll_update(EVBASE *evbase, EVENT *event);
/* Delete event from evbase */
int evpoll_del(EVBASE *evbase, EVENT *event);
/* Loop evbase */
void evpoll_loop(EVBASE *evbase, short, timeval *tv);
/* Clean evbase */
void evpoll_clean(EVBASE **evbase);
#endif
#endif

