#include "event.h"
#ifdef HAVE_RTSIG
#ifndef _EVRTSIG_H
#define _EVRTSIG_H
/* Initialize evrtsig  */
int evrtsig_init(EVBASE *evbase);
/* Add new event to evbase */
int evrtsig_add(EVBASE *evbase, EVENT *event);
/* Update event in evbase */
int evrtsig_update(EVBASE *evbase, EVENT *event);
/* Delete event from evbase */
int evrtsig_del(EVBASE *evbase, EVENT *event);
/* Loop evbase */
void evrtsig_loop(EVBASE *evbase, short, timeval *tv);
/* Clean evbase */
void evrtsig_clean(EVBASE **evbase);
#endif
#endif

