// Test programme for fdmc libraries
#include <stdio.h>
#include <errno.h>

#include "fdmc_global.h"
#include "fdmc_list.h"
#include "fdmc_logfile.h"
#include "fdmc_exception.h"
#include "fdmc_option.h"
#include "fdmc_oci.h"
#include "fdmc_ip.h"
#include "fdmc_thread.h"
#include "fdmc_attrmsg.h"
#include "fdmc_device.h"
#include "fdmc_des.h"

static char *column_sql = 
"select lower(column_name), data_type, nvl(data_length, 0), "
" nvl(data_precision, 0), nvl(data_scale, 0) "
" from all_tab_columns where table_name = upper(:1) order by column_id";

char config_buf[1024];
int config_timeout;
char config_address[256];
int config_port;
int config_headlen;
char config_name[256];
int config_queue_sec;
int config_queue_usec;
int config_flag_sec;
int config_flag_usec;

FDMC_PROCESS *mainproc;

FDMC_MSGFIELD config_msg[] = 
{
	{"MSG", fdmc_attr_message, config_buf, "%s", 0},
	{"timeout", fdmc_int_attrfield, &config_timeout, "%d", 0},
	{"address", fdmc_char_attrfield, config_address, "%s", 0},
	{"port", fdmc_int_attrfield, &config_port, "%d"},
	{"headlen", fdmc_int_attrfield, &config_headlen, "%d", 0},
	{"name", fdmc_char_attrfield, config_name, "%s", 0},
	/*
	{"queue_sec", fdmc_int_attrfield, config_queue_sec, "%d", 0},
	{"queue_usec", fdmc_int_attrfield, config_queue_usec, "%d", 0},
	{"flag_sec", fdmc_int_attrfield, config_flag_sec, "%d", 0},
	{"flag_usec", fdmc_int_attrfield, config_flag_usec, "%d", 0},
	*/
	{0}
};

#define IPORT 19010

MODULE_NAME("sqlgen");

static char column_name[31];
static char data_type[107];
static int data_length;
static int data_precision;
static int data_scale;
static char table_name[31], table_name_lchr[31], rec_name[31];
static char *tname;

static FDMC_OCI_DEFINE coldef[] =
{
	{sizeof(column_name), column_name, SQLT_STR},
	{sizeof(data_type), data_type, SQLT_STR},
	{sizeof(data_length), &data_length, SQLT_INT},
	{sizeof(data_precision), &data_precision, SQLT_INT},
	{sizeof(data_scale), &data_scale, SQLT_INT},
	{0}
};

static FDMC_OCI_BIND colbind[] = 
{
	{sizeof(table_name), table_name, ":1", SQLT_STR},
	{0}
};

static FDMC_OCI_SESSION *fs;
static FDMC_OCI_STATEMENT *wcol;

struct oci_field
{
	char *name;
	int ftype;
	int fsize;
	int fscale;
};

FDMC_LIST *flist;

FDMC_THREAD_TYPE __thread_call thread_main(void *p)
{
	return (FDMC_THREAD_TYPE)0;
}

static int lowcap(char *src)
{
	int i = 0;
	while( (src[i] = tolower(src[i])) != 0)
	{
		i++;
	}
	return 1;
}

static int uppercap(char *src)
{
	int i = 0;
	while((src[i] = toupper(src[i])) != 0)
	{
		i++;
	}
	return 1;
}

