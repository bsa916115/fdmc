#include "fdmc_attrmsg.h"
#include "fdmc_bitop.h"
#include "fdmc_logfile.h"
#include "fdmc_option.h"

#include <string.h>

MODULE_NAME("fdmc_bsamsg.h");

 
static int data_to_hex(char *dest, void *data, int datalen)
{
	//FUNC_NAME("data_to_hex");
	byte *bdata = data;
	int i, len=0;

	for(i = 0; i < datalen; i++)
	{
		len += sprintf(dest+len, "%02X", bdata[i]);
	}
	return len;
}

static int hex_to_data(void *dest, char *source)
{
	//FUNC_NAME("hex_to_data");
	int i, len;
	byte *bdata = dest;
	unsigned newdata;
	char tmp[3];

	len = (int)strlen(source);
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

static int common_format_attr(FDMC_MSGFIELD *field, char *buffer, 
							 char *databuffer, int buflen, FDMC_EXCEPTION *err)
{
	//FUNC_NAME("common_format_attr");
	int len;
	
	len = sprintf(buffer, "%s=\"%.*s\" ", field->name, buflen, databuffer);
	if(debug_formats)
	{
		tprintf("|%s|%.*s|\n", field->name, buflen, databuffer);
	}
	return len;
}

static int common_get_attr(
							FDMC_MSGFIELD *field, 
							char *buffer /* From */, 
							char *databuffer, /* To */
							FDMC_EXCEPTION *err)
{
	//FUNC_NAME("common_get_attr");

	/* Get named attribute value */
	if(fdmc_getattrn(buffer, field->name, "''\"\"", databuffer, 0) == NULL)
	{
		return 0;
	}
	if(debug_formats)
	{
		tprintf("|%s|%s|", field->name, databuffer);
	}
	return 1;
}

int fdmc_int_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int fevent, FDMC_EXCEPTION *err)
{
	//FUNC_NAME("int_attrfield");
	int *data=field->data, len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(fevent)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, *data);
		len = common_format_attr(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_attr(field, buffer, databuffer, err);
		*data = atoi(databuffer);
		return len;
	default:
		len = fdmc_int_field(field, buffer, fevent, err);
		return len;
	}
//	return -1;
}

int fdmc_float_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int fevent,
				   FDMC_EXCEPTION *err)
{
	//FUNC_NAME("float_attrfield");
	double *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(fevent)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, *data);
		len = common_format_attr(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_attr(field, buffer, databuffer, err);
		*data = atof(databuffer);
		return len;
	default:
		len = fdmc_float_field(field, buffer, fevent, err);
		return len;
	}
//	return -1;
}


int fdmc_char_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int fevent, 
				  FDMC_EXCEPTION *err)
{
	//FUNC_NAME("char_attrfield");
	char *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	databuffer[0] = 0;
	switch(fevent)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data);
		len = common_format_attr(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_attr(field, buffer, databuffer, err);
		strcpy(data, databuffer);
		return len;
	default:
		len = fdmc_char_field(field, buffer, fevent, err);
		return len;
	}
//	return -1;
}

typedef struct
{
	_ORA_STRINGLENGTH len;
	char arr[1];

} vchar;

int fdmc_hostchar_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int fevent,
					  FDMC_EXCEPTION *err)
{
	//FUNC_NAME("hostchar_attrfield");
	vchar *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	databuffer[0] = 0;
	switch(fevent)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data->arr);
		len = common_format_attr(field, buffer, databuffer, len, err);
		data->len = len - field->lensize;
		return len;
	case FDMC_EXTRACT:
		len = common_get_attr(field, buffer, databuffer, err);
		strcpy(data->arr, databuffer);
		data->len =len - field->lensize;
		return len;
	default:
		len = fdmc_hostchar_field(field, buffer, fevent, err);
		return len;
	}
//	return -1;
}

int fdmc_attr_message(struct _FDMC_MSGFIELD *field, char *buffer, int fevent, 
				  FDMC_EXCEPTION *err)
{
	//FUNC_NAME("attr_message");
	int len, total=0;
	struct _FDMC_MSGFIELD *curr_field = field+1;

	field->data = buffer;
	switch(fevent)
	{
	case FDMC_FORMAT:
		while(curr_field->data != NULL)
		{
			if(fdmc_common_check_field(curr_field, err) == 0)
			{
				return 0;
			}
			if(debug_formats)
			{
				tprintf("|%04d|", total);
			}
			len = (*(curr_field->field_func))(curr_field, buffer+total, 
				FDMC_FORMAT, err);
			if(len == 0)
			{
				trace("Error while fomatting bsa message %s", field->name);
				return 0;
			}
			total += len;
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
			if(fdmc_common_check_field(curr_field, err) == 0)
			{
				return 0;
			}
			len = (*(curr_field->field_func))(curr_field, buffer+total, 
				FDMC_EXTRACT, err);
			if(debug_formats && len != -1)
			{
				tprintf("|%04d|\n", len);
			}
			/*
			if(len == 0)
			{
				(*(curr_field->field_func))(curr_field, databuffer+total, 
				FDMC_INIT, err);
			}
			*/
			curr_field++;
		}
		return 1;
	case FDMC_INIT:
	default:
		for(;curr_field->data != NULL; curr_field ++)
		{
			if(fdmc_common_check_field(field, err))
			{
				(*(curr_field->field_func))(curr_field, buffer, fevent, err);
			}
		}
		break;
	}
	return -1;
}

int fdmc_binary_attrfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, 
				  FDMC_EXCEPTION *err)
{
	//FUNC_NAME("binary_attrfield");
	int *data=field->data, len;
	char databuffer[MAX_FIELD_SIZE+1];
	switch(op)
	{
	case FDMC_FORMAT:
		len = data_to_hex(databuffer, data, field->size);
		len = common_format_attr(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_attr(field, buffer, databuffer, err);
		hex_to_data(data, databuffer);
		return len;
	case FDMC_PRINT:
		data_to_hex(databuffer, data, field->size);
		tprintf("%s:\t%s\n", field->name, databuffer);
	default:
		return -1;
	}
//	return -1;
}

