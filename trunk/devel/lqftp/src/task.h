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
    unsigned long long times;
    unsigned long long offset;
    unsigned long long size;
}TBLOCK;

typedef struct _TASKTABLE
{
    TASK **table;
    int ntask;
    char taskfile[PATH_MAX_SIZE];
    int running_task_id;
    TBLOCK *status;
    int  nblock;
    char statusfile[PATH_MAX_SIZE];
    void *mutex;
    
    int     (*add)(struct _TASKTABLE *, char *file, char *destfile);
    int     (*ready)(struct _TASKTABLE *, int taskid);
    int     (*check_timeout)(struct _TASKTABLE *, unsigned long long );
    int     (*md5sum)(struct _TASKTABLE *, int taskid);
    int     (*discard)(struct _TASKTABLE *, int taskid);
    int     (*dump_task)(struct _TASKTABLE *);
    int     (*resume_task)(struct _TASKTABLE *);
    int     (*dump_status)(struct _TASKTABLE *);
    int     (*resume_status)(struct _TASKTABLE *);
    TBLOCK  *(*pop_block)(struct _TASKTABLE *, int);
    void    (*update_status)(struct _TASKTABLE *, int , int, int);
    void    (*free_status)(struct _TASKTABLE *);
    void    (*clean)(struct _TASKTABLE **);

}TASKTABLE;

/* Initialize tasktable */
TASKTABLE *tasktable_init(char *taskfile, char *statusfile);
#endif
