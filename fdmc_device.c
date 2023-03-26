#include "fdmc_global.h"
#include "fdmc_device.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"
#include "fdmc_list.h"
#include "fdmc_hash.h"
#include "fdmc_thread.h"
#include "fdmc_ip.h"
#include "fdmc_stack.h"
#include "fdmc_timer.h"

#include <errno.h>
#include <string.h>

MODULE_NAME("fdmc_device.c");

// List function for handlers comparisons
// Compare handlers by their threads
static int hndl_list_func(FDMC_LIST_ENTRY *entry, void *data, int flag)
{
	FDMC_HANDLER *edev;
	FDMC_THREAD *thr1, *thr2;
	switch(flag)
	{
	case FDMC_COMPARE:
		thr1 = ((FDMC_HANDLER*)data)->handler_thread;
		edev = (FDMC_HANDLER*)entry->data;
		thr2 = edev->handler_thread;
		if(fdmc_thread_eq(thr1->thread_id, thr2->thread_id))
			return FDMC_EQUAL;
		return FDMC_LESS;
	}
	return FDMC_OK;
}

// Alternative handler list function
// For fast getting free number (for logfile)
static int hndl_list_func_n(FDMC_LIST_ENTRY *p_entry, void *p_data, int p_flag)
{
	FDMC_HANDLER *v_hndl_entry, *v_hndl_data;

	v_hndl_entry = (FDMC_HANDLER*)p_entry->data;
	v_hndl_data = (FDMC_HANDLER*)p_data;
	switch(p_flag)
	{
	case FDMC_COMPARE:
		if(v_hndl_entry->number == v_hndl_data->number)
		{
			return FDMC_EQUAL;
		}
		if(v_hndl_data->number < v_hndl_entry->number)
		{
			return FDMC_LESS;
		}
		else
		{
			return FDMC_GREATER;
		}
		break;
	}
}

static int fdmc_list_hash_func_n(FDMC_HASH_TABLE *p_hash, void *p_data)
{
	int n = ((FDMC_HANDLER*)p_data)->number % p_hash->size;
	return n;
}

/**************************************** Process ************************************************/

/* Create new fdmc process thread
* 
* start_proc - Thread function for process
* handle_proc - Thread function for handle lookup
* proc_file - name of output log stream for process
* file_mode - 1- Always recreate output on start. 0- Recreate only 1 time per day
* proc_stack_size - Stack size for process thread
* hndl_stack_size - Stack size for handler thread
* sockport - ip port for internal data exchange
*
* BSA Oct 2009
*
* Return
*     Success - Handle of process
*     Failure - NULL
*/
FDMC_PROCESS *fdmc_process_create(
		char *name,								  
		FDMC_THREAD_FUNC start_proc, // Thread function for process loop
		char *proc_file, // Output stream for process
		int file_mode, // Reopen mode for stream (1 - recreate output, 0 - reopen output)
		int proc_stack_size, // Stack size for process thread
		int sockport // Port for administrative data exchange
								  )
{
	FUNC_NAME("fdmc_process_create");
	FDMC_PROCESS *ret = NULL;
	FDMC_THREAD *sio;
	FDMC_EXCEPTION x;
	char fnm[FDMC_MAX_ERRORTEXT];

	dbg_print();
	TRYF(x)
	{
		// Create and initialize new process structure
		ret = (FDMC_PROCESS*)fdmc_malloc(sizeof(*ret), &x);
		if(name)
		{
			ret->name = fdmc_strdup(name, &x);
		}
		ret->pthr = fdmc_thread_create(start_proc, ret, proc_file, file_mode, proc_stack_size, 0, &x);
		ret->start_proc = start_proc;
		ret->device_list = fdmc_list_create(fdmc_list_proc, &x);
		ret->handler_list = fdmc_list_create(hndl_list_func, &x);
		ret->free_handlers = fdmc_list_create(hndl_list_func, &x);
		ret->sleeping_handlers = fdmc_list_create(hndl_list_func, &x);
		ret->sock_port = sockport;
		ret->lock = fdmc_thread_lock_create(&x);
		ret->device_hash = fdmc_hash_table_create(2048, fdmc_hash_proc, fdmc_list_proc, &x);
		// Start assigned sockio thread
		sprintf(fnm, "%.*s_sockio_", FDMC_MAX_ERRORTEXT, proc_file);
		sio = fdmc_thread_create(fdmc_sockio_thread, ret, fnm, file_mode, proc_stack_size, 0, &x);
		fdmc_thread_start(sio, &x);
		// Start handler lookup thread
		//sprintf(fnm, "%.*s_hndlmng_", FDMC_MAX_ERRORTEXT, proc_file);
		//sio = fdmc_thread_create(fdmc_handler_lookup, ret, fnm, file_mode, proc_stack_size, 0, &x);
		//fdmc_thread_start(sio, &x);
		// Create process socket pair
		// Loop because sockio_thread migth not create listener yet
		do
		{
			fdmc_msleep(100);
			func_trace("Creating procees sockpair");
		} 
		while(fdmc_sockpair_create_t(ret, &ret->sockpair, &x) == 0);
		// Start process thread
		// fdmc_thread_start(ret->pthr, &x);
	}
	EXCEPTION
	{
		if(ret)
		{
			fdmc_list_delete(ret->device_list, NULL);
			fdmc_list_delete(ret->handler_list, NULL);
			fdmc_free(ret, NULL);
			ret = NULL;
		}
		trace("%s", x.errortext);
	}
	return ret;
}

//-------------------------------------------
// Name: fdmc_process_start
// 
//-------------------------------------------
// Purpose:
// start process
//-------------------------------------------
// Parameters:
// p_proc - process pointer
//-------------------------------------------
// Returns:
// 0 - on failure
// 1 - on success
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_process_start(FDMC_PROCESS *p_proc)
{
	FUNC_NAME("fdmc_process_start");

	int ret = fdmc_thread_start(p_proc->pthr, NULL);
	time_trace("process %s started", p_proc->name);
	return ret;
}

//-------------------------------------------
// Name: fdmc_process_stop
// 
//-------------------------------------------
// Purpose:
// user for cold restarting of process
// now only logging the fact of process
// thread loop termination
//-------------------------------------------
// Parameters:
// None
//-------------------------------------------
// Returns:
// always 1
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_process_stop(FDMC_PROCESS *p_proc)
{
	FUNC_NAME("fdmc_process_stop");

	dbg_print();
	screen_time_trace("process %s stopped", p_proc->name);
	return 1;
}

