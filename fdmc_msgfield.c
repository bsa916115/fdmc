#include "fdmc_global.h"
#include "fdmc_msgfield.h"
#include "fdmc_bitop.h"
#include "fdmc_option.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"

#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>

MODULE_NAME("fdmc_msgfields.c");

int debug_formats;
static int otstup;
static char private_value[] = "-- Confidentional data --";

int fdmc_parse_format(struct _FDMC_MSGFIELD *field)
{
//	FUNC_NAME("fdmc_parse_format");
	char *pposr, *pposl;

	if(field->format == NULL)
	{
		return 0;
	}
	// Field format specifiers
	pposr = strrchr(field->format, '%');
	pposl = strchr(field->format, '%');
	field->format = pposr;
	if(pposl == NULL)
	{
		return 0;
	}
	if(pposl != pposr)
	{
		// Field with variable length
		sscanf(pposl+1, "%d", &field->lensize);
	}
	pposr++;
	// Field size
	field->size = 0;
	sscanf(pposr, "%d", &field->size);
	if(field->size < 0)
	{
		field->align = FDMC_ALIGN_LEFT;
		field->size *= -1;
	}
	// Alignment of field
	switch(*pposr)
	{
	case '0':
		field->align = FDMC_ALIGN_ZERO;
		break;
	case '+':
		field->align = FDMC_ALIGN_SIGN;
		break;
	default:
		field->align = FDMC_ALIGN_NONE;
		break;
	}
	// Field scale if presents
	pposl = strchr(pposr, '.');
	if(pposl != NULL)
	{
		sscanf(pposl+1, "%d", &field->scale);
	}
	return 1;
}

int fdmc_parse_initstring(struct _FDMC_MSGFIELD *field)
{
//	FUNC_NAME("fdmc_parse_initsring");
	char *params = field->initstring;
	char *paropt;

	paropt = fdmc_getattr(params, "name", NULL);
	if(paropt != NULL)
	{
		field->name = paropt;
	}

	paropt = fdmc_getattr(params, "format", "[]  ");
	if(paropt != NULL)
	{
		field->format = paropt;
		if(fdmc_parse_format(field) == 0)
		{
			trace("Invalid format specifier in initialization string");
			return 0;
		}
	}

	paropt = fdmc_getattr(params, "number", NULL);
	if(paropt != NULL)
	{
		sscanf(paropt, "%d", &field->number);
	}
	fdmc_free(paropt, NULL);

	paropt = fdmc_getattr(params, "size", NULL);
	if(paropt != NULL)
	{
		field->size = atoi(paropt);
	}
	fdmc_free(paropt, NULL);

	paropt = fdmc_getattr(params, "scale", NULL);
	if(paropt != NULL)
	{
		field->scale = atoi(paropt);
	}
	fdmc_free(paropt, NULL);

	paropt = fdmc_getattr(params, "len", NULL);
	if(paropt != NULL)
	{
		field->lensize = atoi(paropt);
	}
	fdmc_free(paropt, NULL);

	paropt = fdmc_getattr(params, "maxsize", NULL);
	if(paropt)
	{
		field->maxlen = atoi(paropt);
		free(paropt);
	}

	paropt = fdmc_getattr(params, "align", NULL);
	if(paropt != NULL)
	{
		if(strcmp(paropt, "left") == 0)
		{
			field->align = FDMC_ALIGN_LEFT;
		}
		else if(strcmp(paropt, "right") == 0)
		{
			field->align = FDMC_ALIGN_RIGHT;
		}
		else if(strcmp(paropt, "zero") == 0)
		{
			field->align = FDMC_ALIGN_ZERO;
		}
		else if(strcmp(paropt, "sign") == 0)
		{
			field->align = FDMC_ALIGN_SIGN;
		}
		else if(strcmp(paropt, "none") == 0)
		{
			field->align = FDMC_ALIGN_NONE;
		}
		else if(strcmp(paropt, "default") == 0)
		{
			field->align = FDMC_ALIGN_DEFAULT;
		}
		else
		{
			field->align = FDMC_ALIGN_DEFAULT;
		}
	}
	fdmc_free(paropt, NULL);

	paropt = fdmc_getattr(params, "private", NULL);
	if(paropt != NULL)
	{
		if(isyes(paropt))
		{
			fdmc_set_bit(field->flags, FDMC_PRIVATE);
		}
		free(paropt);
	}
	paropt = fdmc_getattr(params, "noinit", NULL);
	if(paropt != NULL)
	{
		if(isyes(paropt))
		{
			fdmc_set_bit(field->flags, FDMC_NOINIT);
		}
		free(paropt);
	}

	return 1;
}

