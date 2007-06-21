#include "evbase.h"
#ifdef HAVE_EVPORT
#ifndef _EVPORT_H
#define _EVPORT_H
/* Initialize evport  */
int evport_init(EVBASE *evbase);
/* Add new event to evbase */
int evport_add(EVBASE *evbase, EVENT *event);
/* Update event in evbase */
int evport_update(EVBASE *evbase, EVENT *event);
/* Delete event from evbase */
int evport_del(EVBASE *evbase, EVENT *event);
/* Loop evbase */
void evport_loop(EVBASE *evbase, short, struct timeval *tv);
#endif
#endif

