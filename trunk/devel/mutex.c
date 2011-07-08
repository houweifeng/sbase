#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "mutex.h"
static sem_t mutex = {0};
void thread_run(void *arg)
{
    int i = 0, to = (int)((long)arg);
    MUTEX_COND_WAIT(mutex);
    fprintf(stdout, "thread:%p to:%d\n", (void *)pthread_self(), to);
    for(i = 0; i < to; i++)
    {
        fprintf(stdout, "%d\n", i);
    }
    sleep(10);
    MUTEX_COND_SIGNAL(mutex);
    pthread_exit(NULL);
    return ;
}

int main()
{
    pthread_t threadid;

    MUTEX_INIT(mutex);
    int n = 3;
    if(pthread_create(&threadid, NULL, (void *)&thread_run, (void *)((long )n)) == 0)
    {
        fprintf(stdout, "%s::%d threadid:%p\n", __FILE__, __LINE__, threadid);
    }
    n = 6;
    MUTEX_COND_SIGNAL(mutex);
    if(pthread_create(&threadid, NULL, (void *)&thread_run, (void *)((long )n)) == 0)
    {
        fprintf(stdout, "%s::%d threadid:%p\n", __FILE__, __LINE__, threadid);
    }
    MUTEX_COND_SIGNAL(mutex);
    n = 8;
    if(pthread_create(&threadid, NULL, (void *)&thread_run, (void *)((long )n)) == 0)
    {
        fprintf(stdout, "%s::%d threadid:%p\n", __FILE__, __LINE__, threadid);
    }
    sleep(1);
    MUTEX_LOCK(mutex);
    fprintf(stdout, "%s::%d over\n", __FILE__, __LINE__);
    MUTEX_UNLOCK(mutex);
    return 0;

}

