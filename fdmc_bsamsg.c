#include "fdmc_bsamsg.h"
#include "fdmc_bitop.h"
#include "fdmc_logfile.h"
#include "fdmc_bitop.h"
#include <string.h>

MODULE_NAME("fdmc_bsamsg.h");

int data_to_hex(char *dest, void *data, int datalen)
{
//	FUNC_NAME("data_to_hex");
	byte *bdata = data;
	int i, len=0;

	for(i = 0; i < datalen; i++)
	{
		len += sprintf(dest+len, "%02X", bdata[i]);
	}
	return len;
}

int hex_to_data(void *dest, char *source, int len)
{
//	FUNC_NAME("hex_to_data");
	int i;
	byte *bdata = dest;
	unsigned newdata;
	char tmp[3];

	for(i = 0; i < len; i+=2)
	{
		strncpy(tmp, source+i, 2);
		tmp[2] = 0;
		sscanf(tmp, "%X", &newdata);
		*bdata = (byte)newdata;
		bdata++;
	}
	return len;
}

static int common_format_bsa(FDMC_MSGFIELD *field, char *buffer, 
							 char *databuffer, int buflen, FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_bsa_format");
	int len;
	FDMC_EXCEPTION x;
	TRYF(x)
	{
		if(field == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "NULL field");
		}
		if(field->name == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "NULL field name");
		}
		len = sprintf(buffer, "\n%s\t%.*s", field->name, buflen, databuffer);
		if(debug_formats)
		{
			tprintf("|%s|%.*s|\n", field->name, buflen, databuffer);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return len;
}

static int common_get_bsa(FDMC_MSGFIELD *field, char *buffer, 
							 char *databuffer, FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_get_bsa");
	char name[257];
	char *pos;
	int len, dtpos;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(field == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "NULL field");
		}
		if(field->name == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "NULL field name");
		}
		len = sprintf(name, "\n%s\t", field->name);
		pos = strstr(buffer, name);
		dtpos = (int)(pos - buffer);
		if(pos == NULL)
		{
			return -1;
		}
		fdmc_set_bit(field->flags, FDMC_PRESENTS);
		pos += len;
		for(len = 0; *pos != 0 && *pos != '\n'; pos ++, len++)
		{
			databuffer[len] = *pos;
		}
		databuffer[len] = 0;
		if(debug_formats)
		{
			tprintf("|%s|%.*s|", field->name, len, databuffer);
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return dtpos;
}

