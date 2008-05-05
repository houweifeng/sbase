#include "evport.h"
#ifdef HAVE_EVPORT
#include <sys/resource.h>
/* Initialize evport  */
int evport_init(EVBASE *evbase)
{
}
/* Add new event to evbase */
int evport_add(EVBASE *evbase, EVENT *event)
{
}
/* Update event in evbase */
int evport_update(EVBASE *evbase, EVENT *event)
{
}
/* Delete event from evbase */
int evport_del(EVBASE *evbase, EVENT *event)
{
}
/* Loop evbase */
void evport_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
}
/* Reset evbase */
void evport_reset(EVBASE *evbase)
{
}
/* Clean evbase */
int evport_clean(EVBASE **evbase)
{
}
#endif