//-------------------------------------------
// Name: fdmc_process_delete
// 
//-------------------------------------------
// Purpose:
// Free process record resourses
//-------------------------------------------
// Parameters:
// p_proc - process pointer
//-------------------------------------------
// Returns:
// 0 - failure
// 1 - success
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_process_delete(FDMC_PROCESS *p_proc)
{
	FUNC_NAME("fdmc_process_stop");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		fdmc_list_delete(p_proc->device_list, NULL);
		fdmc_list_delete(p_proc->handler_list, NULL);
		fdmc_list_delete(p_proc->free_handlers, NULL);
		fdmc_hash_table_delete(p_proc->device_hash, NULL);
		fdmc_free(p_proc->name, NULL);
		fdmc_thread_lock_delete(p_proc->lock);
		fdmc_thread_delete(p_proc->pthr);
		fdmc_sockpair_close(&p_proc->sockpair);
		fdmc_free(p_proc, NULL);
	}
	EXCEPTION
	{
		err_trace("%s", x.errortext);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_sockio_thread
//---------------------------------------------------------
//  purpose: function for sokio thread. This function creates
//           sock pair for processes and handlers. Sock pairs
//           are used for data exchange between threads.
//  designer: Serge Borisov (BSA)
//  started: 25.02.10
//	parameters:
//		vp - pointer to parent fdmc_process
//	return:
//  special features:
//		
//---------------------------------------------------------
FDMC_THREAD_TYPE __thread_call fdmc_sockio_thread(void *vp)
{
	FUNC_NAME("fdmc_sockio_thread");
	FDMC_SOCK *hsock;
	FDMC_EXCEPTION x;
	FDMC_PROCESS *p = (FDMC_PROCESS*)vp;

	dbg_trace();

	for(;;)
	{
		TRYF(x)
		{
			// Create listener on process port
			func_trace("create I/O socket");
			hsock = fdmc_sock_create(NULL, p->sock_port, 2, &x);
			func_trace("I/O socket created");
			for(;;)
			{
				FDMC_SOCK *nsock;
				FDMC_SOCKPAIR *v_pair;
				// Create socket pair for application object
				time_trace("%s: Waiting for incoming connections", _function_id);
				if(fdmc_sock_ready(hsock, 1000, 0))
				{
					// Accept connection form sockpair his_mail
					nsock = fdmc_sock_accept(hsock, &x);
					time_trace("%s:socket accepted", _function_id);
					// receive address of sockpair
					fdmc_sock_recv(nsock, &v_pair, sizeof(v_pair), &x);
					// Send second element of result pair
					v_pair->his_mail = nsock;
					fdmc_sock_send(v_pair->his_mail, &v_pair, sizeof(v_pair), &x);
				}
			}
		}
		EXCEPTION
		{
			trace("error accepting request from handler. Restarting loop.");
		}
	}
	return (FDMC_THREAD_TYPE)0;
}


//-------------------------------------------
// Name: fdmc_sockpair_create
// 
//-------------------------------------------
// Purpose: create socket pair for exchange
//	between threads
//-------------------------------------------
// Parameters: 
// p_proc - process context pointer
// p_pair - pair to create
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
// Needs sockio process running
//-------------------------------------------
int fdmc_sockpair_create_t(FDMC_PROCESS *p_proc, FDMC_SOCKPAIR *p_pair, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sockpair_create");
	FDMC_EXCEPTION x;

	dbg_trace();
	TRYF(x)
	{
		// Connect to sockio process (I create my mail)
		p_pair->my_mail = fdmc_sock_create("127.0.0.1", p_proc->sock_port, 2, &x);
		// Send address of sockpair to sockio process
		fdmc_sock_send(p_pair->my_mail, &p_pair, sizeof(p_pair), &x);
		// sockio will set value for his_mail (Register my mail for others)
		fdmc_sock_recv(p_pair->my_mail, &p_pair, sizeof(p_pair), &x);
	}
	EXCEPTION
	{
		return 0;
	}
	return 1;
}

// Close socket pair sockets
int fdmc_sockpair_close(FDMC_SOCKPAIR *p_pair)
{
	FUNC_NAME("fdmc_sockpair_close");

	dbg_trace();
	fdmc_sock_close(p_pair->his_mail);
	fdmc_sock_close(p_pair->my_mail);
	return 1;
}


/*********************************** Handlers *********************************/

/* Create handler for process
* 
* pproc - process descriptor
* err - error handler
*
* BSA Oct 2009
*
* Return
*     Success - handler descriptor
*     Failure - NULL
*/
FDMC_HANDLER *fdmc_handler_create(FDMC_PROCESS *pproc, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_create");
	FDMC_EXCEPTION x;
	FDMC_HANDLER *ret = NULL;
	static int hnum = 1;
	char hname[80];

	dbg_print();
	TRYF(x)
	{
		CHECK_NULL(pproc, "pproc", x);
		// Allocate and fill handler structure
		ret = (FDMC_HANDLER*)fdmc_malloc(sizeof(*ret), &x);
		ret->process = pproc;
		ret->event_handler = pproc->handler_event;
		func_trace("count of handlers is %d", pproc->handler_list->count);
		ret->number = hnum ++;
		// Create sockpair
		while(fdmc_sockpair_create_t(pproc, &ret->sockpair, &x) == 0)
		{
			fdmc_msleep(10);
		}
		// Start thread
		// Add new handler to list
		fdmc_thread_lock(pproc->handler_list->lock, &x);
		// ret->number = pproc->handler_list->count + 1;
		sprintf(hname, "handle_%s_%d", pproc->pthr->stream->prefix, ret->number);
		ret->handler_thread = fdmc_thread_create(pproc->handler_proc, 
			ret, hname, 1, pproc->hndl_stack_size, 0, &x);
		ret->lock = fdmc_thread_lock_create(&x);
		fdmc_thread_start(ret->handler_thread, &x);
		fdmc_list_entry_add(pproc->handler_list, 
			fdmc_list_entry_create(ret, &x), &x);
		fdmc_thread_unlock(pproc->handler_list->lock, &x);
		func_trace("count of handlers is %d", pproc->handler_list->count);
	}
	EXCEPTION
	{
		if(pproc->lock->lock_count > 0)
		{
			fdmc_thread_unlock(pproc->lock, NULL);
		}
		if(ret)
		{
			fdmc_free(ret, NULL);
		}
		fdmc_raiseup(&x, err);
		ret = NULL;
	}
	func_trace("Handler created with number %d. Sockets %d and %d", ret->number, 
		ret->sockpair.his_mail->data_socket, 
		ret->sockpair.my_mail->data_socket);
	return ret;
}

/* Delete handler
* 
* p - process descriptor
* err - error handler
*
* BSA Oct 2009
*
* Return
*     Success - 1
*     Failure - 0
*/
int fdmc_handler_delete(FDMC_HANDLER *p, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_delete");
	FDMC_EXCEPTION x;

	dbg_print();
	TRYF(x)
	{
		if(!p)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "null handler parameter");
		}
		trace("Remove handler with number %d", p->number);
		// Close handler sockets
		fdmc_sockpair_close(&p->sockpair);
		// Delete mutex
		fdmc_thread_lock_delete(p->lock);
		// Delete thread descriptor
		fdmc_thread_delete(p->handler_thread);
		// Free memory
		fdmc_free(p, NULL);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 1;
}


