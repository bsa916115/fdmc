#include "fdmc_global.h"
#include "fdmc_thread.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(_WINDOWS_32)
#include <io.h>
#define W_OK 0 
#elif defined(_UNIX_)
#include <unistd.h>
#endif

MODULE_NAME("fdmc_logfile.c");
static char namestring[] = "%s%c%s_%02d%02d%02d_%03d.log";
static FILE *screen;
FILE *devnull = NULL;
FDMC_LOGSTREAM *mainstream;
static FDMC_THREAD_LOCK *screen_lock = NULL;
// Note: outfiles variable is used to specify directory where
// all outfiles will be created. If it is null then outfiles
// will be created in current directory
char *outfiles = NULL;

//---------------------------------------------------------
//  name: fdmc_sopen
//---------------------------------------------------------
//  purpose: open i/o log stream
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		prefix - first part of file name
//	return:
//		Success - pointer to opened stream
//		Failure - null
//---------------------------------------------------------
FDMC_LOGSTREAM* fdmc_sopen(char *prefix)
{
	FUNC_NAME("s_open");
	size_t namelen;
	FILE *f;
	FDMC_EXCEPTION err;
	time_t tnow=time(NULL);
	struct tm tmnow, tmlog;
	char *newname = NULL;
	FDMC_LOGSTREAM *stream;
	char *outdir;

	if(prefix == NULL)
	{
		screen_trace("%s: Null prefix value", _function_id);
		return 0;
	}
	stream = malloc(sizeof(*stream));
	if(stream == NULL)
	{
		screen_trace("Cannot create new stream: %s\n", strerror(errno));
		return NULL;
	}
	memset(stream, 0, sizeof(*stream));
	stream->prefix = fdmc_strdup(prefix, NULL);
	//fprintf(stderr, "creating thread with prefix %s\n", stream->prefix);
	stream->mode = 0;
	namelen = strlen(prefix);
	outdir = outfiles ? outfiles : ".";
	namelen += strlen(outdir);
	stream->filename = malloc(namelen+32);
	if(stream->filename == NULL)
	{
		screen_trace("Cannot create new stream name: %s\n", strerror(errno));
		free(stream);
		return NULL;
	}
	sprintf(stream->filename, "%s%c%s.out", outdir, DIR_CHAR, prefix);
	TRY(err)
	{
		if(access(stream->filename, W_OK) != -1)
		{
			f = fdmc_fopen(stream->filename, "r", &err);
			if(f == NULL)
			{
				stream->errnum = errno;
				fdmc_raise(2, &err);
			}
			fscanf(f, "%ld%d", &stream->logtime, &stream->lognumber);
			fclose(f);
			tnow = time(NULL);
			fdmc_localtime(&tnow, &tmnow);
			fdmc_localtime(&stream->logtime, &tmlog);
			if(tmnow.tm_yday != tmlog.tm_yday || tmnow.tm_year != tmlog.tm_year)
			{
				newname = malloc(namelen + 32);
				if(newname == NULL)
				{
					fdmc_raisef(3, &err, "newname error");
				}
				sprintf(newname, namestring, outdir, DIR_CHAR, 
					prefix,
					tmlog.tm_year%100, tmlog.tm_mon+1, tmlog.tm_mday,
					stream->lognumber);
				if(rename(stream->filename, newname) == -1)
				{
					stream->errnum = errno;
					fdmc_raisef(4, &err, "rename error");
				}
				free(newname);
				newname = NULL;
				stream->file = fopen(stream->filename, "w");
				stream->fileno = fileno(stream->file);
				if(stream->file == NULL)
				{
					fdmc_raisef(5, &err, "fopen failed");
				}
				setbuf(stream->file, NULL);
				fprintf(stream->file, "%ld %d\n", (long)tnow, 1);
				fprintf(stream->file, "\nOutput file was opened %s\n", ctime(&tnow));
				stream->logtime = tnow;
				stream->lognumber = 1;
			}
			else
			{
				stream->file = fopen(stream->filename, "a");
				if(stream->file == NULL)
				{
					fdmc_raisef(6, &err, "Append failed");
				}
				setbuf(stream->file, NULL);
				fprintf(stream->file, "\nOutput file was continued %s\n",
					ctime(&tnow));
			}
		}
		else
		{
			stream->file = fopen(stream->filename, "w");
			if(stream->file == NULL)
			{
				fdmc_raisef(5, &err, "creation failed");
			}
			setbuf(stream->file, NULL);
			fprintf(stream->file, "%ld %d\n", (long)tnow, 1);
			fprintf(stream->file, "\nOutput file was opened %s\n", ctime(&tnow));
			stream->logtime = tnow;
			stream->lognumber = 1;
		}
		stream->lock = fdmc_thread_lock_create(&err);
		return stream;
	}
	EXCEPTION
	{
		screen_trace("%s", strerror(errno));
		screen_trace("Couldn't create new log stream, internal error %d\n",
			err.errorcode);
		if(newname != NULL)
		{
			free(newname);
		}
		if(stream->filename != NULL)
		{
			free(stream->filename);
		}
		free(stream);
		return NULL;
	}
}

