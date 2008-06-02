#include "sbase.h"
#ifndef _CONN_H
#define _CONN_H

/* set connection */
int conn_set(CONN *conn);

/* close connection */
int conn_close(CONN *conn);

/* over connection */
int conn_over(CONN *conn);

/* terminate connection */
int conn_terminate(CONN *conn);

/* set timeout */
int conn_set_timeout(CONN *conn, int timeout_usec);

/* set session options */
int conn_set_session(CONN *conn, SESSION *session);

/* start client transaction state */
int conn_start_cstate(CONN *conn);

/* over client transaction state */
int conn_over_cstate(CONN *conn);

/* push message to message queue */
int conn_push_message(CONN *conn, int message_id);

/* read handler */
int conn_read_handler(CONN *conn);

/* write handler */
int conn_write_handler(CONN *conn);

/* packet reader */
int conn_packet_reader(CONN *conn);

/* packet handler */
int conn_packet_handler(CONN *conn);

/* oob data handler */
int conn_oob_handler(CONN *conn);

/* chunk data  handler */
int conn_data_handler(CONN *conn);

/* save cache to connection  */
int conn_save_cache(CONN *conn, void *data, int size);

/* chunk reader */
int conn_chunk_reader(CONN *conn);

/* receive chunk */
int conn_recv_chunk(CONN *conn, int size);

/* push chunk */
int conn_push_chunk(CONN *conn, void *chunk_data, int size);

/* receive chunk file */
int conn_recv_file(CONN *conn, char *file, long long offset, long long size);

/* push chunk file */
int conn_push_file(CONN *conn, char *file, long long offset, long long size);

/* set session options */
int conn_set_session(CONN *conn, SESSION *session);

/* clean connection */
void conn_clean(CONN **pconn);
/* event handler */
void conn_event_handler(int event_fd, short event, void *arg);
#endif
