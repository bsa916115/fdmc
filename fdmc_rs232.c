#include "fdmc_rs232.h"

MODULE_NAME("fdmc_rs232.c");

//---------------------------------------------------------
// Name:
// fdmc_rs_create
//---------------------------------------------------------
// Purpose:
// Create serial port descriptor
//---------------------------------------------------------
// Parameters:
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
#ifdef _WINDOWS_32
FDMC_RS232 *fdmc_rs_create(LPCWSTR path, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_rs_create");
	FDMC_EXCEPTION x;
	FDMC_RS232 *rs_new;

	dbg_trace();

	TRYF(x)
	{
		rs_new = (FDMC_RS232*)fdmc_malloc(sizeof(*rs_new), &x);
		rs_new->handle = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(!GetCommState(rs_new->handle, &rs_new->term))
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "Cannot create serial device");
		}
		rs_new->term.DCBlength = sizeof(DCB);
		rs_new->term.BaudRate = CBR_9600;
		rs_new->term.ByteSize = 7;
		rs_new->term.fParity = 1;
		rs_new->term.Parity = EVENPARITY;
		rs_new->term.StopBits = ONESTOPBIT; 
		rs_new->term.fRtsControl = RTS_CONTROL_DISABLE;
		rs_new->term.fDtrControl = DTR_CONTROL_DISABLE;
		if(!SetCommState(rs_new->handle, &rs_new->term))
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "Cannot set serial parameters");
		}
		GetCommState(rs_new->handle, &rs_new->term);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return rs_new;
}
#endif

//---------------------------------------------------------
// Name:
// fdmc_rs_write
//---------------------------------------------------------
// Purpose:
// Write data to serial port
//---------------------------------------------------------
// Parameters:
// rs - port descriptor
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
#ifdef _WINDOWS_32
int fdmc_rs_write(FDMC_RS232 *rs, char* buff, int count, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_rs_write");
	FDMC_EXCEPTION x;

	//dbg_trace();

	TRYF(x)
	{
		if(!WriteFile(rs->handle, buff, count, &rs->len, NULL))
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "Cannot write to serial");
		}
		FlushFileBuffers(rs->handle);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}
#endif

//---------------------------------------------------------
// Name:
// fdmc_rs_read
//---------------------------------------------------------
// Purpose:
// reads data from serial port
//---------------------------------------------------------
// Parameters:
// rs - port descriptor
// buf - buffer to hold data
// count - bytes to read
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
#ifdef _WINDOWS_32
int fdmc_rs_read(FDMC_RS232 *rs, char *buf, int count, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_rs_read");
	FDMC_EXCEPTION x;
	int i;
	char c;

	//dbg_trace();

	TRYF(x)
	{
		for(i=0; i < count; i++)
		{
			if(!ReadFile(rs->handle, &c, 1, &rs->len, NULL))
			{
				fdmc_raisef(FDMC_IO_ERROR, &x, "Cannot read serial");
			}
			buf[i] = c;
			//func_trace("Got %02X", (unsigned)c);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

int fdmc_rs_close(FDMC_RS232 *port)
{
	CloseHandle(port->handle);
	return 1;
}
#endif