//---------------------------------------------------------
//  name: fdmc_screate
//---------------------------------------------------------
//  purpose: Create new stream. If there are old stream file then rename it.
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		prefix - first part of file name
//	return:
//		Success - pointer to opened stream
//		Failure - null
//---------------------------------------------------------
FDMC_LOGSTREAM* fdmc_screate(char *prefix)
{
	FUNC_NAME("s_create");
	time_t tnow=time(NULL);
	FDMC_EXCEPTION err;
	char *newname = NULL;
	FILE *f;
	size_t namelen;
	struct tm tmnow, tmlog;
	FDMC_LOGSTREAM *stream;
	char *outdir;

	if(prefix == NULL)
	{
		err_trace("Null prefix value");
		return 0;
	}
	else
	{
		//printf("prefix is %s\n", prefix);
	}
	stream = malloc(sizeof(*stream));
	if(stream == NULL)
	{
		err_trace("Cannot create new stream: %s", strerror(errno));
		return NULL;
	}
	memset(stream, 0, sizeof(*stream));
	stream->prefix = fdmc_strdup(prefix, NULL);
	stream->mode = 1;
	namelen = strlen(prefix)+10;
	outdir = outfiles ? outfiles : ".";
	namelen += strlen(outdir);
	stream->filename = malloc(namelen+32);
	if(stream->filename == NULL)
	{
		err_trace("Cannot create new stream name: %s", strerror(errno));
		free(stream);
		return NULL;
	}
	sprintf(stream->filename, "%s%c%s.out", outdir, DIR_CHAR, prefix);
	TRY(err)
	{
		if(access(stream->filename, W_OK) != -1)
		{
			f = fopen(stream->filename, "r");
			if(f == NULL)
			{
				fdmc_raisef(2, &err, "Error checking file date");
			}
			fscanf(f, "%ld%d", &stream->logtime, &stream->lognumber);
			fclose(f);
			newname = malloc(namelen + 32);
			if(newname == NULL)
			{
				fdmc_raisef(3, &err, "new name error");
			}
			fdmc_localtime(&stream->logtime, &tmlog);
			sprintf(newname, namestring, outdir, DIR_CHAR,
				prefix,
				tmlog.tm_year%100, tmlog.tm_mon+1, tmlog.tm_mday,
				stream->lognumber);
			if(rename(stream->filename, newname) == -1)
			{
				printf("%s", strerror(errno));
				fdmc_raisef(4, &err, "error renaming file from %s to %s", stream->filename,
					newname);
			}
			free(newname);
			newname = NULL;
			fdmc_localtime(&tnow, &tmnow);
			if(tmnow.tm_year != tmlog.tm_year || tmlog.tm_yday != tmnow.tm_yday)
			{
				stream->lognumber = 1;
			}
			else
			{
				stream->lognumber ++;
			}
		}
		else
		{
			stream->lognumber = 1;
		}
		stream->logtime = tnow;
		stream->file = fdmc_fopen(stream->filename, "w", &err);
		stream->fileno = fileno(stream->file);
		setbuf(stream->file, NULL);
		fprintf(stream->file, "%ld %d\n", (long)stream->logtime, stream->lognumber);
		fprintf(stream->file, "\nOutput file was created %s\n", ctime(&tnow));
		stream->lock = fdmc_thread_lock_create(&err);
		return stream;
	}
	EXCEPTION
	{
		screen_trace("\n%s\nIn exception\n", _function_id);
		screen_trace("%s", err.errortext);
		if(newname != NULL)
		{
			free(newname);
		}
		if(stream->filename != NULL)
		{
			free(stream->filename);
		}
		free(stream);
//		exit(0);
		return NULL;
	}
}

