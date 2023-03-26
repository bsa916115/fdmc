#ifndef _FDMC_OCI_INCLUDE
#define _FDMC_OCI_INCLUDE

#include "fdmc_global.h"
#include "fdmc_exception.h"
#include "fdmc_logfile.h"

#include "oci.h"

typedef struct _fdmc_sql_session
{
	OCIEnv *oci_env_handle; 
	OCIError *oci_error_handle; 
	sword sql_error_code; 
#define oci_error_code sql_error_code
	sb4 db_error_code;
	OCIServer *oci_server_handle; 
	OCISvcCtx *oci_service_handle; 
	OCISession *oci_auth_handle; 
} FDMC_SQL_SESSION;

typedef struct _FDMC_SQL_CPOOL
{
	OCIEnv *oci_env_handle; 
	OCIError *oci_error_handle; 
	OraText *login;
	sb4 login_len;
	OraText *password;
	sb4 password_len;
	sword sql_error_code;
	sb4 db_error_code;
	OCICPool *poolhp;
	OraText *pool_name;
	sb4 pool_name_len;
} FDMC_SQL_CPOOL;

#define FDMC_OCI_SESSION FDMC_SQL_SESSION

typedef struct
{
	unsigned oci_bind_length; 
	void *oci_bind_address; 
	char *name;
	unsigned oci_bind_type; 
	OCIBind *oci_bind_handle; 
	short nullind;
} FDMC_SQL_BIND;

#define FDMC_OCI_BIND FDMC_SQL_BIND

typedef struct
{
	unsigned oci_bind_length; 
	void *oci_bind_address;
	char *bind_name;
	unsigned oci_bind_type; 
	OCIBind *oci_bind_handle; 
} FDMC_OCI_BIND_BYNAME;

typedef struct
{
	unsigned oci_define_length;
	void *oci_define_address;
	unsigned oci_define_type;
	OCIDefine *oci_define_handle; 
	short nullind;
} FDMC_SQL_DEFINE;

#define FDMC_OCI_DEFINE FDMC_SQL_DEFINE

typedef struct _fdmc_oci_statement
{
	FDMC_OCI_SESSION *oci_session;
	OCIStmt *oci_statement;
	FDMC_OCI_BIND *oci_bind;
	FDMC_OCI_DEFINE *oci_define;
} FDMC_SQL_STATEMENT;

#define FDMC_OCI_STATEMENT FDMC_SQL_STATEMENT

typedef struct 
{
	FDMC_OCI_SESSION *oci_session;
	OCIStmt *oci_statement;
	FDMC_OCI_BIND *oci_bind;
	void *oci_bind_data;
	FDMC_OCI_DEFINE *oci_define;
	void *oci_define_data;
} FDMC_EXT_OCI_STATEMENT;

#define SQLSTR(x) {sizeof(x), x, SQLT_STR}
#define SQLINT(x) {sizeof(x), &x, SQLT_INT}
#define SQLFLT(x) {sizeof(x), &x, SQLT_FLT}
#define SQLLNG(x) {sizeof(x), &x, SQLT_LNG}
#define SQLDAT(x) {sizeof(x), x, SQLT_DAT}

#define SQLSTRC(x, c) {sizeof(x), x, c, SQLT_STR}
#define SQLINTC(x, c) {sizeof(x), &x, c, SQLT_INT}
#define SQLFLTC(x, c) {sizeof(x), &x, c, SQLT_FLT}
#define SQLLNGC(x, c) {sizeof(x), &x, c, SQLT_LNG}
#define SQLDATC(x, c) {sizeof(x), x, c, SQLT_DAT}

//OCIStmt *stmthp;

