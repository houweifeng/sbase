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
/* Check connection state  TIMEOUT */
void conn_state_handler(struct _CONN *);
/* Read handler */
void conn_read_handler(struct _CONN *);
/* Write hanler */
void conn_write_handler(struct _CONN *);
/* Packet reader */
int conn_packet_reader(struct _CONN *);
/* Packet Handler */
void conn_packet_handler(struct _CONN *);
/* CHUNK reader */
void conn_chunk_reader(struct _CONN *);
/* Receive CHUNK */
void conn_recv_chunk(struct _CONN *, size_t);
/* Receive FILE CHUNK */
void conn_recv_file(struct _CONN *,  char *, long long ,  long long );
/* Save cache */
int conn_save_cache(struct _CONN *, void *, size_t );
/* Push Chunk */
int conn_push_chunk(struct _CONN *, void *, size_t );
/* Push file */
int conn_push_file(struct _CONN *,  char *, long long ,  long long );
/* Data handler */
void conn_data_handler(struct _CONN *);
/* Transaction handler */
void conn_transaction_handler(struct _CONN *, int tid);
/* Push message */
void conn_push_message(struct _CONN *, int );
/* Set timeout */
void conn_set_timeout(CONN *conn, long long timeout);
/* start cstate on conn */
int conn_start_cstate(struct _CONN *);
/* over  cstate on conn */
void conn_over_cstate(struct _CONN *);
/* Terminate connection  */
void conn_terminate(CONN *conn);
/* Over conneciton */
void conn_over(CONN *conn);
/* Close conneciton */
void conn_close(CONN *conn);
/* Clean Connection */
void conn_clean(CONN **conn);

#ifdef __cplusplus
 }
#endif

#endif
