#ifndef _FDMC_QUEUE_INCLUDE_

#define _FDMC_QUEUE_INCLUDE_

#include "fdmc_global.h"
#include "fdmc_exception.h"
#include "fdmc_list.h"

FDMC_QUEUE *fdmc_queue_create(FDMC_EXCEPTION *err);
int fdmc_queue_delete(FDMC_QUEUE *qptr, FDMC_EXCEPTION *err);
int fdmc_queue_put(FDMC_QUEUE *qptr, void *data, FDMC_EXCEPTION *err);
int fdmc_queue_get(FDMC_QUEUE *qptr, void **data, FDMC_EXCEPTION *err);
int fdmc_queue_get_wait(FDMC_QUEUE *qptr, void **data, int sec, int usec, 
						int attempts, FDMC_EXCEPTION *err);
#endif

