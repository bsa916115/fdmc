#include "fdmc_global.h"
#include "fdmc_logfile.h"
#include "fdmc_ip.h"
#include "fdmc_thread.h"
#include "fdmc_hash.h"
#include "fdmc_list.h"
#include "fdmc_exception.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

MODULE_NAME("fdmc_ip.c");

static struct hostent *hp;
static struct servent *sp;
static struct sockaddr_in myaddr_in;
static struct sockaddr_in peeraddr_in;
static struct in_addr peeraddr;

//FDMC_SOCK_TABLE fdmc_sock_table;

/* Establish connection to remote host.
* 
* addr - IP address of remote host
* port - specified IP port
*
* BSA Oct 2009
*
* Return
*     Success - Handle of connected socket
*     Failure - 0
*/
SOCKET connect_to_host(char *addr, int port)
{
	return connect_to_dest(addr, port, 1, 0);
}

/* 
* Connect to remote host
* 
* addr - IP address
* port - IP port
* attempts - Number of connect attemts. 0 - attempts number is unlimited
* timeout_sec - interval between attempts
*
* RETURN:
* == 0 - not connected
* != 0 - socket number
*
* BSA Oct 2009
*
*/
SOCKET connect_to_dest(char *addr, int port, unsigned int attempts, 
					   long timeout_sec)
{
	FUNC_NAME("connect_to_dest");
	SOCKET s;
	fd_set fds;
	struct timeval tv;
	unsigned attempt;
	struct hostent *hp;
	struct sockaddr_in peeraddr_in;

	dbg_trace();
	for(attempt = 0; attempts == 0 || attempt < attempts; attempt++)
	{
		/* »нициализировать данные */
		FD_ZERO(&fds);
		s_time_trace(mainstream, "try to establish connection on '%s', port %d", addr, port);
		s = socket(AF_INET, SOCK_STREAM, 0);
		if(s == INVALID_SOCKET)
		{
			err_trace("socket() failed");
			return 0;
		}
		FD_SET(s, &fds);
		tv.tv_sec = timeout_sec;
		tv.tv_usec = 0;
		hp = gethostbyname(addr);
		if(!hp)
		{
			hp = gethostbyaddr(addr, (int)strlen(addr), AF_INET);
			if(!hp)
			{
				func_trace("'%s' - unknown host", addr);
				return 0;
			}
		}
		peeraddr_in.sin_family = AF_INET;
		peeraddr_in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
		peeraddr_in.sin_port = htons(port);
		/* ”становить соединение */
		if(connect(s, (struct sockaddr*)&peeraddr_in, sizeof(peeraddr_in)))
		{
			/* —оединение не установлено, печать сообщени€ 
			 * и ожидание указанное количество времени
			 */
			s_time_trace(mainstream, "Connection refused");
			if(timeout_sec) 
			{
				select(1, &fds, NULL, NULL, &tv);
			}
			shutdown(s, 2);
			closesocket(s);
			s = 0;
		}
		else
		{
			break;
		}
	} // for
	if(attempts != 0 && s == 0)
	{
		/* ѕревышено число попыток соединени€ */
		err_trace("could not connect to %s after %d attempts", addr, attempts);
		return 0;
	}
	/* —оединение установлено */
	s_time_trace(mainstream, "Connected to %s with socket %d", addr, s);
	return s;
}

/*  Bind and listen specified port
*
*  port - specified IP port
*  BSA Oct 2009
*
*  return
*      Success - handle of binded socket
*      Failure - zero
*/
SOCKET port_listener(int port)
{
	SOCKET s;
	int opt=1;

	FUNC_NAME("port_listener");

	func_trace("Creating listener for port %d", port);
	/* Create socket */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	{
		func_trace("Cannot create socket: %d", errno);
		return 0;
	}
	/* Allow process to reuse ip address */
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
	/* Bind socket to specified port */
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(port);
	if(bind(s, (void*)&myaddr_in, sizeof(myaddr_in))==EOF)
	{
		err_trace("Cannot bind socket: %d", errno);
		shutdown(s, 2);
		closesocket(s);
		return 0;
	}
	/* Listen socket */
	if(listen(s, 5)==EOF)
	{
		err_trace("Cannot listen socket: %d", errno);
		shutdown(s, 2);
		closesocket(s);
		return 0;
	}
	return s;
}


