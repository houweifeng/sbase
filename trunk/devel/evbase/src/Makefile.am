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
evwin32.h
AM_CXXFLAGS = -Wall -Wno-non-virtual-dtor
include_HEADERS = evbase.h
bin_PROGRAMS = ev_server ev_client
ev_server_SOURCES = ev_server.c evbase.h
ev_server_LDFLAGS = -static
ev_server_LDADD = libevbase.la
ev_client_SOURCES = ev_client.c evbase.h
ev_client_LDFLAGS = -static
ev_client_LDADD = libevbase.la

