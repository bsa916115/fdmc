#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "fdmc_isomsg.h"
#include "fdmc_msgfield.h"
#include "fdmc_logfile.h"

MODULE_NAME("fdmc_isomsg.c");

/* 
Convert bcd presentation into ascii presentation
Only '0'..'9', 'A'..'F' characters are correct
*/
static int bcd_to_str(char *dest, char *source, int len, int padding)
{
//	FUNC_NAME("bcd_to_str");
	int bindx, i, nbytes=0;

	bindx = len % 2;
	if(padding == FDMC_ALIGN_LEFT)
	{
		bindx = 0;
	}

	nbytes = len / 2 + len % 2;
	for(i = 0; i < len; bindx ++, i++, dest++)
	{
		unsigned char c;
		if(bindx % 2 == 0)
		{
			c = ((unsigned char)(source[bindx/2])) >> 4;
		}
		else
		{
			c = ((unsigned char)(source[bindx/2])) & 0xf;
		}
		if(c <= 9)
		{
			c += '0';
		}
		else
		{
			c = c - 10 + 'A';
		}
		*dest = (char)c;
	}
	*dest = 0;
	return nbytes;
}

/* 
Convert ascii presentation into bcd presentation
Only '0'..'9', 'A'..'F' characters are correct
*/
static int str_to_bcd(char *dest, char *source, int padding)
{
//	FUNC_NAME("str_to_bcd");
	int len, i;
	char tmpbuf[1024];
	unsigned char c=0, d=0;

	len = (int)strlen(source);
	if(len % 2 == 0)
	{
		strcpy(tmpbuf, source);
	}
	else
	{
		if(padding == FDMC_ALIGN_LEFT)
		{
			len = sprintf(tmpbuf, "%s0", source);
		}
		else
		{
			len = sprintf(tmpbuf, "0%s", source);
		}
	}
	for(i = 0; i < len; i++)
	{
		if(isdigit(tmpbuf[i])) /* 0..9 */
		{
			c = tmpbuf[i]-'0';
		}
		else /* A..F */
		{
			c = tmpbuf[i] - 'A' + 10;
		}
		if(i % 2 == 0)
		{
			d = c;
		}
		else
		{
			d = (d << 4) + c;
			*dest = (char)d;
			dest++;
		}
	}
	return len/2;
}

int fdmc_bcd_dump(void *bufp /* Буфер */, int len /* Кол-во байт */)
{
//	FUNC_NAME("bcd_dump");

	int i;
	unsigned char *buf = bufp;

	for(i = 0; i < len; i++)
	{
		tprintf("%02X ", buf[i]);
	}

	return 1;
}

static int common_bcd_format(FDMC_MSGFIELD *field, char *buffer, 
							 char *databuffer, int buflen, FDMC_EXCEPTION *err)
{
//	FUNC_NAME("common_bcd_format");
//	int len;
	int dtlen;
	char bcdbuffer[MAX_FIELD_SIZE+1];

	//len = buflen;
	if(field->align == FDMC_ALIGN_SIGN)
	{
		databuffer[0] = databuffer[0] == '+' ? 'C':'D';
	}
	if(field->lensize != 0)
	{
		sprintf(bcdbuffer, "%0*d", field->lensize, buflen);
		dtlen = str_to_bcd(buffer, bcdbuffer, FDMC_ALIGN_ZERO);
		dtlen += str_to_bcd(buffer+dtlen, databuffer, field->align);
		if(debug_formats)
		{
			tprintf("(%03d)[%s]\t|", field->number, field->name);
			fdmc_bcd_dump(buffer, dtlen);
			trace("|");
		}
		return dtlen;
	}
	else
	{
		dtlen = str_to_bcd(buffer, databuffer, field->align);
		if(debug_formats)
		{
			tprintf("(%03d)[%s]\t|", field->number, field->name);
			fdmc_bcd_dump(buffer, dtlen);
			trace("|");
		}
		return dtlen;
	}
//	return len;
}

