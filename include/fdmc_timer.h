#ifndef _FDMC_TIMER_INCLUDE
#define _FDMC_TIMER_INCLUDE
#include "fdmc_global.h"
#include "fdmc_logfile.h"
#include "fdmc_exception.h"
#include "fdmc_ip.h"
#include "fdmc_thread.h"

#define FDMC_EVENT_ENABLED 1
#define FDMC_EVENT_DELETED 2
#define FDMC_EVENT_REMOVED 3
#define FDMC_EVENT_DISABLED 0

typedef struct _FDMC_TIMER_EVENT
{
	char *ev_key; // Magic key for event
	FDMC_LIST_ENTRY *ev_list;
	int ev_wait; // Event delay in units
	int ev_ticks; // Ticks after event creation in units
	int ev_flag; // Event state flag
	FDMC_THREAD_TYPE (__thread_call *ev_proc)(void*); // Thread to activate on event occurred
	void *ev_data; // Additional data to process event
	FDMC_THREAD *ev_thread; // Event last started thread
	FDMC_THREAD_LOCK *lock;
} FDMC_TIMER_EVENT;

typedef struct _FDMC_TIMER
{
	FDMC_LIST *tmr_list; // List holder of events
	char *tmr_key; // Unique key of timer (may be used for output files)
	int tmr_delay; // Timer unit in milliseconds
	FDMC_THREAD *tmr_thread; // Timer i/o stream
	int tmr_flag; // Timer flag. False means to terminate timer.
	FDMC_THREAD_LOCK *lock; // Timer mutex
} FDMC_TIMER;


extern FDMC_TIMER *fdmc_timer_create(char *tmr_key, int tmr_delay, FDMC_EXCEPTION *err);
extern int fdmc_timer_delete(FDMC_TIMER *ptmr, FDMC_EXCEPTION *err);

extern FDMC_TIMER_EVENT *fdmc_tmr_ev_insert(FDMC_TIMER *ptmr, char *ev_key, void *ev_data, int ev_wait, 
									 int ev_flag, FDMC_THREAD_TYPE (__thread_call *ev_proc)(void*),
									 FDMC_EXCEPTION *err);

extern int fdmc_tmr_ev_delete(FDMC_TIMER_EVENT *pevent, FDMC_EXCEPTION *err);

extern FDMC_THREAD_TYPE __thread_call fdmc_timer_thread_proc(void *ptmr);

extern void fdmc_runtime_display(char *p_name);

int fdmc_trace_time(void);

#endif
