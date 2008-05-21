lib_LTLIBRARIES = libsbase.la
libsbase_la_SOURCES = conn.c \
conn.h \
procthread.c \
procthread.h \
sbase.c \
sbase.h \
service.c \
service.h \
utils/mutex.h \
utils/mutex.c \
utils/memb.c \
utils/memb.h \
utils/chunk.c \
utils/chunk.h \
utils/logger.c \
utils/logger.h \
utils/message.h \
utils/message.c \
utils/queue.c \
utils/queue.h \
utils/timer.c \
utils/timer.h 

libsbase_la_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
libsbase_la_LDFLAGS = -levbase
AM_CXXFLAGS = -Wall -Wno-non-virtual-dtor
include_HEADERS = sbase.h

bin_PROGRAMS = lechod sd
lechod_SOURCES = lechod.c iniparser.c iniparser.h
lechod_LDADD = libsbase.la
lechod_LDFLAGS = -static

sd_SOURCES = sd.c
sd_LDADD = libsbase.la
sd_LDFLAGS = -static