static int common_get_bcd(FDMC_MSGFIELD *field, char *buffer,
						  char *databuffer, FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_get_bcd");
	int len, dlen;
	int dtlen;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(field->lensize != 0)
		{
			dtlen = bcd_to_str(databuffer, buffer, field->lensize, field->align);
			databuffer[field->lensize] = 0;
			len = atoi(databuffer);
			if(!len)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x, 
					"Invalid field length in field '%s'", field->name);
			}
			dlen = bcd_to_str(databuffer, buffer+dtlen, len, field->align);
			if(debug_formats)
			{
				tprintf("(%03d)[%s]\t|", field->number, field->name);
				fdmc_bcd_dump(buffer, dlen + dtlen);
				trace("|");
			}
			databuffer[len] = 0;
			len = dtlen + dlen;
		}
		else
		{
			len = bcd_to_str(databuffer, buffer, field->size, field->align);
			databuffer[field->size] = 0;
			if(debug_formats)
			{
				tprintf("(%03d)[%s]\t|", field->number, field->name);
				fdmc_bcd_dump(buffer, len);
				trace("|");
			}
		}
		if(field->align == FDMC_ALIGN_SIGN)
		{
			databuffer[0] = (databuffer[0] == 'C' ? '+':'-');
		}
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return len;
}

int fdmc_int_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				 FDMC_EXCEPTION *err)
{
//	FUNC_NAME("int_bcdfield");
	int *data=field->data, len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, *data);
		len = common_bcd_format(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_bcd(field, buffer, databuffer, err);
		*data = atoi(databuffer);
		return len;
	default:
		len = fdmc_int_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}

int fdmc_float_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				   FDMC_EXCEPTION *err)
{
//	FUNC_NAME("float_bcdfield");
	double *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, *data);
		len = common_bcd_format(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_bcd(field, buffer, databuffer, err);
		*data = atof(databuffer);
		return len;
	default:
		len = fdmc_float_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}

int fdmc_char_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				  FDMC_EXCEPTION *err)
{
//	FUNC_NAME("char_bcdfield");
	char *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data);
		len = common_bcd_format(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_bcd(field, buffer, databuffer, err);
		strcpy(data, databuffer);
		return len;
	default:
		len = fdmc_char_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}

typedef struct
{
	_ORA_STRINGLENGTH len;
	char arr[1];

} vchar;

int fdmc_hostchar_bcdfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
					  FDMC_EXCEPTION *err)
{
//	FUNC_NAME("hostchar_bcdfield");
	vchar *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data->arr);
		len = common_bcd_format(field, buffer, databuffer, len, err);
		data->len = len - field->lensize;
		return len;
	case FDMC_EXTRACT:
		len = common_get_bcd(field, buffer, databuffer, err);
		strcpy(data->arr, databuffer);
		data->len =len - field->lensize;
		return len;
	default:
		len = fdmc_hostchar_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}

int fdmc_bitmap_field(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				 FDMC_EXCEPTION *err)
{
//	FUNC_NAME("bitmap_field");
	unsigned char *data = field->data;
	int len;

	switch(op)
	{
	case FDMC_FORMAT:
		len = fdmc_bit_is_set(data, 1) ? 16 : 8;
		memcpy(buffer, data, len);
		if(debug_formats)
		{
			tprintf("(%03d)[%s]\t|", field->number, field->name);
			fdmc_dump(data, len);
			trace("|");
		}
		return len;
	case FDMC_EXTRACT:
		memcpy(data, buffer, 16);
		len = fdmc_bit_is_set(data, 1) ? 16 : 8;
		if(debug_formats)
		{
			tprintf("(%03d)[%s]\t|", field->number, field->name);
			fdmc_dump(data, len);
			trace("|");
		}
		return len;
	case FDMC_INIT:
		memset(data, 0, 16);
		return 16;
	case FDMC_PRINT:
		tprintf("%s:\t", field->name);
		len = fdmc_bit_is_set(data, 1) ? 16 : 8;
		fdmc_bcd_dump(data, len);
		tprintf("\n");
		return len;
	}
	return -1;
}

