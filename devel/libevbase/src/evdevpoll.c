#include "evdevpoll.h"
#ifdef HAVE_DEVPOLL
#include <sys/resource.h>
/* Initialize evdevpoll  */
int evdevpoll_init(EVBASE *evbase)
{
}
/* Add new event to evbase */
int evdevpoll_add(EVBASE *evbase, EVENT *event)
{
}
/* Update event in evbase */
int evdevpoll_update(EVBASE *evbase, EVENT *event)
{
}
/* Delete event from evbase */
int evdevpoll_del(EVBASE *evbase, EVENT *event)
{
}
/* Loop evbase */
void evdevpoll_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
}
/* Reset evbase */
void evdevpoll_reset(EVBASE *evbase)
{
}
/* Clean evbase */
void evdevpoll_clean(EVBASE **evbase)
{
}
#endif