int fdmc_make_format_string(FDMC_MSGFIELD *field)
{
//	FUNC_NAME("fdmc_make_format_string");
	int n=0;

	field->format = malloc(16);
	if(field->format == NULL)
	{
		return 0;
	}
	if(field->lensize != 0)
	{
		n = sprintf(field->format, "%%0%dd", field->lensize);
	}
	n += sprintf(field->format + n, "%%");
	switch(field->align)
	{
	case FDMC_ALIGN_ZERO:
		n += sprintf(field->format+n, "0");
		break;
	case FDMC_ALIGN_SIGN:
		n += sprintf(field->format+n, "+0");
		break;
	case FDMC_ALIGN_LEFT:
		n += sprintf(field->format+n, "-");
		break;
	}
	if(field->align != FDMC_ALIGN_NONE)
	{
		n += sprintf(field->format+n, "%d", field->size);
	}
	return 1;
}

int fdmc_common_check_field(FDMC_MSGFIELD *field, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_common_check_field");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(field->name == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "NULL field name");
		}
		if(field->data == NULL)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "NULL field descriptor in field %s",
				field->name);
		}
		if(field->align == 0)
		{
			if(fdmc_parse_format(field) == 0)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Cannot parse format string on field %s", field->name);
			}
		}
		if(field->lensize > 4)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x,
				"Invalid length format %d, max is 4", field->lensize);
		}
		if(field->size > MAX_FIELD_SIZE)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x,
				"Invalid field size %d max is %d", field->size, MAX_FIELD_SIZE);
		}
		if(field->field_func == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x,
				"null address of field function in field %s", field->name);
		}
		if(field->initstring != NULL &&	!fdmc_bit_is_set(field->flags, FDMC_INITIALIZED))
		{
			fdmc_parse_initstring(field);
			fdmc_set_bit(field->flags, FDMC_INITIALIZED);
		}
		if(field->format == NULL)
		{
			if(fdmc_make_format_string(field) == 0)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x, "cannot make format string");
			}
			(*(field->field_func))(field, NULL, FDMC_CALCULATE, &x);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

int common_field_format(FDMC_MSGFIELD *field, char *buffer,
							   char *databuffer, int buflen, FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_field_format");
	int len=0;
	int dtlen;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		len = buflen;
		if(field->maxlen && len > field->maxlen)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Too big field length %d. Max length is %d",
				len, field->maxlen);
		}
		if(field->align == FDMC_ALIGN_SIGN)
		{
			databuffer[0] = databuffer[0] == '+' ? 'C':'D';
		}
		if(field->lensize != 0)
		{
			dtlen = sprintf(buffer, "%0*d%s", field->lensize, len, databuffer);
			if(debug_formats)
			{
				trace("(%03d)[%s]\t|%.*s|", field->number, field->name, dtlen, buffer);
			}
			return dtlen;
		}
		else
		{
			strcpy(buffer, databuffer);
			if(debug_formats)
			{
				trace("(%03d)[%s]\t|%.*s|", field->number, field->name, len, buffer);
			}
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
//	return len;
}

int common_get_buffer(FDMC_MSGFIELD *field, char *buffer,
							 char *databuffer, FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_get_buffer");
	int len;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(field->lensize != 0)
		{
			strncpy(databuffer, buffer, field->lensize);
			databuffer[field->lensize] = 0;
			len = atoi(databuffer);
			if(len <= 0)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x,
					"Invalid field length in field '%s'", field->name);
				return 0;
			}
			if(field->maxlen < len)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Too big field length in message %d. Max is %d",
					len, field->maxlen);
			}
			strncpy(databuffer, buffer+field->lensize, len);
			if(debug_formats)
			{
				trace("(%03d)[%s]\t|%.*s|", field->number, field->name, len+field->lensize,
					buffer);
			}
			databuffer[len] = 0;
			len+=field->lensize;
		}
		else
		{
			if(field->size != 0)
			{
				strncpy(databuffer, buffer, field->size);
				databuffer[field->size] = 0;
			}
			else
			{
				strcpy(databuffer, buffer);
				field->size = (int)strlen(databuffer);
			}
			if(debug_formats)
			{
				trace("(%03d)[%s]\t|%.*s|", field->number, field->name, field->size,
					databuffer);
			}
			len = field->size;
		}
		if(field->align == FDMC_ALIGN_SIGN)
		{
			databuffer[0] = databuffer[0] == 'C' ? '+':'-';
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return len;
}