typedef struct 
{
	unsigned short sa_family;
	char sa_data[20];
} str_peername;


/* Get remote host name connected to server
*
* s - accepted socket number
* name_buf - string buffer to hold result
*
* Borisov S. Feb 2009
*
* Return
*    Success - 1 and name_buf is filled with address data
*    Failure - 0 and name_buf is empty string
*
*/
int get_peer_name(SOCKET s, char *name_buf)
{
	FUNC_NAME("get_peer_name");
	struct sockaddr peername;

	int peernamelen = sizeof(peername);

	dbg_trace();
	name_buf[0] = 0;
	if(getpeername(s, &peername, &peernamelen) == EOF)
	{
		err_trace("Cannot get peer name");
		return 0;
	}
	memcpy(&peeraddr, peername.sa_data+2, sizeof(peeraddr));
	strcpy(name_buf, inet_ntoa(peeraddr));
	return 1;
}

/* Receive specified number of bytes from socket
*
* sock - data socket handle
* buf - destination buffer
* count - number of bytes to receive
*
* Borisov S. Feb 2009
*
* Return
*    > 0 - number of bytes received
*    == 0 - connection was closed
*	== EOF - system call of recv() returned error
*/
int fdmc_recvn(SOCKET sock, char *buf, int count, int flag)
{
	//FUNC_NAME("fdmc_recvn");
	int total;
	int bytes_read;

	//dbg_trace();
	for(total = 0; total < count; total += bytes_read) 
	{
		bytes_read = recv(sock, buf+total, count-total, flag);
		if(!bytes_read) 
		{
			return 0;
		}
		if(bytes_read == SOCKET_ERROR) 
		{
			return SOCKET_ERROR;
		}
	}
	return count;
}

/* Initialize ip network. For Windows mandatory 
* call before using any network routines
* 
* BSA Oct 2009
*
* Return
*     Success - 0
*     Failure - Error code
*/
int fdmc_init_ip(void)
{
	//FUNC_NAME("fdmc_init_ip");
#if defined (_WINDOWS_32)
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	
	//dbg_trace();
	wVersionRequested = MAKEWORD( 2, 2 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
#else
	int err = 0;
#endif
	return err;
}

/* 
* Suspend current thread for specified time
* 
* sec - suspend time in seconds
* usec - suspend time in microseconds
*
* Borisov S. 11-02-2002
*
* Return
*    Always 1
*/
int fdmc_delay(int sec, int usec)
{
	//FUNC_NAME("delay");
#if defined (_WINDOWS_32)
	SOCKET s;
	fd_set fds;
	struct timeval tval;

	s = socket(AF_INET, SOCK_STREAM, 0);
	tval.tv_sec = sec;
	tval.tv_usec = usec;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	select(1, &fds, NULL, NULL, &tval);
	shutdown(s, 2);
	closesocket(s);
#elif defined(_UNIX_)
	sleep(sec);
	usleep(usec);
#endif
	return 1;
}
/* 
* Suspend current thread for specified time
* in milliseconds
*
* millisec - suspend time in milliseconds
*
* Borisov S. 11-02-2009
*
* Return
*    Always 1
*/
int fdmc_msleep(unsigned millisec)
{
#if defined(_WINDOWS_32)
	Sleep(millisec);
#elif defined(_UNIX_)
	usleep(millisec * 1000);
#endif
	return 1;
}

/* 
* Try to bind to specified port.
* Usefull to avoid multiple start of programme
*
* p - port to lock
*
* Borisov S. 11-02-2009
*
* Return
*    socket created
*/
SOCKET fdmc_lock_port(int p)
{
//	FUNC_NAME("fdmc_lock_port");
	struct sockaddr_in myaddr_in;
	SOCKET s;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == INVALID_SOCKET)
	{
		return s;
	}
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(p);
	if(bind(s, (struct sockaddr*)&myaddr_in, sizeof(myaddr_in))==EOF)
	{
		shutdown(s, 2);
		closesocket(s);
		return EOF;
	}
	return s;
}