//---------------------------------------------------------
//  name: fdmc_sreopen
//---------------------------------------------------------
//  purpose: Check stream. If stream file exceeds size
//		or date was changed, then close stream, rename it
//		and open new stream
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		stream - pointer to existing stream
//	return:
//		Success - pointer to opened stream
//		Failure - null
//---------------------------------------------------------
int fdmc_sreopen(FDMC_LOGSTREAM *stream)
{
	FUNC_NAME("s_reopen");
	time_t tnow = time(NULL);
	struct tm tmnow, tmlog;
	int recreate = 0, newdate = 0;
	int n;
	char *newname = 0;

	if(stream == NULL)
	{
		err_trace("invalid stream");
		return 0;
	}
	if(stream->max_size != 0)
	{
		long fpos = ftell(stream->file);
		if(fpos == -1)
		{
			err_trace("ftell('%s'): %s", stream->prefix, strerror(errno));
			return 0;
		}
		if(fpos > stream->max_size && stream->max_size > 0)
		{
			recreate = 1;
		}
	}
	fdmc_localtime(&tnow, &tmnow);
	fdmc_localtime(&stream->logtime, &tmlog);
	if(tmnow.tm_year != tmlog.tm_year || tmnow.tm_yday != tmlog.tm_yday)
	{
		newdate = 1;
	}
	//if(strcmp(stream->prefix, "bcard") == 0) newdate = 1;
	if(recreate || newdate )
	{
		newname = (char*)malloc((int)strlen(stream->filename) + 16);
		if(newname == NULL)
		{
			err_trace("'%s':malloc newname: %s", stream->prefix, strerror(errno));
			return 0;
		}

		fclose(stream->file);

		/* This stupid code is for stream which owns stdout */
		if(stream->fileno != 0 && fileno(stream->file) != stream->fileno)
		{
			close(stream->fileno);
		}
		
		sprintf(newname, namestring, outfiles ? outfiles : ".", DIR_CHAR,
			stream->prefix, tmlog.tm_year % 100, tmlog.tm_mon + 1, tmlog.tm_mday,
			stream->lognumber);
		if(rename(stream->filename, newname) == -1)
		{
			fprintf(stderr, "%s-rename failed: %s", stream->prefix, strerror(errno));
			free(newname);
			return 0;
		}
		free(newname);

		stream->file = fopen(stream->filename, "w");
		if(stream->file == NULL)
		{
			fprintf(stderr, "%s-fopen failed: %s", stream->prefix, strerror(errno));
			return 0;
		}
		n = fileno(stream->file);
		if(stream->fileno != 0 && n != stream->fileno)
		{
			dup2(n, stream->fileno);
		}
		setbuf(stream->file, NULL);
		if(newdate)
		{
			stream->lognumber = 1;
		}
		else
		{
			stream->lognumber ++;
		}
		fprintf(stream->file, "%ld %d\n", (long)tnow, stream->lognumber);
		fprintf(stream->file, "\nOutput file was reopened %s\n", ctime(&tnow));
		printf("reopened %s\n", stream->prefix);
		stream->logtime = tnow;
		return 1;
	}
	else
	{
		return 1;
	}
}

