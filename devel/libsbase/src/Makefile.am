lib_LTLIBRARIES = libsbase.la
libsbase_la_SOURCES = conn.c \
conn.h \
procthread.c \
procthread.h \
sbase.c \
sbase.h \
service.c \
service.h \
utils/buffer.c \
utils/buffer.h \
utils/chunk.c \
utils/chunk.h \
utils/logger.c \
utils/logger.h \
utils/message.c \
utils/message.h \
utils/queue.c \
utils/queue.h \
utils/timer.c \
utils/timer.h 
EXTRA_DIST = utils/tree.h \
utils/cache.c \
utils/cache.h \
utils/md5.c \
utils/md5.h 

libsbase_la_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
libsbase_la_LDFLAGS = -levbase
AM_CXXFLAGS = -Wall -Wno-non-virtual-dtor
include_HEADERS = sbase.h

bin_PROGRAMS = lechod lqftp
lechod_SOURCES = lechod.c iniparser.c iniparser.h
lechod_LDADD = libsbase.la
lechod_LDFLAGS = -static

lqftp_SOURCES = lqftp.c iniparser.c iniparser.h
lqftp_LDADD = libsbase.la
lqftp_LDFLAGS = -static