int fdmc_int_field(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				   FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_int_field");
	int *data=(int*)field->data, len;
	char databuffer[MAX_FIELD_SIZE + 1];
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		switch(op)
		{
		case FDMC_FORMAT:
			len = sprintf(databuffer, field->format, *data);
			len = common_field_format(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_buffer(field, buffer, databuffer, &x);
			*data = atoi(databuffer);
			return len;
		case FDMC_INIT:
			if(!fdmc_bit_is_set(field->flags, FDMC_NOINIT))
			{
				*data = 0;
			}
			fdmc_clear_bit(field->flags, FDMC_PRESENTS);
			break;
		case FDMC_PRINT:
			tprintf("%s:\t", field->name);
			if(!fdmc_bit_is_set(field->flags, FDMC_PRIVATE))
				trace(field->format, *data);
			else
				trace("%s", private_value);
			break;
		case FDMC_CALCULATE:
			strcat(field->format, "d");
			break;
		case FDMC_XMLEXPORT:
			len = sprintf(buffer, "<%s>%d</%s>", field->name, *data, field->name);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return -1;
}

int fdmc_float_field(struct _FDMC_MSGFIELD *field, char *buffer, int op,
					 FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_float_field");
	double *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		switch(op)
		{
		case FDMC_FORMAT:
			len = sprintf(databuffer, field->format, *data);
			len = common_field_format(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_buffer(field, buffer, databuffer, &x);
			*data = atof(databuffer);
			return len;
		case FDMC_INIT:
			if(!fdmc_bit_is_set(field->flags, FDMC_NOINIT))
			{
				*data = 0.0;
			}
			fdmc_clear_bit(field->flags, FDMC_PRESENTS);
			break;
		case FDMC_PRINT:
			tprintf("%s:\t", field->name);
			if(!fdmc_bit_is_set(field->flags, FDMC_PRIVATE))
				trace(field->format, *data);
			else
				trace("%s", private_value);
			break;
		case FDMC_CALCULATE:
			strcat(field->format, "lf");
			break;
		case FDMC_XMLEXPORT:
			len = sprintf(buffer, "<%s>%.*lf</%s>", field->name, field->scale,
				*data, field->name);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return -1;
}

int fdmc_char_field(struct _FDMC_MSGFIELD *field, char *buffer, int op,
					FDMC_EXCEPTION* err)
{
	FUNC_NAME("fdmc_char_field");
	char *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];
	FDMC_EXCEPTION x;

	TRYF(x)
	{
	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data);
		len = common_field_format(field, buffer, databuffer, len, &x);
		return len;
	case FDMC_EXTRACT:
		len = common_get_buffer(field, buffer, databuffer, &x);
		strcpy(data, databuffer);
		return len;
	case FDMC_INIT:
		if(!fdmc_bit_is_set(field->flags, FDMC_NOINIT))
		{
			*data = '\0';
		}
		fdmc_clear_bit(field->flags, FDMC_PRESENTS);
		break;
	case FDMC_PRINT:
		tprintf("%s:\t", field->name);
		if(!fdmc_bit_is_set(field->flags, FDMC_PRIVATE))
			trace(field->format, data);
		else
			trace("%s", private_value);
		break;
	case FDMC_CALCULATE:
		strcat(field->format, "s");
		break;
	case FDMC_XMLEXPORT:
		len = sprintf(buffer, "<%s>%s</%s>", field->name, data, field->name);
		return len;
	}
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return -1;
}

typedef struct
{
	_ORA_STRINGLENGTH len;
	char arr[1];

} vchar;

int fdmc_hostchar_field(struct _FDMC_MSGFIELD *field, char *buffer, int op,
						FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_hostchar_field");
	vchar *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		switch(op)
		{
		case FDMC_FORMAT:
			len = sprintf(databuffer, field->format, data->arr);
			len = common_field_format(field, buffer, databuffer, len, &x);
			data->len = len - field->lensize;
			return len;
		case FDMC_EXTRACT:
			len = common_get_buffer(field, buffer, databuffer, &x);
			strcpy(data->arr, databuffer);
			data->len =len - field->lensize;
			return len;
		case FDMC_INIT:
			if(!fdmc_bit_is_set(field->flags, FDMC_NOINIT))
			{
				data->arr[0] = 0;
				data->len = 0;
			}
			fdmc_clear_bit(field->flags, FDMC_PRESENTS);
			break;
		case FDMC_PRINT:
			tprintf("%s:\t", field->name);
			if(!fdmc_bit_is_set(field->flags, FDMC_PRIVATE))
				trace(field->format, data->arr);
			else 
				trace("%s", private_value);
			break;
		case FDMC_CALCULATE:
			strcat(field->format, "s");
			break;
		case FDMC_XMLEXPORT:
			len = sprintf(buffer, "<%s>%s</%s>", field->name, data->arr, field->name);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return -1;
}

int fdmc_message_field(FDMC_MSGFIELD *field, char *buffer, int op,
					   FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_message_field");
	int len;
	FDMC_MSGFIELD *submessage=field->data;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
	if(submessage->field_func == NULL)
	{
		fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Invalid message function");
		return 0;
	}
	len = (*(submessage->field_func))(field, buffer, op, &x);
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
	}
	return len;
}

int fdmc_plain_message(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
					   FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_plain_message");
	int len, total=0, dtlen;
	struct _FDMC_MSGFIELD *curr_field = field+1;
	char databuffer[MAX_FIELD_SIZE+1];
	FDMC_EXCEPTION x;

	TRYF(x)
	{
	field->data = buffer;
	switch(op)
	{
	case FDMC_FORMAT:
		while(curr_field->data != NULL)
		{
			if(fdmc_common_check_field(curr_field, &x) == 0)
			{
				return 0;
			}
			if(debug_formats)
			{
				tprintf("|%04d|", total);
			}
			len = (*(curr_field->field_func))(curr_field, databuffer+total, 
				FDMC_FORMAT, &x);
			if(len == 0)
			{
				trace("Error while fomatting plain message %s", field->name);
				return 0;
			}
			total += len;
			curr_field ++;
		}
		databuffer[total] = 0;
		len = common_field_format(field, buffer, databuffer, total, &x);
		return len;
	case FDMC_EXTRACT:
		dtlen = common_get_buffer(field, buffer, databuffer, &x);
		if(dtlen == 0)
		{
			trace("Cannot get data from buffer");
			return 0;
		}
		while(curr_field->data != NULL)
		{
			if(fdmc_common_check_field(curr_field, &x) == 0)
			{
				return 0;
			}
			if(curr_field->field_func == NULL)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x,
					"Null address of field function %s", curr_field->name);
			}
			if(debug_formats)
			{
				tprintf("|%04d|", total);
			}
			len = (*(curr_field->field_func))(curr_field, databuffer+total, 
				FDMC_EXTRACT, &x);
			if(len == 0)
			{
				trace("Error while extrating plain message");
				return 0;
			}
			total += len;
			curr_field++;
		}
		return dtlen;
	case FDMC_INIT:
		len = 0;
		for(;curr_field->data != NULL; curr_field ++)
		{
			if(curr_field->field_func == NULL)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x,
					"Null address of field function %s", curr_field->name);
			}
			len += (*(curr_field->field_func))(curr_field, buffer+len, op, &x);
		}
		return len;
	default:
		for(;curr_field->data != NULL; curr_field ++)
		{
			if(curr_field->field_func == NULL)
			{
				fdmc_raisef(FDMC_FORMAT_ERROR, &x, 
					"Null address of field function %s", curr_field->name);
			}
			(*(curr_field->field_func))(curr_field, buffer, op, &x);
		}
		break;
	}
	}
	return -1;
}