//---------------------------------------------------------
//  name: fdmc_sclose
//---------------------------------------------------------
//  purpose: Close opened stream
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		stream - pointer to existing stream
//	return:
//		Success - pointer to opened stream
//		Failure - null
//---------------------------------------------------------
int fdmc_sclose(FDMC_LOGSTREAM *stream)
{
	FUNC_NAME("s_close");

	if(stream == NULL)
	{
		err_trace("invalid stream");
		return 0;
	}
	free(stream->filename);
	fclose(stream->file);
	free(stream->prefix);
	free(stream);
	return 1;
}

// Analog of printf
int s_printf(FDMC_LOGSTREAM *stream, char *fmt, ...)
{
	va_list argptr;
	int res;

	if(stream == NULL)
	{
		stream = mainstream;
	}
	if(stream == NULL)
		return 0;
	fdmc_thread_lock(stream->lock, NULL);
	va_start(argptr, fmt);
	res = vfprintf(stream->file, fmt, argptr);
	va_end(argptr);
	fdmc_sreopen(stream);
	fdmc_thread_unlock(stream->lock, NULL);
	return res;
}

// Analog of printf. Adds crlf.
int s_trace(FDMC_LOGSTREAM *stream, char *fmt, ...)
{
//	FUNC_NAME("s_trace");
	va_list argptr;
	int res;

	if(stream == NULL)
	{
		return 0;
	}
	fdmc_thread_lock(stream->lock, NULL);
	va_start(argptr, fmt);
	res = vfprintf(stream->file, fmt, argptr);
	va_end(argptr);
	res += fprintf(stream->file, "\n");
	fdmc_sreopen(stream);
	fdmc_thread_unlock(stream->lock, NULL);
	return res;
}


//---------------------------------------------------------
//  name: trace
//---------------------------------------------------------
//  purpose: Trace data to output in printf style. Add \n.
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		fmt - printf format string
//	return:
//		number of bytes printed
//---------------------------------------------------------
int trace(char *fmt, ...)
{
//	FUNC_NAME("trace");
	va_list argptr;
	int res;
	FILE *f;

	FDMC_LOGSTREAM *stream;
	FDMC_THREAD *thr;

	thr = fdmc_thread_this();
	if(thr && thr->stream)
	{
		stream = thr->stream;
	}
	else
	{
		stream = mainstream;
	}
	if(!stream)
	{
		f = stdout;
	}
	else
	{
		fdmc_thread_lock(stream->lock, NULL);
		f = stream->file;
	}
	va_start(argptr, fmt);
	res = vfprintf(f, fmt, argptr);
	va_end(argptr);
	res += fprintf(f, "\n");
	if(stream)
	{
		fdmc_sreopen(stream);
		fdmc_thread_unlock(stream->lock, NULL);
	}
	return res;
}

// 
//---------------------------------------------------------
//  name: tprintf
//---------------------------------------------------------
//  purpose:Thread analog of printf
//  designer: Serge Borisov (BSA)
//  started: 22.09.10
//	parameters:
//		fmt - format string
//	return:
//		On Success - number of characters printed
//		On Failure - 0
//  special features:
//---------------------------------------------------------
int tprintf(char *fmt, ...)
{
//	FUNC_NAME("trace");
	va_list argptr;
	int res;
	FILE *f;

	FDMC_LOGSTREAM *stream;
	FDMC_THREAD *thr;

	thr = fdmc_thread_this();
	if(thr && thr->stream)
	{
		stream = thr->stream;
	}
	else
	{
		stream = mainstream;
	}
	if(!stream)
	{
		f = stdout;
	}
	else
	{
		f = stream->file;
		fdmc_thread_lock(stream->lock, NULL);
	}
	va_start(argptr, fmt);
	res = vfprintf(f, fmt, argptr);
	va_end(argptr);
	if(stream)
	{
		fdmc_sreopen(stream);
		fdmc_thread_unlock(stream->lock, NULL);
	}
	return res;
}