static int write_field_list(char *outname)
{
	FDMC_LIST_ENTRY *fld;
	FILE *out;
	struct oci_field *foci;

	// Open otuput file for writing
	out = fopen(outname, "w");
	if(!out)
	{
		printf("Cannot create output file '%s'\n");
		printf("open: %s\n", strerror(errno));
		return 0;
	}

	// Print common code
	fprintf(out, "#include \"fdmc_oci.h\"\n");
	fprintf(out, "typedef struct _%s\n", table_name);
	fprintf(out, "{\n");

	//Write struture fields
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t");
		switch(foci->ftype)
		{
		case SQLT_FLT:
		case 2:
			fprintf(out, "double %s;\n", foci->name);
			break;
		case SQLT_INT:
		case 187:
			fprintf(out, "int %s;\n", foci->name);
			break;
		case SQLT_STR:
		case 96:
		case 1:
			fprintf(out, "char %s[%d];\n", foci->name, foci->fsize);
			break;
		case SQLT_DAT:
			fprintf(out, "char %s[7];\n", foci->name);
			break;
		}
	}
	fprintf(out, "} %s;\n\n", table_name);
	sprintf(rec_name, "%s_rec", table_name);
	fprintf(out, "%s %s;\n\n", table_name, rec_name);

	// Write bind structure for insert, delete, update request
	fprintf(out, "FDMC_OCI_BIND %s_bind[] = \n", table_name);
	fprintf(out, "{\n");
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t");
		switch (foci->ftype)
		{
		case SQLT_FLT:
			fprintf(out, "SQLFLTC(%s.%s, \":%s\"),\n", rec_name, foci->name, foci->name);
			break;
		case SQLT_INT:
			fprintf(out, "SQLINTC(%s.%s, \":%s\"),\n", rec_name, foci->name, foci->name);
			break;
		case SQLT_STR:
			fprintf(out, "SQLSTRC(%s.%s, \":%s\"),\n", rec_name, foci->name, foci->name);
			break;
		case SQLT_DAT:
			fprintf(out, "SQLDATC(%s.%s, \":%s\"),\n", rec_name, foci->name, foci->name);
			break;
		}
	}
	fprintf(out, "\t{0}\n};\n");

	//Write define structure for select statement
	fprintf(out, "FDMC_OCI_DEFINE %s_define[] = \n", table_name);
	fprintf(out, "{\n");
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t");
		switch (foci->ftype)
		{
		case SQLT_FLT:
			fprintf(out, "SQLFLT(%s.%s),\n", rec_name, foci->name);
			break;
		case SQLT_INT:
			fprintf(out, "SQLINT(%s.%s),\n", rec_name, foci->name);
			break;
		case SQLT_STR:
			fprintf(out, "SQLSTR(%s.%s),\n", rec_name, foci->name);
			break;
		case SQLT_DAT:
			fprintf(out, "SQLDAT(%s.%s),\n", rec_name, foci->name);
			break;
		}
	}
	fprintf(out, "\t{0}\n};\n");

	// Write select statement
	fprintf(out, "select\n");
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t%s", foci->name);
		if(fld->next)
		{
			fprintf(out, ",");
		}
		fprintf(out, "\n");
	}
	fprintf(out, "from %s\n", table_name);

	// Write insert statement
	fprintf(out, "insert into %s (\n", table_name);
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t%s", foci->name);
		if(fld->next)
		{
			fprintf(out, ",\n");
		}
	}
	fprintf(out, ")\n");
	fprintf(out, "values (\n");
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t:%s", foci->name);
		if(fld->next)
		{
			fprintf(out, ",\n");
		}
	}
	fprintf(out, ")\n");

	// Write update statement
	fprintf(out, "update %s set\n", table_name);
	for(fld = flist->first; fld != NULL; fld = fld->next)
	{
		foci = fld->data;
		fprintf(out, "\t%s = :%s", foci->name, foci->name);
		if(fld->next)
		{
			fprintf(out, ",\n");
		}
	}
	fprintf(out, "\n");
	fclose(out);
	return 1;
}

static int process_table(char *tname)
{
	FDMC_EXCEPTION main_err;
	char fname[64];
	struct oci_field *foci;
	int i, j;

	if(!tname) return 0;
	j = (int)strlen(tname);
	for(i = 0; i <= j; i ++)
	{
		tname[i] = toupper(tname[i]);
		table_name_lchr[i] = tolower(tname[i]);
	}
	fs = NULL;
	TRY(main_err)
	{
		fs = fdmc_create_oci_session("sovcom", "s0vc0m", "imapscb", &main_err);
		wcol = fdmc_sql_prepare(fs, column_sql, (int)strlen(column_sql), colbind, 
			coldef, &main_err);
		fdmc_sql_exec(wcol, &main_err);
		sprintf(fname, "%.64s.c", tname);
		lowcap(fname);
		flist = fdmc_list_create(fdmc_list_proc, &main_err);
		// Read table description
		for(;fdmc_sql_fetch(wcol, &main_err);)
		{
			foci = fdmc_malloc(sizeof(*foci), &main_err);
			foci->name = strdup(column_name);
			if(strncmp(data_type, "NUMBER", 6) == 0)
			{
				if(data_precision >= 8 || data_scale != 0)
				{
					foci->ftype = SQLT_FLT;
				}
				else
				{
					foci->ftype = SQLT_INT;
				}
				foci->fsize = data_precision;
				foci->fscale = data_scale;
			}
			else if(strncmp(data_type, "DATE", 4) == 0)
			{
				foci->ftype = SQLT_DAT;
				foci->fsize = 7;
			}
			else if(strncmp(data_type, "VARCHAR", 7) == 0 || 1)
			{
				foci->ftype = SQLT_STR;
				foci->fsize = data_length;
			}
			fdmc_list_entry_add(flist, 
				fdmc_list_entry_create(foci, &main_err), &main_err);
		}
	}
	EXCEPTION
	{
		if(fs == NULL)
		{
			trace("Session not created:\n%s", main_err.errortext);
			return 0;
		}
		else if(fs->oci_error_code != OCI_NO_DATA)
		{
			trace("Error while processing job");
			trace("%s", main_err.errortext);
			return 0;
		}
	}
	fdmc_close_oci_session(fs, NULL);
	write_field_list(fname);
	return 1;
}

