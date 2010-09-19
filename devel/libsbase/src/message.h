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
#define MESSAGE_NEW_CONN        0x01
#define MESSAGE_NEW_SESSION     0x02
#define MESSAGE_INPUT           0x04
#define MESSAGE_OUTPUT          0x08
#define MESSAGE_BUFFER          0x10
#define MESSAGE_PACKET          0x20
#define MESSAGE_CHUNK           0x40
#define MESSAGE_DATA            0x80
#define MESSAGE_OVER            0x100
#define MESSAGE_QUIT            0x200
#define MESSAGE_TRANSACTION     0x400
#define MESSAGE_TASK            0x800
#define MESSAGE_HEARTBEAT       0x1000
#define MESSAGE_STATE           0x2000
#define MESSAGE_TIMEOUT         0x4000
#define MESSAGE_STOP            0x8000
#define MESSAGE_PROXY           0x10000
#define MESSAGE_ALL		        0x1ffff
#define QLEFT_MAX               256
static char *messagelist[] = 
{
	"MESSAGE_NEW_CONN",
	"MESSAGE_NEW_SESSION",
	"MESSAGE_INPUT",
	"MESSAGE_OUTPUT",
	"MESSAGE_BUFFER",
	"MESSAGE_PACKET",
	"MESSAGE_CHUNK",
	"MESSAGE_DATA",
	"MESSAGE_OVER",
	"MESSAGE_QUIT",
	"MESSAGE_TRANSACTION",
	"MESSAGE_TASK",
	"MESSAGE_HEARTBEAT",
	"MESSAGE_STATE",
	"MESSAGE_TIMEOUT",
	"MESSAGE_STOP",
	"MESSAGE_PROXY"
};
typedef struct _MESSAGE
{
    int             msg_id;
    int             index;
    int             fd;
    int             tid;
    void            *handler;
    void 	        *parent;
    void            *arg;
    struct _MESSAGE *next;
}MESSAGE;
typedef struct _QMESSAGE
{
    int status;
    int total;
    int qtotal;
    int nleft;
    void *mutex;
    MESSAGE *left;
    MESSAGE *first;
    MESSAGE *last;
}QMESSAGE;
int get_msg_no(int message_id);
void *qmessage_init();
void qmessage_handler(void *q, void *logger);
void qmessage_push(void *q, int id, int index, int fd, int tid, void *parent, void *handler, void *arg);
void qmessage_clean(void *q);
/* Initialize message */
#define QMTOTAL(q) ((q)?(((QMESSAGE *)q)->total):0)
#define MESSAGE_INIT() ((MESSAGE *)calloc(1, sizeof(MESSAGE)))
#define MESSAGE_CLEAN(ptr) {if(ptr){free(ptr);ptr = NULL;}}
#define MESSAGE_SIZE    sizeof(MESSAGE)
#define MESSAGE_DESC(id) ((id & MESSAGE_ALL)?messagelist[get_msg_no(id)] : "")
#endif
#ifdef __cplusplus
 }
#endif
#endif