int fdmc_int_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err)
{
	FUNC_NAME("int_bsafield");
	int *data=field->data, len;
	char databuffer[MAX_FIELD_SIZE + 1];

	FDMC_EXCEPTION x;

	TRYF(x)
	{
		switch(op)
		{
		case FDMC_FORMAT:
			len = sprintf(databuffer, field->format, *data);
			len = common_format_bsa(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_bsa(field, buffer, databuffer, &x);
			*data = atoi(databuffer);
			return len;
		default:
			len = fdmc_int_field(field, buffer, op, &x);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
}

int fdmc_float_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				   FDMC_EXCEPTION *err)
{
	FUNC_NAME("float_bsafield");
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
			len = common_format_bsa(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_bsa(field, buffer, databuffer, &x);
			*data = atof(databuffer);
			return len;
		default:
			len = fdmc_float_field(field, buffer, op, &x);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
}


int fdmc_char_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				  FDMC_EXCEPTION *err)
{
	FUNC_NAME("char_bsafield");
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
			len = common_format_bsa(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_bsa(field, buffer, databuffer, &x);
			strcpy(data, databuffer);
			return len;
		default:
			len = fdmc_char_field(field, buffer, op, &x);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
//	return -1;
}

typedef struct
{
	_ORA_STRINGLENGTH len;
	char arr[1];

} vchar;

int fdmc_hostchar_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
					  FDMC_EXCEPTION *err)
{
	FUNC_NAME("hostchar_bsafield");
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
			len = common_format_bsa(field, buffer, databuffer, len, &x);
			data->len = len - field->lensize;
			return len;
		case FDMC_EXTRACT:
			len = common_get_bsa(field, buffer, databuffer, &x);
			strcpy(data->arr, databuffer);
			data->len =len - field->lensize;
			return len;
		default:
			len = fdmc_hostchar_field(field, buffer, op, &x);
			return len;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
//	return -1;
}

int fdmc_bsa_message(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				  FDMC_EXCEPTION *err)
{
	FUNC_NAME("bsa_message");
	int len, total=0;
	struct _FDMC_MSGFIELD *curr_field = field+1;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		field->data = buffer;
		switch(op)
		{
		case FDMC_FORMAT:
			while(curr_field->data != NULL)
			{
				if(fdmc_bit_is_set(curr_field->flags, FDMC_PRESENTS))
				{
					if(fdmc_common_check_field(curr_field, &x) == 0)
					{
						return 0;
					}
					if(debug_formats)
					{
						tprintf("|%04d|", total);
					}
					len = (*(curr_field->field_func))(curr_field, buffer+total,
						FDMC_FORMAT, &x);
					if(len == 0)
					{
						trace("Error while fomatting bsa message %s", field->name);
						return 0;
					}
					total += len;
				}
				curr_field ++;
			}
			if(debug_formats)
			{
				fdmc_dump(buffer, total);
			}
			return total;
		case FDMC_EXTRACT:
			if(debug_formats)
			{
				fdmc_dump(buffer, field->size);
			}
			while(curr_field->data != NULL)
			{
				if(fdmc_common_check_field(curr_field, &x) == 0)
				{
					return 0;
				}
				len = (*(curr_field->field_func))(curr_field, buffer+total,
					FDMC_EXTRACT, &x);
				if(debug_formats && len != -1)
				{
					tprintf("|%04d|\n", len);
				}
				curr_field++;
			}
			return 1;
		case FDMC_INIT:
		default:
			for(;curr_field->data != NULL; curr_field ++)
			{
				if(fdmc_common_check_field(field, &x) == 0)
				{
					func_trace("invalid field");
				}
				(*(curr_field->field_func))(curr_field, buffer, op, &x);
			}
			break;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
	return -1;
}

int fdmc_binary_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				  FDMC_EXCEPTION *err)
{
	FUNC_NAME("binary_bsafield");
	FDMC_EXCEPTION x;
	int *data=field->data, len;
	char databuffer[MAX_FIELD_SIZE+1];

	TRYF(x)
	{
		switch(op)
		{
		case FDMC_FORMAT:
			len = data_to_hex(databuffer, data, field->size);
			len = common_format_bsa(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_bsa(field, buffer, databuffer, &x);
			hex_to_data(data, databuffer, len);
			return len;
		case FDMC_PRINT:
			data_to_hex(databuffer, data, field->size);
			tprintf("%s:\t%s\n", field->name, databuffer);
		default:
			return -1;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
//	return -1;
}

int fdmc_bitmap_bsafield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
				  FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_bitmap_bsafield");
	FDMC_EXCEPTION x;
	int len;

	TRYF(x)
	{
		char databuffer[MAX_FIELD_SIZE+1];
		switch(op)
		{
		case FDMC_FORMAT:
			if(fdmc_bit_is_set(field->data, 1))
				len = 16;
			else
				len = 8;
			len = data_to_hex(databuffer, field->data, len);
			len = common_format_bsa(field, buffer, databuffer, len, &x);
			return len;
		case FDMC_EXTRACT:
			len = common_get_bsa(field, buffer, databuffer, &x);
			hex_to_data(field->data, databuffer, len);
			return len;
		case FDMC_PRINT:
			if(fdmc_bit_is_set(field->data, 1))
				len = 16;
			else
				len = 8;
			data_to_hex(databuffer, field->data, len);
			tprintf("%s:\t%s\n", field->name, databuffer);
			break;
		default:
			return -1;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return -1;
	}
	return -1;
}

int fdmc_data_by_bsaname(char *buffer, char *name, char *data, int datasize)
{
//	FUNC_NAME("data_by_bsaname");
	char *pos;
	int len;
	char id[256];
	int i = 0;
	
	i = sprintf(id, "\n%s\t", name);
	pos = strstr(buffer, id);
	if(pos == NULL)
	{
		return -1;
	}
	len = (int)(pos - buffer);
	pos += i;
	if(data == NULL)
	{
		return len;
	}
	for(i = 0; *pos != '\n' && pos != 0 && i < datasize; pos ++, data++, i++)
	{
		*data = *pos;
	}
	return len;
}