int fdmc_binary_field(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				 FDMC_EXCEPTION *err)
{
//	FUNC_NAME("binary_field");
	unsigned char *data = field->data;

	switch(op)
	{
	case FDMC_FORMAT:
		if(field->lensize == 0)
		{
			memcpy(buffer, data, field->size);
			if(debug_formats)
			{
				tprintf("(%03d)[%s]\t|", field->number, field->name);
				fdmc_bcd_dump(data, field->size);
				trace("|");
			}
			return field->size;
		}
		else
		{
			sprintf(buffer, "%0*d", field->lensize, field->size);
			memcpy(buffer + field->lensize, data, field->size);
			if(debug_formats)
			{
				tprintf("(%03d)[%s]\t|", field->number, field->name);
				fdmc_bcd_dump(buffer, field->lensize + field->size);
				trace("|");
			}
			return field->lensize + field->size;
		}
	case FDMC_EXTRACT:
		if(field->lensize == 0)
		{
			memcpy(data, buffer, field->size);
			if(debug_formats)
			{
				tprintf("(%03d)[%s]\t|", field->number, field->name);
				fdmc_bcd_dump(data, field->size);
				trace("|");
			}
			return field->size;
		}
		else
		{
			char lenstr[6];
			strncpy(lenstr, buffer, field->size);
			lenstr[field->size] = 0;
			field->size = atoi(lenstr);
			memcpy(data, buffer + field->lensize, field->size);
			if(debug_formats)
			{
				tprintf("(%03d)[%s]|", field->number, field->name);
				fdmc_bcd_dump(buffer, field->lensize + field->size);
				trace("|");
			}
			return field->lensize + field->size;
		}
	case FDMC_PRINT:
		tprintf("%s:\t", field->name);
		fdmc_bcd_dump(field->data, field->size);
		return field->size;
	}
	return -1;
}

int fdmc_iso_message(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_iso_message");
	unsigned char *bitmap;
	int maxnumber;
	int len, total;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(op == FDMC_EXTRACT && debug_formats)
		{
			trace("Extracting message:");
			fdmc_dump(buffer, field->size);
		}
		total = 0;
		/* First field is message number */
		field ++;
		fdmc_common_check_field(field, err);
		if(debug_formats && (op == FDMC_FORMAT || op == FDMC_EXTRACT))
		{
			tprintf("|%04d|", total);
		}
		len = (*(field->field_func))(field, buffer, op, err);
		total += len;
		/* Second field is a bitmask */
		field++;
		bitmap = field->data;
		if(debug_formats && (op == FDMC_FORMAT || op == FDMC_EXTRACT))
		{
			tprintf("|%04d|", total);
		}
		len = (*(field->field_func))(field, buffer+total, op, err);
		total += len;
		/* Остальные поля процессируются в зависиости от присутсвия
		соответсвующего бита в маске */
		maxnumber = fdmc_bit_is_set(bitmap, 1) ? 128:64;
		for(field++; field->data && field->number <= maxnumber; field++)
		{
			if(fdmc_bit_is_set(bitmap, field->number))
			{
				if(fdmc_common_check_field(field, err) == 0)
				{
					err_trace("field checking failed");
					return 0;
				}
				if(debug_formats && (op == FDMC_FORMAT || op == FDMC_EXTRACT))
				{
					tprintf("|%04d|", total);
				}
				len = (*(field->field_func))(field, buffer+total, op, err);
				if(len == 0)
				{
					trace("error processing message");
					return 0;
				}
				total += len;
			}
		}
		if(op == FDMC_FORMAT && debug_formats)
		{
			trace("Message formatted as:");
			fdmc_dump(buffer, total);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
	return total;
}