#define fdmc_create_oci_session fdmc_sql_session_create
 FDMC_SQL_SESSION *fdmc_sql_session_create(char *ora_user, char *ora_passwd, 
								char *ora_tnsname, FDMC_EXCEPTION *expt);

 FDMC_SQL_SESSION *fdmc_sql_session_pcreate(FDMC_SQL_CPOOL *p_pool, FDMC_EXCEPTION *expt);
 FDMC_SQL_SESSION *fdmc_sql_session_popen(FDMC_SQL_CPOOL *p_pool, FDMC_EXCEPTION *expt);

 int fdmc_sql_session_close(FDMC_SQL_SESSION *oci_session, FDMC_EXCEPTION *expt);
#define fdmc_close_oci_session fdmc_sql_session_close
 int fdmc_sql_session_pclose(FDMC_OCI_SESSION *p_session, FDMC_EXCEPTION *err);

 int fdmc_sql_error(FDMC_SQL_SESSION *fs, sword code, FDMC_EXCEPTION *expt);
#define fdmc_oci_error fdmc_sql_error

 FDMC_SQL_STATEMENT *fdmc_sql_prepare(
	FDMC_SQL_SESSION *oci_session, char *stmt_text, int stmt_len,
	FDMC_SQL_BIND *oci_bind, FDMC_SQL_DEFINE *oci_define, FDMC_EXCEPTION *expt);

 int fdmc_sql_exec(FDMC_SQL_STATEMENT *stmt, FDMC_EXCEPTION *expt);
 int fdmc_sql_open(FDMC_SQL_STATEMENT *stmt, FDMC_EXCEPTION *expt);
 int fdmc_sql_fetch(FDMC_SQL_STATEMENT *stmt, FDMC_EXCEPTION *expt);
 int fdmc_sql_close(FDMC_SQL_STATEMENT *stmt, FDMC_EXCEPTION *expt);
 int fdmc_sql_process(char *text, FDMC_EXCEPTION *err);

// FDMC_OCI_BIND *fdmc_clone_ocibind(FDMC_SQL_BIND *pattern, void *host_addr);
// FDMC_OCI_DEFINE *fdmc_clone_ocidefine(FDMC_SQL_DEFINE *pattern, void *host_addr);
 int fdmc_stmt_define(FDMC_SQL_STATEMENT *stmt, FDMC_SQL_DEFINE *oci_define, FDMC_EXCEPTION *er);
 int fdmc_stmt_bind(FDMC_SQL_STATEMENT *stmt, FDMC_SQL_BIND *oci_bind, FDMC_EXCEPTION *er);
 int fdmc_stmt_bind_byname(FDMC_SQL_STATEMENT *stmt, FDMC_OCI_BIND_BYNAME *oci_bind, 
						  FDMC_EXCEPTION *er);

 int fdmc_sql_commit(FDMC_SQL_SESSION *fs, FDMC_EXCEPTION *);
 int fdmc_sql_rollback(FDMC_SQL_SESSION *fs, FDMC_EXCEPTION *);

FDMC_SQL_CPOOL *fdmc_sql_cpool_create(
	char *ora_user, char *ora_passwd, char *ora_tnsname,
	int c_min, int c_max, int c_incr,
	FDMC_EXCEPTION *err);

int fdmc_sql_cpool_close(FDMC_SQL_CPOOL *p_pool, FDMC_EXCEPTION *err);

#define DB_GET_REC(db, sql_text, bind, def, x) \
{ \
	FDMC_SQL_STATEMENT *sql_stmt; \
	sql_stmt = fdmc_sql_prepare(db, sql_text, strlen(sql_text), bind, def, x); \
	fdmc_sql_open(sql_stmt, x); \
	fdmc_sql_fetch(sql_stmt, x); \
	fdmc_sql_close(sql_stmt, x); \
}

#define DB_EXEC(db, text, bind, x) { \
	FDMC_SQL_STATEMENT *stmt; \
	stmt = fdmc_sql_prepare(db, text, strlen(text), bind, NULL, &x); \
	fdmc_sql_exec(stmt, &x); \
	fdmc_sql_close(stmt, &x); \
}
#endif