//---------------------------------------------------------
//  name: s_time_trace
//---------------------------------------------------------
//  purpose: Trace data to stream with current time
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		stream - opened stream descriptor
//		fmt - printf format string
//	return:
//		number of bytes printed
//---------------------------------------------------------
int s_time_trace(FDMC_LOGSTREAM *stream, char *fmt, ...)
{
//	FUNC_NAME("s_time_trace");
	va_list argptr;
	int res;
	struct tm tm_now;
	time_t tnow;

	if(stream == NULL)
	{
		stream = mainstream;
	}
	if(stream == NULL)
		return 0;
	fdmc_thread_lock(stream->lock, NULL);
	va_start(argptr, fmt);
	res = vfprintf(stream->file, fmt, argptr);
	va_end(argptr);
	tnow = time(NULL);
	fdmc_localtime(&tnow, &tm_now);
	res += fprintf(stream->file, " at %02d/%02d/%04d %02d:%02d:%02d\n",
		tm_now.tm_mday,
		tm_now.tm_mon + 1,
		tm_now.tm_year + 1900,
		tm_now.tm_hour,
		tm_now.tm_min,
		tm_now.tm_sec
		);
	fdmc_sreopen(stream);
	fdmc_thread_unlock(stream->lock, NULL);
	return res;
}

//---------------------------------------------------------
//  name: time_trace
//---------------------------------------------------------
//  purpose: Trace data to the stream of active thread
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		fmt - printf format string
//	return:
//		number of bytes printed
//---------------------------------------------------------
int time_trace(char *fmt, ...)
{
//	FUNC_NAME("time_trace");
	va_list argptr;
	int res;
	struct tm tm_now;
	time_t tnow;
	FDMC_THREAD *thr;
	FDMC_LOGSTREAM *stream;

	thr = fdmc_thread_this();
	if(thr != NULL && thr->stream)
		stream = thr->stream;
	else
		stream = mainstream;
	if(stream == NULL)
		return 0;
	fdmc_thread_lock(stream->lock, NULL);
	va_start(argptr, fmt);
	res = vfprintf(stream->file, fmt, argptr);
	va_end(argptr);
	tnow = time(NULL);
	fdmc_localtime(&tnow, &tm_now);
	res += fprintf(stream->file, " at %02d/%02d/%04d %02d:%02d:%02d\n",
		tm_now.tm_mday,
		tm_now.tm_mon + 1,
		tm_now.tm_year + 1900,
		tm_now.tm_hour,
		tm_now.tm_min,
		tm_now.tm_sec
		);
	fdmc_sreopen(stream);
	fdmc_thread_unlock(stream->lock, NULL);
	return res;
}


