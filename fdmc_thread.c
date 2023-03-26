#include "fdmc_global.h"
#include "fdmc_thread.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"
#include "fdmc_hash.h"
#include "fdmc_list.h"
#include "fdmc_ip.h"
#include "fdmc_xmlmsg.h"

#if defined (_WINDOWS_32)
#include <windows.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

FDMC_HASH_TABLE *thread_hash = NULL;
FDMC_THREAD *mainthread;

MODULE_NAME("fdmc_thread.c");

//---------------------------------------------------------
//  name: fdmc_create_hread
//---------------------------------------------------------
//  purpose: create new thread based on specified function
//  designer: Serge Borisov (BSA)
//  started: 21.09.10
//	parameters:
//		start_proc - thread procedure
//		param - pointer to thread parameter
//		thread_id - output, thread id retuaned by system
//		stack_size - stack size for created thread
//	return:
//		On Success - valid new thread handle
//		On Failure - invalid thread handle
//---------------------------------------------------------
FDMC_THREAD_HANDLE fdmc_create_thread(FDMC_THREAD_FUNC start_proc, void *param, 
									  FDMC_THREAD_ID *thread_id, int stack_size)
{
	FUNC_NAME("fdmc_create_thread");
	FDMC_THREAD_HANDLE hndl;
#if defined(_UNIX_)
	pthread_attr_t attr_thr;
#endif
#if defined( _WINDOWS_32)
	hndl = CreateThread(NULL, stack_size, start_proc, param, 0, thread_id);
#elif defined (_UNIX_)
	dbg_print();
	pthread_attr_init(&attr_thr);
	pthread_attr_setstacksize(&attr_thr, stack_size);
	pthread_attr_setdetachstate(&attr_thr, PTHREAD_CREATE_DETACHED);
	hndl = pthread_create(thread_id, &attr_thr, start_proc, param);
#endif
	if(THREAD_FAIL(hndl))
	{
		err_trace("Failed to create new thread");
	}
	else
	{
		func_trace("New thread created");
	}
	return hndl;
}

//---------------------------------------------------------
//  name: thread_hash_func
//---------------------------------------------------------
//  purpose: hash function for application thread hash
//  designer: Serge Borisov (BSA)
//  started: 21.09.10
//	parameters:
//		ht - hash table
//		thr_ptr - pointer to thread
//	return:
//		On Success - hash sum for thread
//		On Failure - -1
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static int thread_hash_func(FDMC_HASH_TABLE *ht, void *thr_ptr)
{
	//FUNC_NAME("thread_hash_func");
	FDMC_THREAD_ID pthr;
	int r = 0;

	if(ht == NULL)
	{
		return -1;
	}
	if(thr_ptr == NULL)
	{
		return ht->size+2; // Invalid value
	}
	//fprintf(stderr, "in hash func\n");
	pthr = *(FDMC_THREAD_ID*)thr_ptr;
	//if(sizeof(pthr) == 8)
	{
		long j;
		int n, i;
		char hashstr[64];	
		j = (long)pthr;
		n = sprintf(hashstr, "%lX", j);
		//fprintf(stderr, "j = %ld, hash = %s\n", j, hashstr);
		for(i = 0; i < n; i++)
		{
			r = (r + hashstr[i]);
			if(r >= ht->size)
			{
				r %= ht->size;
			}
		}
	}
	//r = (pthr % ht->size);
	//r = r % ht->size;
	//fprintf(stderr, "hash value for thread %ld is %d\n", pthr, r);
	return r;
}

//---------------------------------------------------------
//  name: thread_list_proc
//---------------------------------------------------------
//  purpose: list function for application thread hash
//  designer: Serge Borisov (BSA)
//  started: 21.09.10
//	parameters:
//		entry - list entry
//		data - data to process
//		flag - operation flag
//	return:
//		On Success - true
//		On Failure - false
//---------------------------------------------------------
int thread_list_proc(FDMC_LIST_ENTRY *entry, void *data, int flag)
{
	//FUNC_NAME("thread_list_proc");
	FDMC_THREAD *thr1, *thr2;

//	dbg_trace();

	switch(flag)
	{
	case FDMC_INSERT:
		return FDMC_OK;
	case FDMC_DELETE:
		//free(entry->data);
		return FDMC_OK;
	case FDMC_COMPARE:
		thr1 = entry->data;
		thr2 = data;
		if(fdmc_thread_eq(thr1->thread_id, thr2->thread_id))
			return FDMC_EQUAL;
		return FDMC_LESS;
	}
	return 0;
}

