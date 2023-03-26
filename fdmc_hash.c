#include "fdmc_global.h"
#include "fdmc_hash.h"
#include "fdmc_logfile.h"
#include "fdmc_thread.h"

#include <malloc.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

MODULE_NAME("fdmc_hash.c");

// Calculate hash value from string
int fdmc_hash_value(FDMC_HASH_TABLE* table, void *data)
{
	unsigned char *key = (unsigned char*)data;
	register unsigned ukey;
	int sum = 0;
	int i=1, j = table->size;

	while(*key)
	{
		ukey = (unsigned)(*key);
		sum = sum + (ukey * 16) - (ukey%2?ukey%i:i);
		if(sum < 0) 
		{
			sum = -sum;
		}
		sum %= j;
		key ++;
		i++; 
	}
	return sum;
}

//Hash function for char keys
int fdmc_hash_proc(FDMC_HASH_TABLE *table, void *data)
{
	struct v {char *key; char _1[1];} *rec;
	int result;

	rec = (struct v*)data;
	result = fdmc_hash_value(table, rec->key);
	return result; 
}

// Hash function for integer keys
int fdmc_hashi_proc(FDMC_HASH_TABLE *table, void *data)
{
	struct v {int key; char _1[1];} *rec;
	int result;

	rec = (struct v*)data;
	result = rec->key % table->size;
	return result;
}

//---------------------------------------------------------
//  name: fdmc_hash_table_create
//---------------------------------------------------------
//  purpose: create new hash table
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		size - Hash table initial size
//		hash_function - Function to calculate key
//		process_entry - Function to manage hash table data lists
//		err - Error handler
//	return:
//		Success - New hash table
//		Failure - NULL
//  special features:
//		Performs long jump
//---------------------------------------------------------
FDMC_HASH_TABLE *fdmc_hash_table_create(int size, 
								   int (*hash_function)(FDMC_HASH_TABLE*, void*), 
								   int (*process_entry)(FDMC_LIST_ENTRY*,
								   void *parameters, int flags), FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hash_table_create");
	FDMC_LIST *cells = NULL;
	FDMC_HASH_TABLE *table = NULL;
	FDMC_EXCEPTION errf;
	int i;

	//dbg_trace();
	TRYF(errf)
	{
		table = (FDMC_HASH_TABLE*)fdmc_malloc(sizeof(FDMC_HASH_TABLE), &errf);
		table->size = size;
		table->hash_function = hash_function;
		table->cells = (FDMC_LIST*)fdmc_malloc(sizeof(FDMC_LIST)*size, &errf);
		for(i = 0; i < table->size; i++)
		{
			table->cells[i].process_entry = process_entry;
		}
		table->lock = fdmc_thread_lock_create(&errf);
	}
	EXCEPTION
	{
		if(table)
		{
			free(table);
			table = NULL;
		}
		if(cells)
		{
			fdmc_free(cells, NULL);
		}
		fdmc_raiseup(&errf, err);
	}
	return table;
}

