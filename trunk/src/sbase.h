#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <locale.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <event.h>
#ifndef _SBASE_H
#define _SBASE_H
#define HAVE_SBASE_H
#ifdef __cplusplus
extern "C" {
#endif

/* INET */
#ifndef TCP_T
#define TCP_T SOCK_STREAM
#endif
#ifndef UDP_T	
#define UDP_T SOCK_DGRAM 
#endif
#ifndef SOCK_MIX      
#define SOCK_MIX (SOCK_DGRAM | SOCK_STREAM)
#endif
#ifndef DGRAM_SIZE 	
#define DGRAM_SIZE 1024 
#endif
#ifndef BUF_SIZE
/* 1M = 0x100000 */
#define	BUF_SIZE   0x100000 
#endif
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 	65535
#endif
#ifndef MAX_PACKET_LEN
#define MAX_PACKET_LEN		65535
#endif
#ifndef MAX_PROCTHREADS
#define MAX_PROCTHREADS		256
#endif
#ifndef MIN_SLEEP_USEC
#define MIN_SLEEP_USEC		1000
#endif 
#ifndef MIN_HEARTBEAT_INTERVAL
#define MIN_HEARTBEAT_INTERVAL  1
#endif
#ifndef MIN_CONN_TIMEOUT
#define MIN_CONN_TIMEOUT	120
#endif
#ifndef	INET_BACKLOG
#define	INET_BACKLOG		65535 
#endif

/* TYPES PREDEFINE */
struct _SERVICE;
struct _PROCTHREAD;
struct _CONN;
struct _LOGGER;
struct _TIMER;
struct _BUFFER;
struct _QUEUE;
struct _CHUNK;

/* BUFFER */
#ifndef _TYPEDEF_BUFFER
#define _TYPEDEF_BUFFER
typedef struct _BUFFER
{
	void *data;
	void *end;
	size_t size;
	pthread_mutex_t mutex;

	void 	*(*calloc)(struct _BUFFER *, size_t);
	void 	*(*malloc)(struct _BUFFER *, size_t);
	void 	*(*recalloc)(struct _BUFFER *, size_t);
	void 	*(*remalloc)(struct _BUFFER *, size_t);
	int 	(*push)(struct _BUFFER *, void *, size_t);
	int 	(*del)(struct _BUFFER *, size_t);
	void 	(*reset)(struct _BUFFER *);
	void 	(*clean)(struct _BUFFER **);

}BUFFER;
BUFFER *buffer_init();
#define BUFFER_SIZE sizeof(BUFFER)
#define BUFFER_VIEW(buf) \
{ \
	if(buf) \
	{ \
		fprintf(stdout, "buf:%08X\n" \
		"buf->data:%08X\n" \
		"buf->end:%08X\n" \
		"buf->size:%ld\n" \
		"buf->recalloc():%08X\n" \
		"buf->remalloc():%08X\n" \
		"buf->push():%08X\n" \
		"buf->del():%08X\n" \
		"buf->reset():%08X\n" \
		"buf->clean():%08X\n", \
		buf, buf->data, buf->end, buf->size, \
		buf->recalloc, buf->remalloc, \
		buf->push, buf->del, \
		buf->reset, buf->clean); \
	} \
}
#endif

/* CHUNK */
#ifndef _TYPEDEF_CHUNK
#define _TYPEDEF_CHUNK
#define MEM_CHUNK   0x02
#define FILE_CHUNK  0x04
#define ALL_CHUNK  (MEM_CHUNK | FILE_CHUNK)
#define FILE_NAME_LIMIT 255 
typedef struct _CHUNK{
	/* property */
	int id;
	int type;
	BUFFER *buf;
	struct {
		int   fd ;
		char  name[FILE_NAME_LIMIT + 1];
	} file;
	uint64_t offset;
	uint64_t len;

	pthread_mutex_t mutex;

	/* method */
	int (*set)(struct _CHUNK *, int , int , char *, uint64_t, uint64_t);
	int (*append)(struct _CHUNK *, void *, size_t); 
	int (*fill)(struct _CHUNK *, void *, size_t); 
	int (*send)(struct _CHUNK *, int , size_t );
	void (*reset)(struct _CHUNK *); 
	void (*clean)(struct _CHUNK **); 
}CHUNK;
#define CHUNK_SIZE sizeof(CHUNK)
/* Initialize struct CHUNK */
CHUNK *chunk_init();
#define CHUNK_VIEW(chunk) \
{ \
	if(chunk) \
	{ \
		fprintf(stdout, "chunk:%08X\n" \
				"chunk->id:%d\n" \
				"chunk->type:%02X\n" \
				"chunk->buf:%08X\n" \
				"chunk->buf->data:%08X\n" \
				"chunk->buf->size:%u\n" \
				"chunk->file.fd:%d\n" \
				"chunk->file.name:%s\n" \
				"chunk->offset:%llu\n" \
				"chunk->len:%llu\n" \
				"chunk->set():%08X\n" \
				"chunk->append():%08X\n" \
				"chunk->fill():%08X\n" \
				"chunk->send():%08X\n" \
                                "chunk->reset():%08X\n" \
                                "chunk->clean():%08X\n\n", \
                                chunk, chunk->id, chunk->type, \
                                chunk->buf, chunk->buf->data, chunk->buf->size, \
                                chunk->file.fd, chunk->file.name, \
                                chunk->offset, chunk->len, chunk->set,\
                                chunk->append, chunk->fill, chunk->send,\
                                chunk->reset, chunk->clean \
                       ); \
        } \
}

#endif


/* Transaction state */
#ifndef S_STATES
#define S_STATE_READY		0x00
#define S_STATE_READ_CHUNK	0x02
#define S_STATE_WRITE_STATE     0x04
#define S_STATE_PACKET_HANDLING	0x08
#define S_STATE_DATA_HANDLING	0x10
#define S_STATES	(S_STATE_READY & S_STATE_READ_CHUNK & S_STATE_WRITE_STATE \
			S_STATE_PACKET_HANDLING & S_STATE_DATA_HANDLING )
