#include "evkqueue.h"
/* Initialize evkqueue  */
int evkqueue_init(EVBASE *evbase);
/* Add new event to evbase */
int evkqueue_add(EVBASE *evbase, EVENT *event);
/* Update event in evbase */
int evkqueue_update(EVBASE *evbase, EVENT *event);
/* Delete event from evbase */
int evkqueue_del(EVBASE *evbase, EVENT *event);
/* Loop evbase */
void evkqueue_loop(EVBASE *evbase, short, timeval *tv);
/* Clean evbase */
void evkqueue_clean(EVBASE **evbase);

