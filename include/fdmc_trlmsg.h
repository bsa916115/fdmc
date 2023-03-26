#ifndef _FDMC_TRLMSG_INCLUDE_
#define _FDMC_TRLMSG_INCLUDE_

#include "fdmc_global.h"
#include "fdmc_msgfield.h"
#include "fdmc_logfile.h"

extern int fdmc_int_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_float_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_char_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);
extern int fdmc_hostchar_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err);

#endif