//---------------------------------------------------------
//  name: s_dump
//---------------------------------------------------------
//  purpose: Dump binary data to stream in traditional style
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		stream - poiter to opened stream
//		data - data to dump
//		len - number of bytes to dump
//	return:
//		number of bytes printed
//---------------------------------------------------------
int s_dump(FDMC_LOGSTREAM *stream, void *data, int len)
{
//	FUNC_NAME("sdump");
	int i, j;
	unsigned char *buf = (unsigned char*)data;

	if(!stream)
	{
		stream = mainstream;
	}
	if(!stream) return 0;
	fdmc_thread_lock(stream->lock, NULL);
	fprintf(stream->file, "\n");
	fprintf(stream->file, "%04X ", 0);
	for (i=1, j=1; i<=len; i++)
	{
		fprintf(stream->file, "%02X ", buf[i-1]);
		if(!(i % 16) || i == len)
		{
			if(i==len)
			{
				int k = i;
				while (k++ % 16)
					fprintf(stream->file, "%2c ", ' ');
			}
			fprintf(stream->file, "    ");
			for(; j <= i; j++)
				fprintf(stream->file, "%c", buf[j-1] >= 32 ? buf[j-1] : '.');
			if(i < len)
			{
				fprintf(stream->file, "\n%04X ", i);
			}
		}
	}
	fprintf(stream->file, "\n\n");
	fdmc_sreopen(stream);
	fdmc_thread_unlock(stream->lock, NULL);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_dump
//---------------------------------------------------------
//  purpose: Dump binary data to active thread stream in traditional style
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		data - data to dump
//		len - number of bytes to dump
//	return:
//		number of bytes printed
//---------------------------------------------------------
int fdmc_dump(void *data, int len)
{
//	FUNC_NAME("sdump");
	int i, j;
	unsigned char *buf = (unsigned char*)data;
	FDMC_LOGSTREAM *stream;
	FDMC_THREAD *th;
	return 1; // For PCI DSS
	th = fdmc_thread_this();
	if(th && th->stream)
		stream = th->stream;
	else
		stream = mainstream;
	if(!stream)
	{
		stream = mainstream;
	}
	if(!stream) return 0;
	fdmc_thread_lock(stream->lock, NULL);
	fprintf(stream->file, "\n");
	fprintf(stream->file, "%04X ", 0);
	for (i=1, j=1; i<=len; i++)
	{
		fprintf(stream->file, "%02X ", buf[i-1]);
		if(!(i % 16) || i == len)
		{
			if(i==len)
			{
				int k = i;
				while (k++ % 16)
					fprintf(stream->file, "%2c ", ' ');
			}
			fprintf(stream->file, "    ");
			for(; j <= i; j++)
				fprintf(stream->file, "%c", buf[j-1] >= 32 ? buf[j-1] : '.');
			if(i < len)
			{
				fprintf(stream->file, "\n%04X ", i);
			}
		}
	}
	fprintf(stream->file, "\n\n");
	fdmc_sreopen(stream);
	fdmc_thread_unlock(stream->lock, NULL);
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_sfdcreate
//---------------------------------------------------------
//  purpose: Create new stream based on opened file handle
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		prefix - First part of file name
//		handle - Existing file handle
//	return:
//		number of bytes printed
//---------------------------------------------------------
FDMC_LOGSTREAM* fdmc_sfdcreate(char *prefix, int handle)
{
//	FUNC_NAME("s_fdcreate");
	FDMC_LOGSTREAM* stream;

	stream = fdmc_screate(prefix);
	if(stream == NULL)
	{
		printf("Cannot fdcreate stream\n");
		return NULL;
	}
	dup2(fileno(stream->file), handle);
	stream->fileno = handle;
	return stream;
}

//---------------------------------------------------------
//  name: fdmc_sfdopen
//---------------------------------------------------------
//  purpose: Open new stream based on opened file handle
//  designer: Serge Borisov (BSA)
//  started: 23.04.10
//	parameters:
//		prefix - First part of file name
//		handle - Existing file handle
//	return:
//		number of bytes printed
//---------------------------------------------------------
FDMC_LOGSTREAM* fdmc_sfdopen(char *prefix, int handle)
{
//	FUNC_NAME("s_fdopen");
	FDMC_LOGSTREAM* stream;

	stream = fdmc_sopen(prefix);
	if(stream == NULL)
	{
		printf("Cannot fdopen stream\n");
		return NULL;
	}
	dup2(fileno(stream->file), handle);
	stream->fileno = handle;
	return stream;
}

int regards(void)
{
	return printf("Believe in your success!\n");
}

int scr_printf(char *fmt, ...)
{
	va_list argptr;
	int res;
	FDMC_LOGSTREAM *stream;
	FDMC_THREAD *t;

	if(!screen_lock)
	{
		screen_lock = fdmc_thread_lock_create(NULL);
	}
	t = fdmc_thread_this();
	if(t && t->stream)
		stream = t->stream;
	else
		stream = mainstream;
	/*
	if(stream == NULL)
	{
		return 0;
	}
	*/
	if(stream)
	{
		fdmc_thread_lock(stream->lock, NULL);
	}
	if(screen != NULL)
	{
		fdmc_thread_lock(screen_lock, NULL);
		va_start(argptr, fmt);
		res = vfprintf(screen, fmt, argptr);
		va_end(argptr);
		//res += fprintf(screen, "\n");
		fdmc_thread_unlock(screen_lock, NULL);
	}
	if(0)
	{
		fprintf(stream->file, "SCREEN: ");
		vfprintf(stream->file, fmt, argptr);
		fprintf(stream->file, "\n");
		fdmc_sreopen(stream);
		fdmc_thread_unlock(stream->lock, NULL);
	}
	return res;
}

/* put string to console */
int screen_trace(char *fmt, ...)
{
	va_list argptr;
	int res = 0;
	FDMC_LOGSTREAM *stream;
	FDMC_THREAD *t;

	if(!screen_lock)
	{
		screen_lock = fdmc_thread_lock_create(NULL);
	}
	t = fdmc_thread_this();
	if(t && t->stream)
		stream = t->stream;
	else
		stream = mainstream;
	if(stream)
	{
		fdmc_thread_lock(stream->lock, NULL);
	}
	if(screen != NULL)
	{
		fdmc_thread_lock(screen_lock, NULL);
		va_start(argptr, fmt);
		res = vfprintf(screen, fmt, argptr);
		va_end(argptr);
		res += fprintf(screen, "\n");
		fdmc_thread_unlock(screen_lock, NULL);
	}
	if(stream)
	{
		res = fprintf(stream->file, "SCREEN: ");
		va_start(argptr, fmt);
		res += vfprintf(stream->file, fmt, argptr);
		va_end(argptr);
		res += fprintf(stream->file, "\n");
		fdmc_sreopen(stream);
		fdmc_thread_unlock(stream->lock, NULL);
	}
	return res;
}

/* Put string to console adding current time */
int screen_time_trace(char *fmt, ...)
{
	va_list argptr;
	int res = 0;
	time_t tnow;
	struct tm tm_now;
	FDMC_LOGSTREAM *stream;
	FDMC_THREAD *t;

	if(!screen_lock)
	{
		screen_lock = fdmc_thread_lock_create(NULL);
	}
	t = fdmc_thread_this();
	if(t && t->stream)
		stream = t->stream;
	else
		stream = mainstream;
	if(!stream) return 0;
	fdmc_thread_lock(stream->lock, NULL);
	/* Check screen variable initialization
	* by dup_screen function
	*/
	if(screen != NULL)
	{
		fdmc_thread_lock(screen_lock, NULL);
		va_start(argptr, fmt);
		fprintf(screen, "\n");
		vfprintf(screen, fmt, argptr);
		va_end(argptr);
		fdmc_thread_unlock(screen_lock, NULL);
	}
	res = fprintf(stream->file, "SCREEN: ");
	va_start(argptr, fmt);
	res += vfprintf(stream->file, fmt, argptr);
	va_end(argptr);
	tnow = time(NULL);
	fdmc_localtime(&tnow, &tm_now);
	if(screen != NULL)
	{
		fdmc_thread_lock(screen_lock, NULL);
		res = fprintf(screen, " at %02d/%02d/%04d %02d:%02d:%02d\n",
			tm_now.tm_mday,
			tm_now.tm_mon + 1,
			tm_now.tm_year + 1900,
			tm_now.tm_hour,
			tm_now.tm_min,
			tm_now.tm_sec
			);
		fdmc_thread_unlock(screen_lock, NULL);
	}
	res += fprintf(stream->file, " at %02d/%02d/%04d %02d:%02d:%02d\n",
			tm_now.tm_mday,
			tm_now.tm_mon + 1,
			tm_now.tm_year + 1900,
			tm_now.tm_hour,
			tm_now.tm_min,
			tm_now.tm_sec
			);
	fdmc_thread_unlock(stream->lock, NULL);
	fdmc_sreopen(stream);
	return res;
}

/* Create console file */
void *dup_screen()
{
	int term;

	/* if screen file was created, then
	* return it
	*/
	if(screen != NULL)
	{
		return screen;
	}
	/* duplicate stdout handle */
	term = dup(fileno(stdout));
	/* check result */
	if(term == EOF)
	{
		return NULL;
	}
	/* created screen file based on descriptor */
	screen = fdopen(term, "w");
	setbuf(screen, NULL);
	setbuf(stdout, NULL);
	return screen;
}

