#include "message.h"
/* Initialize message */
MESSAGE *message_init()
{
        MESSAGE *msg = (MESSAGE *)calloc(1, sizeof(MESSAGE));
        if(msg)
        {
                msg->clean = message_clean;
        }
        return msg;
}
/* Clean message */
void message_clean(MESSAGE **msg)
{
        if((*msg)) free((*msg));
        (*msg) = NULL;
}

