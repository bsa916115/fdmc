#ifndef _FDMC_STACK_INCLUDE_
#define _FDMC_STACK_INCLUDE_
#include "fdmc_global.h"
#include "fdmc_exception.h"
#include "fdmc_list.h"

// Traditional memory stack
typedef struct 
{
	FDMC_LIST *stack_list;
} 
FDMC_STACK;

extern FDMC_STACK *fdmc_stack_create(FDMC_EXCEPTION *err);
extern int fdmc_stack_delete(FDMC_STACK *pst);
extern int fdmc_stack_push(FDMC_STACK *pst, void *data, FDMC_EXCEPTION *err);
extern int fdmc_stack_pop(FDMC_STACK *pst, void **data, FDMC_EXCEPTION *err);
extern int fdmc_stack_count(FDMC_STACK *pst, FDMC_EXCEPTION *err);

int fdmc_stack_lock(FDMC_STACK *p_stack, FDMC_EXCEPTION *err);
int fdmc_stack_unlock(FDMC_STACK *p_stack, FDMC_EXCEPTION *err);

// File stack for large data
#include <stdio.h>

typedef struct
{
	FILE *fst_file; // File to store stack data
	unsigned fst_item_size; // Size of one stack item (file record)
	unsigned fst_items; // Number of items in stack;
	char *fst_name; // Buffer to hold file name
} FDMC_FILE_STACK;

extern FDMC_FILE_STACK *fdmc_fst_create(char *fst_file, unsigned fst_item_size, FDMC_EXCEPTION *err);
extern FDMC_FILE_STACK *fdmc_fst_open(char *fst_file, unsigned fst_item_size, FDMC_EXCEPTION *err);
extern int fdmc_fst_delete(FDMC_FILE_STACK *fst, FDMC_EXCEPTION *err);
extern int fdmc_fst_push(FDMC_FILE_STACK *fst, void *data, FDMC_EXCEPTION *err);
extern int fdmc_fst_pop(FDMC_FILE_STACK *fst, void *data, FDMC_EXCEPTION *err);

#endif