static int process_file(char *fname)
{
	FUNC_NAME("process_file");
	char cout[134];
	char fmt[128];
	char *pname = fmt + 1;
	char *ptype = fmt + 52;
	char *psize; int csize;
	char *pscale; int cscale;
	char *p;
	FDMC_EXCEPTION main_err;
	struct oci_field *foci;

	FILE *fin;

	// Open input file
	fin = fopen(fname, "r");
	sprintf(cout, "%.127s.c", fname);
	lowcap(cout);
	strcpy(table_name, fname); lowcap(table_name);
	if(!fin) 
	{
		printf("Cannot open input file %s\n", fname);
		printf("%s\n", strerror(errno));
		return 0;
	}

	// Create field list
	TRY(main_err)
	{
		flist = fdmc_list_create(fdmc_list_proc, &main_err);
		for(;;)
		{
			if(!fgets(fmt, 128, fin))
				break;
			foci = fdmc_malloc(sizeof(*foci), &main_err);
			strncpy(column_name, pname, 30);
			column_name[30] = 0;
			p = strchr(column_name, ' ');
			if (p) *p = 0;
			foci->name = strdup(column_name);
			lowcap(foci->name);
			psize = strchr(ptype, '(');
			if(!psize)
			{
				csize = 0; cscale = 0;
			}
			else
			{
				sscanf(psize+1, "%d", &csize);
				pscale = strchr(psize, ',');
				if(!pscale)
				{
					cscale = 0;
				}
				else
				{
					sscanf(pscale+1, "%d", &cscale);
				}
			}
			if(strncmp(ptype, "NUMBER", 6) == 0)
			{

				if(csize >= 8 || csize == 0 || cscale != 0)
				{
					foci->ftype = SQLT_FLT;
				}
				else
				{
					foci->ftype = SQLT_INT;
				}
				foci->fsize = csize;
				foci->fscale = cscale;
			}
			else if(strncmp(ptype, "DATE", 4) == 0)
			{
				foci->ftype = SQLT_DAT;
				foci->fsize = 7;
			}
			else if(strncmp(ptype, "VARCHAR", 7) == 0 || 1)
			{
				foci->ftype = SQLT_STR;
				foci->fsize = csize;
			}
			else
			{
				foci->ftype = SQLT_STR;
				foci->fsize = 4;
			}
			fdmc_list_entry_add(flist, 
				fdmc_list_entry_create(foci, &main_err), &main_err);
		}
	}
	EXCEPTION
	{
		printf("%s%s\n", _function_id, main_err.errortext);
		return 0;
	}
	fclose(fin); 
	write_field_list(cout);
	return 1;
}

int rmlf(char *s)
{
	int i;

	i = strlen(s);
	if(i > 0)
	{
		if(s[i-1]=='\n')
		{
			s[i-1] = 0;
		}
	}
	return 1;
}

