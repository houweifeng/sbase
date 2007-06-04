#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "timer.h"
/* Initialize timer */
TIMER *timer_init()
{
	TIMER *timer = (TIMER *)calloc(1, sizeof(TIMER));	
	if(timer)
	{
		gettimeofday(&(timer->tv), NULL);
		timer->start_sec	= timer->tv.tv_sec;
		timer->start_usec	= timer->tv.tv_sec * 1000000llu + timer->tv.tv_usec * 1llu;
		timer->last_sec		= timer->start_sec;
		timer->last_usec	= timer->start_usec;
		timer->sample		= timer_sample;
		timer->reset		= timer_reset;
		timer->check		= timer_check;
		timer->clean		= timer_clean;
#ifdef HAVE_PTHREAD_H
		pthread_mutex_init(&(timer->mutex), NULL);
#endif
	}
	return timer;
}

/* Reset timer */
void timer_reset(TIMER *timer)
{
	if(timer)
	{
#ifdef HAVE_PTHREAD_H
                pthread_mutex_lock(&(timer->mutex));
#endif
		gettimeofday(&(timer->tv), NULL);
                timer->start_sec        = timer->tv.tv_sec;
                timer->start_usec       = timer->tv.tv_sec * 1000000llu + timer->tv.tv_usec * 1llu;
		timer->last_sec         = timer->start_sec;
                timer->last_usec        = timer->start_usec;
#ifdef HAVE_PTHREAD_H
                pthread_mutex_unlock(&(timer->mutex));
#endif
	}
}

/* TIMER  sample */
void timer_sample(TIMER *timer)
{
	if(timer)
	{
#ifdef HAVE_PTHREAD_H
                pthread_mutex_lock(&(timer->mutex));
#endif
		gettimeofday(&(timer->tv), NULL);	
		timer->last_sec_used    = timer->tv.tv_sec - timer->last_sec;
                timer->last_usec_used   = timer->tv.tv_sec * 1000000llu 
						+ timer->tv.tv_usec - timer->last_usec;
		timer->last_sec         = timer->tv.tv_sec;
                timer->last_usec        = timer->tv.tv_sec * 1000000llu + timer->tv.tv_usec;
		timer->sec_used  	= timer->tv.tv_sec - timer->start_sec;
		timer->usec_used 	= timer->last_usec - timer->start_usec;
#ifdef HAVE_PTHREAD_H
                pthread_mutex_unlock(&(timer->mutex));
#endif
	}
}

/* Check timer and Run callback */
void timer_check(TIMER *timer, uint32_t interval)
{
	uint64_t n = 0llu;
	if(timer)
	{
#ifdef HAVE_PTHREAD_H
		pthread_mutex_lock(&(timer->mutex));
#endif
                gettimeofday(&(timer->tv), NULL);       
                n = (timer->tv.tv_sec * 1000000llu + timer->tv.tv_usec);
                if( (n - timer->last_usec) >= interval  && timer->callback)
                {
                        timer->callback();
                        timer->last_sec_used    = timer->tv.tv_sec - timer->last_sec;
                        timer->last_usec_used   = n - timer->last_usec;
                        timer->last_sec         = timer->tv.tv_sec;
                        timer->last_usec        = n;
                        timer->sec_used         = timer->tv.tv_sec - timer->start_sec;
                        timer->usec_used        = timer->last_usec - timer->start_usec;

                }
#ifdef HAVE_PTHREAD_H
                pthread_mutex_unlock(&(timer->mutex));
#endif
	}
}

/* Clean Timer */
void timer_clean(TIMER **timer)
{
	if((*timer)) free((*timer));
	(*timer) = NULL;
}

/* Heartbeat */
void heartbeat()
{
	printf("Heartbeat Testing \n");
}

/* Timer testing */
#ifdef _DEBUG_TIMER
int main()
{
	int i = 10000;
	uint32_t interval = 10000000u;
	TIMER *timer = timer_init();
	timer->callback = &heartbeat;
	
	while (i--)
	{
		timer->check(timer, interval);
		usleep(1000);
	}	
	timer->clean(&timer);
}
#endif