//***********************************************************
//
//                 FDMC_SOCK manage functions 
//
//***********************************************************

/* 
* Create new socket descriptor
* 
* ip_addr - address to connect to. If address is null, function
*           creates port listener.
* ip_port - port number to connect or to listen
* hdsize  - header size of socket messages
* err - exception handler to hold error information
*
* Borisov S. 11-02-2009
*
* Return
*    NULL - socket was not created
*    new socket pointer - on success
*
* if there is an error and err is not null then function performs
* long jump to err descriptor
*/
FDMC_SOCK *fdmc_sock_create(char *ip_addr, int ip_port, int hdsize, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sock_create");
	FDMC_SOCK *sock_new = NULL;
	FDMC_EXCEPTION e;
	struct hostent *hp;
	struct sockaddr_in peeraddr_in;

	//dbg_print();
	TRYF(e)
	{
		// Allocate memory and create socket
		sock_new = (FDMC_SOCK*)fdmc_malloc(sizeof(sock_new[0]), &e);
		sock_new->data_socket = socket(AF_INET, SOCK_STREAM, 0);
		if(hdsize > 10)
		{
			// This means text length of message
			sock_new->header_type = 1;
			hdsize -= 10;
		}
		sock_new->header_size = hdsize;
		sock_new->ip_port = ip_port;
		if(sock_new->data_socket == INVALID_SOCKET)
		{
			fdmc_raisef(FDMC_SOCKET_ERROR, &e, "socket() failed %d", fdmc_sock_error(sock_new));
		}
		sock_new->lock = fdmc_thread_lock_create(&e);
		if(ip_addr && strlen(ip_addr)) // Address specified ?
		{
			// Yes, Connect to peer
			hp = gethostbyname(ip_addr);
			if(!hp)
			{
				hp = gethostbyaddr(ip_addr, strlen(ip_addr), AF_INET);
			}
			if(hp)
			{
				peeraddr_in.sin_family = AF_INET;
				peeraddr_in.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
				peeraddr_in.sin_port = htons(ip_port);
			}
			if(!hp)
			{
				int r = WSAGetLastError();
				fdmc_raisef(FDMC_SOCKET_ERROR, &e, "'%s' - unknown host",
					ip_addr);
			}
			sock_new->ip_address = fdmc_strdup(ip_addr, &e);
			if(connect(sock_new->data_socket, (const void*)&peeraddr_in, sizeof(peeraddr_in)))
			{
				fdmc_raisef(FDMC_CONNECT_ERROR, &e, "connect() to %s failed %d", 
					sock_new->ip_address,
					fdmc_sock_error(sock_new));
			}
			sock_new->active = FDMC_SOCK_ACTIVE;
		}
		else if(ip_port != 0)/* Create listener if no external address specified */
		{
			/* Allow process to reuse ip address */
			int opt = 1;
			setsockopt(sock_new->data_socket, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
			/* Bind socket to specified port */
			myaddr_in.sin_family = AF_INET;
			myaddr_in.sin_addr.s_addr = INADDR_ANY;
			myaddr_in.sin_port = htons(ip_port);
			if(bind(sock_new->data_socket, (void*)&myaddr_in, sizeof(myaddr_in)))
			{
				fdmc_raisef(FDMC_BIND_ERROR, &e, "Cannot bind socket");
				return NULL;
			}
			/* Listen socket */
			if(listen(sock_new->data_socket, 5))
			{
				fdmc_raisef(FDMC_LISTEN_ERROR, &e, "Cannot listen socket");
			}
			sock_new->active = FDMC_SOCK_BIND;
		}
	}
	EXCEPTION
	{
		// When somthing comes wrong
		if(sock_new)
		{
			fdmc_sock_close(sock_new);
		}
		sock_new = NULL;
		// Transfer to higher level handler
		fdmc_raisef(e.errorcode, err, "%s", e.errortext);
	}
	return sock_new;
}

//---------------------------------------------------------
// Name:
// fdmc_sockpair_create
//---------------------------------------------------------
// Purpose:
// create two connected sockets
//---------------------------------------------------------
// Parameters:
// listener - base listen socket
// s0 - first socket
// s1 - second socket
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
// Designer: Borisov S.
//---------------------------------------------------------
int fdmc_sockpair_create(FDMC_SOCK *listener, FDMC_SOCK **s0, FDMC_SOCK **s1, FDMC_EXCEPTION* err)
{
	FDMC_EXCEPTION x;
	FUNC_NAME("fdmc_sockname_create");

	TRYF(x)
	{
		// Check nulls
		CHECK_NULL(listener, "listener", x);
		CHECK_NULL(s0, "s0", x);
		CHECK_NULL(s1, "s1", x);
		*s0 = *s1 = NULL;
		// Lock listener port
		fdmc_thread_lock(listener->lock, &x);
		// s0 connects to listener port
		*s0 = fdmc_sock_create(listener->ip_address, listener->ip_port, listener->header_size, &x);
		// s1 accepts listener
		*s1 = fdmc_sock_accept(listener, &x);
	}
	EXCEPTION
	{
		fdmc_thread_unlock(listener->lock, NULL);
		if(*s0) fdmc_sock_close(*s0);
		if(*s1) fdmc_sock_close(*s1);
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

/* 
* Close socket descriptor
* 
* psock - Socket descriptor to close
*
* Borisov S. 11-02-2009
*
* Return
*	Always 1
*
*/
int fdmc_sock_close(FDMC_SOCK *psock)
{
	FUNC_NAME("fdmc_sock_close");

	//dbg_print();
	if(!psock)
	{
		return 1;
	}
	func_trace("port=%d address=%s", psock->ip_port, psock->ip_address ?
		psock->ip_address:"NULL");
	fdmc_free(psock->ip_address, NULL);
	fdmc_free(psock->dest_buf, NULL);
	if(psock->data_socket != INVALID_SOCKET && psock->data_socket != 0)
	{
		shutdown(psock->data_socket, 2);
		closesocket(psock->data_socket);
	}
	fdmc_thread_lock_delete(psock->lock);
	fdmc_free(psock, NULL);
	return 1;
}

/* 
* Accept connection and create new socket descriptor
* based on it
* 
* psock - listener socket to accept connection
* err - error handler
*
* Borisov S. 11-02-2009
*
* Return
*    NULL - socket was not created
*    accepted socket - on success
*
* if there is an error and err is not null then function performs
* long jump to err descriptor
*/
FDMC_SOCK *fdmc_sock_accept(FDMC_SOCK *psock, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sock_accept");
	SOCKET s;
	struct sockaddr peeraddr_in, addr_in;
	struct in_addr addr;
	FDMC_SOCK *sock_new;
	fd_set fds;
	int addrlen = sizeof(struct sockaddr);

	if(!psock)
	{
		fdmc_raisef(FDMC_SOCKET_ERROR, err, "%s: NULL descriptor passed", _function_id);
	}
	FD_ZERO(&fds);
	FD_SET(psock->data_socket, &fds);
	
	if(select(psock->data_socket+1, &fds, NULL, NULL, NULL) == EOF)
	{
		fdmc_raisef(FDMC_SOCKET_ERROR, err, "select() failed");
	}
	

	sock_new = (FDMC_SOCK*)fdmc_malloc(sizeof(*sock_new), err);
	
	s = accept(psock->data_socket, &peeraddr_in, &addrlen);
	if(s == INVALID_SOCKET)
	{
		fdmc_free(sock_new, NULL);
		fdmc_raisef(FDMC_SOCKET_ERROR, err, "%s: accept() port %d failed", _function_id, psock->ip_port);
		return NULL;
	}
	getpeername(s, &addr_in, &addrlen); 
	memcpy(&addr, addr_in.sa_data+2, sizeof(addr));
	sock_new->ip_address = fdmc_strdup(inet_ntoa(addr), NULL);
	sock_new->data_socket = s;
	sock_new->header_size = psock->header_size;
	sock_new->header_type = psock->header_type;
	sock_new->ip_port = psock->ip_port;
	sock_new->active = FDMC_SOCK_ACTIVE;
	return sock_new;
}

int fdmc_sock_error(FDMC_SOCK *psock)
{
//	FUNC_NAME("fdmc_sock_error");
	if(!psock) return EOF;
#if defined(_WINDOWS_32)
	psock->errnum = WSAGetLastError();
#elif !defined(_UNIX_)
	psock->errnum = errno;
#endif
	return psock->errnum;
}

/* 
* Send specified number of bytes to socket descriptor
* 
* psock - connected socket to send data
* buffer - data to send
* bufsize - number of bytes to send
* err - error handler
*
* Borisov S. 11-02-2009
*
* if there is an error and err is not null then function performs
* long jump to err descriptor
*/
int fdmc_sock_send(FDMC_SOCK *psock, void *buffer, int bufsize, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sock_send");
	char *p;
	FDMC_EXCEPTION x;
	int j, js;

	TRYF(x)
	{
		if(psock == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, err, "%s: NULL socket parameter",
				_function_id);
			return SOCKET_ERROR;
		}
		CHECK_NULL(buffer, "buffer", x);
		j = psock->header_size + bufsize;
		if(psock->buf_size < j)
		{
			if(psock->dest_buf) fdmc_free(psock->dest_buf, NULL);
			psock->dest_buf = NULL;
			psock->buf_size = 0;
			psock->dest_buf = (char*)fdmc_malloc(j, &x);
			psock->buf_size = j;
		}
		p = psock->dest_buf;
		js = bufsize;
		if(psock->header_type == 1)
			sprintf(psock->dest_buf, "%06d", bufsize);
		else
			for(j = psock->header_size-1; j >= 0; j--)
			{
				p[j] = (char)(js % 0x100);
				js /= 0x100;
			}
		memcpy(p + psock->header_size, buffer, bufsize);
		func_trace("Send data to peer");
		//fdmc_dump(p, bufsize + psock->header_size);
		j = send(psock->data_socket, p, bufsize + psock->header_size, 0);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return SOCKET_ERROR;
	}
	return j;
}

/* Receive specified number of bytes from socket in loop
*
* sock - FDMC_SOCK handle
* buf - destination buffer
* count - number of bytes to receive
*
* Borisov S. Feb 2009
*
* Return
*    > 0 - number of bytes received
*    == 0 - connection was closed
*	== EOF - system call of recv() returned error
*/
int fdmc_sock_recvn(FDMC_SOCK *sock, char *buf, int count, int flag, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sock_recvn");
	int total;
	void *bbuf;
	int bytes_read, num;
	fd_set fds;
	struct timeval tv, *ptv = NULL;
	FDMC_EXCEPTION x;

	//dbg_trace();
	TRYF(x)
	{
		for(total = 0; total < count; total += bytes_read)
		{
			FD_ZERO(&fds);
			if(sock->sec != 0 || sock->usec != 0)
			{
				tv.tv_sec = sock->sec;
				tv.tv_usec = sock->usec;
				ptv = &tv;
			}
			FD_SET(sock->data_socket, &fds);
			
			num = select(sock->data_socket+1, &fds, NULL, NULL, ptv);
			switch(num)
			{
			case 0:
				fdmc_raisef(FDMC_TIMEOUT_ERROR, &x, "socket receive timed out");
				return -1;
			case -1:
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "socket receive failed, code = %d", WSAGetLastError());
				return -1;
			}
			
			bytes_read = recv(sock->data_socket, buf+total, count-total, flag);
			bbuf = buf;
			if(!bytes_read) 
			{
				//fdmc_raisef(FDMC_SOCKET_ERROR, &x, "receive failed - connection closed by peer");
				return 0;
			}
			if(bytes_read == SOCKET_ERROR) 
			{
				fdmc_raisef(FDMC_SOCKET_ERROR, &x, "receive failed with code %d", WSAGetLastError());
				return SOCKET_ERROR;
			}
			if(sock->header_size == 0)
			{
				count = bytes_read;
				break;
			}
			// func_trace("received data");
			// fdmc_dump(buf, bytes_read);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		count = 0;
	}
	return count;
}

/* 
* Receive specified number of bytes from descriptor
* 
* psock - connected socket to receive data
* buffer - buffer to store received data
* nbytes - maximum number of bytes to receive
* err - error handler
*
* Borisov S. 11-02-2009
*
* Return
*	SUCCESS - number of bytes received
*	FAILURE - SOCKET_ERROR
*
* if there is an error and err is not null then function performs
* long jump to err descriptor
*/
int fdmc_sock_recv(FDMC_SOCK *psock, void *buffer, int nbytes, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sock_recv");
	int j, ja;
	FDMC_EXCEPTION x;
	byte *p;

	TRYF(x)
	{
		if(!psock)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "NULL socket in parameters");
		}
		j = psock->header_size + nbytes;
		if(psock->buf_size < j)
		{
			if(psock->dest_buf) fdmc_free(psock->dest_buf, NULL);
			psock->dest_buf = NULL;
			psock->buf_size = 0;
			psock->dest_buf = (char*)fdmc_malloc(j, &x);
			psock->buf_size = j;
		}
		p = (byte*)psock->dest_buf;
		if(psock->header_size)
		{
			j = fdmc_sock_recvn(psock, (char*)p, psock->header_size, 0, &x);
			if(j == 0)
				return 0;
			/* Calculate message length */
			if(psock->header_type == 1)
			{
				// This is text length
				char dg[10];
				memset(dg, 0, sizeof(dg));
				strncpy(dg, (char*)p, psock->header_size);
				ja = atoi(dg);
			}
			else
			{
				// This is traditional byte length
				ja = 0;
				for(j = 0; j < psock->header_size; j ++)
				{
					ja = ja * 0x100 + p[j];
				}
				if(ja > nbytes)
				{
					fdmc_raisef(FDMC_DATA_ERROR, &x, "Message length %d is greater than buffer size %d",
						ja, nbytes);
					return SOCKET_ERROR;
				}
			}
		}
		else if(psock->header_size == 0) 
		{
			ja = nbytes;
		}
		//j = recv(psock->data_socket, (char*)buffer, ja, 0); 
		j = fdmc_sock_recvn(psock, (char*)buffer, ja, 0, &x);
		if(j == 0)
		{
			return 0;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return SOCKET_ERROR;
	}
	return j;
}


/* 
* Check socket for ready reading data using "select" function
* 
* cf - socket descriptor to check
* sec - interval in seconds for select function
* usec - interval in microseconds for select function
*
* Borisov S. 11-02-2009
*
* Return
*    1 - socket is ready
*    0 - socket is not ready or socket error occurred
*
* if there is an error and err is not null then function performs
* long jump to err descriptor
*/
int fdmc_sock_ready(FDMC_SOCK *cf, int sec, int usec)
{
//	FUNC_NAME("fdmc_sock_ready");
	struct timeval tv, *vtv;
	fd_set fds;
	int res;

	tv.tv_sec = sec;
	tv.tv_usec = usec;
	vtv = &tv;
	if(sec == -1 && usec == -1) // Without timeout
		vtv = NULL;
	FD_ZERO(&fds);
	FD_SET(cf->data_socket, &fds);
	res = select(cf->data_socket+1, &fds, NULL, NULL, vtv);
	if(res > 0)
	{
		return 1;
	}
	return 0;
}

