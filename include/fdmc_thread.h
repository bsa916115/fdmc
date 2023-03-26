#ifndef _FDMC_THREAD_INCLUDE
#define _FDMC_THREAD_INCLUDE

#include "fdmc_global.h"

extern FDMC_THREAD *mainthread;

// Create thread with parameters 
extern FDMC_THREAD_HANDLE fdmc_create_thread(FDMC_THREAD_FUNC start_proc, void *param, 
									  FDMC_THREAD_ID *thread_id, int stack_size);

extern FDMC_THREAD *fdmc_thread_create(FDMC_THREAD_FUNC thr_start, void *thr_param,
									   char *thr_stream, int thr_stmode, int thr_stack,
									   int immediate,
									   FDMC_EXCEPTION *thr_err);

extern int fdmc_thread_start(FDMC_THREAD *pthr, FDMC_EXCEPTION *err);

extern int fdmc_thread_delete(FDMC_THREAD *pthr);

extern FDMC_MUTEX *fdmc_mutex_create(FDMC_EXCEPTION *err);
extern int fdmc_mutex_delete(FDMC_MUTEX *pmutex);
extern int fdmc_mutex_lock(FDMC_MUTEX *pmutex, FDMC_EXCEPTION *err);
extern int fdmc_mutex_unlock(FDMC_MUTEX *pmutex);

extern FDMC_THREAD_LOCK *fdmc_thread_lock_create(FDMC_EXCEPTION *err);
extern int fdmc_thread_lock_delete(FDMC_THREAD_LOCK *plock);

extern void fdmc_thread_lock(FDMC_THREAD_LOCK *lock, FDMC_EXCEPTION *err);
extern int fdmc_thread_trylock(FDMC_THREAD_LOCK *lock, FDMC_EXCEPTION *err);
extern void fdmc_thread_unlock(FDMC_THREAD_LOCK *lock, FDMC_EXCEPTION *err);

extern void* fdmc_localtime(time_t *src, struct tm *dest);
extern int fdmc_errstr(int code);
extern FDMC_ARG *fdmc_arg_create(int argc, char *argv[], char **envp, char *cmd);

extern FDMC_HASH_TABLE *thread_hash;
extern FDMC_THREAD_ID fdmc_thread_getid(void);
extern int fdmc_thread_eq(FDMC_THREAD_ID thr1, FDMC_THREAD_ID thr2);
extern FDMC_THREAD *fdmc_thread_this(void);
extern int fdmc_thread_init(char *prefix, int recreate);

extern int thread_list_proc(FDMC_LIST_ENTRY *entry, void *data, int flag);
extern FDMC_THREAD_LOCK *fdmc_thread_lock_create(FDMC_EXCEPTION *err);
extern int fdmc_thread_lock_delete(FDMC_THREAD_LOCK *plock);
extern XML_TREE_NODE *config_xml;

#endif
