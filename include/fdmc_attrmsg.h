#ifndef _FDMC_ATTRMSG_INCLUDE_
#define _FDMC_ATTRMSG_INCLUDE_

#include "fdmc_global.h"
#include "fdmc_thread.h"
#include "fdmc_msgfield.h"

extern int fdmc_int_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				 FDMC_EXCEPTION *err);
extern int fdmc_float_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				   FDMC_EXCEPTION *err);
extern int fdmc_char_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				  FDMC_EXCEPTION *err);
extern int fdmc_hostchar_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
					  FDMC_EXCEPTION *err);
extern int fdmc_binary_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
					FDMC_EXCEPTION *err);

extern int fdmc_attr_message(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				  FDMC_EXCEPTION *err);

#endif
