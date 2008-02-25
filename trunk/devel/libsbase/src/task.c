#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "task.h"
#include "md5.h"
#include "basedef.h"

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
        tasktable->dump_task(tasktable);
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
            tasktable->dump_task(tasktable);
            ret = 0;
        }
    }
    return -1;
}

/* Ready for job */
int tasktable_ready(TASKTABLE *tasktable, int taskid)
{
    int i = 0;
    unsigned long long offset = 0llu, size = 0llu;
    struct stat st;
    int nblock = 0;
    TASK *task = NULL;

    if(tasktable)
    {
        if(taskid >= 0 && taskid < tasktable->ntask
                && tasktable->table   
                && (task = tasktable->table[taskid])
                && stat(task->file, &st) == 0
          )
        {
            nblock = (st.st_size / TBLOCK_SIZE);
            if((st.st_size % TBLOCK_SIZE) != 0) ++nblock;
            //for md5sum
            ++nblock;
            if((tasktable->status = (TBLOCK *)realloc(
                            tasktable->status, sizeof(TBLOCK) * nblock)) == NULL)
            {
                tasktable->nblock = 0;
                return -1;
            }
            tasktable->nblock = nblock;
            memset(tasktable->status, 0, sizeof(TBLOCK) * nblock);
            size = st.st_size * 1llu;
            i = 0;
            offset = 0llu;
            while(size > 0llu )
            {
                tasktable->status[i].offset     = offset * 1llu;
                tasktable->status[i].size       = TBLOCK_SIZE * 1llu;
                if(size < TBLOCK_SIZE * 1llu)
                    tasktable->status[i].size   = size;
                size -= tasktable->status[i].size;
                tasktable->status[i].cmdid      = CMD_PUT;
                tasktable->status[i].id   = i++;
            }
            tasktable->status[i].cmdid = CMD_MD5SUM;
            tasktable->dump_status(tasktable);
            return 0;
        }
    }
    return -1;
}

/* md5sum */
int tasktable_md5sum(TASKTABLE *tasktable, int taskid)
{
    unsigned char md5str[_MD5_N];
    int i = 0;
    char *p = NULL, *file = NULL;

    if(tasktable && taskid >= 0 
            && taskid < tasktable->ntask 
            && tasktable->table
            && tasktable->table[taskid] 
            && md5_file(tasktable->table[taskid]->file, md5str) == 0)
    {
        p = tasktable->table[taskid]->md5;
        for(i = 0; i < _MD5_N; i++)
        {
            p += sprintf(p, "%02x", md5str[i]);
        }
        return 0;
    }
    return -1;
}

/* dump tasktable to file */
int tasktable_dump_task(TASKTABLE *tasktable)
{
    int fd = 0;
    int i = 0;

    if(tasktable && tasktable->ntask > 0)
    {
        if((fd = open(tasktable->taskfile, O_CREAT|O_RDWR, 0644)) > 0)
        {
            write(fd, &(tasktable)->ntask, sizeof(int));
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

/* Dump status */
int tasktable_dump_status(TASKTABLE *tasktable)
{
    int taskid = 0;
    int i = 0;
    int fd = 0;

    if(tasktable && tasktable->status && (taskid = tasktable->running_task_id) > 0)
    {
        if((fd = open(tasktable->statusfile, O_CREAT|O_WRONLY, 0644)) > 0)
        {
            write(fd, &(taskid), sizeof(int));
            write(fd, &(tasktable->nblock), sizeof(int));
            for(i = 0; i < tasktable->nblock; i++)
            {
                write(fd, &(tasktable->status[i]), sizeof(TBLOCK));
            }
            close(fd);
            return 0;
        }
    }
    return -1;
}

/* Resume status */
int tasktable_resume_status(TASKTABLE *tasktable)
{
    int i = -1;
    int fd = -1;

    if(tasktable)
    {
        if((fd = open(tasktable->statusfile, O_RDONLY)))  
        {
            read(fd, &(tasktable->running_task_id), sizeof(int));
            read(fd, &(tasktable->nblock), sizeof(int));
            if(tasktable->nblock > 0)
            {
                tasktable->status = (TBLOCK *)calloc(tasktable->nblock, sizeof(TBLOCK));
                for(i = 0; i < tasktable->nblock; i++)
                {
                    read(fd, &(tasktable->status[i]), sizeof(TBLOCK));
                }
            }
            close(fd);
            return 0;
        }
    }
    return -1;
}

/* pop block */
TBLOCK *tasktable_pop_block(TASKTABLE *tasktable)
{
    TBLOCK *block = NULL;
    int i = 0, n = 0;

    if(tasktable && tasktable->status)
    {
        for(i = 0; i < tasktable->nblock; i++)  
        {
            if(tasktable->status[i].status != BLOCK_STATUS_OVER)
            {
                if(tasktable->status[i].status == BLOCK_STATUS_WORKING) 
                {
                    ++n;
                    continue;
                }
                if(tasktable->status[i].cmdid == CMD_MD5SUM && n > 0)
                    break;
                tasktable->status[i].status = BLOCK_STATUS_WORKING;
                block =  &(tasktable->status[i]);
                break;
            }
        }
    }
    return block;
}

/* update status */
void tasktable_update_status(TASKTABLE *tasktable, int blockid, int status)
{
    int taskid = -1;
    if(tasktable && blockid >= 0 
            && blockid < tasktable->nblock)
    {
        tasktable->status[blockid].status = status;
        if(blockid == (tasktable->nblock - 1) 
                && status == BLOCK_STATUS_OVER
                && tasktable->status[blockid].cmdid == CMD_MD5SUM)
        {
            taskid = tasktable->running_task_id;
            tasktable->table[taskid]->status = TASK_STATUS_OVER;
            tasktable->free_status(tasktable);
        }
        else
            tasktable->dump_status(tasktable);
    }
    return ;
}

/* free status */
void tasktable_free_status(TASKTABLE *tasktable)
{
    if(tasktable)
    {
        if(tasktable->status)
            free(tasktable->status);
        tasktable->status = NULL;
        tasktable->nblock = 0;
    }
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
        tasktable->add              = tasktable_add;
        tasktable->discard          = tasktable_discard;
        tasktable->ready            = tasktable_ready;
        tasktable->md5sum           = tasktable_md5sum;
        tasktable->dump_task        = tasktable_dump_task;
        tasktable->resume_task      = tasktable_resume_task;
        tasktable->dump_status      = tasktable_dump_status;
        tasktable->resume_status    = tasktable_resume_status;
        tasktable->pop_block        = tasktable_pop_block;
        tasktable->update_status    = tasktable_update_status;
        tasktable->free_status      = tasktable_free_status;
        tasktable->clean            = tasktable_clean;
        strcpy(tasktable->taskfile, taskfile);
        tasktable->resume_task(tasktable);
        strcpy(tasktable->statusfile, statusfile);
        tasktable->resume_task(tasktable);
    }
    return tasktable;
}
