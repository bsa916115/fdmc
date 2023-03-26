#include "fdmc_global.h"
#include "fdmc_queue.h"
#include "fdmc_ip.h"
#include "fdmc_thread.h"

FDMC_QUEUE *fdmc_queue_create(FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_queue_create");
	FDMC_QUEUE* ret;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		ret = fdmc_malloc(sizeof(*ret), &x);
		ret->queue = fdmc_list_create(NULL, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = NULL;
	}
	return ret;
}

int fdmc_queue_delete(FDMC_QUEUE *qptr, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_queue_delete");
	int ret;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(qptr == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "NULL queue parameter");
		}
		fdmc_list_delete(qptr->queue, NULL);
		fdmc_free(qptr, &x);
		ret = 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

int fdmc_queue_get(FDMC_QUEUE *qptr, void **data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_queue_get");
	FDMC_EXCEPTION x;
//	fd_set fds;
//	struct timeval tv;
	int ret;

	TRYF(x)
	{
		ret = 1;
		if(qptr == NULL)
		{
			fdmc_raisef(FDMC_QUEUE_ERROR, &x, "NULL queue parameter");
		}
		if(qptr->queue->first)
		{
			*data = qptr->queue->first->data;
		}
		else
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, "fdmc queue is empty");
		}
		fdmc_list_entry_delete(qptr->queue->first, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

int fdmc_queue_get_wait(FDMC_QUEUE *qptr, void **data, int sec, int usec, 
						int attempts, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_queue_get_wait");
	FDMC_EXCEPTION x;
	int ret;
	int attempts_left = attempts;

	ret = 1;
	TRYF(x)
	{
		if(qptr == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "NULL queue parameter");
		}
		while (!qptr->queue->first)
		{
			fdmc_delay(sec, usec);
			if(attempts > 0)
			{
				attempts_left --;
				if(attempts_left <= 0)
				{
					fdmc_raisef(FDMC_TIMEOUT, &x, "No data in queue for specified period");
				}
			}
		}
		*data = qptr->queue->first->data;
		fdmc_list_entry_delete(qptr->queue->first, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

int fdmc_queue_put(FDMC_QUEUE *qptr, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_queue_put");
	int ret;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(qptr == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "NULL queue pointer");
		}
		fdmc_list_entry_add(qptr->queue, fdmc_list_entry_create(data, &x), &x);
		ret = 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

