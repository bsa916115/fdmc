
// List module implementation

#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

#include "fdmc_global.h"
#include "fdmc_logfile.h"
#include "fdmc_list.h"
#include "fdmc_exception.h"
#include "fdmc_thread.h"

MODULE_NAME("fdmc_list.c");

//---------------------------------------------------------
//  name: fdmc_list_create
//---------------------------------------------------------
//  purpose: create new list structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		process_entry - pointer to structure function
//		err - Exception structure 
//	return:
//		Success - pointer to new structure
//		Failure - NULL or performs long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
FDMC_LIST *fdmc_list_create(
							int (*process_entry)(FDMC_LIST_ENTRY*,	void *parameters, int flags),
							FDMC_EXCEPTION *err)
{
	FDMC_LIST *newlist = NULL;
	FUNC_NAME("fdmc_list_create");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		newlist = fdmc_malloc(sizeof(FDMC_LIST), &x);
		newlist->lock = fdmc_thread_lock_create(&x);
	}
	EXCEPTION
	{
		if(newlist)
		{
			fdmc_thread_lock_delete(newlist->lock);
			fdmc_free(newlist, NULL);
		}
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return NULL;
	}
	newlist->process_entry = process_entry;
	return newlist;
}


//---------------------------------------------------------
//  name: fdmc_list_entry_allocate
//---------------------------------------------------------
//  purpose: create new list entry structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		err - Exception structure 
//	return:
//		Success - pointer to new structure
//		Failure - NULL or performs long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
FDMC_LIST_ENTRY *fdmc_list_entry_allocate(FDMC_EXCEPTION *err)
{
	FDMC_LIST_ENTRY *newentry;
	FUNC_NAME("fdmc_list_entry_allocate");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		newentry = fdmc_malloc(sizeof(*newentry), &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return newentry;
}


//---------------------------------------------------------
//  name: fdmc_list_entry_create
//---------------------------------------------------------
//  purpose: create new list entry structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		data - user data in list entry
//		err - Exception structure 
//	return:
//		Success - pointer to new structure
//		Failure - NULL or performs long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
FDMC_LIST_ENTRY *fdmc_list_entry_create(void *data, FDMC_EXCEPTION *err)
{
	FDMC_LIST_ENTRY *newentry;
	FUNC_NAME("fdmc_list_entry_create");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		newentry = fdmc_malloc(sizeof(*newentry), &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	newentry->data = data;
	return newentry;
}


//---------------------------------------------------------
//  name: fdmc_list_entry_add
//---------------------------------------------------------
//  purpose: add existing list entry structure to list structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		list - existing list structure
//		entry - existing list entry structure
//		err - Exception structure 
//	return:
//		Success - pointer to new structure
//		Failure - NULL or performs long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
FDMC_LIST_ENTRY *fdmc_list_entry_add(FDMC_LIST *list, FDMC_LIST_ENTRY *entry, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_list_entry_add");
	FDMC_EXCEPTION x;

	//dbg_print();
	TRYF(x)
	{
		CHECK_NULL(list, "list", x);
		CHECK_NULL(entry, "entry", x);
		fdmc_thread_lock(list->lock, &x);
		if(list->first == NULL)
		{
			list->first = entry;
		}
		else
		{
			list->last->next = entry;
		}
		entry->prior = list->last;
		entry->next = NULL;
		entry->list = list;
		list->last = entry;
		list->current = entry;
		list->count ++;
		if(list->process_entry != NULL)
		{
			int flag = FDMC_INSERT; 
			(*(list->process_entry))(entry, NULL, flag);
		}
		fdmc_thread_unlock(list->lock, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		entry = NULL;
	}
	return entry;
}

//---------------------------------------------------------
//  name: fdmc_list_entry_delete
//---------------------------------------------------------
//  purpose: delete existing list entry structure from list structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		entry - existing list entry structure
//		err - Exception structure 
//	return:
//		Success - pointer to new structure
//		Failure - NULL or performs long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------

int fdmc_list_entry_delete(FDMC_LIST_ENTRY *entry, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_list_entry_delete");
	FDMC_LIST *list;
	FDMC_EXCEPTION x;

	//dbg_print();
	TRYF(x)
	{
		CHECK_NULL(entry, "entry", x);
		CHECK_ZERO(entry->list, "entry.list", x);
		list = entry->list;
		fdmc_thread_lock(list->lock, &x);
		if(list->first == entry)
		{
			list->first = entry->next;
		}
		if(list->last == entry)
		{
			list->last = entry->prior;
		}
		if(entry->prior != NULL)
		{
			entry->prior->next = entry->next;
		}
		if(entry->next != NULL)
		{
			entry->next->prior= entry->prior;
		}
		if(list->process_entry != NULL)
		{
			int flag = FDMC_DELETE;
			(*(list->process_entry))(entry, NULL, flag);
		}
		free(entry);
		list->count --;
		list->current = NULL;
		fdmc_thread_unlock(list->lock, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_list_entry_find
//---------------------------------------------------------
//  purpose: find existing list entry structure in list structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		list - existing list structure
//		data - user data to find in list
//		err - Exception structure 
//	return:
//		Success - pointer to list entry
//		Failure - NULL or performs long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
FDMC_LIST_ENTRY *fdmc_list_entry_find(FDMC_LIST *list, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_list_entry_find");
	FDMC_LIST_ENTRY *ent;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(list, "list", x);
		if(list->first == NULL)
		{
			return NULL;
		}
		fdmc_thread_lock(list->lock, &x);
		ent = list->first;
		if(!list->process_entry)
		{
			return NULL;
		}
		while(ent)
		{
			int flag = FDMC_COMPARE;
			flag = (*(list->process_entry))(ent, data, flag);
			if(flag == FDMC_EQUAL)
			{
				break;
			}
			ent = ent->next;
		}
		list->current = ent;
		fdmc_thread_unlock(list->lock, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return ent;
}

//---------------------------------------------------------
//  name: fdmc_list_delete
//---------------------------------------------------------
//  purpose: delete existing list structure 
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		list - existing list structure
//		err - Exception structure 
//	return:
//		Success - 1
//		Failure - 0
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_list_delete(FDMC_LIST *list, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_list_delete");
	FDMC_LIST_ENTRY *ent, *next;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(list, "list", x);
		screen_trace("list is %lX", list);
		if(list->first == NULL)
		{
			return 1;
		}
		fdmc_thread_lock(list->lock, &x);
		ent = list->first;
		while(ent)
		{
			next = ent->next;
			if(list->process_entry)
			{
				int flag = FDMC_DELETE;
				(*(list->process_entry))(ent, NULL, flag);
			}
			free(ent);
			ent = next;
		}
		list->count = 0;
		list->first = list->last = list->current = NULL;
		fdmc_thread_unlock(list->lock, &x);
		fdmc_thread_lock_delete(list->lock);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

static int swap_list_entries(FDMC_LIST_ENTRY *p, FDMC_LIST_ENTRY *q)
{
	//	FUNC_NAME("swap_list_enries");

	void *w;

	if(p == NULL || q == NULL)
	{
		return 0;
	}
	w = p->data;
	p->data = q->data;
	q->data = w;
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_list_sort
//---------------------------------------------------------
//  purpose: sort existing list
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		list - existing list structure
//	return:
//		Success - 1
//		Failure - 0
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_list_sort(FDMC_LIST *list)
{
	FUNC_NAME("fdmc_list_sort");
	FDMC_LIST_ENTRY *p, *q;
	int flag, ret;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(list, "list", x);
		CHECK_ZERO(list->count, "list.count", x);
		CHECK_ZERO(list->process_entry, "list.process_entry", x);

		fdmc_thread_lock(list->lock, &x);
		for(p = list->first; p != NULL; p = p->next)
		{
			for(q = p->next; q != NULL; q = q->next)
			{
				flag = FDMC_COMPARE;
				flag = (*(list->process_entry))(p, q->data, flag); // p > q
				if(flag == FDMC_GREATER)
				{
					swap_list_entries(p, q);
				}
			}
		}
		ret = 1;
	}
	EXCEPTION
	{
		ret = 0;
	}
	fdmc_thread_unlock(list->lock, NULL);
	return ret;
}

// Comparison for char* keys
int fdmc_list_proc(FDMC_LIST_ENTRY *entry, void *data, int flag)
{
	//	FUNC_NAME("fdmc_list_proc");
	struct {char *key; char data[2];} *ent, *list;
	int r;

	//	dbg_trace();

	switch(flag)
	{
	case FDMC_INSERT:
		if(data)
		{
			entry->data = data;
		}
		return FDMC_OK;
	case FDMC_DELETE:
		//free(entry->data);
		return FDMC_OK;
	case FDMC_COMPARE:
		ent = entry->data;
		list = data;
		r = strcmp(ent->key, list->key);
		if(r > 0) return FDMC_GREATER;
		else if (r < 0) return FDMC_LESS;
		else return FDMC_EQUAL;
	}
	return 0;
}

// Comaprison for int keys
int fdmc_listi_proc(FDMC_LIST_ENTRY *entry, void *data, int flag)
{
	//	FUNC_NAME("fdmc_list_proc");
	struct {int key; char data[2];} *ent, *list;

	//	dbg_trace();

	switch(flag)
	{
	case FDMC_INSERT:
		if(data)
		{
			entry->data = data;
		}
		return FDMC_OK;
	case FDMC_DELETE:
		//free(entry->data);
		return FDMC_OK;
	case FDMC_COMPARE:
		ent = entry->data;
		list = data;
		if(ent->key > list->key)
		{
			return FDMC_GREATER;
		}
		if(ent->key < list->key)
		{
			return FDMC_LESS;
		}
		if(ent->key == list->key)
		{
			return FDMC_EQUAL;
		}
		break;
	}
	return 0;
}

