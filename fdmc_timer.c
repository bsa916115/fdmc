#include "fdmc_global.h"
#include "fdmc_list.h"
#include "fdmc_timer.h"
#include "fdmc_thread.h"
#include <sys/timeb.h>

MODULE_NAME("fdmc_timer.c");


//---------------------------------------------------------
//  name: timer_list_proc
//---------------------------------------------------------
//  purpose: list function for event list
//  designer: Serge Borisov (BSA)
//  started: 20/09/10
//	parameters:
//		ent - list entry
//		data - data to process
//		flag - opertion flag
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//---------------------------------------------------------
static int timer_list_proc(FDMC_LIST_ENTRY *ent, void *data, int flag)
{
//	FDMC_TIMER_EVENT *pevent;

	//pevent = ent->data;
	//if(flag == FDMC_DELETE)
	//{
	//	if(pevent->ev_flag != FDMC_EVENT_REMOVED)
	//	{
	//		pevent->ev_flag = FDMC_EVENT_DELETED;
	//		(*pevent->ev_proc)(ent->data);
	//	}
	//}
	return fdmc_list_proc(ent, data, flag);
}

//---------------------------------------------------------
//  name: fdmc_timer_create
//---------------------------------------------------------
//  purpose: create new timer event list
//  designer: Serge Borisov (BSA)
//  started: 21/09/10
//	parameters:
//		tmr_key - unique key for timer
//		tmr_delay - timer unit in milliseconds
//		err - error handler
//	return:
//		On Success - new timer event list
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_TIMER *fdmc_timer_create(char *tmr_key, int tmr_delay, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_timer_create");
	FDMC_EXCEPTION x;
	FDMC_TIMER *new_timer = NULL;
	
	TRYF(x)
	{
		// Allocate memory for structure and set fields
		new_timer = fdmc_malloc(sizeof(FDMC_TIMER), &x);
		new_timer->tmr_key = fdmc_strdup(tmr_key, &x);
		new_timer->tmr_delay = tmr_delay;
		new_timer->tmr_list = fdmc_list_create(timer_list_proc, &x);
		new_timer->tmr_flag = 1;
		new_timer->tmr_thread = fdmc_thread_create(fdmc_timer_thread_proc, new_timer,
			new_timer->tmr_key, 0, 0, 1, &x);
		new_timer->lock = fdmc_thread_lock_create(&x);
	}
	EXCEPTION
	{
		fdmc_timer_delete(new_timer, NULL);
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return new_timer;
}

//---------------------------------------------------------
//  name: fdmc_timer_delete
//---------------------------------------------------------
//  purpose: destructor for timer
//  designer: Serge Borisov (BSA)
//  started: 21/09/10
//	parameters:
//		ptmr - timer event list
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_timer_delete(FDMC_TIMER *ptmr, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_timer_delete");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(ptmr, "ptmr", x);
		fdmc_free(ptmr->tmr_key, NULL);
		fdmc_list_delete(ptmr->tmr_list, NULL);
		fdmc_sclose(ptmr->tmr_thread->stream);
		fdmc_thread_delete(ptmr->tmr_thread);
		fdmc_free(ptmr, NULL);
		return 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 0;
}

//---------------------------------------------------------
//  name: fdmc_tmr_ev_insert
//---------------------------------------------------------
//  purpose: create new event for specified timer
//  designer: Serge Borisov (BSA)
//  started: 21/09/10
//	parameters:
//		ptmr - pointer to existing timer
//		ev_key - unique event key for search operations
//		ev_data - additional data for event thread
//		ev_wait - number of timer units to wait for event
//		ev_flag - enabled/disabled
//		ev_proc - event thread
//		err - error handler
//	return:
//		On Success - new event structure
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_TIMER_EVENT *fdmc_tmr_ev_insert(FDMC_TIMER *ptmr, char *ev_key, void *ev_data, int ev_wait, 
									 int ev_flag, FDMC_THREAD_TYPE (__thread_call *ev_proc)(void*),
									 FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_tmr_ev_insert");
	FDMC_EXCEPTION x;
	FDMC_TIMER_EVENT *new_ev = NULL;

	TRYF(x)
	{
		CHECK_NULL(ptmr, "ptmr", x);
		new_ev = fdmc_malloc(sizeof(*new_ev), &x);
		new_ev->ev_key = fdmc_strdup(ev_key, &x);
		new_ev->ev_data = ev_data;
		new_ev->ev_wait = ev_wait;
		new_ev->ev_flag = ev_flag;
		new_ev->ev_proc = ev_proc;
		fdmc_list_entry_add(ptmr->tmr_list, new_ev->ev_list=fdmc_list_entry_create(new_ev, &x), &x);
		trace("new event %s inserted", new_ev->ev_key);
	}
	EXCEPTION
	{
		fdmc_tmr_ev_delete(new_ev, NULL);
		fdmc_raiseup(&x, err);
	}
	return new_ev;
}

//---------------------------------------------------------
//  name: fdmc_tmr_ev_delete
//---------------------------------------------------------
//  purpose: free memory used by timer event descriptor
//  designer: Serge Borisov (BSA)
//  started: 21/09/10
//	parameters:
//		pevent - event pointer
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_tmr_ev_delete(FDMC_TIMER_EVENT *pevent, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_tmr_ev_delete");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(pevent, "pevent", x);
		trace("event %s will be removed", pevent->ev_key);
		fdmc_list_entry_delete(pevent->ev_list, NULL);
		fdmc_free(pevent->ev_key, NULL);
		fdmc_free(pevent, NULL);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_timer_thread_proc
//---------------------------------------------------------
//  purpose: function for timer thread
//  designer: Serge Borisov (BSA)
//  started: 21.09.10
//	parameters:
//		par - pointer to timer structure
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_THREAD_TYPE __thread_call fdmc_timer_thread_proc(void *par)
{
	FUNC_NAME("fdmc_timer_thread_proc");
	FDMC_LIST *plist;
	FDMC_LIST_ENTRY *p, *q, *first, *last;
	FDMC_TIMER *ptmr = (FDMC_TIMER *)par;
	FDMC_TIMER_EVENT *pev;
	FDMC_EXCEPTION x;
	FDMC_THREAD *t;

	dbg_print();
	if(ptmr == NULL)
	{
		err_trace("NULL parameter when creating timer thread.");
	}
	plist = ptmr->tmr_list;
	if(plist == NULL)
	{
		err_trace("Error in timer structure. Null timer event list.");
	}
	// Loop for looking timer events
	for(;;)
	{
		fdmc_msleep(ptmr->tmr_delay);
		if(ptmr->tmr_flag == 0)
			break;
		fdmc_thread_lock(plist->lock, NULL);
		// Get ranges of event list
		first = plist->first;
		last = plist->last;
		fdmc_thread_unlock(plist->lock, NULL);
		for(p = first; ; )
		{
			if(p == NULL) break;
			pev = p->data;
			q = p->next;
			switch(pev->ev_flag)
			{
			case FDMC_EVENT_DELETED:
			case FDMC_EVENT_REMOVED:
				t = fdmc_thread_this();
				fdmc_tmr_ev_delete(pev, NULL);
				break;
			default:
				pev->ev_ticks ++;
				if(pev->ev_ticks == pev->ev_wait)
				{
					// Timer event occurred
					if(pev->ev_flag == FDMC_EVENT_ENABLED)
					{
						t = fdmc_thread_this();
						TRY(x)
						{
							pev->ev_thread = fdmc_thread_create(pev->ev_proc, pev, NULL, 0, 0, 0, &x);
							pev->ev_thread->stream = ptmr->tmr_thread->stream;
							fdmc_thread_start(pev->ev_thread, &x);
						}
						EXCEPTION
						{
							err_trace("Timer thread failed: %s", x.errortext);
						}
					}
					pev->ev_ticks = 0;
				}
			}
			if(p == last) break;
			p = q;
		}
	} // for
	// Exiting process
	trace("Timer '%s' is going to shutdown", ptmr->tmr_key);
	fdmc_timer_delete(ptmr, NULL);
	return (FDMC_THREAD_TYPE)0;
}

int proc_term = 0;
void fdmc_runtime_display(char *p_name)
{
	FUNC_NAME("fdmc_runtime_display");
	time_t v_running = 0, t;
#define run_time v_running
#define WAITFOR 60
	dbg_trace();
	screen_trace("Process %s started", p_name);
	for(run_time = 0;proc_term == 0;run_time += WAITFOR)
	{
		// Display programme running time in loop
		fdmc_delay(WAITFOR, 0);
		t = run_time+WAITFOR;
		if(t < 60)
			screen_time_trace("\nProcess %s \nrunning for %lu seconds", p_name, t);
		else if (t >= 60 && t < 3600)
		{
			if (t % 60 == 0)
				screen_time_trace("\nProcess %s \nrunning for %lu minutes", p_name, t/60);
		}
		else if (t % 3600 == 0)
		{
			screen_time_trace("\nProcess %s\nrunning for %lu hours", p_name, t/3600);
		}
	}
}

int fdmc_trace_time()
{
	struct timeb now;
	ftime(&now);

	trace("system time is %ld, %hd", now.time, now.millitm);
	return 1;
}