//---------------------------------------------------------
//  name: fdmc_thread_create
//---------------------------------------------------------
//  purpose: create new thread descriptor
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		thr_start - Tread start function
//		thr_param - Parameters passed to function
//		thr_stream - Name of thread log file
//		thr_stmode - Open or create stream flag
//		thr_stack - Tread system stack size
//		immediate - if true - start Thread. if False - create descriptor only.
//		err - error handler
//	return:
//		Success - New descriptor
//		Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_THREAD *fdmc_thread_create(FDMC_THREAD_FUNC thr_start, void *thr_param,
									   char *thr_stream, int thr_stmode, int thr_stack,
									   int immediate, 
									   FDMC_EXCEPTION *thr_err)
{
	FUNC_NAME("fdmc_thread_create");
	FDMC_THREAD *thr_new = NULL;
	FDMC_EXCEPTION err;

	dbg_print();
	func_trace("immediate start = %d", immediate);
	TRYF(err)
	{
		//CHECK_NULL(thr_start, "thr_start", err);
		thr_new = (FDMC_THREAD*)fdmc_malloc(sizeof(FDMC_THREAD), &err);
		thr_new->thread_param = thr_param;
		thr_new->thread_func = thr_start;
		thr_new->thread_stack = thr_stack;
		if(thr_stream != NULL)
		{
			thr_new->stream = thr_stmode==0 ? fdmc_sopen(thr_stream) : fdmc_screate(thr_stream);
			if(thr_new->stream == NULL)
			{
				fdmc_raisef(FDMC_IO_ERROR, &err, "Cannot create stream %s\n%s",
					thr_stream, strerror(errno));
			}
		}
		else
		{
			thr_new->stream = mainstream;
		}
		// Include thread into application thread
		if(thread_hash == NULL)
		{
			thread_hash = fdmc_hash_table_create(1024, thread_hash_func, thread_list_proc, &err);
		}
		// Startup thread function
		if(thr_start != NULL && immediate)
		{
			thr_new->thread_hndl = fdmc_create_thread(thr_new->thread_func, thr_param,
				&thr_new->thread_id, thr_stack);
			//printf("thread created with handle %lx\n", thr_new->thread_id);
			//printf("Hash function for thread is %u\n", thread_hash_func(thread_hash, thr_new));
			if(THREAD_FAIL(thr_new->thread_hndl))
			{
				fdmc_raisef(FDMC_THREAD_ERROR, &err, "Cannot start new thread\n%s",
					strerror(errno));
			}
			fdmc_hash_entry_add(thread_hash, thr_new, &err);
		}
	}
	EXCEPTION
	{
		fdmc_thread_delete(thr_new);
		fdmc_raiseup(&err, thr_err);
		func_trace("failed to create thread");
		return NULL;
	}
	func_trace("thread created succesfully");
	return thr_new;
}

