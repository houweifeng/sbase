#include "event.h"
#ifdef HAVE_EVENT_PORTS
#ifndef _EVPORTS_H
#define _EVPORTS_H
/* Initialize evports  */
int evports_init(EVBASE *evbase);
/* Add new event to evbase */
int evports_add(EVBASE *evbase, EVENT *event);
/* Update event in evbase */
int evports_update(EVBASE *evbase, EVENT *event);
/* Delete event from evbase */
int evports_del(EVBASE *evbase, EVENT *event);
/* Loop evbase */
void evports_loop(EVBASE *evbase, short, timeval *tv);
#endif
#endif