static int process_query(char *inpfname)
{
	FUNC_NAME("process_query");
	unsigned int i = 0;
	sword errc;
	ub4 numcols = 0;
	ub2 type;
	FILE *f, *fp;
	char login[1024], password[1024], tns[1024];
	char *stmt, buf[1024];
	OCIParam *colhd = NULL;
	FDMC_OCI_SESSION *fs;
	FDMC_SQL_CPOOL *v_pool;
	FDMC_OCI_STATEMENT *sql;
	struct oci_field *foci;
	FDMC_LIST_ENTRY *clmn;

	dbg_print();
	func_trace("process sql in file '%s'", inpfname);
	strcpy(table_name, "tbl");
	TRYF(mainerr)
	{
		flist = fdmc_list_create(fdmc_list_proc, &mainerr);
		f = fopen(inpfname, "r");
		if(f == NULL)
		{
			fdmc_raisef(FDMC_IO_ERROR, &mainerr, "Cannot open '%s':\n%s", 
				inpfname, strerror(errno));
		}
		sprintf(buf, "%.128s.c", inpfname);
		fp = fopen(buf, "w");
		if(fp == NULL)
		{
			err_trace("Cannot create output file\n%s", strerror(errno));
			return 0;
		}
		stmt = fdmc_malloc(0x8000, &mainerr);
		if(!fgets(login, 1024, f))
		{
			fdmc_raisef(FDMC_IO_ERROR, &mainerr, "Invalid input file(no login)");
		}
		rmlf(login);
		if(!fgets(password, 1024, f))
		{
			fdmc_raisef(FDMC_IO_ERROR, &mainerr, "Invalid input file(no password)");
		}
		rmlf(password);
		if(!fgets(tns, 1024, f))
		{
			fdmc_raisef(FDMC_IO_ERROR, &mainerr, "Invalid input file(no tnsname)");
		}
		rmlf(tns);
		stmt[0] = 0;
		// Get query body
		while(fgets(buf, 1024, f))
		{
			strcat(stmt, buf);
			rmlf(buf);
			fprintf(fp, " %s\\\n", buf);
		}
		fclose(f);
		func_trace("Result query is \n%s", stmt);
		//v_pool = fdmc_sql_cpool_create(login, password, tns, 3, 6, 3, &mainerr);
		//fs = fdmc_sql_session_pcreate(v_pool, &mainerr);
		fs = fdmc_create_oci_session(login, password, tns, &mainerr);
		sql = fdmc_sql_prepare(fs, stmt, strlen(stmt), NULL, NULL, &mainerr);
		errc = OCIStmtExecute(fs->oci_service_handle, sql->oci_statement, fs->oci_error_handle,
			0, 0, NULL, NULL, OCI_DESCRIBE_ONLY);
		if(errc != OCI_SUCCESS)
		{
			trace("Error executing query");
			fdmc_oci_error(fs, errc, &mainerr);
			return 0;
		}
		errc = OCIAttrGet(sql->oci_statement, OCI_HTYPE_STMT, (dvoid *)&numcols,
			(ub4 *)0, OCI_ATTR_PARAM_COUNT, fs->oci_error_handle);
		if(errc != OCI_SUCCESS)
		{
			trace("Error getting columns number");
			fdmc_oci_error(fs, errc, &mainerr);
			return 0;
		}
		for (i = 1; i <= numcols; i++)
		{
			int col_name_len, char_semantics, col_width, col_scale, col_precision;
			char *col_name;

			foci = fdmc_malloc(sizeof(*foci), &mainerr);
			/* get parameter for column i */
			errc = OCIParamGet(sql->oci_statement, OCI_HTYPE_STMT, fs->oci_error_handle, 
				(dvoid**)&colhd, i);
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting parameter handler for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}
			/* get data-type of column i */
			type = 0;
			errc = OCIAttrGet((dvoid *)colhd, OCI_DTYPE_PARAM,
				(dvoid *)&type, (ub4 *)0, OCI_ATTR_DATA_TYPE, fs->oci_error_handle);
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting type parameter for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}
			trace("TYPE=%d", type);
			foci->ftype = type;

			/* Retrieve the column name attribute */
			col_name_len = 0;
			errc = OCIAttrGet((dvoid*) colhd, (ub4) OCI_DTYPE_PARAM,
				(dvoid**) &col_name, (ub4 *) &col_name_len, OCI_ATTR_NAME,
				fs->oci_error_handle );
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting name parameter for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}
			trace("NAME=%.*s", col_name_len, col_name);
			foci->name = fdmc_malloc(col_name_len+1, &mainerr);
			strncpy(foci->name, col_name, col_name_len);
			foci->name[col_name_len] = 0;
			lowcap(foci->name);

			/* Retrieve the length semantics for the column */
			char_semantics = 0;
			errc = OCIAttrGet((dvoid*) colhd, (ub4) OCI_DTYPE_PARAM,
				(dvoid*) &char_semantics,(ub4 *) 0, (ub4) OCI_ATTR_CHAR_USED,
				(OCIError *) fs->oci_error_handle );
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting length semantics for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}

			col_width = 0;
			if (char_semantics)
				/* Retrieve the column width in characters */
				errc = OCIAttrGet((dvoid*) colhd, (ub4) OCI_DTYPE_PARAM,
				(dvoid*) &col_width, (ub4 *) 0, (ub4) OCI_ATTR_CHAR_SIZE,
				(OCIError *) fs->oci_error_handle );
			else
				/* Retrieve the column width in bytes */
				errc = OCIAttrGet((dvoid*) colhd, (ub4) OCI_DTYPE_PARAM,
				(dvoid*) &col_width,(ub4 *) 0, (ub4) OCI_ATTR_DATA_SIZE,
				(OCIError *) fs->oci_error_handle );
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting width for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}
			trace("WIDTH=%d", col_width);
			foci->fsize = col_width;

			col_precision = 0;
			errc = OCIAttrGet ((dvoid*) colhd, (ub4) OCI_DTYPE_PARAM,
				(dvoid*) &col_precision, (ub4 *) 0,
				(ub4) OCI_ATTR_PRECISION, (OCIError *)fs->oci_error_handle);
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting precision for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}
			trace("PRECISION=%d", col_precision);
			if(col_precision != 0)
			{
				foci->fsize = col_precision;
			}

			col_scale = 0;
			errc = OCIAttrGet ((dvoid*) colhd, (ub4) OCI_DTYPE_PARAM,
				(dvoid*) &col_scale, (ub4 *) 0,
				(ub4) OCI_ATTR_SCALE, (OCIError *)fs->oci_error_handle);
			if(errc != OCI_SUCCESS)
			{
				trace("Error getting scale for column %d", i);
				fdmc_oci_error(fs, errc, &mainerr);
			}
			trace("SCALE=%d", col_scale);
			trace("=======================================");
			foci->fscale = col_scale;
			/*
			switch(foci->ftype)
			{
			case SQLT_NUM:
				if(foci->fsize > 6 || foci->fscale != 0)
				{
					foci->ftype = SQLT_FLT;
				}
				else
				{
					foci->ftype = SQLT_INT;
				}
				break;
			case SQLT_DAT:
				foci->ftype = SQLT_DAT;
				break;
			default:
				foci->ftype = SQLT_STR;
				break;
			}
			*/
			fdmc_list_entry_add(flist, 
				fdmc_list_entry_create(foci, &mainerr), &mainerr);
		}
		// Generate code for pl/sql
		fprintf(fp, "\n/* pl/sql code */\n\n");
		clmn = flist->first;
		while(clmn)
		{
			printf("Hello!\n");
			foci = clmn->data;
			fprintf(fp, "v_%s\t", foci->name);
			switch(foci->ftype)
			{
			case SQLT_CHR:
				fprintf(fp, "varchar2(%d)", foci->fsize);
				break;
			case SQLT_AFC:
				fprintf(fp, "char(%d)", foci->fsize);
				break;
			case SQLT_NUM:
				fprintf(fp, "number");
				if(foci->fsize > 6 || foci->fscale != 0)
				{
					foci->ftype = SQLT_FLT;
				}
				else
				{
					foci->ftype = SQLT_INT;
				}
				break;
			case SQLT_INT:
				fprintf(fp, "integer");
				break;
			case SQLT_FLT:
				fprintf(fp, "float");
				break;
			case SQLT_LNG:
				fprintf(fp, "long");
				break;
			case SQLT_DAT:
				fprintf(fp, "date");
				break;
			case SQLT_TIMESTAMP:
				fprintf(fp, "timestamp");
				break;
			default:
				fprintf(fp, "unknown");
				break;
			}
			fprintf(fp, ";\n");
			clmn = clmn->next;
		}
		fprintf(fp, "\n");
		clmn = flist->first;		
		// Write variables declarations
		while(clmn != NULL)
		{
			foci = clmn->data;
			fprintf(fp, "STORAGE_CLASS ");
			switch(foci->ftype)
			{
			case SQLT_INT:
				fprintf(fp, "int \tv_%s;\n", foci->name);
				break;
			case SQLT_FLT:
				fprintf(fp, "double \tv_%s;\n", foci->name);
				break;
			default:
				if(foci->fsize == 0)
				{
					fprintf(fp, "char \tv_%s[0];\n", foci->name);
				}
				else
				{
					fprintf(fp, "char \tv_%s[%d];\n", foci->name, foci->fsize+1);
				}
				break;
			}
			clmn = clmn->next;
		}
		clmn = flist->first;
		fprintf(fp, "\nFDMC_OCI_DEFINE xdef[] = {\n");
		while(clmn)
		{
			foci = clmn->data;
			fprintf(fp, "\t");
			switch (foci->ftype)
			{
			case SQLT_INT:
				fprintf(fp, "SQLINT(v_%s),\n", foci->name);
				break;
			case SQLT_FLT:
				fprintf(fp, "SQLFLT(v_%s),\n", foci->name);
				break;
			case SQLT_DAT:
				fprintf(fp, "SQLDAT(v_%s),\n", foci->name);
				break;
			default:
				fprintf(fp, "SQLSTR(v_%s),\n", foci->name);
				break;
			}
			clmn = clmn->next;
		}
		fprintf(fp, "\t{0}\n};\n");
		clmn = flist->first;
		fprintf(fp, "\nFDMC_OCI_BIND xdef[] = {\n");
		while(clmn)
		{
			foci = clmn->data;
			fprintf(fp, "\t");
			switch (foci->ftype)
			{
			case SQLT_INT:
				fprintf(fp, "SQLINTC(v_%s, \":v_%s\"),\n", foci->name, foci->name);
				break;
			case SQLT_FLT:
				fprintf(fp, "SQLFLTC(v_%s, \":v_%s\"),\n", foci->name, foci->name);
				break;
			case SQLT_DAT:
				fprintf(fp, "SQLDATC(v_%s, \":v_%s\"),\n", foci->name, foci->name);
				break;
			default:
				fprintf(fp, "SQLSTRC(v_%s, \":v_%s\"),\n", foci->name, foci->name);
				break;
			}
			clmn = clmn->next;
		}
		fprintf(fp, "\t{0}\n};\n");
		// Write message descriptor
		fprintf(fp, "\nFDMC_MSGFILED msgdef[] = {\n");
		clmn = flist->first;
		while(clmn)
		{
			char uname[48];

			foci = (struct oci_field*)clmn->data;
			strcpy(uname, foci->name);
			uppercap(uname);
			fprintf(fp, "\t");
			switch (foci->ftype)
			{
			case SQLT_INT:
				fprintf(fp, "BSAINT(v_%s, \"%s\"),\n", foci->name, uname);
				break;
			case SQLT_FLT:
				fprintf(fp, "BSAFLOAT(v_%s, \"%s\"),\n", foci->name, uname);
				break;
			default:
				fprintf(fp, "BSACHAR(v_%s, \"%s\"),\n", foci->name, uname);
				break;
			}
			clmn = clmn->next;
		}
		fprintf(fp, "\t{0}\n};\n");
		// Write insert statement
		fprintf(fp, "insert into %s (\\\n", table_name);
		for(clmn = flist->first; clmn != NULL; clmn = clmn->next)
		{
			foci = clmn->data;
			fprintf(fp, "\t%s", foci->name);
			if(clmn->next)
			{
				fprintf(fp, ", \\\n");
			}
		}
		fprintf(fp, ") \\\n");
		fprintf(fp, "values ( \\\n");
		for(clmn = flist->first; clmn != NULL; clmn = clmn->next)
		{
			foci = clmn->data;
			fprintf(fp, "\t:v_%s", foci->name);
			if(clmn->next)
			{
				fprintf(fp, ", \\\n");
			}
		}
		fprintf(fp, ")\n");

		// Write update statement
		fprintf(fp, "update %s set \\\n", table_name);
		for(clmn = flist->first; clmn != NULL; clmn = clmn->next)
		{
			foci = clmn->data;
			fprintf(fp, "\t%s = :v_%s", foci->name, foci->name);
			if(clmn->next)
			{
				fprintf(fp, ", \\\n");
			}
		}

		// Write select statement
		fprintf(fp, "\n\n /* select statement */ \n\n");
		fprintf(fp, "select\n");
		for(clmn = flist->first; clmn != NULL; clmn = clmn->next)
		{
			foci = clmn->data;
			fprintf(fp, "\t%s", foci->name);
			if(clmn->next)
			{
				fprintf(fp, ", \\\n");
			}
		}
		fprintf(fp, "\ninto\n");
		for(clmn = flist->first; clmn != NULL; clmn = clmn->next)
		{
			foci = clmn->data;
			fprintf(fp, "\t:v_%s", foci->name);
			if(clmn->next)
			{
				fprintf(fp, ", \\\n");
			}
		}
		fprintf(fp, "\n");
		fclose(fp);
	}
	EXCEPTION
	{
		screen_trace(mainerr.errortext);
		return 0;
	}
	//write_field_list("sqlgen");
	return 1;
}

