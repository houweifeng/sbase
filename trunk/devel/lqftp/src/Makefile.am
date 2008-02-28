bin_PROGRAMS = lqftp lqftpd 

lqftp_SOURCES = lqftp.c iniparser.c iniparser.h task.c task.h \
				utils/md5.c utils/md5.h basedef.h utils/logger.h utils/logger.c
lqftp_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
lqftp_LDFLAGS = -lsbase -static

lqftpd_SOURCES = lqftpd.c iniparser.c iniparser.h utils/md5.c utils/md5.h basedef.h \
				 utils/timer.c utils/timer.h
lqftpd_CPPFLAGS = -I utils -D_FILE_OFFSET_BITS=64
lqftpd_LDFLAGS = -lsbase -static


