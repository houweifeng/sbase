#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef _TASK_H
#define _TASK_H
#ifndef PATH_MAX_SIZE
#define PATH_MAX_SIZE 256
#endif
#define TASK_STATUS_WAIT        0x00
#define TASK_STATUS_OVER        0x01
#define TASK_STATUS_TIMEOUT     0x02
#define TASK_STATUS_DISCARD     0x04

typedef struct _TASK
{
    int id;
    int time;
    int nretry;
    int status;

    char file[PATH_MAX_SIZE];
    char destfile[PATH_MAX_SIZE];
    int  nblock;
}TASK;

typedef struct _TASKTABLE
{
    TASK **table;
    int ntask;
    char taskfile[PATH_MAX_SIZE];
    
    int     (*add)(struct _TASKTABLE *, char *file, char *destfile);
    int     (*discard)(struct _TASKTABLE *, int taskid);
    int     (*dump)(struct _TASKTABLE *);
    int     (*resume)(struct _TASKTABLE *);
    void    (*clean)(struct _TASKTABLE **);

}TASKTABLE;

/* Initialize tasktable */
TASKTABLE *tasktable_init(char *taskfile);
#endif