FDMC_THREAD_TYPE __thread_call thr_connect(void *p)
{
	FDMC_SOCK *psock;
	FDMC_EXCEPTION x;
	FUNC_NAME("thr_connect");

	for(;;)
	{
		TRY(x)
		{
			time_trace("Try to connect to peer");
			psock = fdmc_sock_create("localhost", 19001, 2, &x);
			time_trace("Connected");
			break;
		}
		EXCEPT(x, FDMC_CONNECT_ERROR)
		{
			func_trace("cannot connect:%s", x.errortext);
			fdmc_delay(2, 0);
		}
		EXCEPTION
		{
			func_trace("fatal error, thread ends");
			break;
		}
	}
	if(psock)
	{
		trace("Send ququ to peer");
		fdmc_sock_send(psock, "Hello!", 6, NULL);
		fdmc_delay(2, 0);
		time_trace("exiting thr_connect");
		fdmc_sock_close(psock);
	}
	return (FDMC_THREAD_TYPE)0;
}

FDMC_THREAD_TYPE __thread_call thr_accept(void *p)
{
	FDMC_SOCK *psock, *pacc;
	FDMC_EXCEPTION x;
	char buf[1024];
	int j;
	FUNC_NAME("thr_accept"); 
	FDMC_THREAD *thr_this;

	TRY(x)
	{
		psock = fdmc_sock_create(NULL, 19001, 2, &x);
	}
	EXCEPTION
	{
		func_trace("error creating socket:%s", x.errortext);
		return (FDMC_THREAD_TYPE)0;
	}
	TRY(x)
	{
		time_trace("Waiting for incoming connections");
		if(!fdmc_sock_ready(psock, 3, 0))
		{
			fdmc_raisef(FDMC_SOCKET_ERROR, &x, "Timeout occurred");
		}
		pacc = fdmc_sock_accept(psock, &x);
		if(!fdmc_sock_ready(pacc, 3, 0))
		{
			fdmc_raisef(FDMC_SOCKET_ERROR, &x, "Timeout occurred");
		}
		for(j = 1; j <= 3; j++)
		{
			if(!fdmc_sock_ready(pacc, 0, 10))
			{
				trace("Socket not ready");
			}
			else
			{
				trace("Socket is still ready");
			}
		}
		j = fdmc_sock_recv(pacc, buf, 1024, &x);
		time_trace("received %d bytes from peer", j);
		thr_this = fdmc_thread_this();
		if(thr_this)
		{
			s_dump(thr_this->stream, buf, j);
		}
		trace("Socket is now %s ready", fdmc_sock_ready(pacc, 0, 10)==0 ? "not":"");
		j = recv(pacc->data_socket, buf, 1024, 0);
		trace("Received %d bytes from socket", j);
		//trace("Socket is now % ready", fdmc_sock_ready(pacc, 0, 10)==0 ? "not":"");
	}
	EXCEPTION
	{
		time_trace("Error receiving data: %s", x.errortext);
	}
	return (FDMC_THREAD_TYPE)0;
}

