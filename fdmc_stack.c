#include "fdmc_stack.h"
#include "fdmc_logfile.h"
#include "fdmc_thread.h"
#include "fdmc_ip.h"
#include "fdmc_exception.h"

MODULE_NAME("fdmc_stack.c");

//---------------------------------------------------------
//  name: fdmc_stack_count
//---------------------------------------------------------
//  purpose: return number of elements in stack
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		pst - stack pointer
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_stack_count(FDMC_STACK *pst, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_count");
	int ret = 0;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(pst, "pst", x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return ret;
	}
	ret = pst->stack_list->count;
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_stack_create
//---------------------------------------------------------
//  purpose: create new stack structure
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		err - error handler
//	return:
//		On Success - New stack structure
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_STACK *fdmc_stack_create(FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_create");
	FDMC_EXCEPTION x;
	FDMC_STACK *ret = NULL; 

	TRYF(x)
	{
		ret = (FDMC_STACK*)fdmc_malloc(sizeof(*ret), &x);
		ret->stack_list = fdmc_list_create(NULL, &x);
		return ret;
	}
	EXCEPTION
	{
		if(ret)
		{
			fdmc_free(ret->stack_list, NULL);
		}
		fdmc_free(ret, NULL);
		fdmc_raiseup(&x, err);
		ret = NULL;
	}
	return ret;
}


//---------------------------------------------------------
//  name: fdmc_stack_delete
//---------------------------------------------------------
//  purpose: delete stack structure from memory
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		pst - Stack pointer
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_stack_delete(FDMC_STACK *pst)
{
	FUNC_NAME("fdmc_stack_delete");
	FDMC_EXCEPTION x;
	int ret;

	TRYF(x)
	{
		if(pst == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "null stack pointer");
		}
		fdmc_list_delete(pst->stack_list, &x);
		fdmc_free(pst, &x);
		ret = 1;
	}
	EXCEPTION
	{
		err_trace("%s", x.errortext);
		ret = 0;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_stack_push
//---------------------------------------------------------
//  purpose: push data into stack
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		pst - stack pointer
//		data - data to store
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_stack_push(FDMC_STACK *pst, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_push");
	FDMC_EXCEPTION x;
	int ret;

	TRYF(x)
	{
		CHECK_NULL(pst, "pst", x);
		fdmc_list_entry_add(pst->stack_list, fdmc_list_entry_create(data, &x), &x);
		ret = 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_stack_pop
//---------------------------------------------------------
//  purpose: get data from stack
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		pst - stack pointer
//		data - pointer to buffer
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_stack_pop(FDMC_STACK *pst, void **data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_pop");
	FDMC_EXCEPTION x;
	int ret;

	TRYF(x)
	{
		CHECK_NULL(pst, "pst", x);
		if(pst->stack_list->first == NULL)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, "fdmc stack is empty");
		}
		*data = pst->stack_list->last->data;
		fdmc_list_entry_delete(pst->stack_list->last, &x);
		ret = 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = 0;
	}
	return ret;
}

//-------------------------------------------
// Name: fdmc_stack_lock
// 
//-------------------------------------------
// Purpose: lock stack list for exclusive use
// 
//-------------------------------------------
// Parameters: 
// p_stack - pointer to fdmc stack
//-------------------------------------------
// Returns:
// 1 - success
// 0 - failure
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_stack_lock(FDMC_STACK *p_stack, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_lock");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(p_stack, "p_stack", x);
		fdmc_thread_lock(p_stack->stack_list->lock, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_stack_unlock
// 
//-------------------------------------------
// Purpose: unlock stack list for other threads
// 
//-------------------------------------------
// Parameters: 
// p_stack - pointer to fdmc stack
//-------------------------------------------
// Returns:
// 1 - success
// 0 - failure
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_stack_unlock(FDMC_STACK *p_stack, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_unlock");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(p_stack, "p_stack", x);
		fdmc_thread_unlock(p_stack->stack_list->lock, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

/************************** File Stack *******************************/

//---------------------------------------------------------
//  name: fdmc_fst_create
//---------------------------------------------------------
//  purpose: Create new file stack (constructor)
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		fst_file - name for stack file
//		fst_item_size - size of file record
//		err - error handler
//	return:
//		On Success - new file stack
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_FILE_STACK *fdmc_fst_create(char *fst_file, unsigned fst_item_size, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fst_create");
	FDMC_EXCEPTION x;
	FDMC_FILE_STACK *new_stack = NULL;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(fst_file, "fst_file", x);
		CHECK_ZERO(fst_item_size, "fst_item_size", x);
		new_stack = fdmc_malloc(sizeof(FDMC_FILE_STACK), &x);
		new_stack->fst_item_size = fst_item_size;
		new_stack->fst_name = fdmc_strdup(fst_file, &x);
		new_stack->fst_file = fdmc_fopen(fst_file, "w+b", &x);
		setbuf(new_stack->fst_file, 0);
		return new_stack;
	}
	EXCEPTION
	{
		if(new_stack)
		{
			fdmc_free(new_stack->fst_name, NULL);
			fdmc_free(new_stack, NULL);
		}
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

//---------------------------------------------------------
//  name: fdmc_fst_open
//---------------------------------------------------------
//  purpose: openes file stack with existing file
//  designer: Serge Borisov (BSA)
//  started: 20.01.11
//	parameters:
//		fst_file - name for stack file
//		fst_item_size - size of file record
//		err - error handler
//	return:
//		On Success - new file stack
//		On Failure - null
//		err - error handler
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FDMC_FILE_STACK *fdmc_fst_open(char *fst_file, unsigned fst_item_size, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fst_open");
	FDMC_EXCEPTION x;
	FDMC_FILE_STACK *new_stack;
	long fpos;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(fst_file, "fst_file", x);
		CHECK_ZERO(fst_item_size, "fst_item_size", x);
		new_stack = fdmc_malloc(sizeof(FDMC_FILE_STACK), &x);
		new_stack->fst_item_size = fst_item_size;
		new_stack->fst_name = fdmc_strdup(fst_file, &x);
		new_stack->fst_file = fdmc_fopen(fst_file, "a+b", &x);
		setbuf(new_stack->fst_file, 0);
		fpos = ftell(new_stack->fst_file);
		new_stack->fst_items = fpos / fst_item_size;
		return new_stack;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

//---------------------------------------------------------
//  name: fdmc_fst_delete
//---------------------------------------------------------
//  purpose: destructor of file stack
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		fst - file stack
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_fst_delete(FDMC_FILE_STACK *fst, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fst_delete");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(fst, "fst", x);
		fclose(fst->fst_file);
		unlink(fst->fst_name);
		fdmc_free(fst->fst_name, NULL);
		fdmc_free(fst, NULL);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_fst_push
//---------------------------------------------------------
//  purpose: push data into stack
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		fst - stack pointer
//		data - pointer to data
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_fst_push(FDMC_FILE_STACK *fst, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_stack_push");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(fst, "fst", x);
		CHECK_NULL(fst->fst_file, ".fst_file", x);
		fdmc_fwrite(data, 1, fst->fst_item_size, fst->fst_file, &x);
		fst->fst_items ++;
		return 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 0;
}

//---------------------------------------------------------
//  name: fdmc_fst_pop
//---------------------------------------------------------
//  purpose:
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		fst - stack pointer
//		data - pointer to buffer
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_fst_pop(FDMC_FILE_STACK *fst, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fst_pop");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		// Check parameters
		CHECK_NULL(fst, "fst", x);
		CHECK_NULL(fst->fst_file, ".fst_file", x);
		CHECK_NULL(data, "data", x);
		CHECK_ZERO(fst->fst_items, ".fst_items", x);
		// Pop data 
		fseek(fst->fst_file, fst->fst_item_size * (fst->fst_items - 1), SEEK_SET);
		fdmc_fread(data, 1, fst->fst_item_size, fst->fst_file, &x);
		// decrement couter and seek to the previous position
		fst->fst_items --;
		fseek(fst->fst_file, fst->fst_item_size * fst->fst_items, SEEK_SET);
		chsize(fileno(fst->fst_file), fst->fst_item_size * fst->fst_items);
		return 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 0;
}
