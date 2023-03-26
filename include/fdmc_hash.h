#ifndef _FDMC_HASH_H
#define _FDMC_HASH_H

#include "fdmc_list.h"
#include "fdmc_global.h"
#include "fdmc_exception.h"


FDMC_HASH_TABLE *fdmc_hash_table_create(int size, 
		   int (*hash_function)(FDMC_HASH_TABLE*, void*), 
		   int(*process_entry)(FDMC_LIST_ENTRY*, void *parameters, int flags), 
		   FDMC_EXCEPTION *err);
extern int fdmc_hash_table_delete(FDMC_HASH_TABLE *table, FDMC_EXCEPTION *err);
extern void *fdmc_hash_entry_find(FDMC_HASH_TABLE *table, void *data, FDMC_EXCEPTION *err);
extern int fdmc_hash_entry_add(FDMC_HASH_TABLE *table, void *data, FDMC_EXCEPTION *err);
extern int fdmc_hash_entry_delete(FDMC_HASH_TABLE *table, void *data, FDMC_EXCEPTION *err);
extern int fdmc_hash_proc(FDMC_HASH_TABLE* table, void *data);
#define fdmc_hashc_proc fdmc_hash_proc
extern int fdmc_hashi_proc(FDMC_HASH_TABLE *table, void *data);

#endif