//---------------------------------------------------------
//  name: fdmc_hash_entry_add
//---------------------------------------------------------
//  purpose: add new data to table
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		table - pointer to existing hash table
//		data - data to add to table
//		err - Error handler
//	return:
//		Success - true
//		Failure - false
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_hash_entry_add(FDMC_HASH_TABLE *table, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hash_entry_add");
	int hashvalue;
	FDMC_LIST *cell;
	FDMC_LIST_ENTRY *newentry;
	FDMC_EXCEPTION x;

	//dbg_print();
	TRYF(x)
	{
		CHECK_NULL(table, "table", x);
		CHECK_NULL(data, "data", x);
		hashvalue = (*(table->hash_function))(table, data);
		if(hashvalue >= table->size || hashvalue < 0)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, 
				"Invalid hash value %d.\nValid values are through 0 to %d", 
				hashvalue, table->size - 1);
		}
		cell = table->cells + hashvalue;
		// Lock this table to avoid parallel access from another thread
		fdmc_thread_lock(table->lock, &x);
		newentry = fdmc_list_entry_find(cell, data, &x);
		if(newentry != NULL)
		{
			fdmc_raisef(FDMC_UNIQUE_ERROR, &x, "Duplicated entry");
		}
		newentry = fdmc_list_entry_create(data, &x);
		fdmc_list_entry_add(cell, newentry, &x);
		fdmc_thread_unlock(table->lock, &x);
	}
	EXCEPTION
	{
		fdmc_thread_unlock(table->lock, NULL);
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	//func_trace("hash entry added");
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_hash_entry_delete
//---------------------------------------------------------
//  purpose: delete data from hash table
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		table - pointer to existing hash table
//		data - data to add to table
//		err - Error handler
//	return:
//		Success - true
//		Failure - false
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_hash_entry_delete(FDMC_HASH_TABLE *table, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hash_entry_delete");
	FDMC_LIST_ENTRY *entry; 
	FDMC_LIST *cell;
	FDMC_EXCEPTION x;
	int hashvalue, ret;

	TRYF(x)
	{
		CHECK_NULL(table, "table", x);
		CHECK_NULL(data, "data", x);
		hashvalue = (*(table->hash_function))(table, data);
		if(hashvalue >= table->size || hashvalue < 0)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, "Hash function returned invalid value %d", 
				hashvalue);
			return 0;
		}
		cell = table->cells + hashvalue;
		fdmc_thread_lock(table->lock, &x);
		entry = fdmc_list_entry_find(cell, data, &x);
		if(entry)
			ret = fdmc_list_entry_delete(entry, &x);
		fdmc_thread_unlock(table->lock, &x);
	}
	EXCEPTION
	{
		fdmc_thread_unlock(table->lock, NULL);
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_hash_entry_find
//---------------------------------------------------------
//  purpose: find data in hash table
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		table - pointer to existing hash table
//		data - data to add to table
//		err - Error handler
//	return:
//		Success - pointer to stored data
//		Failure - null
//  special features:
//		Performs long jump on error
//---------------------------------------------------------
void *fdmc_hash_entry_find(FDMC_HASH_TABLE *table, void *data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hash_entry_find");
	int hashvalue;
	FDMC_LIST *cell;
	FDMC_LIST_ENTRY *entry;
	FDMC_EXCEPTION x;
	void *ret;

	TRYF(x)
	{
		CHECK_NULL(table, "table", x);
		CHECK_NULL(data, "data", x);
		if(table->hash_function == NULL)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, "Invalid hash function pointer");
		}
		fdmc_thread_lock(table->lock, &x);
		hashvalue = (*(table->hash_function))(table, data);
		if(hashvalue >= table->size || hashvalue < 0)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, "Hash function returned invalid value %d", 
				hashvalue);
		}
		cell = table->cells + hashvalue;
		entry = fdmc_list_entry_find(cell, data, &x);
	}
	EXCEPTION
	{
		fdmc_thread_unlock(table->lock, NULL);
		fdmc_raiseup(&x, err);
		return NULL;
	}
	if(entry)
	{
		ret = entry->data;
	}
	else
	{
		ret = NULL;
	}
	fdmc_thread_unlock(table->lock, NULL);
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_hash_table_delete
//---------------------------------------------------------
//  purpose: delete hash table
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		table - pointer to existing hash table
//		err - Error handler
//	return:
//		Success - true
//		Failure - false
//  special features:
//		Performs long jump on error
//---------------------------------------------------------
int fdmc_hash_table_delete(FDMC_HASH_TABLE *table, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hash_table_delete");
	int i;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(table, "table", x);
		fdmc_thread_lock(table->lock, &x);
		for(i = 0; i < table->size; i++)
		{
			fdmc_list_delete(table->cells + i, &x);
		}
		fdmc_thread_unlock(table->lock, &x);
		fdmc_thread_lock_delete(table->lock);
		free(table);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}
