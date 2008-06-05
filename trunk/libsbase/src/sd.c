#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include "sbase.h"
int sd_packet_handler(CONN *conn, CB_DATA *packet)
{
    return conn->push_chunk(conn, packet->data, packet->ndata);
}
int sd_data_handler(CONN *conn, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk)
{
    return 0;
}
int main()
{
    SESSION session = {0};
    SBASE *sbase = sbase_init();
    sbase->working_mode = 0;
    sbase->ndaemons = 1;
//    sbase->working_mode = 0;
  //  sbase->max_procthreads = 1;
    sbase->usec_sleep = 1000;
    sbase->connections_limit = 65536;
    //sbase->setrlimit(sbase, "RLIMIT_NOFILE", RLIMIT_NOFILE, SB_CONN_MAX);
    if(sbase->setrlimit(sbase, "RLIMIT_NOFILE", RLIMIT_NOFILE, 10000) == -1)
    {
        fprintf(stderr, "set rlimit failed, %s\n", strerror(errno));
        _exit(-1);
    }
	sbase->set_log(sbase, "/tmp/sd.log");
	sbase->set_evlog(sbase, "/tmp/evsd.log");
    SERVICE *service = NULL;
    if((service = service_init()))
    {
        service->working_mode = 1;
        service->nprocthreads = 4;
        service->ndaemons = 4;
	    service->service_type = S_SERVICE;
	    service->family = AF_INET;
	    service->sock_type = SOCK_STREAM;
	    service->service_name = "sd";
	    service->ip = NULL;
	    service->port = 1418;
        session.packet_type = PACKET_DELIMITER;
        session.packet_delimiter = "\r\n\r\n";
        session.packet_delimiter_length = 4;
        session.packet_handler = &sd_packet_handler;
        session.buffer_size = 2097152;
        service->set_session(service, &session);
    }
/*
    service->max_procthreads = 2;
    service->sleep_usec = 10000;
    service->heartbeat_interval = 100000;
    service->max_connections = 65536;
    service->packet_type = PACKET_DELIMITER;
    service->packet_delimiter = "\r\n";
    service->packet_delimiter_length = 2;
    service->buffer_size = 1024 * 1024;
    service->ops.cb_packet_reader = &cb_packet_reader;
    service->ops.cb_packet_handler = &cb_packet_handler;
    service->ops.cb_data_handler = &cb_data_handler;
    service->ops.cb_file_handler = &cb_file_handler;
    service->ops.cb_oob_handler = &cb_oob_handler;
*/
    if(sbase->add_service(sbase, service) == 0)
        sbase->running(sbase, 0);
    else 
        fprintf(stderr, "add service failed, %s", strerror(errno));
}