// Device event processor (example)
#ifdef reuryttrmm
int sqldevproc(FDMC_DEVICE *p)
{
	FUNC_NAME("sqldevproc");
	FDMC_EXCEPTION x;
	FDMC_THREAD *thr;
	FDMC_LOGSTREAM *s;
	FDMC_DEV_EVENT ev;
	char buf[FDMC_MAX_ERRORTEXT];
	int buflen;
	FDMC_SOCK *psock;
	FDMC_DEVICE *devnew;

	thr = fdmc_thread_this();
	s = thr->stream;
	thr->stream = p->stream;
	dbg_print();

	for(;;)
	{
		ev = p->dev_event;
		if(ev == FDMC_DEV_EVIDLE)
		{
			break;
		}
		time_trace("event %d occurred", ev);
		p->dev_event = FDMC_DEV_EVIDLE;
		TRYF(x)
		{
			switch(ev)
			{
			case FDMC_DEV_EVCREATE:
				trace("Device created");
				fdmc_device_link(p, &x);
				break;
			case FDMC_DEV_EVCONNECTED:
				trace("Device connected");
				break;
			case FDMC_DEV_EVDISCONNECT:
				trace("Device disconnected");
				if(p->role == FDMC_DEV_CHILD)
				{
					p->dev_event = FDMC_DEV_EVDELETE;
				}
				else if(p->role == FDMC_DEV_INITIATOR)
				{
					trace("Reconnect");
					fdmc_device_link(p, &x);
				}
				break;
			case FDMC_DEV_EVREADY:
				trace("Device is ready for process");
				fdmc_device_ready(p, &x);
				break;
			case FDMC_DEV_EVRECEIVE:
				trace("Receive data");
				buflen = fdmc_device_receive(p, buf, sizeof(buf)-2, &x);
				break;
			case FDMC_DEV_EVDATA:
				trace("Data received");
				fdmc_dump(buf, buflen);
				break;
			case FDMC_DEV_EVDELETE:
				trace("Set device delete state");
				p->stat = FDMC_DEV_DELETED;
				break;
			case FDMC_DEV_EVLISTEN:
				trace("Listener for port %d created", p->ip_port);
				break;
			case FDMC_DEV_EVACCEPT:
				trace("About to accept connections");
				fdmc_device_accept(p, &x);
				psock = p->data;
				devnew = fdmc_device_create("pt400", p->process, p->devproc, "pt400", 1, 
					psock->ip_address, psock->ip_port, p->tmout, p->natt, &x);
				devnew->process = p->process;
				devnew->link = psock;
				devnew->role = FDMC_DEV_CHILD;
				devnew->dev_event = FDMC_DEV_EVCONNECTED;
				devnew->stat = FDMC_DEV_FREE;
				(*devnew->devproc)(devnew);
				break;
			case FDMC_DEV_EVERROR:
				trace("Error occurred");
				p->dev_event = FDMC_DEV_EVDELETE;
				break;
			}
		}
		EXCEPTION
		{
			p->dev_event = FDMC_DEV_EVERROR;
			trace("%s", x.errortext);
		}
	}
	thr->stream = s;
	return 1;
}

