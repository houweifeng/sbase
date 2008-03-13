#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "basedef.h"
#ifndef _TASK_H
#define _TASK_H
#define BLOCK_STATUS_READY      0x00
#define BLOCK_STATUS_WORKING    0x01
#define BLOCK_STATUS_ERROR      0x02
#define BLOCK_STATUS_OVER       0x04
#define BLOCK_STATUS_TIMEOUT    0x08

#define TASK_STATUS_WAIT        0x00
#define TASK_STATUS_OVER        0x01
#define TASK_STATUS_TIMEOUT     0x02
#define TASK_STATUS_DISCARD     0x04

typedef struct _TASK
{
    int id;
    int timeout;
    int nretry;
    int status;
    unsigned long long times;
    unsigned long long bytes;

    char md5[MD5SUM_SIZE + 1];
    char file[PATH_MAX_SIZE];
    char destfile[PATH_MAX_SIZE];
}TASK;
typedef struct _TBLOCK
{
    int id;
    int cmdid;
    int status;
    int sid;
    void *arg;
    unsigned long long times;
    unsigned long long offset;
    unsigned long long size;
}TBLOCK;

typedef struct _TASKTABLE
{
    int block_size;
    char taskfile[PATH_MAX_SIZE];
    int ntask;
    TASK running_task;
    int  running_task_id;
    TBLOCK *status;
    int  nblock;
    char statusfile[PATH_MAX_SIZE];
    void *mutex;
    void *logger;
    unsigned long long times;
    unsigned long long bytes;


    
    int     (*add)(struct _TASKTABLE *, char *file, char *destfile);
    int     (*ready)(struct _TASKTABLE *);
    int     (*check_timeout)(struct _TASKTABLE *, unsigned long long );
    int     (*check_status)(struct _TASKTABLE *);
    int     (*md5sum)(struct _TASKTABLE *);
    int     (*statusout)(struct _TASKTABLE *, int taskid, TASK *);
    int     (*discard)(struct _TASKTABLE *, int taskid);
    int     (*dump_task)(struct _TASKTABLE *);
    int     (*resume_task)(struct _TASKTABLE *);
    int     (*dump_status)(struct _TASKTABLE *, int);
    int     (*resume_status)(struct _TASKTABLE *);
    TBLOCK  *(*pop_block)(struct _TASKTABLE *, int, void *);
    void    (*update_status)(struct _TASKTABLE *, int , int, int);
    void    (*free_status)(struct _TASKTABLE *);
    void    (*clean)(struct _TASKTABLE **);

}TASKTABLE;

/* Initialize tasktable */
TASKTABLE *tasktable_init(char *taskfile, char *statusfile, char *logfile);
#endif
