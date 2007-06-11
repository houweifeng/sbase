#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef _MESSAGE_H
#define _MESSAGE_H
#ifdef __cplusplus
extern "C" {
#endif

/* MESSAGE DEFINE*/
#ifndef _TYPEDEF_MESSAGE
#define _TYPEDEF_MESSAGE
/* MESSAGE ACTION ID DEFINE */
#define MESSAGE_QUIT            0x00
#define MESSAGE_NEW_SESSION     0x01
#define MESSAGE_INPUT           0x02
#define MESSAGE_OUTPUT          0x04
#define MESSAGE_PACKET          0x08
#define MESSAGE_DATA            0x10

typedef struct _MESSAGE
{
        int             msg_id;
        int             fd;
        void            *handler;

        void            (*clean)(struct _MESSAGE **);
}MESSAGE;
/* Initialize message */
MESSAGE *message_init();
/* Clean message */
void message_clean(MESSAGE **msg);
#define MESSAGE_SIZE    sizeof(MESSAGE)
#endif

#ifdef __cplusplus
 }
#endif
#endif

