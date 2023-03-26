#include "fdmc_global.h"
#include "fdmc_trlmsg.h"
#include "fdmc_logfile.h"
#include "fdmc_exception.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

MODULE_NAME("fdmc_trlmsg.c");

static int common_format_trl(FDMC_MSGFIELD *field, 
							 char *buffer, 
							 char *databuffer,
							 int buflen, 
							 FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_format_trl");
	int len;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(field->initstring == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "'%s': invalid initialization string", field->name);
		}
		len = sprintf(buffer, "%s%c", databuffer, field->initstring[0]);
		if(debug_formats)
		{
			trace("(%03d)[%s]\t|%.*s|", field->number, field->name, buflen, databuffer);
		}
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return len;
}

static int common_get_trl(FDMC_MSGFIELD *field, 
						  char *buffer, 
						  char *databuffer,
						  FDMC_EXCEPTION *err)
{
	FUNC_NAME("common_get_trl");
	int len; 
	char *trlpos;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		if(field->initstring == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "'%s': invalid initialization string", field->name);
		}
		trlpos = strchr(buffer, field->initstring[0]);
		if(trlpos == NULL)
		{
			fdmc_raisef(FDMC_FORMAT_ERROR, &x, "invalid field layout '%s'", field->name);
		}
		len = (int)(trlpos - buffer);
		trlpos[0] = 0;
		strcpy(databuffer, buffer);
		if(debug_formats)
		{
			trace("(%03d)[%s]\t|%s|", field->number, field->name, databuffer);
		}
		trlpos[0] = field->initstring[0];
		len ++;
	}
	EXCEPTION
	{
		fdmc_raisef(x.errorcode, err, "%s", x.errortext);
		return 0;
	}
	return len;
}

int fdmc_int_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op, FDMC_EXCEPTION *err)
{
//	FUNC_NAME("int_trlfield");
	int *data=field->data, len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, *data);
		len = common_format_trl(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_trl(field, buffer, databuffer, err);
		*data = atoi(databuffer);
		return len;
	default:
		len = fdmc_int_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}

int fdmc_float_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
						FDMC_EXCEPTION *err)
{
//	FUNC_NAME("float_trlfield");
	double *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, *data);
		len = common_format_trl(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_trl(field, buffer, databuffer, err);
		*data = atof(databuffer);
		return len;
	default:
		len = fdmc_float_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}


int fdmc_char_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
					   FDMC_EXCEPTION *err)
{
//	FUNC_NAME("char_trlfield");
	char *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data);
		len = common_format_trl(field, buffer, databuffer, len, err);
		return len;
	case FDMC_EXTRACT:
		len = common_get_trl(field, buffer, databuffer, err);
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

int fdmc_hostchar_trlfield(struct _FDMC_MSGFIELD *field, char *buffer, int op,
						   FDMC_EXCEPTION *err)
{
//	FUNC_NAME("hostchar_trlfield");
	vchar *data=field->data;
	int len;
	char databuffer[MAX_FIELD_SIZE + 1];

	switch(op)
	{
	case FDMC_FORMAT:
		len = sprintf(databuffer, field->format, data->arr);
		len = common_format_trl(field, buffer, databuffer, len, err);
		data->len = len - field->lensize;
		return len;
	case FDMC_EXTRACT:
		len = common_get_trl(field, buffer, databuffer, err);
		strcpy(data->arr, databuffer);
		data->len =len - field->lensize;
		return len;
	default:
		len = fdmc_hostchar_field(field, buffer, op, err);
		return len;
	}
//	return -1;
}

