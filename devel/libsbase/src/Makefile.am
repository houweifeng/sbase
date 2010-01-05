lib_LTLIBRARIES = libsbase.la
libsbase_la_SOURCES = conn.h \
message.h \
procthread.h \
sbase.h \
service.h \
conn.c \
message.c \
procthread.c \
sbase.c \
service.c \
utils/xssl.h 	\
utils/chunk.h \
utils/logger.h \
utils/memb.h \
utils/mutex.h \
utils/queue.h \
utils/evtimer.h \
utils/timer.h

libsbase_la_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
libsbase_la_LDFLAGS = -levbase
AM_CXXFLAGS = -Wall -Wno-non-virtual-dtor
include_HEADERS = sbase.h

bin_PROGRAMS = xhttpd lechod

xhttpd_SOURCES = xhttpd.c mime.h mime.c http.h http.c iniparser.h iniparser.c \
				 utils/trie.h utils/trie.c utils/stime.h utils/stime.c
xhttpd_CPPFLAGS = -I utils
xhttpd_LDADD = libsbase.la
xhttpd_LDFLAGS = -static

lechod_SOURCES = lechod.c iniparser.h iniparser.c
lechod_LDADD = libsbase.la
lechod_LDFLAGS = -static
