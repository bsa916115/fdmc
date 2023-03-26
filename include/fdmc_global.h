#ifndef _FDMC_GLOBAL_H
#define _FDMC_GLOBAL_H

/* FDMC free data manipulating code */

#define VERSION_NUMBER "4.02.3"
#define VERSION 4

#define _WINDOWS_32

//#define _UNIX_32
//#define _UNIX_64
//#define _UNIX_

#if defined(_WINDOWS_32)
	#define DIR_CHAR '\\'
	#define NULL_DEVICE "NUL"
	#define popen _popen
#include <windows.h>

#ifdef BCC32
#include <winsock2.h>
#endif

#if !defined(BCC32)
#pragma warning(disable:4996)
	#define strdup _strdup
	#define popen _popen
#else
	#pragma warning(disable:13360)
#endif
#elif defined(_UNIX_)
	#define DIR_CHAR '/'
	#define NULL_DEVICE "/dev/null"
	#define SOCKET int
#define chsize ftruncate
#include <unistd.h>
#endif

#include <stdio.h>
#include <setjmp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>

//typedef int SOCKET;

typedef unsigned short _ORA_STRINGLENGTH;
typedef unsigned char byte;
#ifdef _USE_PRECOMPILER
#define hostchar(name, length) varchar name[length]
#else
#define hostchar(name, length) \
struct {_ORA_STRINGLENGTH len; unsigned char arr[length];} name
#endif

#define FIXARR(X) X.arr[X.len] = 0
#define FIXLEN(X) X.len = strlen(X.arr)

#define FDMC_EQUAL 0
#define FDMC_GREATER 1
#define FDMC_LESS -1
#define FDMC_OK 1
#define FDMC_FAIL 0
#define FDMC_CONTINUE 2
#define FDMC_NOTHANDLED 3

#define FDMC_DELETE 101
#define FDMC_INSERT 102
#define FDMC_COMPARE 103
#define FDMC_CALCULATE 104
#define FDMC_PRINT 105
#define FDMC_PRINT_STRUCTURE 106
#define FDMC_FORMAT 107
#define FDMC_INIT 108
#define FDMC_EXTRACT 109
#define FDMC_CREATE 110
#define FDMC_GET 111
#define FDMC_PUT 112
#define FDMC_EXPORT 113
#define FDMC_IMPORT 114
#define FDMC_GENSOURCE 115
#define FDMC_DESCVAR 116
#define FDMC_DESCFIELD 117
#define FDMC_XMLEXPORT 118
#define FDMC_TIMEOUT 119

#define FDMC_COMMON_ERROR 200
#define FDMC_MEMORY_ERROR 201
#define FDMC_IO_ERROR 202
#define FDMC_PARAMETER_ERROR 203
#define FDMC_SYNTAX_ERROR 204
#define FDMC_DB_ERROR 205
#define FDMC_DATA_ERROR 206
#define FDMC_SOCKET_ERROR 207
#define FDMC_THREAD_ERROR 208
#define FDMC_UNIQUE_ERROR 209
#define FDMC_CONNECT_ERROR 210
#define FDMC_BIND_ERROR 211
#define FDMC_ACCEPT_ERROR 212
#define FDMC_WRITE_ERROR 213
#define FDMC_READ_ERROR 214
#define FDMC_FORMAT_ERROR 215
#define FDMC_LISTEN_ERROR 216
#define FDMC_QUEUE_ERROR 217
#define FDMC_STACK_ERROR 218
#define FDMC_COMMS_ERROR 219
#define FDMC_DEVICE_ERROR 220
#define FDMC_MUTEX_ERROR 221
#define FDMC_TIMEOUT_ERROR 222
#define FDMC_PROTOCOL_ERROR 223
#define FDMC_SECURITY_ERROR 224
#define FDMC_VERIFY_ERROR 225

#define FDMC_DEV_LENGTH 16
#define FDL FDMC_DEV_LENGTH + 1
struct FDMC_LIST_STR;
struct _FDMC_THREAD_LOCK;
struct _FDMC_THREAD;

/* Data types and macro definitions for fdmc_exception.h*/
#define FDMC_MAX_ERRORTEXT 1025
typedef struct _FDMC_EXCEPTION
{
	int errorcode;
	int errorsubcode;
	char errortext[FDMC_MAX_ERRORTEXT];
	jmp_buf cpu;
} FDMC_EXCEPTION;

/* Data types for fdmc_logstream.h */
struct _FDMC_LOGSTREAM
{
	FILE *file;
	char *prefix; /* prefix code */
	int mode; /* Mode stream was opened (create or open) */
	time_t logtime; /* last created file */
	int lognumber; /* file order  */
	long max_size;  /* file max size */
	int fileno;   /* system file number */
	char *filename; /* current file name */
	int errnum; // last I/O operation error code
	char *errstr; // last I/O operation error text 
	struct _FDMC_THREAD_LOCK *lock;
};

typedef struct _FDMC_LOGSTREAM FDMC_LOGSTREAM;

