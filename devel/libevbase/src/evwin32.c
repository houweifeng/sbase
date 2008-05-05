#include "evwin32.h"
#ifdef WIN32
/* Initialize evwin32  */
int evwin32_init(EVBASE *evbase)
{
}
/* Add new event to evbase */
int evwin32_add(EVBASE *evbase, EVENT *event)
{
}
/* Update event in evbase */
int evwin32_update(EVBASE *evbase, EVENT *event)
{
}
/* Delete event from evbase */
int evwin32_del(EVBASE *evbase, EVENT *event)
{
}
/* Loop evbase */
void evwin32_loop(EVBASE *evbase, short loop_flags, struct timeval *tv)
{
}
/* Reset evbase */
void evwin32_reset(EVBASE *evbase)
{
}
/* Clean evbase */
void evwin32_clean(EVBASE **evbase)
{
}
#endif
