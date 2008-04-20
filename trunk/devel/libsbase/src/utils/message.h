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
#define MESSAGE_QUIT            0x20
#define MESSAGE_NEW_SESSION     0x01
#define MESSAGE_INPUT           0x02
#define MESSAGE_OUTPUT          0x04
#define MESSAGE_PACKET          0x08
#define MESSAGE_DATA            0x10
#define MESSAGE_TRANSACTION     0x30
#define MESSAGE_TASK            0x40
#define MESSAGE_ALL		(MESSAGE_QUIT | MESSAGE_NEW_SESSION | MESSAGE_INPUT | MESSAGE_OUTPUT \
				| MESSAGE_PACKET | MESSAGE_DATA | MESSAGE_TRANSACTION| MESSAGE_TASK)
static char *messagelist[] = 
{
	"",
	"MESSAGE_NEW_SESSION",
	"MESSAGE_INPUT",
	"",
	"MESSAGE_OUTPUT",
	"","","",
	"MESSAGE_PACKET",
	"","","","","","","",
	"MESSAGE_DATA","","","","","","","","","","","","","","","",
	"MESSAGE_QUIT"
};
typedef struct _MESSAGE
{
    int             msg_id;
    int             fd;
    void            *handler;
    void		    *parent;
    void            *arg;
    int             tid;
}MESSAGE;
/* Initialize message */
#define MESSAGE_INIT() ((MESSAGE *)calloc(1, sizeof(MESSAGE)))
#define MESSAGE_CLEAN(ptr) {if(ptr){free(ptr);ptr = NULL;}}
#define MESSAGE_SIZE    sizeof(MESSAGE)
#endif
#ifdef __cplusplus
 }
#endif
#endif

