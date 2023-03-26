#ifndef _FDMC_IP_INCLUDE
#define _FDMC_IP_INCLUDE
#include "fdmc_global.h"
#if defined (_WINDOWS_32)
/* Windows includes */
#include <windows.h>
#include <io.h>
#include <time.h>
extern int fdml_init_ip(void);
#elif defined (_UNIX_32_) || defined(_UNIX_64_)
/* UNIX includes */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#define SOCKET int
#define closesocket close
#define fdml_init_ip() (0)
#endif

/* Непрерывные попытки соединиться */
#define connect_to_host fdml_connect_to_host
extern SOCKET connect_to_host(char *addr, int port);

/* Попытка соединиться опреленной количество раз с временным интервалом */
#define connect_to_dest fdml_connect_to_dest
extern SOCKET connect_to_dest(char *addr, int port, unsigned attempts, long timeout_sec);

#define port_listener fdml_port_listener
extern SOCKET port_listener(int port);

#define get_peer_name fdml_get_peer_name
extern int get_peer_name(SOCKET s, char *name_buf);

/* Анало функции usleep */
#define delay fdml_delay
extern int delay(int sec, int mcsec);

extern int fdml_ip_receive_bytes(SOCKET sock, char *buf, int count, int flag);

#endif
