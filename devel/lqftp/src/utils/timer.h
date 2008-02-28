#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifndef _TIMER_H
#define _TIMER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TYPEDEF_TIMER
#define _TYPEDEF_TIMER
typedef struct _TIMER 
{
	struct timeval tv;
	time_t start_sec;
	uint64_t start_usec;
	time_t sec_used;
	uint64_t usec_used;
	time_t  last_sec;
	uint64_t last_usec;
	time_t last_sec_used;
        uint64_t last_usec_used;

	void *mutex;

	void (*reset)(struct _TIMER *);
	int  (*check)(struct _TIMER *, uint32_t );
	void (*sample)(struct _TIMER *);
	void (*clean)(struct _TIMER **);
	void (*callback)(void);
	
}TIMER;
/* Initialize timer */
TIMER *timer_init();
#endif 
/* Reset timer */
void timer_reset(TIMER *);
/* Check timer and Run callback */
int timer_check(TIMER *, uint32_t );
/* TIMER  gettime */
void timer_sample(TIMER *);
/* Clean Timer */
void timer_clean(TIMER **);
/* VIEW timer */
#define TIMER_VIEW(_timer) \
{ \
	if(_timer) \
	{ \
		printf("timerptr:%08x\n" \
				"timer->start_sec:%u\n" \
				"timer->start_usec:%llu\n" \
				"timer->tv.tv_sec:%u\n" \
				"timer->tv.tv_usec:%u\n" \
				"timer->sec_used:%u\n" \
				"timer->usec_used:%llu\n" \
                                "timer->last_sec:%u\n" \
                                "timer->last_usec:%llu\n" \
                                "timer->last_sec_used:%u\n" \
                                "timer->last_usec_used:%llu\n" \
				"timer->reset():%08x\n" \
				"timer->check():%08x\n" \
				"timer->sample():%08x\n" \
				"timer->clean():%08x\n\n", \
				_timer, \
				_timer->start_sec, \
				_timer->start_usec, \
				_timer->tv.tv_sec, \
				_timer->tv.tv_usec, \
				_timer->sec_used, \
				_timer->usec_used, \
				_timer->last_sec, \
				_timer->last_usec, \
                                _timer->last_sec_used, \
                                _timer->last_usec_used, \
				_timer->reset, \
				_timer->check, \
				_timer->sample, \
				_timer->clean); \
	} \
}
#define PTIMER(timer) ((TIMER *)timer)
#define CLEAN_TIMER(timer) ((TIMER *)timer)->clean((TIMER **)&timer)
#define RESET_TIMER(timer) ((TIMER *)timer)->reset((TIMER *)timer)
#define SAMPLE_TIMER(timer) ((TIMER *)timer)->reset((TIMER *)timer)
#define CHECK_TIMER(timer, interval) ((TIMER *)timer)->check((TIMER *)timer, interval)
#define LAST_SEC_TIMER(timer) ((TIMER *)timer)->last_sec
#define LAST_USEC_TIMER(timer) ((TIMER *)timer)->last_usec

#ifdef __cplusplus
 }
#endif

#endif