// Thread function for handler
static FDMC_THREAD_TYPE __thread_call sql_prc_thread(FDMC_PROCESS *p)
{
	FUNC_NAME("sql_prc_thread");
	FILE *fcfg;
	FDMC_EXCEPTION x;

	debug_formats = 1;
	p->wait_sec = 10;
	fcfg = fopen("config.txt", "r");
	TRYF(x)
	{
		for(;;)
		{
			char *s;
			trace("Creating devices");
			if(!fcfg)
			{
				screen_trace("cannot open config.txt:%s", strerror(errno));
				break;
			}
			while((s = fgets(config_buf, sizeof(config_buf) - 1, fcfg)))
			{
				rmlf(s);
				if(strlen(s) == 0) continue;
				if(s[0] == '#') continue;
				fdmc_process_message(config_msg, config_buf, FDMC_INIT, &x);
				fdmc_extract_message(config_msg, config_buf, &x);
				fdmc_process_message(config_msg, config_buf, FDMC_PRINT, &x);
				fdmc_device_create(config_name, p, sqldevproc, 
					config_name, 1, 
					config_address, 
					config_port, 
					config_timeout, 
					0, 
					&x);

			}
			trace("Start device management");
			fdmc_process_links_create(p, &x);
			fdmc_msleep(200);
			for(;;)
			{
				fdmc_device_lookup(p);
				fdmc_msleep(1);
			}
			break;
		}
	}
	EXCEPTION
	{
		err_trace("Application terminated %s", x.errortext);
	}
	return (FDMC_THREAD_TYPE)0;	
}
#endif

