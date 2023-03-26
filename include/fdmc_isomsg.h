#ifndef _FDMC_ISOMSG_INCLUDE_
#define _FDMC_ISOMSG_INCLUDE_

#include "fdmc_global.h"
#include "fdmc_logfile.h"
#include "fdmc_msgfield.h"
#include "fdmc_exception.h"

extern int fdmc_int_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_float_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_char_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_hostchar_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_bitmap_field(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_binary_field(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_iso_message(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
#endif
