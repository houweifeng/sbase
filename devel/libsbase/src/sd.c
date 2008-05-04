#include <stdio.h>
#include <unistd.h>
#include <sbase.h>
#include <errno.h>

int cb_packet_reader(CONN *conn, BUFFER *buffer)
{
}

void cb_packet_handler(CONN *conn, BUFFER *packet)
{
            if(conn && conn->push_chunk)
                                conn->push_chunk((CONN *)conn, ((BUFFER *)packet)->data, packet->size);
}

void cb_data_handler(CONN *conn, BUFFER *packet, CHUNK *chunk, BUFFER *cache)
{
}

void cb_oob_handler(CONN *conn, BUFFER *oob)
{
}

int main()
{
    SBASE *sbase = sbase_init();
    sbase->working_mode = 1;
    sbase->sleep_usec = 10000;
    sbase->set_log(sbase, "/tmp/abcd.log");
    SERVICE *service = service_init();
    service->family = AF_INET;
    service->socket_type = SOCK_STREAM;
    service->service_type = S_SERVICE;
    service->ip=NULL;
    service->logfile = "/tmp/abc.log";
    service->port=62800;
    service->working_mode = 1;
    service->max_procthreads = 2;
    service->sleep_usec = 10000;
    service->max_connections = 1024;
    service->packet_type = PACKET_DELIMITER;
    service->packet_delimiter = "\r\n";
    service->buffer_size = 1024 * 1024;
    service->cb_packet_reader = &cb_packet_reader;
    service->cb_packet_handler = &cb_packet_handler;
    service->cb_data_handler = &cb_data_handler;
    service->cb_oob_handler = &cb_oob_handler;
    if(sbase->add_service(sbase, service) == 0)
        sbase->running(sbase, 0);
    else 
        fprintf(stderr, "add service failed, %s", strerror(errno));
}