int main(int argc, char* argv[], char *envp[])
{
	int argn = 0;
	int fno;
	//FDMC_THREAD *pcon, *pacc;
	FUNC_NAME("main");
	FDMC_SQL_CPOOL *v_cpool;
	FDMC_SQL_SESSION *v_sess;
//	FDMC_PROCESS *pprc;

	setbuf(stdout, 0);
	fno = fileno(stdout);
	fdmc_thread_init("sqlgen", 1);
	devnull = fopen(NULL_DEVICE, "w");
	//mainstream = fdmc_sfdopen("sqlgen", fno);
	screen_trace("\nOracle SQL statement generator.\nDesigned by Borisov Sergey, october 2009\n");
	{
		FDMC_EXCEPTION x;
		TRY(x)
		{
		//	v_cpool = fdmc_sql_cpool_create("fcard", "fcard", "card5p", 1, 15, 3, &x);
		//	screen_trace("Connection pool created");
		//	v_sess = fdmc_sql_session_popen(v_cpool, &x);
		//	screen_trace("Session via pool created");
		//	v_sess = fdmc_sql_session_popen(v_cpool, &x);
		//	screen_trace("Session via pool created");
		//	v_sess = fdmc_sql_session_popen(v_cpool, &x);
		//	screen_trace("Session via pool created");
		//	//fdmc_sql_session_pclose(v_sess, &x);
		//	fdmc_sql_cpool_close(v_cpool, &x);
		}
		EXCEPTION
		{
			screen_trace("Pool test:\n %s", x.errortext);
		}
	}
	mainstream->max_size = 0x100000;
	fdmc_init_ip();
	argn=0;

	if(!(tname = fdmc_getopt(argc, argv, "-table", &argn)))
	{
		trace("-table parameter was not specified");
		tname="bis2imap";
	}
	else
	{
		strcpy(table_name, tname);
		process_table(table_name);
		//return 0;
	}
	argn=0;
	if(!(tname = fdmc_getopt(argc, argv, "-ifile", &argn)))
	{
		trace("-ifile parameter was not specified");
		tname="listfile";
	}
	else
	{
		process_file(tname);
		//return 0;
	}
	argn = 0;
	if(!(tname = fdmc_getopt(argc, argv, "-sqlfile", &argn)))
	{
		trace("-sqlfile parameter was not specified");
	}
	else
	{
		process_query(tname);
		//return 0;
	}
	//process_query("auth_tab");
	exit(0);
	//pcon = fdmc_thread_create(thr_connect, &pcon, "conn", 0, 0, NULL, NULL);
	//pacc = fdmc_thread_create(thr_accept, &pacc, "acc", 0, 0, NULL, NULL);
	//exit(0);

	{
		char key_hex[17] = "0123456789ABCDEF", buf[8] = {0,0,0,0,0,0,0,0};
		unsigned char key[8] ;//= {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
		int p;
		fdmc_des_hex_to_key(key_hex, key, 1);
		trace("Key converted to");
		fdmc_dump(key, 8);
		p = fdmc_des_parity_check(key, 1);
		trace("parity check ok");
		fdmc_des_buffer_encrypt(buf, 8, key);
		if(!p) exit(0);
		trace("Zero string encrypted to");
		fdmc_dump(buf, 8);
		trace("Now decripted to");
		fdmc_des_buffer_decrypt(buf, 8, key);
		fdmc_dump(buf, 8);
		trace("Generate ascii presentation of key");
		fdmc_des_key_to_hex(key, key_hex, 1);
		fdmc_dump(key_hex, 16);
		trace("");
	}
	exit(0);
	argn = 0;
	tname = fdmc_getopt(argc, argv, "-port", &argn);
	if(tname)
	{
		if(!sscanf(tname, "%d", &argn))
		{
			argn = 19201;
		}
	}
	else
	{
		argn = 19201;
	}
	//pprc = fdmc_process_create(sql_prc_thread, fdmc_handler_thread, "tpr", 1, 0, 0, argn);
	

	for(argn = 0; argn < 100; argn ++)
	{
		screen_time_trace("Cycle loop is %d", argn);
		fdmc_msleep(1000);
	}

	regards();
	return 0;
}