#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include "sbase.h"

int main()
{
    SBASE *sbase = sbase_init();
//    sbase->working_mode = 0;
  //  sbase->max_procthreads = 1;
    sbase->usec_sleep = 100;
    //sbase->setrlimit(sbase, "RLIMIT_NOFILE", RLIMIT_NOFILE, SB_CONN_MAX);
    sbase->setrlimit(sbase, "RLIMIT_NOFILE", RLIMIT_NOFILE, 65536);
    SERVICE *service = NULL;
    if((service = service_init()))
    {
	    service->service_type = S_SERVICE;
	    service->family = AF_INET;
	    service->sock_type = SOCK_STREAM;
	    service->service_name = "sd";
	    service->ip = NULL;
	    service->port = 1418;
	    service->set_log(service, "/tmp/sd.log");
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
