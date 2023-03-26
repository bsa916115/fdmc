#ifndef _FDMC_LIST_H
#define _FDMC_LIST_H

#include "fdmc_global.h"


// Creating, adding, removing, and other usefull list functions
extern FDMC_LIST *fdmc_list_create(	int (*process_entry)(FDMC_LIST_ENTRY*,
		void *parameters, int flags), FDMC_EXCEPTION *err);
extern FDMC_LIST_ENTRY *fdmc_list_entry_allocate(FDMC_EXCEPTION *err);

extern FDMC_LIST_ENTRY *fdmc_list_entry_create(void *data, FDMC_EXCEPTION *err);

extern FDMC_LIST_ENTRY *fdmc_list_entry_add(FDMC_LIST *list, FDMC_LIST_ENTRY *entry, FDMC_EXCEPTION *err);

extern FDMC_LIST_ENTRY *fdmc_list_entry_find(FDMC_LIST *list, void *data, FDMC_EXCEPTION *err);

extern int fdmc_list_entry_delete(FDMC_LIST_ENTRY *entry, FDMC_EXCEPTION *err);

extern int fdmc_list_delete(FDMC_LIST *list, FDMC_EXCEPTION *err);

extern int fdmc_list_sort(FDMC_LIST *list);

extern int fdmc_list_process(FDMC_LIST *list, int (*action)(FDMC_LIST_ENTRY *list));
extern int fdmc_list_proc(FDMC_LIST_ENTRY *ent, void *data, int flag);
extern int fdmc_listi_proc(FDMC_LIST_ENTRY *ent, void *data, int flag);

struct _FDMC_FILE_LIST
{
	FILE *flist_file;
	char *flist_name;
	unsigned flist_first;
	unsigned flist_last;
	unsigned flist_size;
	unsigned flist_count;
};

typedef struct _FDMC_FILE_LIST FDMC_FILE_LIST;

FDMC_FILE_LIST *fdmc_flist_create(char *name, unsigned item_size, FDMC_EXCEPTION *err);
FDMC_FILE_LIST *fdmc_flist_open(char *name, unsigned item_size, FDMC_EXCEPTION *err);

struct _FDMC_FILE_LIST_ENTRY
{
	unsigned first;
	unsigned next;
	FDMC_FILE_LIST *owner;
};


#endif