/* Data type definitions for fdmc_thread.h */
#ifdef _WINDOWS_32
#define FDMC_THREAD_ID DWORD
#define FDMC_THREAD_TYPE DWORD
#define FDMC_THREAD_FUNC LPTHREAD_START_ROUTINE 
#define FDMC_THREAD_HANDLE HANDLE
#define THREAD_FAIL(X) (X == NULL)
#define INVALID_THREAD NULL
#define __thread_call __stdcall
// Mutex definitions
#define FDMC_MUTEX HANDLE
#endif

#ifdef _UNIX_
#include <pthread.h>
#define  FDMC_THREAD_ID pthread_t
#define FDMC_THREAD_TYPE void*
typedef void* (*FDMC_THREAD_FUNC)(void*);
#define FDMC_THREAD_HANDLE int
#define THREAD_FAIL(X) (X != 0)
#define INVALID_THREAD -1
#define __thread_call
// Mutex definition
#define FDMC_MUTEX pthread_mutex_t
#endif


typedef struct // As received in main()
{
	int argc;
	char **argv;
	char **envp;
	char *command_line;
}
FDMC_ARG;

typedef struct _FDMC_THREAD_FLAG
{
	int fl;
	SOCKET fsock;
} FDMC_THREAD_FLAG;


typedef struct _FDMC_THREAD
{
	FDMC_THREAD_ID thread_id; // System thread id
	FDMC_THREAD_TYPE (__thread_call *thread_func)(void *); // Thread start function
	void *thread_param; // Data parameter for thread descriptor
	FDMC_THREAD_HANDLE thread_hndl; // Returned thread handle
	FDMC_LOGSTREAM *stream; // Thread stream to output runtime information
	int thread_stack;
} FDMC_THREAD;

struct _FDMC_THREAD_LOCK
{
	int lock_count;
	FDMC_MUTEX *lock;
	FDMC_THREAD_ID lock_owner;
};

typedef struct _FDMC_THREAD_LOCK FDMC_THREAD_LOCK;

typedef struct _FDMC_QUEUE
{
	struct FDMC_LIST_STR *queue;
	FDMC_THREAD_FLAG *flag;
	FDMC_THREAD_LOCK lock;
} FDMC_QUEUE;


/* Data type definitions for fdmc_ip.h */

#if defined (_WINDOWS_32)
/* Windows includes */
#include <io.h>
#include <time.h>
#elif defined (_UNIX_) 
/* UNIX includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#define SOCKET int
#define closesocket close
#define init_ip() (0)
#define INVALID_SOCKET EOF
#define SOCKET_ERROR EOF
#define WSAGetLastError() errno
#endif

typedef struct FDMC_SOCK
{
	SOCKET data_socket; // system socket number
	char *ip_address;   // ip address socket connected with
	int ip_port;        // ip port
	int header_size;    // size of message header in bytes
	int header_type;    // byte or text
	int active;         // socket is ready to exchange with peer
	int errnum;         // last system error
	char *dest_buf;     // buffer for input/output text
	int buf_size;       // size of buffer in bytes
	unsigned sec;       // timeout in seconds
	unsigned usec;      // timeout in microseconds
	void *data;         // pointer to user associated data
} FDMC_SOCK;

/* Data types definition for fdmc_list.h */
// FDMC list structures
typedef struct FDMC_LIST_ENTRY_STR
{
	struct FDMC_LIST_STR *list;
	struct FDMC_LIST_ENTRY_STR *next;
	struct FDMC_LIST_ENTRY_STR *prior;
	void *data;
} FDMC_LIST_ENTRY;

// List header structure
struct FDMC_LIST_STR
{
	struct FDMC_LIST_ENTRY_STR *first;
	struct FDMC_LIST_ENTRY_STR *last;
	struct FDMC_LIST_ENTRY_STR *current;
	int count;
	int (*process_entry)(FDMC_LIST_ENTRY*,
		void *parameters, int flags);
	void *listdata;
	FDMC_THREAD_LOCK *lock;
};
typedef struct FDMC_LIST_STR FDMC_LIST;


/* Data types definitions for fdmc_hash.h */
struct _FDMC_HASH_TABLE
{
	int size;
	int (*hash_function)(struct _FDMC_HASH_TABLE*, void *);
	FDMC_LIST *cells;
	FDMC_THREAD_LOCK *lock;
};

typedef struct _FDMC_HASH_TABLE FDMC_HASH_TABLE;

typedef enum 
{
	XML_EMPTY = 0,
	XML_TAG,
	XML_EMPTYTAG,
	XML_ATTRIBUTE,
	XML_CDATA,
	XML_INSTRUCTION,
	XML_DEFINITION,
	XML_PCDATA,
	XML_CLOSETAG // Not used in syntax parser, only in xml message functions
} XML_NODETYPE;

typedef struct _XML_TREE_NODE
{
	XML_NODETYPE nodetype;
	char *name;
	char *value;
	FDMC_LIST_ENTRY *brother;
	FDMC_LIST *child;
} XML_TREE_NODE;

#define FDMC_USE_SQL

#endif
