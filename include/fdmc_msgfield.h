#ifndef _FDMC_MSGFIELDS_INCLUDE
#define _FDMC_MSGFIELDS_INCLUDE

#include "fdmc_bitop.h"
#include "fdmc_logfile.h"
#include "fdmc_thread.h"

/* Флаги выравнивания данных поля */
#define FDMC_ALIGN_DEFAULT 0
#define FDMC_ALIGN_LEFT 1
#define FDMC_ALIGN_RIGHT 2
#define FDMC_ALIGN_ZERO 3
#define FDMC_ALIGN_SIGN 4
#define FDMC_ALIGN_NONE 5

#define FDMC_BINARY_CODED 1
#define FDMC_BCD_CODED 2
#define FDMC_PRIVATE 3
#define FDMC_NOINIT 4
#define FDMC_PRESENTS 5
#define FDMC_INITIALIZED 6

struct _FDMC_MSGFIELD
{
	char *name; 
	int (*field_func)(struct _FDMC_MSGFIELD *field, char *buffer, 
		int op, FDMC_EXCEPTION *err); // field function
	void *data; // Data buffer
	char *format; // Length format
	int number; // Field number
	char *initstring; // Parameter string
	int size; // Formatted field size
	int scale; // Formatted field scale
	int lensize; // Field length format
	int align; // Field alignment
	int maxlen; // Maximum field length
	byte flags[16]; // Field additional
}; 

typedef struct _FDMC_MSGFIELD FDMC_MSGFIELD;

extern int debug_formats;

extern int fdmc_int_field(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);

extern int fdmc_float_field(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);

extern int fdmc_char_field(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);

extern int fdmc_hostchar_field(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);

extern int fdmc_plain_message(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);

extern int fdmc_process_message(FDMC_MSGFIELD *msg, char *buffer, int op, FDMC_EXCEPTION *err);
#define fdmc_format_message(msg, buf, err) fdmc_process_message(msg, buf, FDMC_FORMAT, err)
#define fdmc_extract_message(msg, buf, err) fdmc_process_message(msg, buf, FDMC_EXTRACT, err)
#define fdmc_print_message(msg, buf, err) fdmc_process_message(msg, buf, FDMC_PRINT, err)
#define fdmc_init_message(msg, buf, err) fdmc_process_message(msg, buf, FDMC_INIT, err)

int fdmc_parse_format(struct _FDMC_MSGFIELD *field);

int fdmc_parse_inintstring(struct _FDMC_MSGFIELD *field);

int fdmc_common_check_field(FDMC_MSGFIELD *field, FDMC_EXCEPTION *err);

FDMC_MSGFIELD* fdmc_clone_message(FDMC_MSGFIELD *pattern, void *host_addr);

int common_field_format(FDMC_MSGFIELD *field, char *buffer,
							   char *databuffer, int buflen, FDMC_EXCEPTION *err);

int common_get_buffer(FDMC_MSGFIELD *field, char *buffer,
							 char *databuffer, FDMC_EXCEPTION *err);

FDMC_MSGFIELD* fdmc_clone_message(FDMC_MSGFIELD *pattern, void *host_addr);

int isyes(char *c);
int fdmc_dump(void *bufp, int len);

FDMC_MSGFIELD *fdmc_field_by_name(FDMC_MSGFIELD *msg, char *name);
FDMC_MSGFIELD *fdmc_field_by_number(FDMC_MSGFIELD *msg, int number);

#define MAX_FIELD_SIZE 2048

#endif