#endif

/* WORK TYPE */
#define WORK_PROC	0x00
#define WORK_THREAD	0x01

#ifndef TYPEDEF_SBASE
#define TYPEDEF_SBASE
typedef struct _SBASE{
	/* PROCTHREAD */
	int 	work ;
	int	max_procthreads;

	/* Service options */
        struct _SERVICE **services;
	int	running_services;

	/* Event options */
	struct event_base *ev_eventbase;

	/* Running options */
	int running_status;
	uint32_t sleep_usec;

	/* Global options */
	struct _TIMER *timer;
	struct _LOGGER *logger;

        /* APIS */
        int  (*add_service)(struct _SBASE * , struct _SERVICE *);
	int  (*set_log)(struct _SBASE * , const char *);
        int  (*start)(struct _SBASE * );
        int  (*stop)(struct _SBASE * );
        void (*clean)(struct _SBASE *);
}SBASE;
/* Initialize struct sbase */
SBASE *sbase();
#endif

/* SERVICE */
#ifndef _TYPEDEF_SERVICE
#define _TYPEDEF_SERVICE
typedef struct _SERVICE
{
        /* INET options */
        int sock_type;
        int family;
        int fd;
        char *ip;
        int port;
        struct sockaddr_in sa;

        /* Event options */
        struct event_base *ev_eventbase;
        struct event ev_event;

        /* Prothread options */
        int work;
        int max_procthreads;
        int running_procthreads;
       	struct _PROCTHREAD **procthreads;
        struct _PROCTHREAD *procthread;

        /* Connection options */
        int max_connections;
        int running_connections;

        /* Packet options */
        int packet_type;
        int packet_length;
        char *packet_delimiter;
        int  packet_delimiter_length;
        uint32_t buffer_size;

        /* Global options */
        struct _TIMER *timer;
        struct _LOGGER *logger;

        /* Running options */
        int      running_status;
        uint32_t heartbeat_interval;
        uint32_t sleep_usec;
        uint32_t conn_timeout;

        /* Callback options */
        /* Heartbeat Handler */
        void (*cb_heartbeat_handler)(void *arg);
        /* Read From Buffer and return packet length to get */
        int (*cb_packet_reader)(const struct _CONN*, const struct _BUFFER *);
        /* Packet Handler */
        void (*cb_packet_handler)(const struct _CONN*, const struct _BUFFER *);
        /* Data Handler */
        void (*cb_data_handler)(const struct _CONN*, const struct _BUFFER *,
		 const struct _CHUNK *, const struct _BUFFER *);
        /* OOB Data Handler */
        void (*cb_oob_handler)(const struct _CONN *, const struct _BUFFER *oob);

        /* Methods */
        void (*event_handler)(int, short, void*);
        int  (*set)(struct _SERVICE * );
        void (*run)(struct _SERVICE * );
        int  (*addconn)(struct _SERVICE *, int , struct sockaddr_in);
        void (*terminate)(struct _SERVICE * );
        void (*clean)(struct _SERVICE * );
}SERVICE;
/* Initialize service */
SERVICE *service_init();
#endif

