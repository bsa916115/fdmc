#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>
#include <errno.h>

#include "fdmc_global.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"
#include "fdmc_thread.h"

// Public global error handler
FDMC_EXCEPTION mainerr;

//---------------------------------------------------------
//  name: fdmc_raise
//---------------------------------------------------------
//  purpose: perform long jump to specifed location
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		code - Application error code
//		handler - Exception structure 
//	return:
//		1 - When handler is null
//		no return else
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_raise(int code, FDMC_EXCEPTION *handler)
{
	if(handler == NULL)
		return 1;
	handler->errorcode = code;
	longjmp(handler->cpu, code);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_raisef
//---------------------------------------------------------
//  purpose: perform long jump to specifed location 
//		and set error text for exception
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		code - Application error code
//		handler - Exception structure 
//		fmt - Error text in printf() format
//	return:
//		1 - When handler is null
//		no return else
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_raisef(int code, FDMC_EXCEPTION *handler, char* fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	if(handler == NULL)
	{
		FDMC_THREAD *t;
		t = fdmc_thread_this();
		if(t != NULL && t->stream != NULL)
		{
			fprintf(t->stream->file, "UNHANDLED EXCEPTION:%d:", code);
			vfprintf(t->stream->file, fmt, argptr);
			fprintf(t->stream->file, "\n");
		}
	}
	else
	{
		int l = strlen(handler->errortext);
		if(0 && devnull)
		{
			int chk;
			chk = vfprintf(devnull, fmt, argptr);
			if(chk+l > FDMC_MAX_ERRORTEXT)
			{
				FDMC_THREAD *mythread = fdmc_thread_this();
				tprintf("TOO BIG ERROR MESSAGE:");
				if(!mythread) mythread = mainthread;
				vfprintf(mythread->stream->file, fmt, argptr);
			}
		}
		else
		{
			vsprintf(handler->errortext+l, fmt, argptr);
		}
	}
	va_end(argptr);
	if(!handler) return 1;
	handler->errorcode = code;
	longjmp(handler->cpu, code);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_raiseup
//---------------------------------------------------------
//  purpose: perform long jump to specifed location
//		using information from source structure
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		src - Exception structure containing exception information
//		dst - Exception structure to long jump to
//	return:
//		1 - When dst handler is null
//		no return else
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_raiseup(FDMC_EXCEPTION *src, FDMC_EXCEPTION *dst)
{
	int j;

	if(!src) return 0;
	if(dst) dst->errorsubcode = src->errorsubcode;
	j = fdmc_raisef(src->errorcode, dst, "%s", src->errortext);
	return j;
}

//---------------------------------------------------------
//  name: fdmc_strdup
//---------------------------------------------------------
//  purpose: allocate memory for new string and copy source
//		string into it
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		src - Source string
//		err - Exception handler
//	return:
//		Success - Pointer to new string
//		Failure - NULL or perform long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
char *fdmc_strdup(char *src, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_strdup");
	FDMC_EXCEPTION x;
	char *ret;
	int l;
	TRYF(x)
	{
		CHECK_NULL(src, "src", x);
		l = strlen(src);
		ret = fdmc_malloc(l+1, &x);
		strcpy(ret, src);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		ret = NULL;
	}
	return ret;
}

//---------------------------------------------------------
//  name: fdmc_malloc
//---------------------------------------------------------
//  purpose: allocate nbytes of heap memory and initiate it
//		by zeroes
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		nbytes - Number of bytes to allocate
//		ex - Exception handler
//	return:
//		Success - Pointer to allocated memory
//		Failure - NULL or perform long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
void *fdmc_malloc(size_t nbytes, FDMC_EXCEPTION *ex)
{
	FUNC_NAME("fdmc_malloc");
	FDMC_EXCEPTION x;
	void *result;
	int nerr;

	TRYF(x)
	{
		result = malloc(nbytes);
		nerr = errno;
		if(result == NULL)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, &x, "Cannot allocate %ld bytes of memory:\n%s", 
				nbytes, strerror(nerr));
			return NULL;
		}
		memset(result, 0, nbytes);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, ex);
		return NULL;
	}
	return result;
}

//---------------------------------------------------------
//  name: fdmc_free
//---------------------------------------------------------
//  purpose: free allocated memory
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		ptr - Pointer to allocated memory
//		ex - Exception handler
//	return:
//		Success - 1
//		Failure - 0 or perform long jump
//  special features:
//		Performs long jump
//---------------------------------------------------------
int fdmc_free(void *ptr, FDMC_EXCEPTION *ex)
{
	//FUNC_NAME("fdmc_free");

	if(ptr == NULL)
	{
		if(ex)
		{
			fdmc_raisef(FDMC_MEMORY_ERROR, ex, "fdmc_free:NULL pointer");
			return 0;
		}
	}
	else
		free(ptr);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_fopen
//---------------------------------------------------------
//  purpose: open file using exception processing
//  designer: Serge Borisov (BSA)
//  started: 24.09.10
//	parameters:
//		name - file name to open
//		mode - open mode like in fopen
//		err - error handler
//	return:
//		On Success - new file pointer
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
FILE *fdmc_fopen(char *name, char *mode, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fopen");
	FDMC_EXCEPTION x;
	FILE *f;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(name, "name", x);
		CHECK_NULL(mode, "mode", x);
		f = fopen(name, mode);
		if(f == NULL)
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "cannot open '%s'\n%s", name, strerror(errno));
		}
		return f;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

//---------------------------------------------------------
//  name: fdmc_fwrite
//---------------------------------------------------------
//  purpose: write data to file using exception handling
//  designer: Serge Borisov (BSA)
//  started: 20/01/2011
//	parameters:
//		buf - i/o buffer
//		size - size of item
//		count - number of items to write
//		f - file pointer
//		err - error handler
//	return:
//		On Success - number of items writtn
//		On Failure - 0
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
size_t fdmc_fwrite(void *buf, size_t size, size_t count, FILE *f, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fwrite");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		if(fwrite(buf, size, count, f) != count)
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "error writing to file %s", strerror(errno));
		}
		return count;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 0;
}

//---------------------------------------------------------
//  name: fdmc_fread
//---------------------------------------------------------
//  purpose: read data from file using exception handling
//  designer: Serge Borisov (BSA)
//  started: 20/01/2011
//	parameters:
//		buf - i/o buffer
//		size - size of item
//		count - number of items to read
//		f - file pointer
//		err - error handler
//	return:
//		On Success - number of items read
//		On Failure - 0
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
size_t fdmc_fread(void *buf, size_t size, size_t count, FILE *f, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_fwrite");
	FDMC_EXCEPTION x;
	size_t n;

	TRYF(x)
	{
		// Start error handling
		if((n=fread(buf, size, count, f)) == 0)
		{
			if(!feof(f))
			{
				fdmc_raisef(FDMC_IO_ERROR, &x, "error reading file %s", strerror(errno));
			}
		}
		return n;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 0;
}
