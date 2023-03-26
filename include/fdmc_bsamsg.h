#ifndef _FDMC_BSAMSG_INCLUDE_
#define _FDMC_BSAMSG_INCLUDE_

#include "fdmc_global.h"
#include "fdmc_thread.h"
#include "fdmc_msgfield.h"
#include "fdmc_exception.h"

extern int fdmc_int_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				 FDMC_EXCEPTION *err);
extern int fdmc_float_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				   FDMC_EXCEPTION *err);
extern int fdmc_char_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				  FDMC_EXCEPTION *err);
extern int fdmc_hostchar_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
					  FDMC_EXCEPTION *err);
extern int fdmc_binary_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
					FDMC_EXCEPTION *err);

extern int fdmc_bsa_message(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				  FDMC_EXCEPTION *err);

extern int fdmc_data_by_bsaname(char *buffer, char *name, char *data, int datasize);
extern int data_to_hex(char *dest, void *data, int datalen);
int hex_to_data(void *dest, char *source, int len);

#define BSAINT(var, name) {name, fdmc_int_bsafield, &var, "%d"}
#define BSAFLOAT(var, name) {name, fdmc_float_bsafield, &var, "%lf"}
#define BSACHAR(var, name) {name, fdmc_char_bsafield, var, "%s"}

#endif