//---------------------------------------------------------
//  name: fdmc_thread_start
//---------------------------------------------------------
//  purpose: start thread specified by descriptor
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		pthr - Existing thread descriptor
//		err - error handler
//	return:
//		Success - True
//		Failure - False
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_thread_start(FDMC_THREAD *pthr, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_thread_start");
	FDMC_EXCEPTION x;
	FDMC_THREAD_HANDLE p;
	int ret = 1;

	dbg_print();

	TRYF(x)
	{
		CHECK_NULL(pthr, "pthr", x);
		CHECK_NULL(pthr->thread_func, "pthr.thread_func", x);
		if(pthr->thread_id)
		{
			fdmc_raisef(FDMC_THREAD_ERROR, &x, "thread already started");
		}
		p = fdmc_create_thread(pthr->thread_func, 
			pthr->thread_param, &pthr->thread_id, pthr->thread_stack);
		//printf("thread created with handle %lx\n", pthr->thread_id);
		//printf("Hash function for thread is %u\n", thread_hash_func(thread_hash, pthr));
		if(THREAD_FAIL(p))
		{
			fdmc_raisef(FDMC_THREAD_ERROR, &x, "Cannot start new thread");
		}
		fdmc_hash_entry_add(thread_hash, pthr, &x);
	}
	EXCEPTION
	{
		ret = 0;
		fdmc_raiseup(&x, err);
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_thread_delete
//---------------------------------------------------------
//  purpose: Free memory, allocated by fdmc thread descriptor 
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		thr_new - Existing thread descriptor
//	return:
//		Success - True
//		Failure - False
//---------------------------------------------------------
int fdmc_thread_delete(FDMC_THREAD *thr_new)
{
	FUNC_NAME("fdmc_thread_delete");
	dbg_print();
	if(thr_new)
	{
		fdmc_hash_entry_delete(thread_hash, thr_new, NULL);
		if(thr_new->stream != mainstream);
//			fdmc_sclose(thr_new->stream);
		fdmc_free(thr_new, NULL);
	}
	return 1;
}

/* Decrement locked global variable */
void fdmc_thread_uunlock(int *key)
{
//	FUNC_NAME("thread_unlock");

	(*key) --;
}

//---------------------------------------------------------
//  name: fdmc_mutex_create
//---------------------------------------------------------
//  purpose: Create new mutex
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		err - Error descriptor
//	return:
//		Success - New Mutex descriptor
//		Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_MUTEX *fdmc_mutex_create(FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_mutex_create");
	FDMC_EXCEPTION x;
	FDMC_MUTEX *ret = NULL;
	TRYF(x)
	{
		ret = (FDMC_MUTEX*)fdmc_malloc(sizeof(*ret), &x);
#if defined( _WINDOWS_32)
		*ret = CreateMutex(NULL, FALSE, NULL);
		if(ret == NULL)
		{
			fdmc_raisef(FDMC_MUTEX_ERROR, &x, "error creating mutex");
		}
#elif defined(_UNIX_)
		if(pthread_mutex_init(ret, NULL))
		{
			fdmc_raisef(FDMC_MUTEX_ERROR, &x, "error creating mutex");
		}
#endif
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_mutex_delete
//---------------------------------------------------------
//  purpose: Delete existing mutex
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		pmutex - existing mutex
//	return:
//		Success - New Mutex descriptor
//		Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_mutex_delete(FDMC_MUTEX *pmutex)
{
	FUNC_NAME("fdmc_mutex_delete");
	//dbg_trace();
	if(!pmutex)
	{
		func_trace("Invalid (NULL) mutex descriptor");
	}
#if defined(_WINDOWS_32)
	CloseHandle(*pmutex);
#elif defined(_UNIX_)
	pthread_mutex_destroy(pmutex);
#endif
	fdmc_free(pmutex, NULL);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_mutex_lock
//---------------------------------------------------------
//  purpose: Lock existing mutex
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		pmutex - existing mutex descriptor
//		err - Error descriptor
//	return:
//		Success - true
//		Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_mutex_lock(FDMC_MUTEX *pmutex, FDMC_EXCEPTION *err)
{
	unsigned ret;
	FUNC_NAME("fdmc_mutex_lock");
	FDMC_EXCEPTION x;
	TRYF(x)
	{
	CHECK_NULL(pmutex, "pmutex", x);
#if defined(_WINDOWS_32)
	ret = WaitForSingleObject(*pmutex, INFINITE);
	if(ret != WAIT_OBJECT_0) 
	{
		fdmc_raisef(FDMC_MUTEX_ERROR, &x, "cannot lock mutex. ret = %XH", ret);
	}
#elif defined(_UNIX_)
	ret = pthread_mutex_lock(pmutex);
	if(ret != 0)
	{
		fdmc_raisef(FDMC_MUTEX_ERROR, &x, "cannot lock mutex. %s", strerror(errno));
	}
#endif
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_mutex_trylock
//---------------------------------------------------------
//  purpose: Lock existing mutex. If mutex is locked return 
//    immediatly with error state
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		pmutex - existing mutex descriptor
//		err - Error descriptor
//	return:
//		Success - true
//		Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_mutex_trylock(FDMC_MUTEX *pmutex, FDMC_EXCEPTION *err)
{
	unsigned ret;
	FUNC_NAME("fdmc_mutex_trylock");
	FDMC_EXCEPTION x;
	TRYF(x)
	{
	CHECK_NULL(pmutex, "pmutex", x);
#if defined(_WINDOWS_32)
	ret = WaitForSingleObject(*pmutex, 0);
	switch(ret)
	{
	case WAIT_OBJECT_0:
		return 1;
	case WAIT_TIMEOUT:
		return 0;
	default:
		fdmc_raisef(FDMC_MUTEX_ERROR, &x, "cannot lock mutex, error code %XH", ret);
	}
#elif defined(_UNIX_)
	ret = pthread_mutex_trylock(pmutex);
	switch(ret)
	{
	case 0:
		return 1;
	case EBUSY:
		return 0;
	default:
		fdmc_raisef(FDMC_MUTEX_ERROR, &x, "cannot lock mutex, error code %XH", ret);
	}
#endif
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_mutex_unlock
//---------------------------------------------------------
//  purpose: unlock existing mutex
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		pmutex - existing mutex descriptor
//	return:
//		Success - true
//		Failure - false
//---------------------------------------------------------
int fdmc_mutex_unlock(FDMC_MUTEX *pmutex)
{
	//FUNC_NAME("fdmc_mutex_unlock");
	if(!pmutex) return 0;
#if defined(_WINDOWS_32)
	ReleaseMutex(*pmutex);
#elif defined(_UNIX_)
	pthread_mutex_unlock(pmutex);
#endif
	return 1;
}


//---------------------------------------------------------
//  name: fdmc_thread_lock_create
//---------------------------------------------------------
//  purpose: Create thread lock
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		err - Error descriptor
//	return:
//		Success - New THREAD_LOCK descriptor
//		Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_THREAD_LOCK *fdmc_thread_lock_create(FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_thread_lock_create");
	FDMC_EXCEPTION x;
	FDMC_THREAD_LOCK *ret = NULL;

	//dbg_print();
	TRYF(x)
	{
		ret = fdmc_malloc(sizeof(*ret), &x);
		ret->lock = fdmc_mutex_create(&x);
	}
	EXCEPTION
	{
		fdmc_free(ret, NULL);
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_thread_lock_delete
//---------------------------------------------------------
//  purpose: Delete thread lock
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		plock - existing thred lock
//	return:
//		Success - true
//		Failure - false
//---------------------------------------------------------
int fdmc_thread_lock_delete(FDMC_THREAD_LOCK *plock)
{
	FUNC_NAME("fdmc_thread_lock_delete");

	dbg_print();
	if(!plock)
	{
		return 0;
	}
	fdmc_mutex_delete(plock->lock);
	fdmc_free(plock, NULL);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_thread_lock
//---------------------------------------------------------
//  purpose: lock thread_lock
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		lock - thread_lock descriptor
//		err - Error descriptor
//	return:
//		Success - true
//		Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
void fdmc_thread_lock(FDMC_THREAD_LOCK *lock, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_thread_lock");
	FDMC_THREAD_ID id;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(lock == NULL) return;
		id = fdmc_thread_getid();
		// Check if thread is owner of mutex
		if(fdmc_thread_eq(lock->lock_owner, id))
		{
			// Increment lock and return with success
			lock->lock_count ++;
			return;
		}
		fdmc_mutex_lock(lock->lock, &x);
		lock->lock_count = 1;
		memcpy(&lock->lock_owner, &id, sizeof(id));
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
}

//---------------------------------------------------------
//  name: fdmc_thread_trylock
//---------------------------------------------------------
//  purpose: lock thread_lock
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		lock - thread_lock descriptor
//		err - Error descriptor
//	return:
//		Success - true
//		Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_thread_trylock(FDMC_THREAD_LOCK *lock, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_thread_trylock");
	FDMC_THREAD_ID id;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(lock, "lock", x);
		id = fdmc_thread_getid();
		// Check if thread is owner of mutex
		if(fdmc_thread_eq(lock->lock_owner, id))
		{
			// Increment lock and return with success
			lock->lock_count ++;
			return 1;
		}
		if(fdmc_mutex_trylock(lock->lock, &x))
		{	
			lock->lock_count = 1;
			memcpy(&lock->lock_owner, &id, sizeof(id));
			return 1;
		}
		return 0;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
}

//---------------------------------------------------------
//  name: fdmc_thread_unlock
//---------------------------------------------------------
//  purpose: lock thread_unlock
//  designer: Serge Borisov (BSA)
//  started: 23.05.10
//	parameters:
//		lock - thread_lock descriptor
//		err - Error descriptor
//	return: void
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
void fdmc_thread_unlock(FDMC_THREAD_LOCK *lock, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_thread_unlock");
	FDMC_THREAD_ID id;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(!lock) return;
		id = fdmc_thread_getid();
		// Check if thread is owner of mutex
		if(!fdmc_thread_eq(id, lock->lock_owner))
		{
			fdmc_raisef(FDMC_MUTEX_ERROR, &x, "thread that try to unlock mutex is not the owner");
		}
		lock->lock_count --;
		if(lock->lock_count <= 0)
		{
			memset(&lock->lock_owner, 0, sizeof(lock->lock_owner));
			lock->lock_count = 0;
			fdmc_mutex_unlock(lock->lock);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
}

// 
static FDMC_THREAD_LOCK *ltime_mut = NULL;
//---------------------------------------------------------
//  name: fdmc_localtime
//---------------------------------------------------------
//  purpose:fdmc thread safe localtime function
//  designer: Serge Borisov (BSA)
//  started: 21/09/10
//	parameters:
//		src - system time
//		dest - pointer to tm structure
//	return:
//		On Success - pointer to tm structure
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
void* fdmc_localtime(time_t *src, struct tm *dest)
{
//	FUNC_NAME("fdmc_localtime");
	struct tm *tms;


	if(ltime_mut == NULL)
	{
		ltime_mut = fdmc_thread_lock_create(NULL);
		if(!ltime_mut)
			return NULL;
	}
	fdmc_thread_lock(ltime_mut, NULL);
	tms = localtime(src);
	if(tms)
	{
		memcpy(dest, tms, sizeof(struct tm));
		tms = dest;
	}
	fdmc_thread_unlock(ltime_mut, NULL);
	return tms;
}

FDMC_ARG *fdmc_arg_create(int argc, char *argv[], char **envp, char *cmd)
{
//	FUNC_NAME("fdmc_arg");
	FDMC_ARG *arg_new;

	arg_new = fdmc_malloc(sizeof(*arg_new), NULL);
	if(arg_new)
	{
		arg_new->argc = argc;
		arg_new->argv = argv;
		arg_new->envp = envp;
		arg_new->command_line = cmd;
	}
	return arg_new;
}


//---------------------------------------------------------
//  name: argv_to_cmdline
//---------------------------------------------------------
//  purpose: convert main() programme arguments to WinMain
//  designer: Serge Borisov (BSA)
//  started: 23/09/10
//	parameters:
//		arg - pointer to argument structure
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int argv_to_cmdline(FDMC_ARG *arg, FDMC_EXCEPTION *err)
{
	FUNC_NAME("argv_to_cmdline");
	int i;
	size_t n;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(arg, "arg", x);
		for(n = 0, i = 1; i < arg->argc; i ++)
		{
			CHECK_NULL(arg->argv[i], ".argv[i]", x);
			n += strlen(arg->argv[i]);
		}
		arg->command_line = fdmc_malloc(n + arg->argc + 10, NULL);
		if(arg->command_line)
		{
			for(i = 1; i < arg->argc; i++)
			{
				strcat(arg->command_line, arg->argv[i]);
				strcat(arg->command_line, " ");
			}
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
//  name: cmdlne_to_argv
//---------------------------------------------------------
//  purpose: convert command line from WinMain to main() style
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		arg - pointer to arguments structure
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int cmdline_to_argv(FDMC_ARG *arg, FDMC_EXCEPTION *err)
{
	FUNC_NAME("cmdline_to_argv");
	FDMC_EXCEPTION x;
	char *c, *p;
	int n = 0, i;

	TRYF(x)
	{
		CHECK_NULL(arg, "arg", x);
		c = fdmc_strdup(arg->command_line, &x);
		p = strtok(c, " ");
		while (p)
		{
			n ++;
			p = strtok(NULL, " ");
		}
		fdmc_free(c, &x);
		if(n == 0) return 0;
		arg->argv = fdmc_malloc(sizeof(char*) * n, &x);
		c = fdmc_strdup(arg->command_line, &x);
		p = strtok(c, " ");
		i = 0;
		while(p)
		{
			fdmc_strdup(arg->argv[i], &x);
			p = strtok(NULL, " ");
		}
		fdmc_free(c, &x);
		arg->argc = n;
		return 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 0;
}

// Get current thread identifier
FDMC_THREAD_ID fdmc_thread_getid(void)
{
//	FUNC_NAME("fdmc_thread_getid");
	FDMC_THREAD_ID ret;
#if defined (_WINDOWS_32)
	ret = GetCurrentThreadId();
#elif defined (_UNIX_)
	ret = pthread_self();
#endif
	return ret;
}

// Compare two thread descriptors
int fdmc_thread_eq(FDMC_THREAD_ID thr1, FDMC_THREAD_ID thr2)
{
	//FUNC_NAME("fdmc_thread_id");
#if defined (_WINDOWS_32)
	if(thr1 == thr2) return 1;
	return 0;
#elif defined(_UNIX_)
	if(pthread_equal(thr1, thr2) != 0)
		return 1;
	return 0;
#endif
}

// 
//---------------------------------------------------------
//  name: fdmc_thread_this
//---------------------------------------------------------
//  purpose: Return current thread identifier using appication
//		thread hash. Usable only if thread was created using
//		fdmc_thread_create function.
//  designer: Serge Borisov (BSA)
//  started: 22/09/10
//	parameters:
//		
//	return:
//		On Success - current thread descriptor
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_THREAD *fdmc_thread_this()
{
	//FUNC_NAME("fdmc_thread_this");
	FDMC_THREAD_ID kthr; 
	FDMC_THREAD *ret;
	int cnt = 0;

	if(thread_hash == NULL)
		return NULL;
	kthr = fdmc_thread_getid();
	do
	{
		ret = (FDMC_THREAD*)fdmc_hash_entry_find(thread_hash, &kthr, NULL);
	} while (!ret && cnt++ < 10 && fdmc_msleep(10));
	return ret;
}

// Config tree
XML_TREE_NODE *config_xml;
//---------------------------------------------------------
//  name:fdmc_thread_init
//---------------------------------------------------------
//  purpose: Thread module intialiations
//  designer: Serge Borisov (BSA)
//  started: 22/09/10
//	parameters:
//		prefix - file prefix for mainstream
//		recreate - stream creation flag
//		err - exception handler
//	return:
//		On Success - true
//		On Failure -  false
//---------------------------------------------------------
int fdmc_thread_init(char *prefix, int recreate)
{
	FUNC_NAME("fdmc_thread_init");
	FDMC_THREAD *t;
	FDMC_THREAD_ID tid;
	FDMC_EXCEPTION x, x1;
	size_t namelen;
	char *conf_dir; // Config directory
	char *conf_name; // Name of config file

	// Duplicate stdout file
	dup_screen();
	TRYF(x)
	{
		conf_dir = getenv("FDMC_CONFIG");
		if(conf_dir == NULL) conf_dir = ".";
		namelen = strlen(prefix) + strlen(conf_dir);
		conf_name = fdmc_malloc(namelen + 16, &x);
		sprintf(conf_name, "%s%c%s.xml", conf_dir, DIR_CHAR, prefix);
		TRYF(x1)
		{
			config_xml = fdmc_parse_xml_file(conf_name, &x1);
		}
		EXCEPTION
		{
			if(x1.errorcode == FDMC_SYNTAX_ERROR)
			{
				screen_trace("%s", x1.errortext);
			}
		}
		if(!config_xml)
		{
			;
			//func_trace("%s was not processed. Use defaults", conf_name);
		}
		else
		{
			XML_TREE_NODE *outnode;
			outnode = fdmc_xml_find_node(config_xml, "outfiles", 1, NULL);
			if(outnode)
			{
				outfiles = fdmc_xml_find_data(outnode, NULL);
			}
		}
		if(!recreate)
		{
			mainstream = fdmc_sfdopen(prefix, fileno(stdout));
		}
		else
		{
			mainstream = fdmc_sfdcreate(prefix, fileno(stdout));
		}
		thread_hash = fdmc_hash_table_create(1024, thread_hash_func, thread_list_proc, &x);
		t = fdmc_malloc(sizeof(*t), &x);
		tid = fdmc_thread_getid();
		printf("mainthread created with id %u\n", tid);
		memcpy(&t->thread_id, &tid, sizeof(tid));
		t->stream = mainstream;
		mainthread = t;
		//fprintf(stderr, "before hash add\n");
		fdmc_hash_entry_add(thread_hash, t, &x);
		//fprintf(stderr, "after hash add\n");
		srand((unsigned)time(NULL));
		// Create mainstream file (stdout)
	}
	EXCEPTION
	{
		trace("Initialization failure.\n%s", x.errortext);
		return 0;
	}
	return 1;
}

