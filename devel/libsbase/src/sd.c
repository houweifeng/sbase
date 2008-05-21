#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "sbase.h"

int cb_packet_reader(CONN *conn, SDATA *buffer)
{
}

void cb_packet_handler(CONN *conn, SDATA *packet)
{
        if(conn && conn->push_chunk)
                    conn->push_chunk((CONN *)conn, ((SDATA *)packet)->data, packet->ndata);
}

void cb_data_handler(CONN *conn, SDATA *packet, SDATA *cache, SDATA *chunk)
{
}

void cb_file_handler(CONN *conn, SDATA *packet, SDATA *cache, char *file)
{
}

void cb_oob_handler(CONN *conn, SDATA *oob)
{
}

int main()
{
    SBASE *sbase = sbase_init();
    sbase->working_mode = 1;
    sbase->max_procthreads = 32;
    sbase->sleep_usec = 10000;
    sbase->set_log(sbase, "/tmp/sb.log");
    sbase->set_evlog(sbase, "/tmp/sb.evlog");
    SERVICE *service = service_init();
    service->service_type = S_SERVICE;
    service->family = AF_INET;
    service->socket_type = SOCK_STREAM;
    service->name = "sd";
    service->ip = NULL;
    service->port = 1418;
    service->max_procthreads = 2;
    service->logfile = "/tmp/sd.log";
    service->evlogfile = "/tmp/sd.evlog";
    service->sleep_usec = 10000;
    service->heartbeat_interval = 100000;
    service->max_connections = 1024;
    service->packet_type = PACKET_DELIMITER;
    service->packet_delimiter = "\r\n";
    service->packet_delimiter_length = 2;
    service->buffer_size = 1024 * 1024;
    service->ops.cb_packet_reader = &cb_packet_reader;
    service->ops.cb_packet_handler = &cb_packet_handler;
    service->ops.cb_data_handler = &cb_data_handler;
    service->ops.cb_file_handler = &cb_file_handler;
    service->ops.cb_oob_handler = &cb_oob_handler;
    if(sbase->add_service(sbase, service) == 0)
        sbase->running(sbase, 0);
    else 
        fprintf(stderr, "add service failed, %s", strerror(errno));
}
