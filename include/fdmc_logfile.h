#ifndef _FDMC_LOGFILE_INCLUDE_
#define _FDMC_LOGFILE_INCLUDE_

#include <time.h>
#include "fdmc_global.h"
#include <stdio.h>

extern FDMC_LOGSTREAM* fdmc_screate(char *prefix);
extern FDMC_LOGSTREAM* fdmc_sopen(char *prefix);
extern int fdmc_sreopen(FDMC_LOGSTREAM *stream);

extern FDMC_LOGSTREAM* fdmc_sfdcreate(char *prefix, int handle);
extern FDMC_LOGSTREAM* fdmc_sfdopen(char *prefix, int handle);

extern int fdmc_sclose(FDMC_LOGSTREAM *stream);

extern int trace(char *fmt,...); // to thread stream
extern int time_trace(char *fmt, ...);
extern int fdmc_dump(void *, int);
extern int tprintf(char *fmt, ...);

extern int s_printf(FDMC_LOGSTREAM *stream, char *fmt, ...);
extern int s_trace(FDMC_LOGSTREAM *stream, char *fmt, ...);
extern int s_time_trace(FDMC_LOGSTREAM *stream, char *fmt, ...);
extern int s_dump(FDMC_LOGSTREAM *stream, void *data, int size);
extern int s_byte_print(FDMC_LOGSTREAM *stream, void *data, int size);


extern int regards(void);

extern FDMC_LOGSTREAM *mainstream;
#define MODULE_NAME(str) static char _module_id[] = str
#define FUNC_NAME(id) char* _function_id=id

#define LOGSTREAM_ID stream
#define dbg_sprint() s_printf(LOGSTREAM_ID, "%s:%s()", _module_id, _function_id)
#define dbg_strace() s_trace(LOGSTREAM_ID, "%s:%s()", _module_id, _function_id)
#define dbg_print() trace("%s:%s()", _module_id, _function_id)

#define dbg_trace dbg_print
#define func_trace tprintf("%s():", _function_id); trace
#define err_trace screen_time_trace("*** ERROR on %s:%s():", _module_id, _function_id); screen_trace

int scr_printf(char*, ...);
int screen_trace(char *fmt, ...); // to thread stream and screen
int screen_time_trace(char *fmt, ...);

// Create duplcate screen descriptor for using by screen_trace. 
// Use before any redirection.
void *dup_screen(void);
extern FILE* devnull;
// Log files dir
extern char *outfiles;
#endif

