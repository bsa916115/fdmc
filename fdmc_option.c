#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>
#include <stdio.h>

#include "fdmc_option.h"
#include "fdmc_logfile.h"
#include "fdmc_global.h"

MODULE_NAME("fdmc_option.c");

char* fdmc_getopt(int argc, char *argv[], char *key, int *argn)
{
	FUNC_NAME("fdmc_getopt");
	int i;

	dbg_trace();
	func_trace("Searching for parameter %s", key);
	if (argn == NULL)
	{
		err_trace("NULL pointer in parameter argn");
		return NULL;
	}
	for(i = *argn; i < argc; i++) 
	{
		if(!strcmp(argv[i], key))
			break;
	}
	if(i >= argc) 
	{
		*argn = 0;
		func_trace("Parameter was not found in arguments array");
		return NULL;
	}
	if(i==argc-1 || argv[i+1][0] == '-')
	{
		func_trace("Parameter was found without value");
		*argn = i;
		return NULL;
	}
	else if(argv[i+1][0] != '-')
	{
		*argn = i;
		func_trace("Parameter was found with value");
		return argv[i+1];
	}
	return NULL;
}

static char get_quote(char ch, char *quotes)
{
	size_t i, len;

	if(quotes == NULL)
	{
		return ' ';
	}

	len = strlen(quotes);
	for(i = 0; i < len; i += 2)
	{
		if(ch == quotes[i])
		{
			ch = quotes[i+1];
			break;
		}
	}
	if(i >= len || ch == '\0')
	{
		ch = ' ';
	}
	return ch;
}

char *fdmc_getattr(char *attrstr, char *key, char *quotes)
{
	FUNC_NAME("fdmc_getattr");
	char *res;
	char *p, quot, *pq;
	char parbuf[MAXPARLEN+1];
	int len;

	len = sprintf(parbuf, "%.*s=", MAXPARLEN-1, key);
	p = strstr(attrstr, parbuf);
	if(p == NULL)
	{
		return NULL;
	}
	if(p != attrstr)
	{
		while(p != NULL)
		{
			if(*(p-1) == ' ')
			{
				break;
			}
			p += len;
			p = strstr(p, parbuf);
		}
	}
	if(p == NULL)
	{
		return NULL;
	}
	p += len;

	quot = get_quote(*p, quotes);
	if(quot != ' ')
	{
		p ++;
	}
	pq = strchr(p, quot);
	if(pq == NULL)
	{
		if(quot == ' ')
		{
			len = (int)strlen(p);
		}
		else
		{
			func_trace("Expected %c in input string to match %c in position %d",
				quot, p[-1], (int)(p - attrstr));
			return NULL;
		}
	}
	else
	{
		len = (int)(pq-p);
	}
	res = malloc(len + 1);
	if(res == NULL)
	{
		err_trace("Cannot allocate %d bytes", len + 1);
		trace(strerror(errno));
		return NULL;
	}
	strncpy(res, p, len);
	res[len] = 0;
	return res;
}

char *fdmc_getattrto(char *attrstr, char *key, char *quotes, char *dest)
{
	FUNC_NAME("fdmc_getattrto");
	char *res = dest;
	char *p, quot, *pq;
	char parbuf[MAXPARLEN+1];
	int len;


	dbg_trace();
	if(dest == NULL)
	{
		return fdmc_getattr(attrstr, key, quotes);
	}
	len = sprintf(parbuf, "%.*s=", MAXPARLEN-1, key);
	p = strstr(attrstr, parbuf);
	if(p == NULL)
	{
		return NULL;
	}
	if(p != attrstr)
	{
		while(p != NULL)
		{
			if(*(p-1) == ' ')
			{
				break;
			}
			p += len;
			p = strstr(p, parbuf);
		}
	}
	if(p == NULL)
	{
		return NULL;
	}
	p += len;
	quot = get_quote(*p, quotes);
	if(quot != ' ')
	{
		p ++;
	}
	pq = strchr(p, quot);
	if(pq == NULL)
	{
		if(quot == ' ')
		{
			len = (int)strlen(p);
		}
		else
		{
			func_trace("Expected %c in input string to match %c in position %d",
				quot, p[-1], (int)(p - attrstr));
			return NULL;
		}
	}
	else
	{
		len = (int)(pq-p);
	}
	if(len > MAXPARLEN)
	{
		len = MAXPARLEN;
	}
	strncpy(res, p, len);
	res[len] = 0;
	return res;
}

char *fdmc_getattrn(char *attrstr, char *key, char *quotes, char *dest, int n)
{
	FUNC_NAME("fdmc_getattrn");
	char *res = dest;
	char *p, quot, *pq;
	char parbuf[MAXPARLEN+1];
	int len;


	dbg_trace();
	trace("getting attribute %s", key);
	if(dest == NULL)
	{
		return fdmc_getattr(attrstr, key, quotes);
	}
	len = sprintf(parbuf, "%.*s=", MAXPARLEN-1, key);
	p = strstr(attrstr, parbuf);
	if(p == NULL)
	{
		return NULL;
	}
	if(p != attrstr)
	{
		while(p != NULL)
		{
			if(*(p-1) == ' ')
			{
				break;
			}
			p += len;
			p = strstr(p, parbuf);
		}
	}
	if(p == NULL)
	{
		return NULL;
	}
	p += len;
	quot = get_quote(*p, quotes);
	if(quot != ' ')
	{
		p ++;
	}
	pq = strchr(p, quot);
	if(pq == NULL)
	{
		if(quot == ' ')
		{
			len = (int)strlen(p);
		}
		else
		{
			func_trace("Expected %c in input string to match %c in position %d",
				quot, p[-1], (int)(p - attrstr));
			return NULL;
		}
	}
	else
	{
		len = (int)(pq-p);
	}
	if(len > MAXPARLEN)
	{
		len = MAXPARLEN;
	}
	if(n > 0 && len > n)
	{
		len = n;
	}
	strncpy(res, p, len);
	res[len] = 0;
	return res;
}

