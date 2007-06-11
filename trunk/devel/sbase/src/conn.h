#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sbase.h"
#ifndef _CONN_H
#define _CONN_H
#ifdef __cplusplus
extern "C" {
#endif
/* Initialize setting  */
int conn_set(CONN *conn);
/* Event handler */
void conn_event_handler(int , short, void *);
/* Read handler */
void conn_read_handler(struct _CONN *);
/* Write hanler */
void conn_write_handler(struct _CONN *);
/* Packet reader */
size_t conn_packet_reader(struct _CONN *);
/* Packet Handler */
void conn_packet_handler(struct _CONN *);
/* CHUNK reader */
void conn_chunk_reader(struct _CONN *);
/* Receive CHUNK */
void conn_recv_chunk(struct _CONN *, size_t);
/* Receive FILE CHUNK */
void conn_recv_file(struct _CONN *,  char *, uint64_t,  uint64_t);
/* Push Chunk */
int conn_push_chunk(struct _CONN *, void *, size_t );
/* Push file */
int conn_push_file(struct _CONN *,  char *, uint64_t,  uint64_t);
/* Data handler */
void conn_data_handler(struct _CONN *);
/* Push message */
void conn_push_message(struct _CONN *, int );
/* Connect to remote Host */
int conn_connect(CONN *conn);
/* Setting Non-Block */
int conn_set_nonblock(CONN *conn);
/* Read data  via fd */
int conn_read(CONN *conn, void *buf, size_t size, uint32_t timeout);
/* Write data  via fd */
int conn_write(CONN *conn, void *buf, size_t size, uint32_t timeout);
/* Close Connection to remote Host */
int conn_close(CONN *conn);
/* Check Connection STATE */
void conn_stats(CONN *conn);
/* Clean Connection */
void conn_clean(CONN **conn);
/* vprintf log */
void conn_vlog(const char *format,...);

#ifdef __cplusplus
 }
#endif

#endif
