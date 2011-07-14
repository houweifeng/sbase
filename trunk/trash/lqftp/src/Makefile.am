bin_PROGRAMS = lqftp lqftpd 
lqftp_SOURCES = lqftp.c iniparser.c iniparser.h task.c task.h common.h common.c \
				utils/md5.c utils/md5.h basedef.h utils/logger.h utils/mutex.h
lqftp_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
lqftp_LDFLAGS = -lsbase

lqftpd_SOURCES = lqftpd.c iniparser.c iniparser.h utils/md5.c utils/md5.h basedef.h \
				 utils/timer.h utils/logger.h common.h common.c
lqftpd_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
lqftpd_LDFLAGS = -lsbase



