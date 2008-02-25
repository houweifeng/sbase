#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "task.h"

/* add file transmssion task to tasktable */
int tasktable_add(TASKTABLE *tasktable, char *file, char *destfile)
{
    int taskid = -1;
    TASK *task = NULL;

    if(tasktable)
    {
        if((task = (TASK *)calloc(1, sizeof(TASK))))    
        {
            tasktable->table = (TASK **)realloc(tasktable->table, 
                    (tasktable->ntask + 1) * sizeof(TASK *));
            if(tasktable->table)
            {
                taskid = tasktable->ntask;
                strcpy(task->file, file);
                strcpy(task->destfile, destfile);
                task->id = taskid;
                task->status = TASK_STATUS_WAIT;
                tasktable->table[taskid] = task;
                tasktable->ntask++;
            }
        }
        tasktable->dump(tasktable);
    }
    return taskid;
}

/* discard task */
int tasktable_discard(TASKTABLE *tasktable, int taskid)
{
    int ret = -1;
    if(tasktable && tasktable->table)
    {
        if(taskid < tasktable->ntask && tasktable->table[taskid])
        {
            tasktable->table[taskid]->status = TASK_STATUS_DISCARD;   
            tasktable->dump(tasktable);
            ret = 0;
        }
    }
    return -1;
}

/* dump tasktable to file */
int tasktable_dump(TASKTABLE *tasktable)
{
    int fd = 0;
    int i = 0;

    if(tasktable && tasktable->ntask)
    {
        if((fd = open(tasktable->taskfile, O_CREAT|O_RDWR, 0644)) > 0)
        {
            for(i = 0; i < tasktable->ntask; i++) 
            {
                if(tasktable->table[i])
                    write(fd, tasktable->table[i], sizeof(TASK));
            }
            close(fd);
            return 0;
        }
    }
    return -1;
}

/* resume */
int tasktable_resume_task(TASKTABLE *tasktable)
{
    int ntask = 0;
    int fd = -1, ret = -1;
    TASK task, *ptask = NULL;

    if(tasktable)
    {
        if((fd = open(tasktable->taskfile, O_RDONLY)) > 0)
        {
            if(read(fd, &ntask, sizeof(int)) > 0 
                    && (tasktable->table = (TASK **)calloc(ntask, sizeof(TASK *))))
            {
                tasktable->ntask = ntask;
                while(read(fd, &task, sizeof(TASK)) > 0)
                {

                    if((ptask = (TASK *)calloc(1, sizeof(TASK)))) 
                    {
                        memcpy(ptask, &task, sizeof(TASK));
                        tasktable->table[ptask->id] = ptask;
                    }
                }
                ret = 0;
            }
            close(fd);
        }
    }
    return ret;
}

/* Clean tasktable */
void tasktable_clean(TASKTABLE **tasktable)
{
    if(*tasktable)
    {
        if((*tasktable)->table)
            free((*tasktable)->table);
        free(*tasktable);
        (*tasktable) = NULL;
    }
}
/* Initialize tasktable */
TASKTABLE *tasktable_init(char *taskfile, char *statusfile)
{
    TASKTABLE *tasktable = NULL;

    if(taskfile && (tasktable = (TASKTABLE *)calloc(1, sizeof(TASKTABLE))))
    {
        tasktable->add          = tasktable_add;
        tasktable->discard      = tasktable_discard;
        tasktable->dump         = tasktable_dump;
        tasktable->resume_task  = tasktable_resume_task;
        tasktable->clean        = tasktable_clean;
        strcpy(tasktable->taskfile, taskfile);
        tasktable->resume_task(tasktable);
        strcpy(tasktable->statusfile, statusfile);
        //tasktable->resume_task(tasktable);
    }
    return tasktable;
}
