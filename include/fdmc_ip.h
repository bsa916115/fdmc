#ifndef _FDMC_IP_INCLUDE
#define _FDMC_IP_INCLUDE
#include "fdmc_global.h"
#include "fdmc_hash.h"

#define FDMC_SOCK_INACTIVE 0
#define FDMC_SOCK_BIND 1
#define FDMC_SOCK_ACTIVE 2
#define FDMC_SOCK_CONNECT 3
#define FDMC_SOCK_ERROR SOCKET_ERROR

extern SOCKET connect_to_host(char *addr, int port);
extern SOCKET connect_to_dest(char *addr, int port, unsigned attempts, long timeout_sec);
extern SOCKET port_listener(int port);
extern int get_peer_name(SOCKET s, char *name_buf);

extern int fdmc_ip_recvn(SOCKET sock, char *buf, int n, int flag);

extern FDMC_SOCK *fdmc_sock_create(char *ip_addr, int ip_port, int hdsize,
								   FDMC_EXCEPTION *err);

extern int fdmc_sock_close(FDMC_SOCK *psock);
extern int fdmc_init_ip(void);
extern int fdmc_delay(int sec, int usec);
extern int fdmc_msleep(unsigned millisec);
extern FDMC_SOCK *fdmc_sock_accept(FDMC_SOCK *psock, FDMC_EXCEPTION *err);
extern int fdmc_sock_error(FDMC_SOCK *psock);
extern SOCKET fdmc_lock_port(int p);
extern int fdmc_sock_send(FDMC_SOCK *psock, void *buffer, int bufsize, FDMC_EXCEPTION *err);
extern int fdmc_sock_recv(FDMC_SOCK *psock, void *buffer, int nbytes, FDMC_EXCEPTION *err);
extern int fdmc_sock_ready(FDMC_SOCK *cf, int sec, int usec);
int fdmc_sockpair_create(FDMC_SOCK *listener, FDMC_SOCK **s0, FDMC_SOCK **s1, FDMC_EXCEPTION* err);

typedef struct FDMC_SOCK_TABLE
{
	FDMC_HASH_TABLE *sock_hash; // Hash for fast access
	FDMC_LIST *sock_list; // Sorted socket list
	fd_set sock_set; // fd_set for fast access
	int lock;
} FDMC_SOCK_TABLE;

//extern FDMC_SOCK_TABLE fdmc_sock_table;

extern FDMC_SOCK_TABLE *fdmc_socktab_create(int size, FDMC_EXCEPTION *err);
extern int fdmc_sock_add(FDMC_SOCK *cf, FDMC_SOCK_TABLE *tf, FDMC_EXCEPTION *err);
extern int fdmc_sock_remove(FDMC_SOCK *cf, FDMC_SOCK_TABLE *tf, FDMC_EXCEPTION *err);
extern int fdmc_socktab_sel(FDMC_SOCK_TABLE *tf, fd_set *dest, long sec, long usec);

#endif
