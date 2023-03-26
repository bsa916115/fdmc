#include "fdmc_oci.h"
#include "fdmc_logfile.h"

#include <string.h>
#include <stdlib.h>

MODULE_NAME("fdmc_oci");

//-------------------------------------------
// Purpose: Create oracle connection pool
// 
//-------------------------------------------
// Parameters:
// ora_user
// ora_passwd     - parameters for oracle connection
// ora_tnsname
//
// c_min - minimum physical connections
// c_max - maximum physical connections
// c_incr - number of connections to increment
//-------------------------------------------
// Returns: new pool descriptor or null if fail
// 
//-------------------------------------------
// Special features:
// long jump on error
//-------------------------------------------
FDMC_SQL_CPOOL *fdmc_sql_cpool_create(
	char *ora_user, char *ora_passwd, char *ora_tnsname,
	int c_min, int c_max, int c_incr,
	FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_sql_cpool_create");
	FDMC_EXCEPTION x, ox;
	FDMC_SQL_CPOOL *pool_new;
	sword code;

	TRYF(x)
	{
		// Allocate memory
		pool_new = (FDMC_SQL_CPOOL*)fdmc_malloc(sizeof(*pool_new), &x);
		pool_new->login = (OraText*)fdmc_strdup(ora_user, &x);
		pool_new->login_len = (sb4)strlen(ora_user);
		pool_new->password = (OraText*)fdmc_strdup(ora_passwd, &x);
		pool_new->password_len = (sb4)strlen(ora_passwd);
		// Create oci environment variable
		if ((code = OCIEnvCreate((OCIEnv **) &pool_new->oci_env_handle,
			(ub4)OCI_THREADED|OCI_OBJECT, (dvoid *)0,
			(dvoid * (*)(dvoid *, size_t)) 0,
			(dvoid * (*)(dvoid *, dvoid *, size_t))0,
			(void (*)(dvoid *, dvoid *)) 0,
			(size_t) 0, (dvoid **) 0 )) != OCI_SUCCESS)
		{
			fdmc_raisef(FDMC_DB_ERROR, &x, "Error creating oci environment. Code = %d.", code);
		}
		// Allocate error handle
		if ((code = OCIHandleAlloc((dvoid *) pool_new->oci_env_handle, (dvoid **) &pool_new->oci_error_handle,
			(ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
		{
			fdmc_raisef(FDMC_DB_ERROR, &x, "Cannot create oci error handler. Code = %d.", code);
		}
		// Create pool handle
		TRY(ox)
		{
			if ((code = OCIHandleAlloc(pool_new->oci_env_handle, (void**)&pool_new->poolhp, OCI_HTYPE_CPOOL,
					(size_t)0, (void **)0)) != OCI_SUCCESS)
			{
				fdmc_sql_error((FDMC_SQL_SESSION*)pool_new, code, &ox);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(FDMC_DB_ERROR, &x, "OCIHanleAlloc: %s", ox.errortext);
		}
		// Create pool
		TRY(ox)
		{
			if((code = OCIConnectionPoolCreate(pool_new->oci_env_handle, pool_new->oci_error_handle, pool_new->poolhp,
				(OraText **)&pool_new->pool_name, (sb4*)&pool_new->pool_name_len, 
				(OraText *)ora_tnsname, (sb4)strlen(ora_tnsname),
				(ub4) c_min, (ub4) c_max, (ub4) c_incr, 
				pool_new->login, pool_new->login_len,
				pool_new->password, pool_new->password_len,
				OCI_DEFAULT)) != OCI_SUCCESS)
			{
				fdmc_sql_error((FDMC_SQL_SESSION*)pool_new, code, &ox);
			}
		}
		EXCEPTION
		{
			fdmc_sql_cpool_close(pool_new, NULL);
			fdmc_raisef(FDMC_DB_ERROR, &x, "OCIConnectionPoolCreate: %s", ox.errortext);
		}

	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return pool_new;
}


int fdmc_sql_cpool_close(FDMC_SQL_CPOOL *p_pool, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fmdc_sql_cpool_close");
	FDMC_EXCEPTION x;
	sword code;

	TRYF(x)
	{
		CHECK_NULL(p_pool, "p_pool", x);
		code = OCIConnectionPoolDestroy(p_pool->poolhp, p_pool->oci_error_handle, OCI_DEFAULT);
		if(code != OCI_SUCCESS)
		{
			fdmc_sql_error((FDMC_SQL_SESSION*)p_pool, code, &x);
		}
		code = OCIHandleFree(p_pool->poolhp, OCI_HTYPE_CPOOL);
		if(code != OCI_SUCCESS)
		{
			fdmc_raisef(FDMC_DB_ERROR, &x, "Error while free pool handle");
		}
		code = OCIHandleFree(p_pool->oci_error_handle, OCI_HTYPE_ERROR);
		if(code != OCI_SUCCESS)
		{
			fdmc_raisef(FDMC_DB_ERROR, &x, "Error while free pool error handle");
		}
		code = OCIHandleFree(p_pool->oci_env_handle, OCI_HTYPE_ENV);
		if(code != OCI_SUCCESS)
		{
			fdmc_raisef(FDMC_DB_ERROR, &x, "Error while free pool env handle");
		}
		fdmc_free(p_pool->login, NULL);
		fdmc_free(p_pool->password, NULL);
		fdmc_free(p_pool, NULL);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return 1;
}

FDMC_SQL_SESSION *fdmc_sql_session_create(char *ora_user, char *ora_passwd, char *ora_tnsname, 
	FDMC_EXCEPTION *expt)
{
	FDMC_OCI_SESSION *result;
	FUNC_NAME("fdmc_sql_session_create");
	FDMC_EXCEPTION ex, ex1;
	sword code;

	TRYF(ex)
	{
		result = (FDMC_OCI_SESSION*)fdmc_malloc(sizeof(FDMC_OCI_SESSION), &ex);
		memset(result, 0, sizeof(*result));
		//code = 0; 
		ex1.errortext[0] = 0;

		// Create oci environment
		TRY(ex1)
		{
			if ((code = OCIEnvCreate((OCIEnv **) &result->oci_env_handle,
				(ub4)OCI_THREADED|OCI_OBJECT, (dvoid *)0,
				(dvoid * (*)(dvoid *, size_t)) 0,
				(dvoid * (*)(dvoid *, dvoid *, size_t))0,
				(void (*)(dvoid *, dvoid *)) 0,
				(size_t) 0, (dvoid **) 0 )) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "OCIEnvCreate:\n%s", ex1.errortext);
		}
		// Create error handler
		TRY(ex1)
		{
			if ((code = OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_error_handle,
				(ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc error handler:\n%s", ex1.errortext);
		}
		// Create server handle
		TRY(ex1)
		{
			if ((code = OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_server_handle,
				(ub4) OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc server handle:\n%s", ex1.errortext); 
		}
		TRY(ex1)
		{
			// Create service handle
			if ((code=OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_service_handle,
				(ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc service handle:\n%s", ex1.errortext); 
		}
		TRY(ex1) 
		{
			// Session handle
			if ((code=OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_auth_handle,
				(ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc session handle:\n%s", ex1.errortext); 
		}

		// Work database
		if ((code=OCIServerAttach(result->oci_server_handle, result->oci_error_handle, (text *) ora_tnsname,
			(sb4) strlen((char *) ora_tnsname), (ub4) OCI_DEFAULT)) != OCI_SUCCESS )
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(ex1.errorcode, &ex, "OCIServerAttach('%s'):\n%s", ora_tnsname,
					ex1.errortext);
			}
		}

		if ((code=OCIAttrSet((dvoid *) result->oci_service_handle, (ub4) OCI_HTYPE_SVCCTX,
			(dvoid *) result->oci_server_handle, (ub4) 0, (ub4) OCI_ATTR_SERVER,
			result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(ex1.errorcode, &ex, "Set Server Attribute:\n%s", ex1.errortext);
			}
		}
		// User
		if ((code = OCIAttrSet((dvoid *) result->oci_auth_handle, (ub4) OCI_HTYPE_SESSION,
			(dvoid *) ora_user, (ub4) strlen(ora_user),
			(ub4) OCI_ATTR_USERNAME, result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(ex1.errorcode, &ex, "Set User Name:\n%s", ex1.errortext);
			}
		}

		/* Пароль */
		if ((code = OCIAttrSet((dvoid *) result->oci_auth_handle, (ub4) OCI_HTYPE_SESSION,
			(dvoid *) ora_passwd, (ub4) strlen(ora_passwd),
			(ub4) OCI_ATTR_PASSWORD, result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(FDMC_DB_ERROR, &ex, "Set User Password:\n%s", ex1.errortext);
			}
		}

		/* Соединение с listener */
		if((code=OCISessionBegin(result->oci_service_handle, 
			result->oci_error_handle, result->oci_auth_handle, 
			(ub4) OCI_CRED_RDBMS,(ub4) OCI_DEFAULT )) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(FDMC_DB_ERROR, &ex, "OCISessionBegin:\n%s", ex1.errortext);
			}
		}

		if ((code = OCIAttrSet((dvoid *) result->oci_service_handle, (ub4) OCI_HTYPE_SVCCTX,   
			(dvoid *) result->oci_auth_handle, (ub4) 0, (ub4) OCI_ATTR_SESSION,
			result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(FDMC_DB_ERROR, &ex, "Set Session Attribute:\n%s", ex1.errortext);
			}
		}
	}
	EXCEPTION
	{
		fdmc_close_oci_session(result, NULL);
		if(expt)
		{
			expt->errorsubcode = code;
		}
		fdmc_raiseup(&ex, expt);
		result = NULL;
	}
	return result;
}

FDMC_SQL_SESSION *fdmc_sql_session_popen(FDMC_SQL_CPOOL *p_pool, FDMC_EXCEPTION *expt)
{
	FDMC_OCI_SESSION *result;
	FUNC_NAME("fdmc_sql_session_popen");
	FDMC_EXCEPTION ex, ex1;
	sword code;

	TRYF(ex)
	{
		CHECK_NULL(p_pool, "p_pool", ex);
		result = (FDMC_OCI_SESSION*)fdmc_malloc(sizeof(FDMC_OCI_SESSION), &ex);
		result->oci_env_handle = p_pool->oci_env_handle;
		//code = 0; 
		ex1.errortext[0] = 0;

		// Create error handler
		TRY(ex1)
		{
			if ((code = OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_error_handle,
				(ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc error handler:\n%s", ex1.errortext);
		}
		// Create server handle
		TRY(ex1)
		{
			if ((code = OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_server_handle,
				(ub4) OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc server handle:\n%s", ex1.errortext); 
		}
		TRY(ex1)
		{
			// Create service handle
			if ((code=OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_service_handle,
				(ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc service handle:\n%s", ex1.errortext); 
		}
		TRY(ex1) 
		{
			// Session handle
			if ((code=OCIHandleAlloc((dvoid *) result->oci_env_handle, (dvoid **) &result->oci_auth_handle,
				(ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
			{
				fdmc_oci_error(result, code, &ex1);
			}
		}
		EXCEPTION
		{
			fdmc_raisef(ex1.errorcode, &ex, "Alloc session handle:\n%s", ex1.errortext); 
		}

		// Work database
		if ((code=OCIServerAttach(result->oci_server_handle, result->oci_error_handle, p_pool->pool_name,
			p_pool->pool_name_len, (ub4) OCI_CPOOL)) != OCI_SUCCESS )
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(ex1.errorcode, &ex, "OCIServerAttach(pool='%s'):\n%s", p_pool->pool_name,
					ex1.errortext);
			}
		}

		if ((code=OCIAttrSet((dvoid *) result->oci_service_handle, (ub4) OCI_HTYPE_SVCCTX,
			(dvoid *) result->oci_server_handle, (ub4) 0, (ub4) OCI_ATTR_SERVER,
			result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(ex1.errorcode, &ex, "Set Server Attribute:\n%s", ex1.errortext);
			}
		}
		// User
		if ((code = OCIAttrSet((dvoid *) result->oci_auth_handle, (ub4) OCI_HTYPE_SESSION,
			p_pool->login, p_pool->login_len,
			(ub4) OCI_ATTR_USERNAME, result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(ex1.errorcode, &ex, "Set User Name:\n%s", ex1.errortext);
			}
		}

		/* Пароль */
		if ((code = OCIAttrSet((dvoid *) result->oci_auth_handle, (ub4) OCI_HTYPE_SESSION,
			p_pool->password, p_pool->password_len,
			(ub4) OCI_ATTR_PASSWORD, result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(FDMC_DB_ERROR, &ex, "Set User Password:\n%s", ex1.errortext);
			}
		}

		/* Соединение с listener */
		if((code=OCISessionBegin((dvoid *)result->oci_service_handle, 
			result->oci_error_handle, result->oci_auth_handle, 
			(ub4) OCI_CRED_RDBMS,(ub4) OCI_DEFAULT )) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(FDMC_DB_ERROR, &ex, "OCISessionBegin:\n%s", ex1.errortext);
			}
		}

		if ((code = OCIAttrSet((dvoid *) result->oci_service_handle, (ub4) OCI_HTYPE_SVCCTX,   
			(dvoid *) result->oci_auth_handle, (ub4) 0, (ub4) OCI_ATTR_SESSION,
			result->oci_error_handle)) != OCI_SUCCESS)
		{
			TRY(ex1)
			{
				fdmc_oci_error(result, code, &ex1);
			}
			EXCEPTION
			{
				fdmc_raisef(FDMC_DB_ERROR, &ex, "Set Session Attribute:\n%s", ex1.errortext);
			}
		}
	}
	EXCEPTION
	{
		fdmc_close_oci_session(result, NULL);
		if(expt)
		{
			expt->errorsubcode = code;
		}
		fdmc_raiseup(&ex, expt);
		result = NULL;
	}
	return result;
	//FUNC_NAME("fdmc_sql_session_popen");
	//FDMC_EXCEPTION x;

	//TRYF(x)
	//{
	//}
	//EXCEPTION
	//{
	//}
}
//
int fdmc_sql_session_close(FDMC_OCI_SESSION *oci_session, FDMC_EXCEPTION *expt)
{
	FUNC_NAME("fdmc_close_oci_session");
	sword code;
	FDMC_EXCEPTION oex;

	if(!oci_session)
	{
		return 0;
	}
	oci_session->sql_error_code = OCI_SUCCESS;
	TRYF(oex)
	{
		code = OCISessionEnd(oci_session->oci_service_handle, oci_session->oci_error_handle,
			oci_session->oci_auth_handle, (ub4) OCI_DEFAULT);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIServerDetach(oci_session->oci_server_handle, oci_session->oci_error_handle, (ub4) OCI_DEFAULT);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_error_handle, (ub4) OCI_HTYPE_ERROR);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_server_handle, (ub4) OCI_HTYPE_SERVER);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_service_handle, (ub4) OCI_HTYPE_SVCCTX);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_auth_handle, (ub4) OCI_HTYPE_SESSION);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_env_handle, (ub4) OCI_HTYPE_ENV);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}
	}
	EXCEPTION
	{
		if(expt != NULL)
		{
			fdmc_raisef(FDMC_DB_ERROR, expt, "%s", oex.errorcode);
		}
		return 0;
	}
	return 1;
}

int fdmc_sql_session_pclose(FDMC_OCI_SESSION *oci_session, FDMC_EXCEPTION *expt)
{
	FUNC_NAME("fdmc_sql_session_pclose");
	FDMC_EXCEPTION oex;
	sword code;

	TRYF(oex)
	{
		oci_session->sql_error_code = OCI_SUCCESS;
		code = OCISessionEnd(oci_session->oci_service_handle, oci_session->oci_error_handle,
			oci_session->oci_auth_handle, (ub4) OCI_DEFAULT);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIServerDetach(oci_session->oci_server_handle, oci_session->oci_error_handle, (ub4) OCI_DEFAULT);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_error_handle, (ub4) OCI_HTYPE_ERROR);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_server_handle, (ub4) OCI_HTYPE_SERVER);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_service_handle, (ub4) OCI_HTYPE_SVCCTX);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}

		code = OCIHandleFree((dvoid *) oci_session->oci_auth_handle, (ub4) OCI_HTYPE_SESSION);
		if(code && expt)
		{
			fdmc_oci_error(oci_session, code, &oex);
		}
	}
	EXCEPTION
	{
		if(expt != NULL)
		{
			fdmc_raisef(FDMC_DB_ERROR, expt, "%s", oex.errorcode);
		}
		return 0;
	}
	return 1;
}

int fdmc_sql_error(FDMC_OCI_SESSION *fs, sword code, FDMC_EXCEPTION *expt)
{
	text errbuf[2048];
	sb4 errcode = 0;

	fs->sql_error_code = code;
	fs->db_error_code = 0;
	switch (code)
	{
	case OCI_SUCCESS_WITH_INFO:
		if(fs->oci_error_handle == NULL)
		{
			trace("OCI Success with info");
		}
		else
		{
			OCIErrorGet((dvoid *)fs->oci_error_handle, (ub4) 1, (text *) NULL, &errcode,
				errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
			fs->db_error_code = errcode;
			trace("OCI Warning %d:\n%s", errcode, errbuf);
		}
		//fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Success with info");
		break;
	case OCI_NEED_DATA:
		fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error (need data)");
		break;
	case OCI_NO_DATA:
		fs->db_error_code = OCI_NO_DATA;
		fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error (no data)");
		break;
	case OCI_ERROR:
		if(fs->oci_error_handle == NULL)
		{
			fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error (OCI ERROR)");
		}
		else
		{
			OCIErrorGet((dvoid *)fs->oci_error_handle, (ub4) 1, (text *) NULL, &errcode,
				errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
			fs->db_error_code = errcode;
			fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error %d:\n%s", errcode, errbuf);
		}
		break;
	case OCI_INVALID_HANDLE:
		fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error (Invalid Handle)");
		break;
	case OCI_STILL_EXECUTING:
		fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error (Still Execute)");
		break;
	case OCI_CONTINUE:
		fdmc_raisef(FDMC_DB_ERROR, expt, "OCI Error (OCI Continue)");
		break;
	default:
		fdmc_raisef(FDMC_DB_ERROR, expt, "OCI unhandled error (%d)", code);
		break;
	}
	return 0;
}

FDMC_OCI_STATEMENT *fdmc_sql_prepare(
	FDMC_OCI_SESSION *oci_session, char *stmt_text, int stmt_len, 
	FDMC_OCI_BIND *oci_bind, FDMC_OCI_DEFINE *oci_define, FDMC_EXCEPTION *expt)
{
	FUNC_NAME("fdmc_create_oci_statement");
	FDMC_OCI_STATEMENT *result;
	FDMC_EXCEPTION err;
	sword ret;

	oci_session->sql_error_code = OCI_SUCCESS;
	TRYF(err)
	{
		// Initialization
		result = fdmc_malloc(sizeof(*result), &err);
		//memset(result, 0, sizeof(*result));
		result->oci_session = oci_session;
		// Preparing request
		ret = OCIHandleAlloc((void*)oci_session->oci_env_handle, (void**)&result->oci_statement, OCI_HTYPE_STMT, 0, 0);
		if(ret != OCI_SUCCESS)
		{
			fdmc_oci_error(oci_session, ret, &err);
		}
		ret = OCIStmtPrepare(result->oci_statement, result->oci_session->oci_error_handle, (OraText*)stmt_text,
			(ub4) stmt_len,
			(ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
		if(ret != OCI_SUCCESS)
		{
			fdmc_oci_error(oci_session, ret, &err);
		}
		if(oci_bind != NULL)
		{
			fdmc_stmt_bind(result, oci_bind, &err);
			result->oci_bind = oci_bind;
		}
		if(oci_define != NULL)
		{
			fdmc_stmt_define(result, oci_define, &err);
			result->oci_define = oci_define;
		}
	}
	EXCEPTION
	{
		fdmc_sql_close(result, NULL);
		fdmc_free(result, NULL);
		result = NULL;
		fdmc_raisef(err.errorcode, expt, "%s", err.errortext);
	}
	return result;
}


int fdmc_stmt_bind(FDMC_OCI_STATEMENT *stmt, FDMC_OCI_BIND *oci_bind, FDMC_EXCEPTION *er)
{
	//FUNC_NAME("fmdc_stmt_bind");
	int indx;
	sword ret;

	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	for(indx = 0; oci_bind[indx].oci_bind_address; indx++)
	{
		if(oci_bind[indx].name[0] != 0)
		{
			// Bind name was found, bind by name
			ret = OCIBindByName(stmt->oci_statement, &oci_bind[indx].oci_bind_handle, 
				stmt->oci_session->oci_error_handle, (const OraText*)oci_bind[indx].name,
				(sb4)strlen(oci_bind[indx].name), oci_bind[indx].oci_bind_address,
				oci_bind[indx].oci_bind_length, (ub2)oci_bind[indx].oci_bind_type,
				/*&oci_bind[indx].nullind*/NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
			if(ret != OCI_SUCCESS)
			{
				fdmc_oci_error(stmt->oci_session, ret, er);
			}
		}
		else
		{
			// Bind by index, because name was not found
			ret = OCIBindByPos(stmt->oci_statement, &oci_bind[indx].oci_bind_handle, 
				stmt->oci_session->oci_error_handle, indx+1, oci_bind[indx].oci_bind_address,
				oci_bind[indx].oci_bind_length, (ub4)oci_bind[indx].oci_bind_type, 
				&oci_bind[indx].nullind, NULL, NULL,
				(ub4)0, NULL, (ub4) OCI_DEFAULT);
			if(ret != OCI_SUCCESS)
			{
				fdmc_oci_error(stmt->oci_session, ret, er);
			}
		}
	}
	return 1;
}


int fdmc_stmt_bind_byname(FDMC_OCI_STATEMENT *stmt, FDMC_OCI_BIND_BYNAME *oci_bind, 
	FDMC_EXCEPTION *er)
{
	//FUNC_NAME("fmdc_stmt_bind_byname");
	int indx;
	sword ret;

	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	for(indx = 0; oci_bind[indx].oci_bind_address; indx++)
	{
		ret = OCIBindByName(stmt->oci_statement, &oci_bind[indx].oci_bind_handle, 
			stmt->oci_session->oci_error_handle, (const OraText*)oci_bind[indx].bind_name,
			(sb4)strlen(oci_bind[indx].bind_name), oci_bind[indx].oci_bind_address,
			oci_bind[indx].oci_bind_length, (ub2)oci_bind[indx].oci_bind_type,
			NULL, NULL,NULL, 0, NULL, OCI_DEFAULT);
		if(ret != OCI_SUCCESS)
		{
			fdmc_oci_error(stmt->oci_session, ret, er);
		}
	}
	return 1;
}

int fdmc_stmt_define(FDMC_OCI_STATEMENT *stmt, FDMC_OCI_DEFINE *oci_define, FDMC_EXCEPTION *er)
{
	//FUNC_NAME("fmdc_stmt_define");
	int indx;
	sword ret;

	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	for(indx = 0; oci_define[indx].oci_define_address; indx++)
	{
		ret = OCIDefineByPos(stmt->oci_statement, &oci_define[indx].oci_define_handle, 
			stmt->oci_session->oci_error_handle, indx+1, oci_define[indx].oci_define_address,
			oci_define[indx].oci_define_length, (ub4)oci_define[indx].oci_define_type, 
			&oci_define[indx].nullind, NULL, NULL, (ub4) OCI_DEFAULT);
		if(ret != OCI_SUCCESS)
		{
			fdmc_oci_error(stmt->oci_session, ret, er);
		}
	}
	return 1;
}

int fdmc_sql_exec(FDMC_OCI_STATEMENT *stmt, FDMC_EXCEPTION *expt)
{
	FUNC_NAME("fdmc_sql_exec");
	FDMC_EXCEPTION err;
	sword stat;

	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	TRYF(err)
	{
		stat = OCIStmtExecute(
			stmt->oci_session->oci_service_handle, 
			stmt->oci_statement,
			stmt->oci_session->oci_error_handle,
			(ub4)1, (ub4)0, NULL, NULL, (ub4) OCI_DEFAULT);
		if(stat != OCI_SUCCESS)
		{
			fdmc_oci_error(stmt->oci_session, stat, &err);
			return 0;
		}
	}
	EXCEPTION
	{
		fdmc_raisef(FDMC_DB_ERROR, expt, "%s", err.errortext);
		return 0;
	}
	return 1;
}

int fdmc_sql_open(FDMC_OCI_STATEMENT *stmt, FDMC_EXCEPTION *expt)
{
	FUNC_NAME("fdmc_sql_open");
	FDMC_EXCEPTION err;
	sword stat;

	//dbg_print();

	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	TRYF(err)
	{
		stat = OCIStmtExecute(
			stmt->oci_session->oci_service_handle,
			stmt->oci_statement,
			stmt->oci_session->oci_error_handle,
			(ub4)0, (ub4)0, NULL, NULL, (ub4) OCI_DEFAULT); return 1;
		if(stat != OCI_SUCCESS)
		{
			fdmc_oci_error(stmt->oci_session, stat, &err);
			return 0;
		}
	}
	EXCEPTION
	{
		fdmc_raisef(FDMC_DB_ERROR, expt, "%s", err.errortext);
		return 0;
	}
	return 1;
}

int fdmc_sql_fetch(FDMC_OCI_STATEMENT *stmt, FDMC_EXCEPTION *expt)
{
	//FUNC_NAME("fdmc_sql_fetch");
	sword stat;

	//dbg_print();
	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	stat = OCIStmtFetch(
		stmt->oci_statement, 
		stmt->oci_session->oci_error_handle, 
		(ub4)1, (ub2)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT);
	if(stat != OCI_SUCCESS)
	{
		fdmc_oci_error(stmt->oci_session, stat, expt);
		return 0;
	}
	return 1;
}

int fdmc_sql_close(FDMC_OCI_STATEMENT *stmt, FDMC_EXCEPTION *expt)
{
	//FUNC_NAME("fdmc_sql_close");
	sword stat;

	if(stmt == NULL) return 0;
	stmt->oci_session->sql_error_code = OCI_SUCCESS;
	stat = OCIHandleFree(stmt->oci_statement, OCI_HTYPE_STMT);
	if(stat != OCI_SUCCESS && expt)
	{
		fdmc_oci_error(stmt->oci_session, stat, expt);
		return 0;
	}
	return 1;
}

FDMC_OCI_BIND *fdmc_clone_ocibind(FDMC_OCI_BIND *pattern, void *host_addr)
{
	FUNC_NAME("fdmc_clone_ocibind");
	FDMC_EXCEPTION ex;
	int i, asize, shift;
	FDMC_OCI_BIND *ret;

	i = 0;
	while(pattern[i].oci_bind_address)
	{
		i++;
	}
	asize = sizeof(FDMC_OCI_BIND) * (i + 1);
	TRYF(ex)
	{
		ret = fdmc_malloc(asize, &ex);
		memcpy(ret, pattern, asize);
		ret[0].oci_bind_address = host_addr;
		for(i = 0; pattern[i].oci_bind_address; i++)
		{
			shift = (int)((char*)pattern[i].oci_bind_address - 
				(char*)pattern[0].oci_bind_address);
			ret[i].oci_bind_address = (char*)ret[0].oci_bind_address + shift;
		}
	}
	EXCEPTION
	{
		ret = NULL;
	}
	return ret;
}

FDMC_OCI_DEFINE *fdmc_clone_ocidefine(FDMC_OCI_DEFINE *pattern, void *host_addr)
{
	FUNC_NAME("fdmc_clone_ocibind");
	FDMC_EXCEPTION ex;
	int i, asize, shift;
	FDMC_OCI_DEFINE *ret;

	i = 0;
	while(pattern[i].oci_define_address)
	{
		i++;
	}
	asize = sizeof(FDMC_OCI_BIND) * (i + 1);
	TRYF(ex)
	{
		ret = fdmc_malloc(asize, &ex);
		memcpy(ret, pattern, asize);
		ret[0].oci_define_address = host_addr;
		for(i = 0; pattern[i].oci_define_address; i++)
		{
			shift = (int)((char*)pattern[i].oci_define_address - 
				(char*)pattern[0].oci_define_address);
			ret[i].oci_define_address = (char*)ret[0].oci_define_address + shift;
		}
	}
	EXCEPTION
	{
		ret = NULL;
	}
	return ret;
}

int fdmc_sql_commit(FDMC_SQL_SESSION *fs, FDMC_EXCEPTION *err)
{
	//FUNC_NAME("fdmc_sql_commit");
	sword res;

	res = OCITransCommit(fs->oci_service_handle, fs->oci_error_handle, 0);
	if(res != OCI_SUCCESS)
	{
		fdmc_sql_error(fs, res, err);
		return 0;
	}
	return 1;
}

int fdmc_sql_rollback(FDMC_SQL_SESSION *fs, FDMC_EXCEPTION *err)
{
	//FUNC_NAME("fdmc_sql_rollback");
	sword res;
	res = OCITransRollback(fs->oci_service_handle, fs->oci_error_handle, 0);
	if(res != OCI_SUCCESS)
	{
		fdmc_sql_error(fs, res, err);
		return 0;
	}
	return 1;
}




//FDMC_SQL_SESSION *fdmc_sql_session_pcreate(FDMC_SQL_CPOOL *p_pool, FDMC_EXCEPTION *expt)
//{
//	FUNC_NAME("fdmc_sql_session_pceate");
//	FDMC_OCI_SESSION *result;
//	FDMC_EXCEPTION ex, ex1;
//	sword code;
//
//	TRYF(ex)
//	{
//		result = (FDMC_OCI_SESSION*)fdmc_malloc(sizeof(FDMC_OCI_SESSION), &ex);
//		memset(result, 0, sizeof(*result));
//		result->oci_env_handle = p_pool->oci_env_handle;
//		//code = 0; 
//		ex1.errortext[0] = 0;
//
//		// Create error handler
//		TRY(ex1)
//		{
//			if ((code = OCIHandleAlloc((dvoid *) p_pool->oci_env_handle, (dvoid **) &result->oci_error_handle,
//				(ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
//			{
//				fdmc_oci_error(result, code, &ex1);
//			}
//		}
//		EXCEPTION
//		{
//			fdmc_raisef(ex1.errorcode, &ex, "Alloc error handler:\n%s", ex1.errortext);
//		}
//		TRY(ex1)
//		{
//			// Create service handle
//			if ((code=OCIHandleAlloc((dvoid *) p_pool->oci_env_handle, (dvoid **) &result->oci_service_handle,
//				(ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0)) != OCI_SUCCESS)
//			{
//				fdmc_oci_error(result, code, &ex1);
//			}
//		}
//		EXCEPTION
//		{
//			fdmc_raisef(ex1.errorcode, &ex, "Alloc service handle:\n%s", ex1.errortext); 
//		}
//
//		// Work pool
//
//		if ((code= OCILogon2(p_pool->oci_env_handle, result->oci_error_handle,
//				&result->oci_service_handle, p_pool->login, p_pool->login_len, 
//				p_pool->password, p_pool->password_len, p_pool->pool_name, 
//				p_pool->pool_name_len, OCI_LOGON2_CPOOL))  != OCI_SUCCESS )
//		{
//			TRY(ex1)
//			{
//				fdmc_oci_error(result, code, &ex1);
//			}
//			EXCEPTION
//			{
//				fdmc_raisef(ex1.errorcode, &ex, "OCIServerAttach('%.*s'):\n%s", p_pool->pool_name_len, p_pool->pool_name,
//					ex1.errortext);
//			}
//		}
//	}
//	EXCEPTION
//	{
//		fdmc_sql_session_pclose(result, NULL);
//		if(expt)
//		{
//			expt->errorsubcode = code;
//		}
//		fdmc_raiseup(&ex, expt);
//		result = NULL;
//	}
//	return result;
//}
