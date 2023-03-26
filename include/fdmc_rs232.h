#ifndef _FDMC_RS232
#define _FDMC_RS232

//-------------------------------------------
// Name:
// fdmc_rs232
//-------------------------------------------
// Purpose:
// Provide a row of functions to work with
// serial port. Use for HSM or embossing
// application
//-------------------------------------------
// Version 1.0
// July 2, 2014

#include "fdmc_base.h"

#ifdef _WINDOWS_32
#define FDMC_RS_HANDLE HANDLE
#define rs_open(path) CreateFile(path,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL)
#define PORT_PARAMS DCB
#else
#include <sys/types.h>
#include <fcntl.h>
#define FDMC_RS_HANDLE int
#define PORT_PARAMS struct termio
#endif

typedef struct 
{
	FDMC_RS_HANDLE handle;
	char buffer[1024*16];
	DWORD len;
	DCB term;
	int port;
	FDMC_THREAD_LOCK *lock;
} FDMC_RS232;

FDMC_RS232 *fdmc_rs_create(LPCWSTR address, FDMC_EXCEPTION *err);
int fdmc_rs_close(FDMC_RS232 *port);
int fdmc_rs_write(FDMC_RS232 *rs, char* buff, int count, FDMC_EXCEPTION *err);
int fdmc_rs_read(FDMC_RS232 *rs, char *buf, int count, FDMC_EXCEPTION *err);

#endif
