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
sd.c \
service.c \
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

bin_PROGRAMS = sd lechod

sd_SOURCES = sd.c
sd_LDADD = libsbase.la
sd_LDFLAGS = -static

lechod_SOURCES = lechod.c iniparser.h iniparser.c
lechod_LDADD = libsbase.la
lechod_LDFLAGS = -static


