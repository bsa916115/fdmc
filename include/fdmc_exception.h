#ifndef _FDMC_EXEPTIONS_H
#define _FDMC_EXEPTIONS_H


#include "fdmc_global.h"
#include "fdmc_logfile.h"


extern FDMC_EXCEPTION mainerr;

#define TRY(handler) (handler).errortext[0] = 0; (handler).errorsubcode=0; \
if(((handler).errorcode = setjmp((handler).cpu)) == 0)
#define EXCEPTION else
#define EXCEPT(handler, x) else if((handler).errorcode == (x))
extern int fdmc_raise(int code, FDMC_EXCEPTION *handler);
extern int fdmc_raisef(int code, FDMC_EXCEPTION *handler,
	char *fmt, ...);
extern int fdmc_sraise(int code, FDMC_EXCEPTION *handler,
	FDMC_LOGSTREAM *s, char *fmt, ...);
extern int fdmc_raiseup(FDMC_EXCEPTION *src, FDMC_EXCEPTION *dst);
extern char *fdmc_strdup(char *src, FDMC_EXCEPTION *err);
extern void *fdmc_malloc(size_t nbytes, FDMC_EXCEPTION *ex);
extern int fdmc_free(void *ptr, FDMC_EXCEPTION *ex);
extern FILE *fdmc_fopen(char *name, char *mode, FDMC_EXCEPTION *err);
size_t fdmc_fwrite(void *buf, size_t size, size_t count, FILE *f, FDMC_EXCEPTION *err);
size_t fdmc_fread(void *buf, size_t size, size_t count, FILE *f, FDMC_EXCEPTION *err);

#define TRYF(handler) \
sprintf((handler).errortext, "%.*s:", FDMC_MAX_ERRORTEXT-1, _function_id); (handler).errorsubcode=0; \
if(((handler).errorcode = setjmp((handler).cpu)) == 0)
#endif

#define CHECK_NULL(ptr, name, err) \
	if((ptr) == NULL) fdmc_raisef(FDMC_PARAMETER_ERROR, &(err), "%s:Invalid parameter value(NULL):%s\n", \
	_function_id, (name))

#define CHECK_ZERO(val, name, err) \
	if(!(val)) fdmc_raisef(FDMC_PARAMETER_ERROR, &(err), "%s:Invalid value (ZERO) of %s", _function_id, (name))

#define CHECK_NEGZERO(val, name, err) \
	if((val) <= 0) fdmc_raisef(FDMC_PARAMETER_ERROR, &(err), "%s:Invalid value (NEEGATIVE OR ZERO) of %s", _function_id, (name))

#define CHECK_NEG(val, name, err) \
	if((val) < 0) fdmc_raisef(FDMC_PARAMETER_ERROR, &(err), "%s:Invalid value (NEEGATIVE) of %s", _function_id, (name))
