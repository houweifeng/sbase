#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include "task.h"
#include "md5.h"
#include "basedef.h"
#include "mutex.h"
#include "logger.h"

/* add file transmssion task to tasktable */
int tasktable_add(TASKTABLE *tasktable, char *file, char *destfile)
{
    int fd = -1;
    int taskid = -1;
    TASK task;

    if(tasktable && tasktable->taskfile)
    {
        MUTEX_LOCK(tasktable->mutex);
        if((fd = open(tasktable->taskfile, O_CREAT|O_RDWR|O_APPEND, 0644)) > 0) 
        {
            memset(&task, 0, sizeof(TASK));
            strcpy(task.file, file);
            strcpy(task.destfile, destfile);
            task.id = tasktable->ntask;
            task.status = TASK_STATUS_WAIT;
            if(write(fd, &task, sizeof(TASK)))
            {
                taskid = tasktable->ntask;
                tasktable->ntask++;
            }
            close(fd);
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return taskid;
}

/* discard task */
int tasktable_discard(TASKTABLE *tasktable, int taskid)
{
    TASK task  = {0};
    int ret = -1;
    int fd = -1;
    unsigned long long offset = 0llu;

    if(tasktable)
    {
        MUTEX_LOCK(tasktable->mutex);
        if(taskid < tasktable->ntask)
        {
            if((fd = open(tasktable->taskfile, O_RDWR)) > 0) 
            {
                offset = taskid * sizeof(TASK) * 1llu;
                if(lseek(fd, offset, SEEK_SET) == offset)
                {
                    if(read(fd, &task, sizeof(TASK)) == sizeof(TASK) 
                            && task.id == taskid)
                    {
                        task.status = TASK_STATUS_DISCARD;
                        lseek(fd, offset, SEEK_SET);
                        write(fd, &task, sizeof(TASK));
                        ret = 0;
                    }
                }
                close(fd);
            }
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return ret;
}

/* Ready for job */
int tasktable_ready(TASKTABLE *tasktable)
{
    int i = 0;
    unsigned long long offset = 0llu, size = 0llu;
    struct stat st;
    int nblock = 0;
    int ret = -1;

    if(tasktable)
    {
        MUTEX_LOCK(tasktable->mutex);
        if(tasktable->running_task.id >= 0
                && tasktable->running_task.id < tasktable->ntask
                && stat(tasktable->running_task.file, &st) == 0
          )
        {
            DEBUG_LOGGER(tasktable->logger, "Ready transport file %s on task %d",
                tasktable->running_task.file, tasktable->running_task.id);
            //DEBUG_LOGGER(tasktable->logger, "%s\n", task->file);
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
            //for truncate commond 
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
            //dump all blocks 
            tasktable->dump_status(tasktable, -1);
            ret = 0;
        }
end:
        MUTEX_UNLOCK(tasktable->mutex);
        
    }
    return ret;
}

/* status out*/
int tasktable_statusout(TASKTABLE *tasktable, int taskid, TASK *task)
{
    int fd = -1;
    unsigned long long offset = 0llu;
    int ret = -1;

    if(tasktable && taskid >= 0 && taskid < tasktable->ntask)
    {
        if((fd = open(tasktable->taskfile, O_RDONLY)) > 0)    
        {
            offset = sizeof(TASK) * taskid * 1llu;
            if(lseek(fd, offset, SEEK_SET) == offset 
                    && read(fd, task, sizeof(TASK)) == sizeof(TASK))
            {
                ret = 0;
            }
            close(fd);
        }
    }
    return ret;
}

/* md5sum */
int tasktable_md5sum(TASKTABLE *tasktable)
{
    unsigned char md5str[MD5_LEN];
    int i = 0, ret = -1;
    char *p = NULL, *file = NULL;

    if(tasktable  && tasktable->running_task.id >= 0 
            && tasktable->running_task.id < tasktable->ntask
            && md5_file(tasktable->running_task.file, md5str) == 0)
    {
        MUTEX_LOCK(tasktable->mutex);
        p = tasktable->running_task.md5;
        for(i = 0; i < MD5_LEN; i++)
        {
            p += sprintf(p, "%02x", md5str[i]);
        }
        ret = 0;
        tasktable->dump_task(tasktable);
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
    unsigned long long offset = 0llu;

    if(tasktable && tasktable->running_task.id >= 0 
            && tasktable->running_task.id < tasktable->ntask)
    {
        if((fd = open(tasktable->taskfile, O_RDWR)) > 0)
        {
            offset = sizeof(TASK) * tasktable->running_task.id * 1llu; 
            //DEBUG_LOGGER(tasktable->logger, "Ready for dump running_task:%d offset:%llu\n", 
            //    tasktable->running_task.id, offset);
            if(lseek(fd, offset, SEEK_SET) == offset)
            {
                if(write(fd, &(tasktable->running_task), sizeof(TASK)) == sizeof(TASK))
                {
                    //DEBUG_LOGGER(tasktable->logger, "Dump task %d\n", tasktable->running_task.id);
                    ret = 0;
                }
            }
            else
            {
                DEBUG_LOGGER(tasktable->logger, "LSEEK %s offset:%llu failed, %s\n", 
                        tasktable->taskfile, offset, strerror(errno));
            }
            close(fd);
        }
        else
        {
            DEBUG_LOGGER(tasktable->logger, "dump taskfile %s failed, %s", tasktable->taskfile, strerror(errno));
        }
    }
    return ret;
}

/* resume */
int tasktable_resume_task(TASKTABLE *tasktable)
{
    int fd = -1, ret = -1;
    TASK task;

    if(tasktable)
    {
        if((fd = open(tasktable->taskfile, O_RDONLY, 0644)) > 0)
        {
            //DEBUG_LOGGER(tasktable->logger, "open file:%s fd:%d\n", tasktable->taskfile, fd);
            while(read(fd, &task, sizeof(TASK)) == sizeof(TASK))
            {
                //DEBUG_LOGGER(tasktable->logger, "read %d \n", task.id);
                tasktable->ntask++;
                if(tasktable->running_task.id >= 0) continue;
                if(task.status != TASK_STATUS_OVER)
                {
                    memcpy(&(tasktable->running_task), &task, sizeof(TASK));
                }
            }
            //DEBUG_LOGGER(tasktable->logger, "read file:%s failed, %s\n", tasktable->taskfile, strerror(errno));
            ret = 0;
            close(fd);
        }
        else
        {
            DEBUG_LOGGER(tasktable->logger, "resume taskfile:%s failed, %s", tasktable->taskfile, strerror(errno));
        }
    }
    return ret;
}

/* Dump status */
int tasktable_dump_status(TASKTABLE *tasktable, int blockid)
{
    int taskid = 0;
    int i = 0;
    int fd = 0;
    int ret = -1;
    unsigned long long offset = 0llu;

    if(tasktable && tasktable->status && (taskid = tasktable->running_task.id) >= 0)
    {
        if(blockid >= 0)
        {
            if((fd = open(tasktable->statusfile, O_RDWR)) > 0)
            {
                offset = 0llu;
                offset += sizeof(int) * 1llu;
                offset += sizeof(int) * 1llu;
                offset += sizeof(TBLOCK) * blockid * 1llu;
                if(lseek(fd, offset, SEEK_SET) == offset)
                {
                    write(fd, &(tasktable->status[i]), sizeof(TBLOCK));
                }
                //DEBUG_LOGGER(tasktable->logger, "dump block:%d on task:%d\n", blockid, taskid);
                close(fd);
                ret = 0;
            }
        }
        else
        {
            if((fd = open(tasktable->statusfile, O_CREAT|O_WRONLY, 0644)) > 0)
            {
                write(fd, &(tasktable->running_task_id), sizeof(int));
                write(fd, &(tasktable->nblock), sizeof(int));
                write(fd, tasktable->status, sizeof(TBLOCK) * tasktable->nblock);
                ret = 0;
                close(fd);
            }
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
            //DEBUG_LOGGER(tasktable->logger, "Resume running_task.id:%d nblock:%d\n", 
             //       tasktable->running_task.id, tasktable->nblock);
            if(tasktable->nblock > 0)
            {
                tasktable->status = (TBLOCK *)calloc(tasktable->nblock, sizeof(TBLOCK));
                read(fd, tasktable->status, sizeof(TBLOCK) * tasktable->nblock);
            }
            close(fd);
            return 0;
        }
    }
    return -1;
}

/* check timeout */
int tasktable_check_timeout(TASKTABLE *tasktable, unsigned long long timeout)
{
    int i = 0, n = 0;
    struct timeval tv;
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
                ++n;
            }
        }
        if(n > 0 && tasktable->running_task.id >= 0)
        {
                tasktable->running_task.timeout += n;
                tasktable->dump_task(tasktable);
        }
end:
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return n;
}

/* check task status */
int tasktable_check_status(TASKTABLE *tasktable)
{
    TASK task = {0};
    int fd = -1;
    int ret = -1;
    unsigned long long offset = 0llu;

    if(tasktable)
    {
        if(tasktable->status != NULL) return 0;
        MUTEX_LOCK(tasktable->mutex);
        //check running taskid
        if (tasktable->running_task_id < tasktable->ntask)
        {
            if((fd = open(tasktable->taskfile, O_RDONLY)) > 0) 
            {
                //DEBUG_LOGGER(tasktable->logger, "taskfile fd:%d\n", fd);
                offset = sizeof(TASK ) * tasktable->running_task_id * 1llu;
                if(lseek(fd, offset, SEEK_SET) == offset)
                {
                    while(read(fd, &(tasktable->running_task), sizeof(TASK)) == sizeof(TASK))
                    {
                        tasktable->running_task_id = tasktable->running_task.id;
                        if(tasktable->running_task.status != TASK_STATUS_OVER)
                        {
                            //DEBUG_LOGGER(tasktable->logger, "running_task taskid:%d id:%d file:%s\n", 
                            //        tasktable->running_task_id, tasktable->running_task.id,
                            //        tasktable->running_task.file);
                            ret = 0;
                            break;
                        }
                    }
                    //DEBUG_LOGGER(tasktable->logger, "taskfile fd:%d offset:%llu \n", fd, offset);
                }
                else
                {
                    DEBUG_LOGGER(tasktable->logger, "LSEEK taskfile %s offset:%llu failed, %s\n", 
                            tasktable->taskfile, offset, strerror(errno));
                }
                close(fd);
            }
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return ret;
}

/* pop block */
TBLOCK *tasktable_pop_block(TASKTABLE *tasktable, int sid, void *arg)
{
    TASK *task = NULL;
    int taskid   = 0;
    TBLOCK *block = NULL;
    int i = 0, n = 0;
    int nover = 0;
    struct timeval tv;
    

    if(tasktable && tasktable->ntask > 0)
    {
        //check status and ready 
        if(tasktable->check_status(tasktable) != 0)
        {
            //DEBUG_LOGGER(tasktable->logger, 
            //        "check running_task_id:%d id:%d ntask:%d status failed\n", 
            //        tasktable->running_task_id, tasktable->running_task.id, tasktable->ntask);
            //_exit(-1);
            return NULL;
        }
        //ready for new task
        if(tasktable->status == NULL && tasktable->ready(tasktable) != 0)
        {
            task = &(tasktable->running_task);
            DEBUG_LOGGER(tasktable->logger, "task[%d] file[%s] destfile[%s]\n",
                    task->id, task->file, task->destfile);
            return NULL;
        }
        //DEBUG_LOGGER(tasktable->logger, "running_task:%d block:%d\n", 
        //tasktable->running_task.id, tasktable->nblock);
        //no task
        MUTEX_LOCK(tasktable->mutex);
        n = 0;
        for(i = 0; i < tasktable->nblock; i++) 
        {
            if(tasktable->status[i].status != BLOCK_STATUS_OVER)
            {
                if(tasktable->status[i].cmdid == CMD_TRUNCATE)
                {
                    if(tasktable->status[i].status == BLOCK_STATUS_WORKING)
                        break;
                    if(tasktable->status[i].status != BLOCK_STATUS_OVER)
                    {
                        tasktable->status[i].status = BLOCK_STATUS_WORKING;
                        block =  &(tasktable->status[i]);
                        break;
                    }
                }
                if(tasktable->status[i].status == BLOCK_STATUS_WORKING
                        && tasktable->status[i].arg != NULL)
                {
                    n++;
                    continue;
                }
                else if(tasktable->status[i].cmdid == CMD_MD5SUM)
                {
                    if(n == 0)
                    {
                        tasktable->status[i].status = BLOCK_STATUS_WORKING;
                        block =  &(tasktable->status[i]);
                    }
                    break;
                }
                else
                {
                    tasktable->status[i].status = BLOCK_STATUS_WORKING;
                    block =  &(tasktable->status[i]);
                    break;
                }
            }
            else nover++;
        }
        //DEBUG_LOGGER(tasktable->logger, "running_task:%d nblock:%d n:%d over:%d block:%08x\n", 
        //        taskid, tasktable->nblock, n, nover, block);
        if(nover == tasktable->nblock)
        {
            task = &(tasktable->running_task);
            task->status = TASK_STATUS_OVER;
            tasktable->dump_task(tasktable);
            tasktable->running_task_id++;
        }
        if(block)
        {
            gettimeofday(&tv, NULL);
            block->times = tv.tv_sec * 1000000llu + tv.tv_usec;
            block->sid = sid;
            block->arg = arg;
            //DEBUG_LOGGER(tasktable->logger, "select block:%d\n", block->id);
        }
        MUTEX_UNLOCK(tasktable->mutex);
    }
    return block;
}

/* update status */
void tasktable_update_status(TASKTABLE *tasktable, int blockid, int status, int sid)
{
    struct timeval tv;
    unsigned long long times = 0llu;
    double speed = 0.0;

    if(tasktable && blockid >= 0 && blockid < tasktable->nblock
            && tasktable->status[blockid].status != BLOCK_STATUS_OVER
            && tasktable->status[blockid].sid == sid)
    {
        MUTEX_LOCK(tasktable->mutex);
        if(status == BLOCK_STATUS_OVER)
        {
            gettimeofday(&tv, NULL);
            times = tv.tv_sec * 1000000llu + tv.tv_usec - tasktable->status[blockid].times;
            if(times > 0llu)
            {
                tasktable->running_task.times += times;
                tasktable->running_task.bytes += tasktable->status[blockid].size;
                tasktable->dump_task(tasktable);
            }
        }

        tasktable->status[blockid].status   = status;
        tasktable->status[blockid].sid      = -1;
        tasktable->status[blockid].arg      = NULL;
        tasktable->dump_status(tasktable, blockid);
        //DEBUG_LOGGER(tasktable->logger, "Update status:%d block:%d sid:%d cmdid:%d\n",
        //      status, blockid, sid, status, blockid, sid);
        if(tasktable->status[blockid].cmdid == CMD_MD5SUM)
        {
            if(status == BLOCK_STATUS_OVER)
            {
                //DEBUG_LOGGER(tasktable->logger, "Update status:%d block:%d sid:%d cmdid:%d\n",
                //   status, blockid, sid, status, blockid, sid);
                tasktable->times += tasktable->running_task.times;
                tasktable->bytes += tasktable->running_task.bytes;
                if(tasktable->running_task.times > 0llu)
                {
                    speed = (double )(tasktable->running_task.bytes * 1000000llu
                            / tasktable->running_task.times);
                }
                DEBUG_LOGGER(tasktable->logger, "OVER %d %llu/%llu (%f) %s",
                        tasktable->running_task.id, tasktable->running_task.bytes,
                        tasktable->running_task.times, speed,
                        tasktable->running_task.file);

                tasktable->running_task.status = TASK_STATUS_OVER;
                tasktable->dump_task(tasktable);
                tasktable->free_status(tasktable);
                tasktable->running_task_id++;
            }
            else
            {
                tasktable->free_status(tasktable);
            }
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
        if((*tasktable)->status)
            free((*tasktable)->status);
        (*tasktable)->status = NULL;
        MUTEX_DESTROY((*tasktable)->mutex);
        free(*tasktable);
        (*tasktable) = NULL;
    }
}
/* Initialize tasktable */
TASKTABLE *tasktable_init(char *taskfile, char *statusfile, char *logfile)
{
    TASKTABLE *tasktable = NULL;

    if(taskfile && (tasktable = (TASKTABLE *)calloc(1, sizeof(TASKTABLE))))
    {
        tasktable->add              = tasktable_add;
        tasktable->discard          = tasktable_discard;
        tasktable->ready            = tasktable_ready;
        tasktable->check_status     = tasktable_check_status;
        tasktable->check_timeout    = tasktable_check_timeout;
        tasktable->statusout        = tasktable_statusout;
        tasktable->md5sum           = tasktable_md5sum;
        tasktable->dump_task        = tasktable_dump_task;
        tasktable->resume_task      = tasktable_resume_task;
        tasktable->dump_status      = tasktable_dump_status;
        tasktable->resume_status    = tasktable_resume_status;
        tasktable->pop_block        = tasktable_pop_block;
        tasktable->update_status    = tasktable_update_status;
        tasktable->free_status      = tasktable_free_status;
        tasktable->clean            = tasktable_clean;
        tasktable->running_task.id = -1;
        tasktable->logger = logger_init(logfile);
        strcpy(tasktable->taskfile, taskfile);
        tasktable->resume_task(tasktable);
        strcpy(tasktable->statusfile, statusfile);
        tasktable->resume_status(tasktable);
        MUTEX_INIT(tasktable->mutex);
    }
    return tasktable;
}
