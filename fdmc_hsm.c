#include "fdmc_hsm.h"
#include "fdmc_bsamsg.h"

MODULE_NAME("fdmc_hsm.c");

#define LMK_ID "01"

// List of Racal/Thales errors
FDMC_HSM_ERRMSG hsm_errmsg[] =
{
	{-1, "Invalid response"},
	{0, "No error"},
	{1, "Verification failure"},
	{2, "Inappropriate key length"},
	{4, "Invalid code"},
	{5, "Invalid type"},
	{10, "Source key parity error"},
	{11, "Destination key parity error"},
	{12, "Contents of user storage not available"},
	{13, "Master key parity error"},
	{14, "Encrypted PIN is invalid"},
	{15, "Invalid input data"},
	{16, "Printer error"},
	{17, "Not in authorized state"},
	{18, "Document format definition not loaded"},
	{19, "Inalid Diebold table"},
	{20, "Not valid PIN block"},
	{21, "Invalid index value"},
	{22, "Invalid account number"},
	{23, "Invalid PIN block format code"},
	{24, "Invalid PIN length"},
	{25, "Decimalization table error"},
	{26, "Invalid key scheme"},
	{27, "Incompatible key length"},
	{28, "Invalid key type"},
	{29, "Key function not permitted"},
	{30, "Invalid reference number"},
	{31, "Insufficient solicitation entries for batch"},
	{33, "LMK key change storage is corrupted"},
	{40, "Invalid firmware checksum"},
	{41, "Internal error"},
	{42, "DES failure"},
	{68, "Operation not licensed"},
	{80, "Data length error"},
	{90, "Data parity error in request message"},
	{91, "LRC error"},
	{92, "Count value is not within limits"},
	{100, "Unknown error"}
};

