#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
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
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "buffer.h"

#ifndef _CHUNK_H
#define _CHUNK_H
#ifdef __cplusplus
extern "C" {
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
        struct _BUFFER *buf;
        struct {
                int   fd ;
                char  name[FILE_NAME_LIMIT + 1];
        } file;
        unsigned long long  offset;
        unsigned long long  len;
	void *mutex;

        /* method */
        int (*set)(struct _CHUNK *, int , int , char *, unsigned long long , unsigned long long );
        int (*append)(struct _CHUNK *, void *, size_t); 
        int (*fill)(struct _CHUNK *, void *, size_t); 
        int (*send)(struct _CHUNK *, int , size_t );
        void (*reset)(struct _CHUNK *); 
        void (*clean)(struct _CHUNK **); 
}CHUNK;
#define CHUNK_SIZE sizeof(CHUNK)
/* Initialize struct CHUNK */
struct _CHUNK *chunk_init();
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
		"chunk->len:%llu\n\n", \
		chunk, chunk->id, chunk->type, \
		chunk->buf, chunk->buf->data, chunk->buf->size, \
		chunk->file.fd, chunk->file.name, \
		chunk->offset, chunk->len \
		); \
	} \
}
#endif

/* Initialzie CHUNK */
int chk_set( CHUNK *, int , int, char *, unsigned long long , unsigned long long );
/* append data to CHUNK BUFFER */
int chk_append(CHUNK *, void *, size_t );
/* fill CHUNK with data */
int chk_fill(CHUNK *, void *, size_t);
/* write CHUNK data to fd */
int chk_send(CHUNK *, int , size_t );
/* reset CHUNK */
void chk_reset(CHUNK *);
/* clean CHUNK */
void chk_clean(CHUNK **);

#ifdef __cplusplus
 }
#endif

#endif
