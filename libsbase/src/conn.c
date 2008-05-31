#include "conn.h"
#include "queue.h"
#include "memb.h"
#include "chunk.h"
#include "logger.h"

/* set connection */
int conn_set(CONN *conn)
{
}

/* close connection */
int conn_close(CONN *conn)
{
}

/* terminate connection */
int conn_terminate(CONN *conn)
{

}

/* set timeout */
int conn_set_timeout(CONN *conn, int timeout_usec)
{
}

/* start client transaction state */
int conn_start_cstate(CONN *conn)
{
}

/* over client transaction state */
int conn_over_cstate(CONN *conn)
{
}

/* push message to message queue */
int conn_push_message(CONN *conn, int message_id)
{
}

/* read handler */
int conn_read_handler(CONN *conn)
{
}

/* write handler */
int conn_write_handler(CONN *conn)
{
}

/* packet reader */
int conn_packet_reader(CONN *conn)
{
}

/* packet handler */
int conn_packet_handler(CONN *conn)
{
}

/* oob data handler */
int conn_oob_handler(CONN *conn)
{
}

/* chunk data  handler */
int conn_data_handler(CONN *conn)
{
}

/* save cache to connection  */
int conn_save_cache(CONN *conn, void *data, int size)
{
}

/* chunk reader */
int conn_chunk_reader(CONN *conn)
{
}

/* receive chunk */
int conn_recv_chunk(CONN *conn, int size)
{
}

/* push chunk */
int conn_push_chunk(CONN *conn, void *chunk_data, int size)
{
}

/* receive chunk file */
int conn_recv_file(CONN *conn, char *file, long long offset, long long size)
{
}

/* push chunk file */
int conn_push_file(CONN *conn, char *file, long long offset, long long size)
{
}

/* clean connection */
void conn_clean(CONN **pconn)
{
}

/* Initialize connection */
CONN *conn_init(int fd, char *ip, int port)
{
    CONN *conn = NULL;

    if(fd > 0 && ip && port > 0 && (conn = calloc(1, sizeof(CONN))))
    {
        conn->fd = fd;
        strcpy(conn->ip, ip);
        conn->port = port;
        MB_INIT(conn->buffer, MB_BLOCK_SIZE);
        MB_INIT(conn->packet, MB_BLOCK_SIZE);
        MB_INIT(conn->cache, MB_BLOCK_SIZE);
        MB_INIT(conn->oob, MB_BLOCK_SIZE);
        CK_INIT(conn->chunk);
        conn->send_queue            = QUEUE_INIT();
        conn->set                   = conn_set;
        conn->close                 = conn_close;
        conn->terminate             = conn_terminate;
        conn->start_cstate          = conn_start_cstate;
        conn->over_cstate           = conn_over_cstate;
        conn->set_timeout           = conn_set_timeout;
        conn->push_message          = conn_push_message;
        conn->read_handler          = conn_read_handler;
        conn->write_handler         = conn_write_handler;
        conn->packet_reader         = conn_packet_reader;
        conn->packet_handler        = conn_packet_handler;
        conn->oob_handler           = conn_oob_handler;
        conn->data_handler          = conn_data_handler;
        conn->save_cache            = conn_save_cache;
        conn->chunk_reader          = conn_chunk_reader;
        conn->recv_chunk            = conn_recv_chunk;
        conn->push_chunk            = conn_push_chunk;
        conn->recv_file             = conn_recv_file;
        conn->push_file             = conn_push_file;
        conn->clean                 = conn_clean;
    }
    return conn;
}