int fdmc_handler_sleep(FDMC_HANDLER *p_hndl, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_sleep");
	FDMC_EXCEPTION x;
	FDMC_LIST *v_hlist;

	dbg_print();

	TRYF(x)
	{
		CHECK_NULL(p_hndl, "p_hndl", x);
		v_hlist = p_hndl->process->sleeping_handlers;
		func_trace("handler with number %d is going to sleep", p_hndl->number);
		// Free hadler resources
		fdmc_sockpair_close(&p_hndl->sockpair);
		fdmc_thread_lock_delete(p_hndl->lock);
		fdmc_thread_delete(p_hndl->handler_thread);
		// Add handler to sleeping list
		fdmc_thread_lock(v_hlist->lock,&x);
		fdmc_list_entry_add(v_hlist, fdmc_list_entry_create(p_hndl, &x), &x);
		fdmc_thread_unlock(v_hlist->lock, &x);
	}
	EXCEPTION
	{
		err_trace("%s", x.errortext);
		if(v_hlist->lock->lock_count > 0)
		{
			fdmc_thread_unlock(v_hlist->lock, NULL);
		}
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

FDMC_HANDLER* fdmc_handler_wake(FDMC_PROCESS *p_proc, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_wake");
	FDMC_EXCEPTION x;
	FDMC_HANDLER *ret = NULL;
	FDMC_LIST *v_list;
	char hname[80];

	dbg_trace();

	TRYF(x)
	{
		CHECK_NULL(p_proc, "p_proc", x);
		v_list = p_proc->sleeping_handlers;
		fdmc_thread_lock(v_list->lock, &x);
		if(v_list->count == 0)
		{
			func_trace("there is no sleeping handlers");
		}
		else
		{
			ret = (FDMC_HANDLER*)v_list->first->data;
			fdmc_list_entry_delete(v_list->first, &x);
		}
		fdmc_thread_unlock(v_list->lock, &x);
		// No sleeping handlers
		if(ret == NULL)
		{
			return NULL;
		}

		// Create sockpair
		while(fdmc_sockpair_create_t(ret->process, &ret->sockpair, &x) == 0)
		{
			fdmc_msleep(1);
		}
		// Start thread
		// Add new handler to list
		fdmc_thread_lock(p_proc->handler_list->lock, &x);
		sprintf(hname, "handle_%s_%d", p_proc->pthr->stream->prefix, ret->number);
		ret->handler_thread = fdmc_thread_create(p_proc->handler_proc, 
			ret, hname, 0, p_proc->hndl_stack_size, 0, &x);
		ret->lock = fdmc_thread_lock_create(&x);
		ret->hndl_stat = FDMC_HNDL_BISY;
		fdmc_thread_start(ret->handler_thread, &x);
		fdmc_list_entry_add(p_proc->handler_list, 
			fdmc_list_entry_create(ret, &x), &x);
		fdmc_thread_unlock(p_proc->handler_list->lock, &x);
		//func_trace("count of handlers is %d", p_proc->handler_list->count);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return ret;
}

/* Get first free handler from list
* or create new one if all existing
* handlers are busy
* 
* p - process descriptor
* err - error handler
*
* BSA 2012
*
* Return
*     Success - handler descriptor
*     Failure - 0
*/
FDMC_HANDLER* fdmc_handler_get(FDMC_PROCESS *p, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_get");
	FDMC_EXCEPTION x;
	FDMC_HANDLER *ret = NULL, *rct;
	FDMC_LIST_ENTRY *pent;

	dbg_print();
	TRYF(x)
	{
		// First we lock handler list to avoid list modification by
		// another thread - fdmc_handler_lookup()
		fdmc_thread_lock(p->handler_list->lock, &x);
		func_trace("Number of handlers is %d", p->handler_list->count);
		for(pent = p->handler_list->first; pent != NULL; pent = pent->next)
		{
			rct = (FDMC_HANDLER*)pent->data;
			// Check if handler is free - it isn't locked and ACTIVE
			if(rct->hndl_stat == FDMC_HNDL_ACTIVE) 
			{
				// Check if handler is locked
				func_trace("Handler %d is in active state", rct->number);
				if (fdmc_thread_trylock(rct->lock, NULL))
				{
					func_trace("succesfully lock handler %d", rct->number);
					// Was not locked, but now locked
					// Set status to bisy
					rct->hndl_stat = FDMC_HNDL_BISY;
					ret = rct;
					fdmc_thread_unlock(rct->lock, NULL);
					break;
				}
				else
				{
					func_trace("handler %d is locked at the moment", rct->number);
				}
			}
			else
			{
				func_trace("handler %d is bisy at the moment", rct->number);
			}
		} //for
		if(ret == NULL)
		{
			// if there are sliping handlers, wake one of them
			ret = fdmc_handler_wake(p, &x);
		}
		if(!ret)
		{
			// We need more handlers
			func_trace("no free handlers where found, create new one.");
			ret = fdmc_handler_create(p, &x);
			// New handler is bisy
			ret->hndl_stat = FDMC_HNDL_BISY;
#ifndef DYING_HANDLERS
			ret->permanent = 1;
#endif
		}
	}
	EXCEPTION
	{
		if(p->handler_list->lock->lock_count > 0)
		{
			fdmc_thread_unlock(p->handler_list->lock, NULL);
		}
		ret = NULL;
		fdmc_raiseup(&x, err);
	}
	// Unlock handler list
	if(p->handler_list->lock->lock_count > 0)
	{
		fdmc_thread_unlock(p->handler_list->lock, NULL);
	}
	return ret;
}


//---------------------------------------------------------
//  name: fdmc_handler_thread (Example)
//---------------------------------------------------------
//  purpose: Example of possible handler thread
//       Handler thread receives address of assigned device
//       from sockpair and calls device driver function to
//       handle op. If handler has not permanent status
//       it dies after specified timeout without data.
//  designer: Serge Borisov (BSA)
//  started: 23.03.12
//	parameters:
//		vp - Handler pointer
//	return:
//  special features:
//---------------------------------------------------------
FDMC_THREAD_TYPE __thread_call fdmc_handler_thread(void *vp)
{
	FUNC_NAME("fdmc_handler_thread");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *pdev;
	FDMC_LIST_ENTRY *v_list_entry;
	char c;
	int r;
	FDMC_HANDLER *p = ((FDMC_HANDLER*)vp);

	dbg_print();
	func_trace("start thread for handler %d", p->number);
	if(!p->event_handler)
	{
		err_trace("can't start handler. No event function.");
	}
	
	// Here may be some initializations
	(*p->event_handler)(p, FDMC_DEV_EVCREATE);
	//
	// Start main loop
	//
	c = FDMC_HANDLE_DATA;
	for(;;)
	{
		TRYF(x)
		{
			// Wait for data from process
			pdev = NULL;
			// if handler is permanent we do not check timeout
			// and immediatly jump to fdmc_sock_recv
			time_trace("\n********************* in handler loop **********************");
			if(!p->permanent && p->hndl_stat == FDMC_HNDL_IDLE) 
			{
				//Check timeout wait_sec only if handler is not permanent
				time_trace("Check ready handler %d", p->number);
				r = fdmc_sock_ready(p->sockpair.my_mail, p->process->wait_sec, 0);
				if(r == 0)
				{
					if(fdmc_thread_trylock(p->lock, NULL)) // Nobody needs us
					{
						if(p->hndl_stat == FDMC_HNDL_ACTIVE)
						{
							// Long jump to function exit
							(*p->event_handler)(p, FDMC_DEV_EVSHUTDOWN);
							fdmc_raisef(FDMC_TIMEOUT_ERROR, &x, 
								"data was not received for specified time for handler %d", p->number);
						}
					}
				}
				// r <> 0, data ready, receive
			}
			// Receive device address through handler's socket
			func_trace("waiting for data,  handler %d", p->number);
			r = fdmc_sock_recv(p->sockpair.my_mail, &p->dev_ref, sizeof(pdev), &x);
			tprintf("-- received %d bytes (%X,%d) from sockpair: ", r, p->dev_ref, p->dev_copy.package_number);
			fdmc_trace_time();
			// When we receive data handler is always in BISY state (by fdmc_handler_get)
			if(r <= 0)
			{
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "error receiving device address");
			}
			time_trace("data received for device %s", p->dev_ref->name);
			if(p->dev_ref)
			{
				//FDMC_LOGSTREAM *s;
				//FDMC_THREAD *thr;

				// Change io stream to device stream
				//thr = fdmc_thread_this();
				// Save origin stream
				//s = thr->stream;
				//thr->stream = p->dev_ref->device_thread->stream;
				func_trace("Call event handler");
				(*p->event_handler)(p, FDMC_DEV_EVDATA);
				// Restore original handler stream
				//thr->stream = s;
				// Return device into process fd_set (for passive device)
				//fdmc_process_sock_add(pdev->process, pdev->link);
			}
			p->hndl_stat = FDMC_HNDL_ACTIVE;
		}
		EXCEPTION
		{
			func_trace("%s", x.errortext);
			break;
		}
	}// for
	func_trace("Handler %d thread terminated", p->number);
	p->hndl_stat = FDMC_HNDL_DELETED;
	// Lock handler list
	fdmc_thread_lock(p->process->handler_list->lock, NULL);
	fdmc_thread_unlock(p->lock, NULL);
	v_list_entry = fdmc_list_entry_find(p->process->handler_list, p, NULL);
	// Delete handler from list
	if(v_list_entry != NULL)
	{
		fdmc_list_entry_delete(v_list_entry, NULL);
		trace("Handler had been deleted from list");
	}
	// Unlock list
	fdmc_thread_unlock(p->process->handler_list->lock, NULL);
	fdmc_handler_sleep(p, NULL);
	return (FDMC_THREAD_TYPE)0;
}

//  name: fdmc_handler_thread_s (Example)
//---------------------------------------------------------
//  purpose: 
//		This is another example of possible handler thread
//		It uses more resources, but possibly run faster.
//      Handler thread receives address of assigned device
//      from sockpair and calls device driver function to
//      handle op. If handler has not permanent status
//      it dies after specified timeout without data.
//		After handler competes all tasks it returns itself
//		to free handler stack.
//      When you use this thread, use fdmc_handler_pop instead of
//		fdmc_handler_get
//  designer: Serge Borisov (BSA)
//  started: 23.03.12
//	parameters:
//		vp - Handler pointer
//	return:
//  special features:
//---------------------------------------------------------
FDMC_THREAD_TYPE __thread_call fdmc_handler_thread_s(void *p_hndl)
{
	FUNC_NAME("fdmc_handler_thread_s");
	FDMC_EXCEPTION x;
	FDMC_HANDLER *v_hndl = (FDMC_HANDLER*)p_hndl;
	FDMC_DEVICE *v_dev;
	FDMC_LIST_ENTRY *v_list_entry;
	int r;

	dbg_print();

	func_trace("start thread for handler %d", v_hndl->number);
	if(!v_hndl->event_handler)
	{
		err_trace("can't start handler. No event function.");
	}
	//
	// Here may be some initializations on this event 
	// 
//	(*v_hndl->event_handler)(v_hndl, FDMC_DEV_EVCREATE);
	//
	// Start main loop
	//
	for(;;)
	{
		TRYF(x)
		{
			// Wait for data from process
			v_dev = NULL;
			// if handler is permanent we do not check timeout
			// and immediatly jump to fdmc_sock_recv
			time_trace("********************* in handler loop **********************");
			if(!v_hndl->permanent && v_hndl->hndl_stat == FDMC_HNDL_ACTIVE /*idle*/) 
			{
				//Check timeout wait_sec only if handler is not permanent
				time_trace("Check ready handler %d, %X", v_hndl->number, v_hndl);
				r = fdmc_sock_ready(v_hndl->sockpair.my_mail, v_hndl->process->wait_sec, 0);
				if(r == 0)
				{
					if(fdmc_thread_trylock(v_hndl->lock, NULL)) // Nobody needs us
					{
						if(v_hndl->hndl_stat == FDMC_HNDL_ACTIVE)
						{
							// Long jump to function exit
							(*v_hndl->event_handler)(v_hndl, FDMC_DEV_EVSHUTDOWN);
							time_trace("");
							fdmc_raisef(FDMC_TIMEOUT_ERROR, &x, 
								"data was not received for specified (%d seconds) time for handler %d, %X", 
								v_hndl->process->wait_sec, v_hndl->number, v_hndl);
						}
					}
				}
				// r <> 0, data ready, receive
			}
			// Receive device address through handler's socket
			func_trace("waiting for data,  handler %d", v_hndl->number);
			r = fdmc_sock_recv(v_hndl->sockpair.my_mail, &v_hndl->dev_ref, sizeof(v_dev), &x);
			// We are in bisy state
			r = fdmc_sock_recv(v_hndl->sockpair.my_mail, &v_hndl->dev_ref, sizeof(FDMC_DEVICE*), &x);
			tprintf("-- received %d bytes (%X,%d) from sockpair: ", r, v_hndl->dev_ref, 
				v_hndl->dev_copy.package_number);
			fdmc_trace_time();

			if(r <= 0)
			{
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "error receiving device address");
			}
			// Process received data
			time_trace("");
			tprintf("data received for device %s: ", v_hndl->dev_ref->name); fdmc_trace_time();
			if(v_hndl->dev_ref)
			{
				func_trace("Call event handler");
				(*v_hndl->event_handler)(v_hndl, FDMC_DEV_EVDATA);
				// Return handler into process handler stack
				// It was pop'ed by fdmc_handler_pop, called by device thread
				func_trace("return handler %d into handler stock", v_hndl->number);
				fdmc_handler_push(v_hndl, &x);
			}
		}
		EXCEPTION
		{
			func_trace("%s", x.errortext);
			fdmc_thread_lock(v_hndl->process->free_handlers->lock, NULL);
			fdmc_thread_lock(v_hndl->process->handler_list->lock, NULL);
			break;
		} // try
	} // for(;;)
	// Perform memory cleanup
	func_trace("Handler %d thread terminated", v_hndl->number);
	v_hndl->hndl_stat = FDMC_HNDL_DELETED;
	// Lock handler list
	fdmc_thread_unlock(v_hndl->lock, NULL);
	func_trace("deleting handler %d from process handler list", v_hndl->number);
	v_list_entry = fdmc_list_entry_find(v_hndl->process->handler_list, v_hndl, NULL);
	// Delete handler from list
	if(v_list_entry != NULL)
	{
		fdmc_list_entry_delete(v_list_entry, NULL);
		trace("Handler had been deleted from list");
	}
	// Delete from stack
	func_trace("deleting handler %d from process free handler list", v_hndl->number);
	v_list_entry = fdmc_list_entry_find(v_hndl->process->free_handlers, v_hndl, NULL);
	if(v_list_entry != NULL)
	{
		fdmc_list_entry_delete(v_list_entry, NULL);
		trace("Handler had been removed from stock");
	}
	// Unlock list
	fdmc_thread_unlock(v_hndl->process->handler_list->lock, NULL);
	fdmc_thread_unlock(v_hndl->process->free_handlers->lock, NULL);
	fdmc_handler_sleep(v_hndl, NULL);
	return (FDMC_THREAD_TYPE)0;
}

//-------------------------------------------
// Name: fdmc_handler_pop
// 
//-------------------------------------------
// Purpose: Alternative fast method to get free handler
//	 get free handler from stack
//-------------------------------------------
// Parameters: p_proc - process descriptor
// None
//-------------------------------------------
// Returns: 
//   1 - Success
//   0 - Failure
//-------------------------------------------
// Special features:
//
//-------------------------------------------
FDMC_HANDLER *fdmc_handler_pop(FDMC_PROCESS *p_proc, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_pop");
	FDMC_EXCEPTION x;
	FDMC_HANDLER *ret = NULL;

	dbg_trace();
	TRYF(x)
	{
		// Lock stack to avoid collisions
		fdmc_thread_lock(p_proc->free_handlers->lock, &x);
		// Check if there is any handler
		func_trace("number of handlers in stock = %d", p_proc->free_handlers->count);
		if(p_proc->free_handlers->count == 0)
		{
			// There is no free handlers in stack
			func_trace("there is no free handlers in stock. Lookup for sleeping handler");
			// Lookup sleeping handlers
			ret = fdmc_handler_wake(p_proc, &x);
			if(ret == NULL)
			{
				func_trace("there is no sleeping handler, create new");
				ret = fdmc_handler_create(p_proc, &x);
				func_trace("handler created with number %d", ret->number);
			}
		}
		else
		{
			// There are free handlers
			ret = p_proc->free_handlers->first->data;
			func_trace("got free handler with number %d", ret->number);
			fdmc_list_entry_delete(p_proc->free_handlers->first, &x);
		}
	}
	EXCEPTION
	{
		fdmc_thread_unlock(p_proc->free_handlers->lock, NULL);
		fdmc_raiseup(&x, err);
		return NULL;
	}
	// Unlock process handler stack
	fdmc_thread_unlock(p_proc->free_handlers->lock, NULL);
	return ret;
}

//-------------------------------------------
// Name: fdmc_handler_push
// 
//-------------------------------------------
// Purpose: Alternative fast method to get
//	free handler. Returns handler to stack
//	when handler becomes free.
//-------------------------------------------
// Parameters:
// None
//-------------------------------------------
// Returns:
//	1 - Success
//	0 - Error
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_handler_push(FDMC_HANDLER *p_hndl, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_handler_push");
	FDMC_EXCEPTION x;
	FDMC_LIST *free_handlers;

	dbg_trace();

	TRYF(x)
	{
		CHECK_NULL(p_hndl, "p_hndl", x);
		free_handlers = p_hndl->process->free_handlers;
		// Lock process handler stack
		fdmc_thread_lock(free_handlers->lock, &x);
		// Return handler to stack
		fdmc_list_entry_add(free_handlers, fdmc_list_entry_create(p_hndl, &x), &x);
	}
	EXCEPTION
	{
		fdmc_thread_unlock(free_handlers->lock, NULL);
		fdmc_raiseup(&x, err);
		return 0;
	}
	// Unlock stack and return
	fdmc_thread_unlock(free_handlers->lock, NULL);
	return 1;
}

/***********************************Devices*************************************************/

//---------------------------------------------------------
// Name:
// fdmc_device_alloc
//---------------------------------------------------------
// Purpose:
// Allocate memory for new device and fill default values
// Other values could be set at runtime
//---------------------------------------------------------
// Parameters:
// p_proc - process for wich device is created
// err - Exception handler
//---------------------------------------------------------
// Returns:
// device descriptor if success
// NULL on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
FDMC_DEVICE* fdmc_device_alloc(FDMC_PROCESS* p_proc, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_alloc");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *dev_new;

	dbg_trace();

	TRYF(x)
	{
		dev_new = (FDMC_DEVICE*)fdmc_malloc(sizeof(FDMC_DEVICE), &x);
		dev_new->process = p_proc;
		dev_new->event_handler = p_proc->device_event;
		dev_new->buflen = DEFAULT_BUFSIZE;
		dev_new->lock = fdmc_thread_lock_create(&x);
		fdmc_sockpair_create_t(p_proc, &dev_new->sockpair, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return dev_new;
}

//---------------------------------------------------------
//  name: fdmc_device_create
//---------------------------------------------------------
//  purpose: create new device
//  designer: Serge Borisov (BSA)
//  started: 23.03.10
//	parameters:
//		name - logical name of device
//		pproc - parent process for device
//		devproc - op handler function
//		devfile - name of output stream
//		devfilemode - 1= recreate output stream 0= open exsisting
//		ip_address - if device is initiator = address to connect to
//		      if device is listener this parameter must be NULL
//		ip_port - ip port to connect or to listen
//		sleeptime - time to sleep before next attempt to connect or listen
//		attempts - number of unsuccessfull attemts to connect or listen
//			If this parameter is 0 then try permanently
//		err - error handler
//	return:
//		Success - New device descriptor
//		Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_DEVICE *fdmc_device_create(
								char *name,
								FDMC_PROCESS *pproc,
								int p_headsize,
								char *devfile, int devfilemode,
								char *ip_address, 
								int ip_port, 
								int sleeptime, 
								int attempts,
								FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_create");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *ret = NULL;
	
	dbg_print();
	TRYF(x)
	{
		// Allocate memory and initialize fields
		ret = (FDMC_DEVICE*)fdmc_device_alloc(pproc, &x);
		if(name)
		{
			ret->name = fdmc_strdup(name, &x);
			fdmc_list_entry_add(pproc->device_list, fdmc_list_entry_create(ret, &x), &x);
		}
		// Add device to parent process
		//if(fdmc_list_entry_find(pproc->device_list, ret, &x))
		//{
		//	fdmc_raisef(FDMC_DEVICE_ERROR, &x, "duplicate device name <%s>", name);
		//}
		ret->ip_port = ip_port;
		ret->sleeptime = sleeptime;
		ret->natt = attempts;
		ret->header_size = p_headsize;
		// If address specified then device is an initiator of connection
		if(ip_address && strlen(ip_address))
		{
			strcpy(ret->ip_addr,ip_address);
			ret->role = FDMC_DEV_INITIATOR;
		}
		else if(ip_port != 0)
		{
			// Otherwise we create listener for port
			ret->role = FDMC_DEV_PARENT;
		}
		else
		{
			// Create accepted connection
			ret->role = FDMC_DEV_CHILD;
		}
		// Invoke create op
		ret->device_thread = fdmc_thread_create(pproc->device_proc, ret, 
			devfile, devfilemode, 0, 0, &x);
		fdmc_thread_start(ret->device_thread, &x);
		if(ret->name)
		{
			func_trace("device %s created", ret->name);
		}
		else
		{
			func_trace("null device created");
		}
	}
	EXCEPTION
	{
		// Something wrong
		fdmc_free(ret->name, NULL);
		fdmc_free(ret->ip_addr, NULL);
		fdmc_free(ret->device_buf, NULL);
		fdmc_free(ret, NULL);
		fdmc_raiseup(&x, err);
		ret = NULL;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_device_delete
//---------------------------------------------------------
//  purpose: Close device and remove it from process
//  designer: Serge Borisov (BSA)
//  started: 25.02.10
//	parameters:
//		pdev - pointer to dvice structure to close 
//	return:
//		Success - True
//		Failure - False
//  special features:
//		
//---------------------------------------------------------
int fdmc_device_delete(FDMC_DEVICE *pdev)
{
	FUNC_NAME("fdmc_device_delete");
	FDMC_EXCEPTION x;
	FDMC_LOGSTREAM *f;
	FDMC_LIST_ENTRY *ent;

	dbg_trace();
	TRYF(x)
	{
		if(pdev == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "null device parameter");
		}
		// Get process output stream
		f = pdev->process->pthr->stream;
		s_time_trace(f, "Delete device %s", pdev->name);
		// Remove socket from process set and close it
#ifdef _USE_PASSIVE_SOCKETS
		fdmc_process_sock_delete(pdev->process, pdev->link);
#endif
		fdmc_sock_close(pdev->link);
		fdmc_sockpair_close(&pdev->sockpair);
		s_time_trace(pdev->stream, "\n\n\nFile closed");
		// Close output stream
		fdmc_thread_delete(pdev->device_thread);
		fdmc_thread_lock_delete(pdev->lock);
		// Remove device from list
		if(pdev->name && (ent = fdmc_list_entry_find(pdev->process->device_list, pdev, &x)) != NULL)
		{
			fdmc_list_entry_delete(ent, &x);
		}
		// Free allocated buffers
		fdmc_free(pdev->name, NULL);
		fdmc_free(pdev, NULL);
	}
	EXCEPTION
	{
		s_trace(f, "%s", x.errortext);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_device_continue
//---------------------------------------------------------
// Purpose:
// if device is in waiting mode, then allow it to continue
// it's thread
//---------------------------------------------------------
// Parameters:
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
int fdmc_device_continue(FDMC_HANDLER *p_hndl, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_continue");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *v_dev;

	dbg_trace();

	TRYF(x)
	{
		v_dev = p_hndl->dev_ref;
		if(v_dev->stat == FDMC_DEV_BISY && v_dev->dev_wait)
		{
			v_dev->stat = FDMC_DEV_IDLE;
			fdmc_sock_send(v_dev->sockpair.his_mail, " ", 1, &x);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

/******************************************************************
 *
 *  Active devices - each device has it's own thread
 *
 ******************************************************************/

//-------------------------------------------
// Name: fdmc_device_shutdown
// 
//-------------------------------------------
// Purpose: some steps after device received
// shutdown adminitsrative message
//-------------------------------------------
// Parameters: p_dev - device
// 
//-------------------------------------------
// Returns:
// 
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_device_shutdown(FDMC_DEVICE *p_dev)
{
	// Shutdown administrative message
	(*p_dev->event_handler)(p_dev, FDMC_DEV_EVSHUTDOWN);
	time_trace("device %s is going to shutdown", p_dev->name);
	fdmc_device_delete(p_dev);
	return 1;
}

//-------------------------------------------
// Name: fdmc_device_events
// 
//-------------------------------------------
// Purpose:
// This is event handler for communication devices
// The only we do here is to get free application handler
// and send self address to it
//-------------------------------------------
// Parameters:
// p_dev - device pointer
// p_ev - event number
//-------------------------------------------
// Returns:
// 0 on succes
// EOF on failure
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_device_events(FDMC_DEVICE *p_dev, FDMC_DEV_EVENT p_ev)
{
	FUNC_NAME("fdmc_device_events");
	FDMC_EXCEPTION x;
	FDMC_HANDLER *v_hndl;
	int r;

	dbg_trace();
	TRYF(x)
	{
		v_hndl = fdmc_handler_get(p_dev->process, &x);
		p_dev->dev_event = p_ev;
		trace("Got event %d", (int)p_ev);
		p_dev->package_number++;
		memcpy(&v_hndl->dev_copy, p_dev, sizeof(FDMC_DEVICE));
		time_trace("");
		tprintf("-- sending data (%X,%d) to handler %d: ", p_dev, p_dev->package_number, v_hndl->number); fdmc_trace_time();
		r = fdmc_sock_send(v_hndl->sockpair.his_mail, &p_dev, sizeof(p_dev), &x);
		func_trace("data was sent with result %d", r);
		if(p_dev->dev_wait && p_dev->dev_event == FDMC_DEV_EVDATA)
		{
			// So we wait until handler ends
			func_trace("Wait until handler completes task");
			p_dev->stat = FDMC_DEV_BISY;
			while(p_dev->stat == FDMC_DEV_BISY)
			{
				if(fdmc_sock_ready(p_dev->sockpair.my_mail, -1, -1))
				{
					fdmc_sock_recv(p_dev->sockpair.my_mail, &r, 1, &x);
				}
			}
			time_trace("Task completed");
			fdmc_trace_time();
		}
	}
	EXCEPTION
	{
		err_trace("%s", x.errortext);
		return EOF;
	}
	return 0;
}

// DeviceRoles
//-------------------------------------------
// Name: fdmc_device_initiator
// 
//-------------------------------------------
// Purpose: Loop for initiator device
// i.e device which connects to other host
//-------------------------------------------
// Parameters:
// p_dev - device pointer
// err - eror handler
//-------------------------------------------
// Returns:
// 
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_device_initiator(FDMC_DEVICE *p_dev, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_initiator");
	FDMC_EXCEPTION x, x1;
	fd_set v_lnk;
	SOCKET v_data_sock, v_ctrl_sock = p_dev->sockpair.my_mail->data_socket;

	dbg_trace();

	for(;;) // Until control shutdown received
	{
		TRYF(x)
		{
			for(;;) // Until connected
			{
				TRY(x1)
				{
					time_trace("**************** in connection loop *******************");
					// Connect to specified address on specified port
					time_trace("");
					tprintf("-- try to connect to peer: "); fdmc_trace_time();
					p_dev->link = fdmc_sock_create(p_dev->ip_addr, p_dev->ip_port, p_dev->header_size, &x1);
					time_trace("");
					tprintf("-- connection successfull: ");
					fdmc_trace_time();
					(*p_dev->event_handler)(p_dev, FDMC_DEV_EVCONNECTED);
					v_data_sock = p_dev->link->data_socket;
					break;
				}
				EXCEPTION
				{
					// Connection refused
					time_trace("connection to peer '%s' failed %s", p_dev->ip_addr, x1.errortext);
					// Check control message
					if(fdmc_sock_ready(p_dev->sockpair.my_mail, 0, 0))
					{
						int cmd;
						cmd = (*p_dev->event_handler)(p_dev, FDMC_DEV_EVCTRL);
						if(cmd == EOF)
						{
							fdmc_device_shutdown(p_dev);
							return 0;
						}
					}
					// Wait for specified time
					fdmc_delay(p_dev->sleeptime, 0);
				}
			}
			// Read data and control sockets
			for(;;)
			{
				int r;

				time_trace("************ in connected loop ****************");
				FD_ZERO(&v_lnk);
				FD_SET(v_data_sock, &v_lnk);
				FD_SET(v_ctrl_sock, &v_lnk);
				r = select((v_data_sock > v_ctrl_sock ? v_data_sock : v_ctrl_sock) + 1,
					&v_lnk, NULL, NULL, NULL);
				if(r == EOF)
				{
					fdmc_raisef(FDMC_SOCKET_ERROR, &x, "error selecting connection %s", p_dev->name);
				}
				if(FD_ISSET(v_ctrl_sock, &v_lnk))
				{
					int cmd;

					cmd = (*p_dev->event_handler)(p_dev, FDMC_DEV_EVCTRL);
					if(cmd == EOF)
					{
						// Shutdown administrative message
						fdmc_device_shutdown(p_dev);
						return 0;
					}
				}
				if(FD_ISSET(v_data_sock, &v_lnk))
				{
					p_dev->msglen = fdmc_sock_recv(p_dev->link, p_dev->device_buf, sizeof(p_dev->device_buf)-1, &x);
					switch(p_dev->msglen)
					{
					case 0:
						tprintf("-- connection closed: "); fdmc_trace_time();
						(*p_dev->event_handler)(p_dev, FDMC_DEV_EVDISCONNECT);
						fdmc_raisef(FDMC_SOCKET_ERROR, &x, "connection closed by peer");
						break;
					case EOF:
						(*p_dev->event_handler)(p_dev, FDMC_DEV_EVERROR);
						fdmc_raisef(FDMC_SOCKET_ERROR, &x, "receive error");
						break;
					default:
						tprintf("-- data received: "); fdmc_trace_time();
						(*p_dev->event_handler)(p_dev, FDMC_DEV_EVDATA);
						break;
					}
				}
			}
		}
		EXCEPTION
		{
			func_trace("%s", x.errortext);
			fdmc_sock_close(p_dev->link);
			fdmc_delay(p_dev->sleeptime, 0);
		}
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_device_parent
// 
//-------------------------------------------
// Purpose: Loop for listener, i.e device
// which listnens port and creates child connections
//-------------------------------------------
// Parameters:
// p_dev device pointer
// err - exception handler
//-------------------------------------------
// Returns:
// no return
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_device_parent(FDMC_DEVICE *p_dev, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_parent");
	FDMC_EXCEPTION x;
	fd_set lnk;
	int r, cmd;
	SOCKET v_lstn, v_ctl;

	dbg_trace();

	TRYF(x)
	{
		FDMC_EXCEPTION lfail;
		int v_attemts = p_dev->natt;
		for(;;)
		{
			TRY(lfail)
			{
				func_trace("Try to create listener for port %d", p_dev->ip_port);
				p_dev->link = fdmc_sock_create(NULL, p_dev->ip_port, p_dev->header_size, &lfail);
				break;
			}
			EXCEPTION
			{
				time_trace("Error creating listener for port %d", p_dev->ip_port);
				trace("%s", lfail.errortext);
				if(v_attemts != 0)
				{
					if(--v_attemts == 0)
					{
						func_trace("Number of attempts excedeed for port %d", p_dev->ip_port);
						fdmc_raisef(FDMC_SOCKET_ERROR, &x, "%s", lfail.errortext);
					}
				}
				fdmc_delay(p_dev->sleeptime, 0);
			}
		}
		// Listener created
		time_trace("Listener created");
		(*p_dev->event_handler)(p_dev, FDMC_DEV_EVLISTEN);
		v_lstn = p_dev->link->data_socket;
		v_ctl = p_dev->sockpair.my_mail->data_socket;
		for(;;)
		{
			FD_ZERO(&lnk);
			time_trace("******************** in parent loop ***********************");
			// Read listen socket and control socket
			FD_SET(v_lstn, &lnk);
			FD_SET(v_ctl, &lnk);
			r = select((v_lstn > v_ctl ? v_lstn:v_ctl)+1, &lnk, NULL, NULL, NULL);
			if(r == 0)
			{
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "error selecting sockets for device %s", p_dev->name);
			}
			if(FD_ISSET(v_ctl, &lnk))
			{
				// Control message received
				cmd = (*p_dev->event_handler)(p_dev, FDMC_DEV_EVCTRL);
				if(cmd == EOF)
				{
					// Shutdown device
					func_trace("Device %s is going to shutdown", p_dev->name);
					(*p_dev->event_handler)(p_dev, FDMC_DEV_EVSHUTDOWN);
					return 0;
				}
			}
			if(FD_ISSET(v_lstn, &lnk))
			{
				// Data received, accept connection
				int r;
				time_trace("");
				tprintf("-- remote host acquired acception: ");
				fdmc_trace_time();
				p_dev->accepted = fdmc_sock_accept(p_dev->link, &x);
				r = (*p_dev->event_handler)(p_dev, FDMC_DEV_EVACCEPT);
			}
		}
	}
	EXCEPTION
	{
		func_trace("%s", x.errortext);
		fdmc_device_delete(p_dev);
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_device_child
// 
//-------------------------------------------
// Purpose: Loop for child device, i.e. device
// which was created by parent device
//-------------------------------------------
// Parameters:
// p_dev - device pointer
// err - error handler
//-------------------------------------------
// Returns:
// 0,1
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_device_child(FDMC_DEVICE *p_dev, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_child");
	FDMC_EXCEPTION x;
	SOCKET v_data_sock;
	SOCKET v_ctrl_sock;
	fd_set v_lnk;
	int r;

	dbg_trace();

	v_data_sock = p_dev->link->data_socket; 
	v_ctrl_sock = p_dev->sockpair.my_mail->data_socket;

	for(;;)
	{
		TRYF(x)
		{
			// Read data and control sockets
			time_trace("*********************** in child loop ***************************");
			FD_ZERO(&v_lnk);
			FD_SET(v_data_sock, &v_lnk);
			FD_SET(v_ctrl_sock, &v_lnk);
			r = select((v_data_sock > v_ctrl_sock ? v_data_sock : v_ctrl_sock) + 1,
				&v_lnk, NULL, NULL, NULL);
			if(r == EOF)
			{
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "error selecting child device %s", p_dev->name);
			}
			// Control received
			if(FD_ISSET(v_ctrl_sock, &v_lnk))
			{
				r = (*p_dev->event_handler)(p_dev, FDMC_DEV_EVCTRL);
				if(r == EOF)
				{
					fdmc_device_shutdown(p_dev);
					return 0;
				}
			}
			// Data socket is ready
			if(FD_ISSET(v_data_sock, &v_lnk))
			{
				p_dev->msglen = fdmc_sock_recv(p_dev->link, p_dev->device_buf, 
					sizeof(p_dev->device_buf) - 1, &x);
				func_trace("received %d bytes from device", p_dev->msglen);
				p_dev->device_buf[p_dev->msglen] = 0;
				switch(p_dev->msglen)
				{
				case 0:
					// Connection closed by peer
					time_trace("");
					tprintf("-- child connection closed: "); fdmc_trace_time();
					(*p_dev->event_handler)(p_dev, FDMC_DEV_EVCHILDLOST);
					fdmc_device_shutdown(p_dev);
					return 0;
				case EOF:
					(*p_dev->event_handler)(p_dev, FDMC_DEV_EVERROR);
					fdmc_device_shutdown(p_dev);
					return 0;
				default:
					time_trace("");
					tprintf("-- child data received: "); fdmc_trace_time();
					(*p_dev->event_handler)(p_dev, FDMC_DEV_EVDATA);
					break;
				}
			}
		}
		EXCEPTION
		{
			time_trace("%s", x.errortext);
			break;
		}
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_device_thread
// 
//-------------------------------------------
// Purpose:
// Default thread of active devices
//-------------------------------------------
// Parameters:
// p_par - pointer to device
//-------------------------------------------
// Returns:
// never
//-------------------------------------------
// Special features:
//
//-------------------------------------------
FDMC_THREAD_TYPE __thread_call fdmc_device_thread(void *p_par)
{
	FUNC_NAME("fdmc_device_thread");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *v_dev = (FDMC_DEVICE*)p_par;

	dbg_trace();

	TRYF(x)
	{
		switch(v_dev->role)
		{
		case FDMC_DEV_PARENT:
			fdmc_device_parent(v_dev, &x);
			break;
		case FDMC_DEV_INITIATOR:
			fdmc_device_initiator(v_dev, &x);
			break;
		case FDMC_DEV_CHILD:
			fdmc_device_child(v_dev, &x);
			break;
		}
	}
	EXCEPTION
	{
		func_trace("%s", x.errortext);
	}
	return (FDMC_THREAD_TYPE)0;
}

#ifdef _USE_PASSIVE_SOCKETS
/*******************************************************
 *
 *  The additional feature - passive devices without own thread
 *  driven by <select> library function
 *
 *******************************************************/
/* Add socket to process fd_set
* 
* p - process descriptor
* s - socket descriptor
*
* BSA Oct 2009
*
* Return
*     Success - Handle of connected socket
*     Failure - 0
*/
int fdmc_process_sock_add(FDMC_PROCESS *p, FDMC_SOCK *s)
{
	FUNC_NAME("fdmc_process_sock_add");
	char c = 1;

	dbg_print();
	if(!FD_ISSET(s->data_socket, &p->sock_set))
	{
		fdmc_thread_lock(p->lock, NULL);
		FD_SET(s->data_socket, &p->sock_set);
		// Signal to selector about new socket
		fdmc_sock_send(p->sockpair.his_mail, &c, 1, NULL);
		time_trace("Adding socket %d to process", s->data_socket);
		if(p->sock_min == 0)
		{
			p->sock_min = s->data_socket;
		}
		else if(p->sock_min > s->data_socket)
		{
			p->sock_min = s->data_socket;
		}
		if(s->data_socket > p->sock_max)
		{
			p->sock_max = s->data_socket;
		}
		p->sock_count++;
		fdmc_thread_unlock(p->lock, NULL);
	}
	// Set minimum and maximum socket numbers
	return 1;
}

/* Remove socket from process fd_set
* 
* p - process descriptor
* s - socket descriptor
*
* BSA Oct 2009
*
* Return
*     Success - 1
*     Failure - 0
*/
int fdmc_process_sock_delete(FDMC_PROCESS *p, FDMC_SOCK *s)
{

	SOCKET j;

	// Clear bit in process fd_set
	trace("deleting socket %d from process", s->data_socket);
	fdmc_thread_lock(p->lock, NULL);
	FD_CLR(s->data_socket, &p->sock_set);
	p->sock_count --;
	// No more active connections
	if(p->sock_count <= 0)
	{
		p->sock_count = 0;
		p->sock_max = 0;
		p->sock_min = 0;
		fdmc_thread_unlock(p->lock, NULL);
		return 1;
	}
	// Check if we need to change sock_max
	if(s->data_socket == p->sock_max)
	{
		for(j = s->data_socket-1; j >= p->sock_min; j--)
		{
			if(FD_ISSET(j, &p->sock_set))
			{
				break;
			}
		}
		p->sock_max = j;
	}
	// Check if we need to change sock_min
	if(s->data_socket == p->sock_min)
	{
		for(j = s->data_socket+1; j <= p->sock_max; j++)
		{
			if(FD_ISSET(j, &p->sock_set))
			{
				break;
			}
		}
		p->sock_min = j;
	}
	fdmc_thread_unlock(p->lock, NULL);
	return 1;
}


//---------------------------------------------------------
//  name: fdmc_device_select
//---------------------------------------------------------
//  purpose: select devices ready to process and assign
//           handler to each of them. If device is in deleted
//           state, close it and delete from device list.
//  designer: Serge Borisov (BSA)
//  started: 25.02.10
//	parameters:
//		pptr - pointer to parent process 
//	return:
//		Success - True
//		Failure - False
//  special features:
//		
//---------------------------------------------------------
int fdmc_device_select(FDMC_PROCESS *pptr)
{
	FUNC_NAME("fdmc_device_select");
	FDMC_EXCEPTION x;
	FDMC_LIST_ENTRY *devp, *vnext;
	FDMC_DEVICE *pdev;
	fd_set wset;
	int r;
	char c = 0;

	dbg_trace();
	TRYF(x)
	{
		// Check function parameters
		if(pptr == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "null process parameter");
		}
		// Copy process socket set to work set
		memcpy(&wset, &pptr->sock_set, sizeof(wset));
		// select work set
		r = select(pptr->sock_max + 1, &wset, NULL, NULL, NULL);
		//func_trace("after select data sockets");
		if(r <= 0)
		{
			trace("Error selecting device, error code is %d", WSAGetLastError());
			return 0;
		}

		// Check if there is data in special control socket
		// This socket is used to stop selecting work set for example
		// when another thread changes process sock set or 
		// it is nessesary to reconfigure process
		if(FD_ISSET(pptr->sockpair.my_mail->data_socket, &wset))
		{
			func_trace("control received");
			while(fdmc_sock_ready(pptr->sockpair.my_mail, 0, 0))
			{
				fdmc_sock_recv(pptr->sockpair.my_mail, &c, 1, &x);
			}
			r --;
		}
		// if there no socket
		if(r == 0)
		{
			return 1;
		}
		// Lock device list and get first
		fdmc_thread_lock(pptr->device_list->lock, &x);
		/*----------------------------------------------*/
		for(vnext = devp = pptr->device_list->first; vnext != NULL && r != 0; devp = vnext)
		{
			// Get current device
			pdev = (FDMC_DEVICE*)devp->data;
			vnext = devp->next;
			// Device has something for process
			if(FD_ISSET(pdev->link->data_socket, &wset))
			{
				FDMC_HANDLER *h;
				// Clear device socket from process fd_set
				// Then device handler must set it
				fdmc_process_sock_delete(pptr, pdev->link);
				if(!fdmc_thread_trylock(pdev->lock, NULL))
				{
					// Device is served
					continue;
				}
				// Device is in deleted state
				if(pdev->stat == FDMC_DEV_DELETED)
				{
					// Device is in deleted state
					func_trace("device %s in deleted state. Delete.", pdev->name);
					fdmc_thread_unlock(pdev->lock, NULL);
					fdmc_device_delete(pdev);
					fdmc_list_entry_delete(devp, &x);
					continue;
				}
				// Device is waiting to be served
				// Get free handler to manage device
				func_trace("device is ready to be served");
				pdev->stat = FDMC_DEV_SERVED;
				func_trace("get handler ready for event processing");
				h = fdmc_handler_get(pptr, &x);
				trace("Assign handler %d to device %s", h->number, pdev->name);
				// Device is ready to process. Send address of device to free handler
				pdev->dev_event = FDMC_DEV_EVREADY;
				fdmc_thread_unlock(pdev->lock, NULL);
				fdmc_sock_send(h->sockpair.his_mail, &pdev, sizeof(pdev), &x);
				r--;
			} // if isset
		} // for r
		/*-----------------------------------------------*/
		fdmc_thread_unlock(pptr->device_list->lock, NULL);
		c = 1;
	}
	EXCEPTION
	{
		fdmc_thread_unlock(pptr->device_list->lock, NULL);
		time_trace("lookup failed %s", x.errortext);
	}
	if(c == 0) 
		return 0;
	return 1;
}

// Create if nessesary listeners and connections
int fdmc_process_links_create(FDMC_PROCESS *p, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_process_links_create");
	FDMC_EXCEPTION x;
	FDMC_LIST_ENTRY *ldev; // device list of process
	FDMC_DEVICE *dev;
	FDMC_HANDLER *h;

	TRYF(x)
	{
		dbg_trace();
		// Lookup device list
		for(ldev = p->device_list->first; ldev; ldev = ldev->next)
		{
			dev = (FDMC_DEVICE*)ldev->data;
			func_trace("create link for device %s", dev->name);
			if(dev->link)
			{
				continue;
			}
			// Initiate create event for handler
			dev->dev_event = FDMC_DEV_EVCREATE;
			// Get first free handler
			func_trace("get handler ready for connection creating %s", dev->name);
			h = fdmc_handler_get(p, &x);
			func_trace("got handler number %d", h->number);
			// Send address of device to handler.
			// Handler is independed thread, that handles device events.
			fdmc_sock_send(h->sockpair.his_mail, &dev, sizeof(dev), &x);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_device_connect
//---------------------------------------------------------
//  purpose: create connection for device
//  designer: Serge Borisov (BSA)
//  started: 23.03.10
//	parameters:
//		pdev - pointer to device structure
//		err - error handler
//	return:
//		Success - True
//		Failure - False
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_device_connect(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_connect");
	FDMC_EXCEPTION x, x1;
	int j, ret;

	dbg_print();
	// Get value of connection attempts
	j = pdev->natt;
	TRYF(x)
	{
		for(;;)
		{
			TRY(x1)
			{
				// Connect to specified address on specified port
				time_trace("Creating connection");
				pdev->link = fdmc_sock_create(pdev->ip_addr, pdev->ip_port, pdev->header_size, &x1);
				time_trace("Connected");
				// Adding socket to process fd_set
				fdmc_process_sock_add(pdev->process, pdev->link);
				pdev->dev_event = FDMC_DEV_EVCONNECTED;
				ret = -1;
				break;
			}
			EXCEPTION
			{
				func_trace("%s", x1.errortext);
				// Connection failed. If number of unsuccessfull attempts is set
				// then repeat loop for specified number of attempts. If this number
				// is set to zero then repeat loop permanently.
				if(j)
				{
					if(--j == 0)
					{
						fdmc_raisef(FDMC_SOCKET_ERROR, &x, "number of connection attempts exceeded");
					}
				}
				fdmc_delay(pdev->sleeptime, 0);
			}
		}
		//fdmc_hash_entry_add(pdev->process->sock_hash, pdev, &x);
	}
	EXCEPTION
	{
		// Number of atempts exceeded
		pdev->dev_event = FDMC_DEV_EVERROR;
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}


//---------------------------------------------------------
//  name: fdmc_device_listen
//---------------------------------------------------------
//  purpose: create listener device
//  designer: Serge Borisov (BSA)
//  started: 23.03.10
//	parameters:
//		pdev - pointer to device structure
//		err - error handler
//	return:
//		Success - True
//		Failure - False
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_device_listen(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_listen");
	FDMC_EXCEPTION x, x1;
	int j, ret;

	dbg_print();
	j = pdev->natt;
	TRYF(x)
	{
		for(;;)
		{
			TRY(x1)
			{
				trace("Creating listener for port %d", pdev->ip_port);
				// Create listener for specified port
				pdev->link = fdmc_sock_create(NULL, pdev->ip_port, pdev->header_size, &x1);
				pdev->dev_event = FDMC_DEV_EVLISTEN;
				ret = -1;
				trace("Listener created with socket %d", pdev->link->data_socket);
				fdmc_process_sock_add(pdev->process, pdev->link);
				break;
			}
			EXCEPTION
			{
				func_trace("%s", x1.errortext);
				// Listen failed. If number of unsuccessfull attempts is set to nonzero
				// then repeat loop for specified number of attempts. If this number
				// is set to zero then repeat loop permanently.
				if(j)
				{
					if(--j == 0)
					{
						fdmc_raisef(FDMC_SOCKET_ERROR, &x, "number of listen attempts exceeded");
					}
				}
				fdmc_delay(pdev->sleeptime, 0);
			}
		}
	}
	EXCEPTION
	{
		// Number of listen attemts exceeded
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_device_accept
//---------------------------------------------------------
//  purpose: Example function to process socket accept
//  designer: Serge Borisov (BSA)
//  started: 23.03.10
//	parameters:
//		pdev - pointer to device structure
//		err - error handler
//	return:
//		Success - True
//		Failure - False
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_device_accept(FDMC_DEVICE *pdev, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_device_accept");
	FDMC_EXCEPTION x;
	FDMC_SOCK *psock = NULL;
	int ret = 1;
	//char c = 1;

	dbg_print();
	TRYF(x)
	{
		CHECK_NULL(pdev, "pdev", x);
		// Create new socket
		psock = fdmc_sock_accept(pdev->link, &x);
		pdev->accepted = psock;
	}
	EXCEPTION
	{
		pdev->dev_event = FDMC_DEV_EVERROR;
		func_trace("%s", x.errortext);
		ret = 0;
	}
	return ret;
}

// Example of event handler for passive devices
int fdmc_ip_event(FDMC_HANDLER *p_hndl)
{
	FUNC_NAME("fdmc_ip_handler");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *v_dev = p_hndl->dev_ref;
	int ret;

	TRYF(x)
	{
		switch (v_dev->dev_event)
		{
		case FDMC_DEV_EVCREATE:
			// We create new device. 
			switch (v_dev->role)
			{
			case FDMC_DEV_INITIATOR:
				ret = fdmc_device_connect(v_dev, &x);
				break;
			case FDMC_DEV_PARENT:
				// Device is listener
				ret = fdmc_device_listen(v_dev, &x);
				break;
			}
			break;
		case FDMC_DEV_EVREADY:
			// Device needs to be served
			switch (v_dev->role)
			{
			case FDMC_DEV_PARENT:
				// Accept new connection
				fdmc_device_accept(v_dev, &x);
				break;
			case FDMC_DEV_INITIATOR:
			case FDMC_DEV_CHILD:
				v_dev->msglen = fdmc_sock_recv(v_dev->link, v_dev->device_buf, v_dev->buflen, &x);
				if(v_dev->msglen == 0)
				{
					v_dev->dev_event = FDMC_DEV_EVDISCONNECT;
					if(v_dev->role == FDMC_DEV_CHILD)
					{
						fdmc_process_sock_delete(p_hndl->process, v_dev->link);
					}
				}
				else
				{
					v_dev->dev_event = FDMC_DEV_EVDATA;
				}
				break;
			}
		}
	}
	EXCEPTION
	{
		func_trace("%s", x.errorcode);
		v_dev->dev_event = FDMC_DEV_EVERROR;
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_handler_thread (Example for passive devices)
//---------------------------------------------------------
//  purpose: Example of possible handler thread
//       Handler thread receives address of assigned device
//       from sockpair and calls device driver function to
//       handle op. If handler has not permanent status
//       it dies after specified timeout without data.
//  designer: Serge Borisov (BSA)
//  started: 23.03.12
//	parameters:
//		vp - Handler pointer
//	return:
//  special features:
//---------------------------------------------------------
FDMC_THREAD_TYPE __thread_call fdmc_handler_thread1(void *vp)
{
	FUNC_NAME("fdmc_handler_thread");
	FDMC_EXCEPTION x;
	FDMC_DEVICE *pdev;
	char c;
	int r;
	FDMC_HANDLER *p = ((FDMC_HANDLER*)vp);

	dbg_print();
	func_trace("start thread for handler %d", p->number);
	if(!p->event_handler)
	{
		err_trace("can't start handler. No event function.");
	}
	(*p->event_handler)(p, FDMC_DEV_EVCREATE);
	//
	// Here may be some initializations
	// 
	c = FDMC_HANDLE_DATA;
	for(;;)
	{
		TRYF(x)
		{
			// Wait for data from process
			pdev = NULL;
			// if handler is permanent we do not check timeout
			// and immediatly jump to fdmc_sock_recv
			if(!p->permanent && p->hndl_stat == FDMC_HNDL_ACTIVE /*idle*/) 
			{
				//Check timeout wait_sec only if handler is not permanent
				time_trace("Check ready handler %d", p->number);
				r = fdmc_sock_ready(p->sockpair.my_mail, p->process->wait_sec, 0);
				if(r == 0)
				{
					// Данных в сокете нет уже долго 
					if(fdmc_thread_trylock(p->lock, NULL)) // Мы никому не нужны
					{
						if(p->hndl_stat == FDMC_HNDL_ACTIVE)
						{
							// Long jump to function exit
							(*p->event_handler)(p, FDMC_DEV_EVSHUTDOWN);
							fdmc_raisef(FDMC_TIMEOUT_ERROR, &x, 
								"data was not received for specified time for handler %d", p->number);
						}
					}
				}
				// r <> 0, data ready, receive
			}
			// Receive device address through socket
			func_trace("receiving data for handler %d", p->number);
			r = fdmc_sock_recv(p->sockpair.my_mail, &p->dev_ref, sizeof(pdev), &x);
			// When we receive data handler is always in BISY state (by fdmc_handler_get)
			if(r <= 0)
			{
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "error receiving device address");
			}
			time_trace("Received data for device %s", pdev->name);
			if(pdev)
			{
				FDMC_LOGSTREAM *s;
				FDMC_THREAD *thr;

				// Change io stream to device stream
				thr = fdmc_thread_this();
				// Save origin stream
				s = thr->stream;
				thr->stream = pdev->stream;
				// Call event handler
				(*p->event_handler)(p, FDMC_DEV_EVDATA);
				// Restore original handler stream
				thr->stream = s;
				// Return device into process fd_set (for passive device)
				fdmc_process_sock_add(pdev->process, pdev->link);
			}
			p->hndl_stat = FDMC_HNDL_ACTIVE;
		}
		EXCEPTION
		{
			func_trace("%s", x.errortext);
			break;
		}
	}// for
	func_trace("Handler %d thread terminated", p->number);
	p->hndl_stat = FDMC_HNDL_DELETED;
	return (FDMC_THREAD_TYPE)0;
}
#endif

// Я все время становился мудрее. Но ненадолго.
// 
// С. Борисов
// Из воспоминаний
//
