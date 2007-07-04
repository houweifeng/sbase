bin_PROGRAMS = lhttpd
lhttpd_SOURCES = lhttpd.h lhttpd.c \
mystring.c mystring.h \
iniparser.c  iniparser.h log.h
lhttpd_CPPFLAGS   = -D_FILE_OFFSET_BITS=64
lhttpd_LDFLAGS   =  -lsbase
AM_CXXFLAGS = -Wall -lsbase -Wno-non-virtual-dtor -D_FILE_OFFSET_BITS=64

