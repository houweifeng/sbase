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
utils/chunk.c \
utils/logger.h \
utils/mmblock.h \
utils/mmblock.c \
utils/mutex.h \
utils/queue.h \
utils/queue.c \
utils/evtimer.h \
utils/timer.h

libsbase_la_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64 -Wall
libsbase_la_LDFLAGS = -levbase
include_HEADERS = sbase.h

sbin_PROGRAMS = xhttpd lechod

xhttpd_SOURCES = xhttpd.c mime.h mime.c http.h http.c iniparser.h iniparser.c \
				 utils/trie.h utils/trie.c utils/stime.h utils/stime.c utils/logger.h
xhttpd_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64 -Wall 
xhttpd_LDADD = libsbase.la
xhttpd_LDFLAGS = -static

lechod_SOURCES = lechod.c iniparser.h iniparser.c
lechod_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64 -Wall 
lechod_LDADD = libsbase.la
lechod_LDFLAGS = -static

bin_PROGRAMS = wbenchmark

wbenchmark_CPPFLAGS = -I utils/ -Wall 
wbenchmark_LDADD = libsbase.la
wbenchmark_LDFLAGS = -static