//-------------------------------------------
// Name:
// fdmc_hsm_create
//-------------------------------------------
// Purpose:
// Create new hsm descriptor and allocate buffers
//-------------------------------------------
// Parameters:
// p_addr - hsm network address
//   When p_addr is null we assume that HSM connected through serial port
// p_port - public hsm ip or COM port
// p_bufsize - memory size for buffers
//-------------------------------------------
// Returns:
// true - new hsm descriptor 
// NULL - fail
//-------------------------------------------
// Special features:
//
//-------------------------------------------
FDMC_HSM *fdmc_hsm_create(char *p_addr, int p_port, int p_bufsize, char *p_header, char *p_trailer, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_create");
	FDMC_EXCEPTION x;
	FDMC_HSM *v_new;

	dbg_trace();

	TRYF(x)
	{
		CHECK_ZERO(p_bufsize, "p_bufsize", x);
		v_new = (FDMC_HSM*)fdmc_malloc(sizeof(*v_new), &x);
		v_new->hsm_send_buffer = (char*)fdmc_malloc(p_bufsize, &x);
		v_new->hsm_recv_buffer = (char*)fdmc_malloc(p_bufsize, &x);
		v_new->hsm_bufsize = p_bufsize;
		strcpy(v_new->hsm_header, p_header);
		v_new->hsm_header_length = strlen(p_header);
		if(p_trailer)
		{
			strcpy(v_new->hsm_trailer, p_trailer);
		}
		v_new->hsm_lock = fdmc_thread_lock_create(&x);
		//func_trace("Variables initialized");
		if(p_addr != NULL)
		{
			//func_trace("creating socket for HSM");
			v_new->hsm_sock = fdmc_sock_create(p_addr, p_port, 2, &x);
		}
#ifdef _WINDOWS_32
		else
		{
			wchar_t v_port[10];
			if((unsigned)p_port < 10)
			{
				func_trace("creating port %d", p_port);
				wsprintf(v_port, L"COM%d", p_port);
				v_new->hsm_serial = fdmc_rs_create(v_port, &x);
			}
			else
			{
				char *emsg = "Invalid COM port value %d";
				screen_trace(emsg, p_port);
				fdmc_raisef(FDMC_IO_ERROR, &x, emsg, p_port);
			}
		}
#endif
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	func_trace("Returning from hsm create");
	return v_new;
}


//-------------------------------------------
// Name:
// fdmc_hsm_close
//-------------------------------------------
// Purpose:
// Delete hsm descriptor
//-------------------------------------------
// Parameters:
// p_hsm - hsm descriptor
//-------------------------------------------
// Returns:
// true - success
// false - fail
//-------------------------------------------
// Special features:
// 
//-------------------------------------------
int fdmc_hsm_close(FDMC_HSM *p_hsm)
{
	FUNC_NAME("fdmc_hsm_close");
	FDMC_EXCEPTION x;

	dbg_trace();

	TRYF(x)
	{
		CHECK_NULL(p_hsm, "p_hsm", x);
		fdmc_thread_lock_delete(p_hsm->hsm_lock);
		fdmc_free(p_hsm->hsm_recv_buffer, &x);
		fdmc_free(p_hsm->hsm_send_buffer, &x);
		if(p_hsm->hsm_sock) fdmc_sock_close(p_hsm->hsm_sock);
#ifdef _WINDOWS_32
		else fdmc_rs_close(p_hsm->hsm_serial);
#endif
		fdmc_free(p_hsm, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, NULL);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name:
// fdmc_hsm_send
//-------------------------------------------
// Purpose:
// send data to host security module
//-------------------------------------------
// Parameters:
// p_hsm - host security module descriptor
// p_data - output data
// p_len - length of output data
// err - exception
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_hsm_send(FDMC_HSM *p_hsm, char *p_data, int p_len, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_send");
	FDMC_EXCEPTION x;

	dbg_trace();

	TRYF(x)
	{
		if(p_hsm->hsm_sock)
		{
			strcpy(p_hsm->hsm_send_buffer, p_hsm->hsm_header);
			memcpy(p_hsm->hsm_send_buffer + p_hsm->hsm_header_length, p_data, p_len);
			func_trace("send data to HSM");
			fdmc_dump(p_hsm->hsm_send_buffer, p_len + p_hsm->hsm_header_length);
			fdmc_sock_send(p_hsm->hsm_sock, p_hsm->hsm_send_buffer, p_len + p_hsm->hsm_header_length, &x);
		}
#ifdef _WINDOWS_32
		else if(p_hsm->hsm_serial)
		{
			int len;
			len = sprintf(p_hsm->hsm_send_buffer, "\02%s%s\03", p_hsm->hsm_header, p_data);
			func_trace("Send data to HSM");
			fdmc_dump(p_hsm->hsm_send_buffer, len);
			fdmc_rs_write(p_hsm->hsm_serial, p_hsm->hsm_send_buffer, len, &x);
		}
#endif
		else
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "No communication channel for HSM");
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name:
// fdmc_hsm_recv
//-------------------------------------------
// Purpose:
// receive response from hsm
//-------------------------------------------
// Parameters:
// p_hsm - hsm descriptor
// p_data - data buffer
//-------------------------------------------
// Returns:
// number of bytes received
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_hsm_recv(FDMC_HSM *p_hsm, char *p_data, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_recv");
	FDMC_EXCEPTION x;
	int n;

	dbg_trace();

	TRYF(x)
	{
		CHECK_NULL(p_hsm, "p_hsm", x);
		if(p_hsm->hsm_sock)
		{
			func_trace("Receive data from socket");
			n = fdmc_sock_recv(p_hsm->hsm_sock, p_hsm->hsm_recv_buffer, p_hsm->hsm_bufsize, &x);
			if(n == 0)
			{
				func_trace("connection with hsm lost");
				return 0;
			}
			func_trace("received from hsm");
			fdmc_dump(p_hsm->hsm_recv_buffer, n);
			memcpy(p_data, p_hsm->hsm_recv_buffer + p_hsm->hsm_header_length, n - p_hsm->hsm_header_length);
			n -= p_hsm->hsm_header_length;
		}
#ifdef _WINDOWS_32
		else if(p_hsm->hsm_serial)
		{
			char c;
			func_trace("Receive data from seril port");
			do
			{
				fdmc_rs_read(p_hsm->hsm_serial, &c, 1, &x);
			} while (c != '\02');
			n = 0;
			p_hsm->hsm_recv_buffer[n++] = c;
			for(;;)
			{
				fdmc_rs_read(p_hsm->hsm_serial, &c, 1, &x);
				p_hsm->hsm_recv_buffer[n++] = c;
				if(c == '\03')
				{
					break;
				}
			}
			func_trace("Received from HSM");
			fdmc_dump(p_hsm->hsm_recv_buffer, n);
			sprintf(p_data, "%.*s", n-(2+p_hsm->hsm_header_length), p_hsm->hsm_recv_buffer + 1 + p_hsm->hsm_header_length);
		}
#endif
		else
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "No communication channel for HSM");
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return n;
}

//-------------------------------------------
// Name:
// fdmc_hsm_resp_fail
//-------------------------------------------
// Purpose:
// check hsm response
//-------------------------------------------
// Parameters:
// p_hsm - hsm descriptor
//-------------------------------------------
// Returns:
// NULL - no error
// otherwize - pointer to error structure
//-------------------------------------------
// Special features:
//
//-------------------------------------------
FDMC_HSM_ERRMSG* fdmc_hsm_resp_fail(FDMC_HSM *p_hsm, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_resp_fail");
	FDMC_EXCEPTION x;
	char rbuf[3];
	int i;

	TRYF(x)
	{
		CHECK_NULL(p_hsm, "p_hsm", x);
		if(strlen(p_hsm->hsm_recv_buffer) < (size_t)p_hsm->hsm_header_length + 4)
		{
			fdmc_raisef(FDMC_SECURITY_ERROR, &x,  "Incorrect HSM response");
		}
		if(p_hsm->hsm_sock)
		{
			strncpy(rbuf, p_hsm->hsm_recv_buffer + p_hsm->hsm_header_length + 2, 2);
			rbuf[2] = 0;
		}
#ifdef _WINDOWS_32
		else if(p_hsm->hsm_serial)
		{
			sprintf(rbuf, "%.2s", p_hsm->hsm_recv_buffer + p_hsm->hsm_header_length + 1 + 2);
		}
#endif
		else
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "No communication channel for HSM");
		}
		if(!isdigit(rbuf[0]) || !isdigit(rbuf[1]))
		{
			return &hsm_errmsg[0];
		}
		x.errorsubcode = atoi(rbuf);
		if(x.errorsubcode == 0 || x.errorsubcode == 2)
		{
			return NULL;
		}
		for(i = 0; ;i++)
		{
			if(x.errorsubcode == hsm_errmsg[i].code)
				break;
			if(hsm_errmsg[i].code == 100)
				break;
		}
		fdmc_raisef(FDMC_SECURITY_ERROR, &x, "HSM responded with error %d, %s", hsm_errmsg[i].code, hsm_errmsg[i].text);
	}
	EXCEPTION
	{
		trace("HSM returned an error %s", x.errortext);
		fdmc_raiseup(&x, err);
	}
	return &hsm_errmsg[i];
}

//---------------------------------------------------------
//  name: fdmc_hsm_account
//---------------------------------------------------------
//  purpose: generate account number for HSM command (12 digits)
//  designer: Serge Borisov (BSA)
//  started: 10.04.10
//	parameters:
//		cardnum_in - source card number
//		hsmacc_out - buffer to hold result account
//	return:
//		True - Success
//		False - Failure
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_hsm_account(char *cardnum_in, char *hsmacc_out)
{
	FUNC_NAME("fdmc_hsm_account");
	FDMC_EXCEPTION x;
	size_t len;

	TRYF(x)
	{
		CHECK_NULL(cardnum_in, "cardnum_in", x);
		if((len = strlen(cardnum_in)) < 12)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "too short card number '%s'", cardnum_in);
		}
		CHECK_NULL(hsmacc_out, "hsmacc_out", x);
		if(len == 12)
		{
			strcpy(hsmacc_out, cardnum_in);
			return 1;
		}
		len --;
		strncpy(hsmacc_out, cardnum_in + len - 12, 12);
		hsmacc_out[12] = 0;
	}
	EXCEPTION
	{
		if(hsmacc_out)
		{
			*hsmacc_out = 0;
		}
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name:
// fdmc_hsm_hash_block
//-------------------------------------------
// Purpose:
// hash data block
//-------------------------------------------
// Parameters:
// p_hsm - open hsm descriptor
// p_data - data block
// p_dest - destination buffer
// err - exception
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_hsm_hash_block(FDMC_HSM *p_hsm, char *p_data, int length, char *p_type, char *p_dest, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_hash_block");
	FDMC_EXCEPTION x;
	dbg_trace();

	TRYF(x)
	{
		char buf[1024*8];
		int n = sprintf(buf, "GM%.2s%05d", p_type, length);
		memcpy(buf + n, p_data, length);
		fdmc_hsm_send(p_hsm, buf, length + n, &x);
		n = fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
		data_to_hex(p_dest, buf, n);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name:
// fdmc_hsm_hash_password
//-------------------------------------------
// Purpose:
// generate login/password hash
//-------------------------------------------
// Parameters:
// p_hsm - hsm descriptor
// login - user login
// password - user password
// dest - destination buffer
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_hsm_hash_password(FDMC_HSM *p_hsm, char *login, char *password, char *dest, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_hash_password");
	FDMC_EXCEPTION x;

	dbg_trace();

	TRYF(x)
	{
		char buf[256];
		int n = sprintf(buf, "%s%s", login, password);
		fdmc_hsm_hash_block(p_hsm, buf, n, "01", dest, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}


//-------------------------------------------
// Name:
// fdmc_hsm_cvc
//-------------------------------------------
// Purpose:
// generate cvc1/2 value for card
//-------------------------------------------
// Parameters:
// p_hsm - hsm descriptor
// p_key - cvc keyblock
// p_card - card number
// p_mmyy - card expiration date
// p_srvc - Service code of card
// p_cvc - buffer to hold cvc value
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_hsm_cvc_calc(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_mmyy, char *p_srvc, char *p_cvc, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_cvc");
	FDMC_EXCEPTION x;
	int n;

	dbg_trace();

	TRYF(x)
	{
		char buf[1024*8];
		n = sprintf(buf, "CW%s%s;%s%s", p_key, p_card, p_mmyy, p_srvc); 
		fdmc_hsm_send(p_hsm, buf, n, &x);
		n = fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
		strncpy(p_cvc, buf + 4, 3);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_hsm_cvc_check
// 
//-------------------------------------------
// Purpose:
// Check cvc1/2 in incoming message
//-------------------------------------------
// Parameters:
// p_hsm - hsm descriptor
// p_key - cvc keyblock
// p_card - card number
// p_mmyy - card expiration date
// p_srvc - card service code
// p_cvc - buffer of checked cvc value
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_hsm_cvc_check(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_mmyy, char* p_srvc, char *p_cvc, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_cvc_check");
	FDMC_EXCEPTION x;
	int n;

	dbg_trace();

	TRYF(x)
	{
		char buf[1024*8];
		n = sprintf(buf, "CY%s%s%s;%s", p_key, p_cvc, p_card, p_mmyy, p_srvc, LMK_ID); 
		fdmc_hsm_send(p_hsm, buf, n, &x);
		n = fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_gen_component
// 
//-------------------------------------------
// Purpose:
// generate component for key
//-------------------------------------------
// Parameters:
// None
//-------------------------------------------
// Returns:
// true or false
//-------------------------------------------
// Special features:
//
//-------------------------------------------
int fdmc_gen_component(FDMC_HSM *p_hsm, char *p_keyusage, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_gen_component");
	FDMC_EXCEPTION x;

	dbg_trace();

	TRYF(x)
	{
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

int translate_pin(FDMC_HSM *hsm, char *v_acc, char *pin, char *clear)
{
	char buf[402];
	int len;
	FDMC_EXCEPTION x;
	FUNC_NAME("translate");
	char cpin[256], acc[16]; 

	TRYF(x)
	{
		fdmc_hsm_account(v_acc, acc);
		len = sprintf(buf, "NG%s%s", acc, pin);
		fdmc_hsm_send(hsm, buf, len, &x);
		memset(cpin, 0, sizeof(cpin));
		fdmc_hsm_recv(hsm, cpin, &x);
		fdmc_hsm_resp_fail(hsm, &x);
		sprintf(clear, "%4.4s", cpin+4);
		//trace("Open Pin is %s", clear);
	}
	EXCEPTION
	{
		trace("NG error %s", x.errortext);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name: fdmc_hsm_ibm_gen
// 
//---------------------------------------------------------
// Purpose:
// Generate PIN using IBM method
//---------------------------------------------------------
// Parameters:
// p_hsm - pointer to hsm structure
// p_key - pin verification key
// p_card - primary account number
// p_pin - destination buffer
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
int fdmc_hsm_ibm_gen(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_pin, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_ibm_gen");
	FDMC_EXCEPTION x;
	char buf[1024];
	char v_acc[20];
	char v_valid[20];
	int l;

	dbg_trace();

	TRYF(x)
	{
		fdmc_hsm_account(p_card, v_acc);
		sprintf(v_valid, "%11.11sN", p_card);
		l = sprintf(buf, "EE%s0000FFFFFFFF04%s%s%s", p_key, v_acc, "0123456789012345", v_valid);
		fdmc_hsm_send(p_hsm, buf, l, &x);
		memset(buf, 0, sizeof(buf));
		fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
		strncpy(p_pin, buf + 4, 5);
		p_pin[5] = 0;
		//decrypt_pin(p_hsm, v_acc, p_pin);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

int fdmc_hsm_ibm_check(FDMC_HSM *p_hsm, char* zpk, char *pvk, char *pblock, char *pformat, char *p_card , char *p_pin, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_ibm_check");
	FDMC_EXCEPTION x;
	char buf[1024];
	char v_acc[20];
	char v_valid[20];
	int l;

	dbg_trace();
	TRYF(x)
	{
	fdmc_hsm_account(p_card, v_acc);
	sprintf(v_valid, "%11.11sN", p_card);
	l = sprintf(buf, "EA%s%s12%s%s04%s0123456789012345%s0000FFFFFFFF", zpk, pvk, pblock, pformat, v_acc, v_valid);
	fdmc_hsm_send(p_hsm, buf, l, &x);
	fdmc_hsm_recv(p_hsm, buf, &x);
	fdmc_hsm_resp_fail(p_hsm, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name:
// fdmc_hsm_load_format
//-------------------------------------------
// Purpose:
// Load pin mail format into hsm
//-------------------------------------------
// Parameters:
// p_hsm - pointer to hsm structure
// format - format string
// err - Exception handler
//-------------------------------------------
// Returns:
// true if success
// false on failure
//-------------------------------------------
// Special features:
// performs longjump in case of error
//-------------------------------------------
int fdmc_hsm_load_format(FDMC_HSM *p_hsm, char *format, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_load_format");
	FDMC_EXCEPTION x;
	char buf[1024];
	int l;
	dbg_trace();

	TRYF(x)
	{
		l = sprintf(buf, "PA%s", format);
		fdmc_hsm_send(p_hsm, buf, l, &x);
		fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_hsm_gen_key
//---------------------------------------------------------
// Purpose:
// 
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
int fdmc_hsm_gen_key(FDMC_HSM *p_hsm, char *zmk, char *p_type, char *p_scheme_key, char *p_scheme_zmk, char *p_key, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_gen_key");
	FDMC_EXCEPTION x;
	char buf[128];
	int len;
	dbg_trace();


	TRYF(x)
	{
		CHECK_NULL(p_hsm, "p_hsm", x);
		CHECK_NULL(p_type, "p_type", x);
		CHECK_NULL(p_key, "p_key", x);
		len = sprintf(buf, "A0%s%s%s%s%s", zmk ? "1":"0", p_type, p_scheme_key, zmk ? zmk:"", zmk ? p_scheme_zmk : "");
		fdmc_hsm_send(p_hsm, buf, len, &x);
		fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
		switch(buf[4])
		{
		case 'U':
			sprintf(p_key, "%33.33s", buf + 4);
			break;
		case 'T':
			sprintf(p_key, "%49.49s", buf + 4);
			break;
		default:
			sprintf(p_key, "%16.16s", buf + 4);
			break;
		}
	}
	EXCEPTION
	{
		func_trace("%s", &x);
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_hsm_gen_mac
//---------------------------------------------------------
// Purpose:
// 
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
int fdmc_hsm_gen_mac(FDMC_HSM *hsm, char *mac_key, char *buffer, char *mac_result, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_gen_mac");
	FDMC_EXCEPTION x;
	char buf[1024*2];
	int len;

	dbg_trace();

	TRYF(x)
	{
		len = sprintf(buf, "MA%s%s", mac_key, buffer);
		fdmc_hsm_send(hsm, buf, len, &x);
		len = fdmc_hsm_recv(hsm, buf, &x); buf[len] = 0;
		fdmc_hsm_resp_fail(hsm, &x);
		strcpy(mac_result, buf+4);
		func_trace("MAC generated as %s", buf);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_hsm_print_mail
//---------------------------------------------------------
// Purpose:
// print pin mailer with specified format
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
int fdmc_hsm_print_mail(FDMC_HSM *p_hsm, char *p_card, char *pin, char *name, char *bank, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_print_mail");
	FDMC_EXCEPTION x;
	char buf[512];
	char v_acc[20];
	int len;
	dbg_trace();

	TRYF(x)
	{
		fdmc_hsm_account(p_card, v_acc);
		//trace("'%s'", v_acc);
		//trace("'%s'", pin);
		//trace("'%s'",name);
		//trace("'%s'",bank);
		len = sprintf(buf, "PEC%s%s%s;%s;STREAMPAY", v_acc, pin, name, bank);
		//trace("buffer is %s", buf);
		fdmc_hsm_send(p_hsm, buf, len, &x);
		fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
		fdmc_msleep(5);
		fdmc_hsm_recv(p_hsm, buf, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_hsm_pin_lmk_zpk
//---------------------------------------------------------
// Purpose:
// Translates PIN from lmk into ZPK encryption
//---------------------------------------------------------
// Parameters:
// p_hsm hsm descriptor
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
int fdmc_hsm_pin_lmk_zpk(FDMC_HSM *p_hsm, char *p_key, char *p_card, char *p_pin, char *p_pin_format, char *pblock, FDMC_EXCEPTION *err)
{
	FUNC_NAME("");
	FDMC_EXCEPTION x;
	char buf[256], v_acc[20];
	int len; char pblck[64];
	dbg_trace();

	TRYF(x)
	{
		memset(buf, 0, sizeof(buf));
		memset(pblck, 0, sizeof(pblck));
		fdmc_hsm_account(p_card, v_acc);
		len = sprintf(buf, "JG%s%s%s%s", p_key, p_pin_format, v_acc, p_pin, &x);
		//trace("%s", buf);
		fdmc_hsm_send(p_hsm, buf, len, &x);
		fdmc_hsm_recv(p_hsm, pblck, &x);
		fdmc_hsm_resp_fail(p_hsm, &x);
		strncpy(pblock, pblck + 4, 16);
		pblock[16] = 0;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_hsm_diag
//---------------------------------------------------------
// Purpose:
// Perform HSM self test before start to work
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
int fdmc_hsm_diag(FDMC_HSM *hsm, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_diag");
	FDMC_EXCEPTION x;
	char buf[1024];
	int len;

	dbg_trace();

	TRYF(x)
	{
		len = sprintf(buf, "NC");
		fdmc_hsm_send(hsm, buf, len, &x);
		memset(buf, 0, sizeof(buf));
		fdmc_hsm_recv(hsm, buf, &x);
		fdmc_hsm_resp_fail(hsm, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
// Name:
// fdmc_hsm_change_key
//---------------------------------------------------------
// Purpose:
// Encrypt key_stored TMK under key_current TMK
// New value will returned to key_stored
//---------------------------------------------------------
// Parameters:
// hsm - hsm descriptor
// key_current - Master TMK
// key_stored - TMK to encrypt
// err - Exception handler
//---------------------------------------------------------
// Returns:
// true if success
// false on failure
//---------------------------------------------------------
// Special features:
// performs longjump in case of error
//---------------------------------------------------------
int fdmc_hsm_change_key(FDMC_HSM *hsm, char *key_current, char* key_stored, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hsm_change_key");
	FDMC_EXCEPTION x;
	char buf[1024];
	int len;

	dbg_trace();

	TRYF(x)
	{
		// Generate AE command
		CHECK_NULL(hsm, "hsm", x);
		CHECK_NULL(key_current, "key_current", x);
		CHECK_NULL(key_stored, "key_stored", x);
		len = sprintf(buf, "AE%s%s", key_current, key_stored);
		fdmc_hsm_send(hsm, buf, len, &x);
		memset(buf, 0, sizeof(buf));
		len = fdmc_hsm_recv(hsm, buf, &x);
		buf[len+1] = 0;
		func_trace("Checking responce");
		fdmc_hsm_resp_fail(hsm, &x);
		trace("responce ok");
		switch(buf[4])
		{
		case 'U':
			len = 33;
			break;
		case 'T':
			len = 65;
			break;
		default:
			len = 16;
			break;
		}
		//func_trace("Buffer is %s, length is %d", buf, len);
		len -= 4;
		sprintf(key_stored, "%s", &buf[4]);
		//func_trace("key_stored as %s", key_stored);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

