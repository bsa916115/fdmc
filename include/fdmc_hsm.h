#ifndef _FDMC_HSM_INCLUDE
#define _FDMC_HSM_INCLUDE

//-------------------------------------------
// Name:
// fdmc_hsm
//-------------------------------------------
// Purpose:
// Provides functions for managing 
// security module
//-------------------------------------------

#include "fdmc_base.h"
#include <fcntl.h>
#ifdef _UNIX_
#include <termio.h>
#else
#include "fdmc_rs232.h"
#endif

// Structure for basic HSM operations
typedef struct _FDMC_HSM
{
	FDMC_SOCK *hsm_sock; // data socket to exchange data
#ifdef _WINDOWS_32
	FDMC_RS232 *hsm_serial; // HSM may be connected by serial port
#endif
	char *hsm_recv_buffer; // buffer to receive data from hsm
	char *hsm_send_buffer; // buffer to send data to hsm
	int hsm_bufsize; // size of hsm buffer 
	char hsm_header[256]; // hsm message header 
	int hsm_header_length; // length of hsm message header
	char hsm_trailer[33]; // hsm message trailer
	FDMC_THREAD_LOCK *hsm_lock; // hsm mutex
} FDMC_HSM;

// Structure for HSM error descrition
typedef struct _FDMC_HSM_ERRMSG
{
	int code;
	char *text;
} FDMC_HSM_ERRMSG;

// Exported functions
FDMC_HSM *fdmc_hsm_create(char *p_addr, int p_port, int p_bufsize, char *p_header, char *p_trailer, FDMC_EXCEPTION *err);
int fdmc_hsm_close(FDMC_HSM *p_hsm);
int fdmc_hsm_send(FDMC_HSM *p_hsm, char *p_data, int p_len, FDMC_EXCEPTION *err);
int fdmc_hsm_recv(FDMC_HSM *p_hsm, char *p_data, FDMC_EXCEPTION *err);
FDMC_HSM_ERRMSG* fdmc_hsm_resp_fail(FDMC_HSM *p_hsm, FDMC_EXCEPTION *err);
int fdmc_hsm_hash_password(FDMC_HSM *p_hsm, char *login, char *password, char *dest, FDMC_EXCEPTION *err);
int fdmc_hsm_cvc_calc(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_mmyy, char *p_srvc, char *p_cvc, FDMC_EXCEPTION *err);
int fdmc_hsm_cvc_check(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_mmyy, char* p_srvc, char *p_cvc, FDMC_EXCEPTION *err);
int fdmc_hsm_ibm_gen(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_pin, FDMC_EXCEPTION *err);
int fdmc_hsm_pin_lmk_zpk(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_pin, char *p_pin_format, char *pblock, FDMC_EXCEPTION *err);
int fdmc_hsm_ibm_check(FDMC_HSM *p_hsm, char* zpk, char *pvk, char *pblock, char *pformat, char *p_card , char *p_pin, FDMC_EXCEPTION *err);
int fdmc_hsm_zpk_gen(FDMC_HSM *p_hsm, char *p_zmk, char *p_zpk, FDMC_EXCEPTION *err);
int fdmc_hsm_load_format(FDMC_HSM *p_hsm, char *format, FDMC_EXCEPTION *err);
//int fdmc_hsm_gen_key(FDMC_HSM *p_hsm, char *zmk, char *p_type, char *p_key, FDMC_EXCEPTION*);
int fdmc_hsm_gen_key(FDMC_HSM *p_hsm, char *zmk, char *p_type, char *p_scheme_key, char *p_scheme_zmk, char *p_key, FDMC_EXCEPTION *err);
int fdmc_hsm_print_mail(FDMC_HSM *p_hsm, char *p_card, char *pin, char *name, char *bank, FDMC_EXCEPTION *x);
int fdmc_hsm_diag(FDMC_HSM *hsm, FDMC_EXCEPTION *err);
int fdmc_hsm_change_key(FDMC_HSM *hsm, char *key_current, char* key_stored, FDMC_EXCEPTION *err);
int fdmc_hsm_gen_mac(FDMC_HSM *hsm, char *mac_key, char *buffer, char *mac_result, FDMC_EXCEPTION *err);

#endif