/* PROCTHREAD */
#ifndef _TYPEDEF_PROCTHREAD
#define _TYPEDEF_PROCTHREAD
typedef struct _PROCTHREAD
{
        /* PROCTHREAD ID INDEX */
        int id;
        int index;

        /* Event options */
        int                     fd ;
        struct event_base       *ev_eventbase;

        /* Running options */
        int                     running_status;
        struct _CONN            **connections;
        uint32_t                max_connections;
        uint32_t                running_connections;
        uint32_t                sleep_usec;

        /* MESSAGE QUEUE OPTIONS */
        struct _QUEUE		*message_queue;

        /* Timer options */
        struct _TIMER		*timer;

        /* Logger option */
        struct _LOGGER		*logger;

        /* Methods */
        void (*run)(struct _PROCTHREAD *)
        void (*addconn)(struct _PROCTHREAD *, int, struct sockaddr_in )
        void (*add_connection)(struct _PROCTHREAD *, int, struct sockaddr_in )
        void (*terminate_connection)(struct _PROCTHREAD *, struct _CONN *)
        void (*terminate)(struct _PROCTHREAD*);
        void (*clean)(struct _PROCTHREAD**);

}PROCTHREAD;
/* Initialize procthread */
PROCTHREAD *procthread_init();
#endif


/* CONNECTION */
#ifndef _TYPEDEF_CONN
#define _TYPEDEF_CONN
#define C_STATE_UNCONNECT 0x00
#define C_STATE_CONNECTED 0x01
#define C_STATE_FREE      0x02
#define C_STATE_USING     0x04
#define C_STATE_CLOSE     0x08
#define IP_MAX            16
#define CONN_SLEEP_USEC	  100u
#define CONN_IO_TIMEOUT   100000u
#define RECONNECT_MAX     10000
#ifndef CONN_MAX
#define CONN_MAX          65535
#endif
/* struct CONN */
typedef struct _CONN
{
	/* INET options */
	char			ip[IP_MAX];
	int			port;
	struct			sockaddr_in sa;
	int			fd;
	/* Transaction option */
	int			s_id;
	int			s_state;
	int			c_state;
	/* Packet options */
	int			packet_type;
	int			packet_length;
	char			packet_delimiter;
	char			packet_delimiter_length;
	/* Global  options */
	//buffer 
	struct _BUFFER		*buffer;
	//packet 
	struct _BUFFER		*packet;
	// chunk 
	struct _CHUNK		*chunk;
	// send queue 
	struct _QUEUE		*send_queue;
	// Logger
        struct _LOGGER          *logger;
	// Timer
        struct _TIMER           *timer;

	/* Event options */
	struct    event_base 	*ev_eventbase;
	struct    event 	ev_event;
	short     		event_flags;

	/* Connection options */
	int                     reconnect_count ;
        int                     connect_state;
	uint32_t  		sleep_usec;/* Waiting for I/O TIMEOUT */
	uint32_t  		timeout;/* I/O TIMEOUT */
	int       		is_nonblock ;

	/* Methods */
	int       (*event_init)(struct _CONN *);
	void      (*event_update)(struct _CONN *, short);
	void      (*event_handler)(int , short, void *);
	size_t    (*packet_reader)(struct _CONN *);
	void      (*read_handler)(struct _CONN *);
	void      (*write_handler)(struct _CONN *);
	void      (*packet_handler)(struct _CONN *);
	void      (*chunk_reader)(struct _CONN *);
	void      (*recv_chunk)(struct _CONN *, size_t);
	void      (*recv_file)(struct _CONN *, char *, uint64_t, uint64_t);
	int       (*push_chunk)(struct _CONN *, void *, size_t);
	int       (*push_file)(struct _CONN *, char *, uint64_t, uint64_t);
	void      (*data_handler)(struct _CONN *);
	void      (*oob_handler)(struct _CONN *);
	int       (*connect)(struct _CONN *);
	int       (*set_nonblock)(struct _CONN *);
	int       (*read)(struct _CONN *, void *, size_t , uint32_t);
	int       (*write)(struct _CONN *, void *, size_t , uint32_t);
	int       (*close)(struct _CONN *); 
	void      (*stats)(struct _CONN *);
	void      (*clean)(struct _CONN *);
	void      (*vlog)(char *,...);
} CONN;
/* Initialize CONN */
CONN *conn_init(char *ip, int port);
#endif

#ifdef __cplusplus
}
#endif
#endif