int isyes(char *c)
{
	switch(*c)
	{
	case 'Y':
	case 'y':
	case '1':
		return 1;
	}
	return 0;
}

int fdmc_process_message(FDMC_MSGFIELD *msg, char *buffer, int op, 
						 FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_process_message");

	if(msg->field_func == NULL)
	{
		fdmc_raisef(FDMC_FORMAT_ERROR, err, "%s:Invalid message function", _function_id);
		return 0;
	}
	return (*(msg->field_func))(msg, buffer, op, err);
}

FDMC_MSGFIELD *fdmc_field_by_name(FDMC_MSGFIELD *msg, char *name)
{
//	FUNC_NAME("fdmc_field_by_name");

	if(!msg || !name)
	{
		return NULL;
	}
	while(msg->data != NULL)
	{
		if(strcmp(msg->name, name) == 0)
		{
			return msg;
		}
		msg++;
	}
	return NULL;
}

FDMC_MSGFIELD *fdmc_field_by_number(FDMC_MSGFIELD *msg, int number)
{
//	FUNC_NAME("fdmc_field_by_name");

	while(msg->data != NULL)
	{
		if(msg->number == number)
		{
			return msg;
		}
		msg++;
	}
	return NULL;
}

FDMC_MSGFIELD* fdmc_clone_message(FDMC_MSGFIELD *pattern, void *host_addr)
{
	FUNC_NAME("fdmc_clone_message");

	FDMC_MSGFIELD *new_message;
	FDMC_EXCEPTION ex;
	int i, asize, shift;

	// Calculate number of elements
	i = 0;
	while(pattern[i].data)
	{
		i++;
	}
	// Size of message
	asize = sizeof(FDMC_MSGFIELD) * (i + 1);
	TRYF(ex)
	{
		// Create new message
		new_message = (FDMC_MSGFIELD*)fdmc_malloc(asize, &ex);
		// Copy message descriptors
		memcpy(new_message, pattern, asize);
		new_message[0].data = host_addr;
		for(i = 0; pattern[i].data; i++)
		{
			shift = (int)((char*)pattern[i].data - (char*)pattern[0].data);
			new_message[i].data = (char*)new_message[0].data + shift;
		}
	}
	EXCEPTION
	{
		fdmc_free(new_message, NULL);
		new_message = NULL;
	}
	return new_message;
}
