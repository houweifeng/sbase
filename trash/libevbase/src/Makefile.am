lib_LTLIBRARIES = libevbase.la
libevbase_la_SOURCES = evdevpoll.c \
evdevpoll.h \
evbase.c \
evbase.h \
evepoll.c \
evepoll.h \
evkqueue.c \
evkqueue.h \
evpoll.c \
evpoll.h \
evport.c \
evport.h \
evrtsig.c \
evrtsig.h \
evselect.c \
evselect.h \
evwin32.c \
evwin32.h \
logger.h \
xmm.h 	\
xmm.c 	\
mutex.h 

libevbase_la_CPPFLAGS = -Wall
include_HEADERS = evbase.h
bin_PROGRAMS = ev_serv ev_cli evdns_cli
ev_serv_SOURCES = ev_serv.c evbase.h log.h
ev_serv_CPPFLAGS = -Wall
ev_serv_LDFLAGS = -static
ev_serv_LDADD = libevbase.la
ev_cli_SOURCES = ev_cli.c evbase.h log.h
ev_cli_CPPFLAGS = -Wall
ev_cli_LDFLAGS = -static
ev_cli_LDADD = libevbase.la
evdns_cli_SOURCES = evdns_cli.c evbase.h log.h evdns.h evdns.c timer.h
evdns_cli_CPPFLAGS = -Wall
evdns_cli_LDFLAGS = -static
evdns_cli_LDADD = libevbase.la
