#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "task.h"
#include "md5.h"
#include "basedef.h"
#include "mutex.h"

/* add file transmssion task to tasktable */
int tasktable_add(TASKTABLE *tasktable, char *file, char *destfile)
{
    int taskid = -1;
    TASK *task = NULL;

    if(tasktable)
    {
        MUTEX_LOCK(tasktable->mutex);
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
        MUTEX_UNLOCK(tasktable->mutex);
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
        MUTEX_LOCK(tasktable->mutex);
        if(taskid < tasktable->ntask && tasktable->table[taskid])
        {
            tasktable->table[taskid]->status = TASK_STATUS_DISCARD;   
            tasktable->dump_task(tasktable);
            ret = 0;
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return ret;
}

/* Ready for job */
int tasktable_ready(TASKTABLE *tasktable, int taskid)
{
    int i = 0;
    unsigned long long offset = 0llu, size = 0llu;
    struct stat st;
    int nblock = 0;
    TASK *task = NULL;
    int ret = -1;

    if(tasktable)
    {
        MUTEX_LOCK(tasktable->mutex);
        if(taskid >= 0 && taskid < tasktable->ntask
                && tasktable->table   
                && (task = tasktable->table[taskid])
                && stat(task->file, &st) == 0
          )
        {
            //fprintf(stdout, "%s\n", task->file);
            nblock = (st.st_size / tasktable->block_size);
            if((st.st_size % tasktable->block_size) != 0) ++nblock;
            //for truncate 
            ++nblock;
            //for md5sum
            ++nblock;
            if((tasktable->status = (TBLOCK *)realloc(
                            tasktable->status, sizeof(TBLOCK) * nblock)) == NULL)
            {
                tasktable->nblock = 0;
                ret = -1;
                goto end;
            }
            tasktable->nblock = nblock;
            memset(tasktable->status, 0, sizeof(TBLOCK) * nblock);
            size = st.st_size * 1llu;
            i = 0;
            //for truncate 
            tasktable->status[i].offset = 0llu;
            tasktable->status[i].size = st.st_size * 1llu;
            tasktable->status[i].id = i;
            tasktable->status[i].cmdid = CMD_TRUNCATE;
            i++;
            //for block list
            offset = 0llu;
            while(size > 0llu )
            {
                tasktable->status[i].offset     = offset * 1llu;
                tasktable->status[i].size       = tasktable->block_size * 1llu;
                if(size < tasktable->block_size * 1llu)
                    tasktable->status[i].size   = size;
                size -= tasktable->status[i].size;
				offset += tasktable->status[i].size;
                tasktable->status[i].cmdid      = CMD_PUT;
                tasktable->status[i].id   = i++;
            }
			//last block for md5sum
            tasktable->status[i].offset = 0llu;
            tasktable->status[i].size = st.st_size * 1llu;
            tasktable->status[i].id = i;
            tasktable->status[i].cmdid = CMD_MD5SUM;
            tasktable->dump_status(tasktable);
            ret = 0;
        }
end:
        MUTEX_UNLOCK(tasktable->mutex);
        
    }
    return ret;
}

/* md5sum */
int tasktable_md5sum(TASKTABLE *tasktable, int taskid)
{
    unsigned char md5str[MD5_LEN];
    int i = 0, ret = -1;
    char *p = NULL, *file = NULL;

    if(tasktable && taskid >= 0 
            && taskid < tasktable->ntask 
            && tasktable->table
            && tasktable->table[taskid] 
            && md5_file(tasktable->table[taskid]->file, md5str) == 0)
    {
        MUTEX_LOCK(tasktable->mutex);
        p = tasktable->table[taskid]->md5;
        for(i = 0; i < MD5_LEN; i++)
        {
            p += sprintf(p, "%02x", md5str[i]);
        }
        ret = 0;
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return ret;
}

/* dump tasktable to file */
int tasktable_dump_task(TASKTABLE *tasktable)
{
    int fd = 0;
    int i = 0;
    int ret = -1;

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
            ret = 0;
        }
    }
    return ret;
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
    int ret = -1;

    if(tasktable && tasktable->status && (taskid = tasktable->running_task_id) >= 0)
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
            ret = 0;
        }
    }
    return ret;
}

/* Resume status */
int tasktable_resume_status(TASKTABLE *tasktable)
{
    int i = -1;
    int fd = -1;

    if(tasktable)
    {
        if((fd = open(tasktable->statusfile, O_RDONLY)) > 0)  
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

int tasktable_check_timeout(TASKTABLE *tasktable, unsigned long long timeout)
{
    int i = 0, n = 0;
    struct timeval tv;
    TASK *task = NULL;
    unsigned long long times = 0llu;

    if(tasktable && tasktable->status)
    {
        MUTEX_LOCK(tasktable->mutex);
        gettimeofday(&tv, NULL);
        times = tv.tv_sec * 1000000llu + tv.tv_usec;
        for(i = 0; i < tasktable->nblock; i++)  
        {
            if(tasktable->status[i].status == BLOCK_STATUS_WORKING
                    && (times - tasktable->status[i].times) >= timeout) 
            {
                tasktable->status[i].status = BLOCK_STATUS_TIMEOUT;
                tasktable->status[i].sid    = -1;
                ++n;
            }
        }
        if(n > 0 && tasktable->table && tasktable->running_task_id >= 0)
        {
            if((task = tasktable->table[tasktable->running_task_id]))
            {
                task->timeout += n;
                tasktable->dump_task(tasktable);
            }
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return n;
}

int tasktable_check_status(TASKTABLE *tasktable)
{
    int taskid = 0;
    int ret = -1;

    if(tasktable && tasktable->table)
    {
        MUTEX_LOCK(tasktable->mutex);
        //check running taskid
        if(tasktable->running_task_id >= 0 
                && tasktable->running_task_id < tasktable->ntask)
            taskid = tasktable->running_task_id;
        while(taskid < tasktable->ntask)
        {
            if(tasktable->table[taskid] 
                    && tasktable->table[taskid]->status != TASK_STATUS_OVER)
            {
                tasktable->running_task_id = taskid;
                ret = 0;
                break;
            }
            taskid++;
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return ret;
}

/* pop block */
TBLOCK *tasktable_pop_block(TASKTABLE *tasktable, int sid)
{
    int taskid   = 0;
    TBLOCK *block = NULL;
    int i = 0, n = 0;
    struct timeval tv;
    

    if(tasktable && tasktable->table && tasktable->ntask > 0)
    {
        //check status and ready 
        if(tasktable->check_status(tasktable) != 0)
            return NULL;
        taskid = tasktable->running_task_id;
        if(tasktable->status == NULL)
        {
            tasktable->ready(tasktable, taskid);
        }
        //no task
        MUTEX_LOCK(tasktable->mutex);
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
				else
				{
                	tasktable->status[i].status = BLOCK_STATUS_WORKING;
                	block =  &(tasktable->status[i]);
                	break;
				}
            }
        }
        if(block)
        {
            gettimeofday(&tv, NULL);
            block->times = tv.tv_sec * 1000000llu + tv.tv_usec;
            block->sid = sid;
        }
		//if(block)fprintf(stdout, "%d:block:%08x:%llu:%llu\n", i, block, block->offset, block->size);
		//else fprintf(stdout, "no block\n");
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return block;
}

/* update status */
void tasktable_update_status(TASKTABLE *tasktable, int blockid, int status, int sid)
{
    int taskid = -1;
    
    if(tasktable && blockid >= 0 && blockid < tasktable->nblock
			&& tasktable->status[blockid].status != BLOCK_STATUS_OVER
            && tasktable->status[blockid].sid == sid)
    {
        MUTEX_LOCK(tasktable->mutex);
        tasktable->status[blockid].status = status;
        if(blockid == (tasktable->nblock - 1) 
                && tasktable->status[blockid].cmdid == CMD_MD5SUM)
        {
            if(status == BLOCK_STATUS_OVER)
            {
                taskid = tasktable->running_task_id;
                tasktable->table[taskid]->status = TASK_STATUS_OVER;
                tasktable->free_status(tasktable);
                tasktable->running_task_id++;
                tasktable->dump_task(tasktable);
            }
            {
                tasktable->free_status(tasktable);
            }
        }
        else
        {
            tasktable->dump_status(tasktable);
        }
        MUTEX_UNLOCK(tasktable->mutex);
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
		unlink(tasktable->statusfile);
    }
}

/* Clean tasktable */
void tasktable_clean(TASKTABLE **tasktable)
{
    if(*tasktable)
    {
        if((*tasktable)->table)
            free((*tasktable)->table);
        MUTEX_DESTROY((*tasktable)->mutex);
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
        tasktable->check_status     = tasktable_check_status;
        tasktable->check_timeout    = tasktable_check_timeout;
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
        MUTEX_INIT(tasktable->mutex);
    }
    return tasktable;
}
