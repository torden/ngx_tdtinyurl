/**
 * Torden TinyURL
 *
 * @license : GPLv3, http://www.gnu.org/licenses/gpl.html
 * @author : torden <https://github.com/torden/>
 */
/* $Id$ */

#ifndef PHP_TDTINYURL_H
#define PHP_TDTINYURL_H

extern zend_module_entry tdtinyurl_module_entry;
#define phpext_tdtinyurl_ptr &tdtinyurl_module_entry

#ifdef PHP_WIN32
#	define PHP_TDTINYURL_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_TDTINYURL_API __attribute__ ((visibility("default")))
#else
#	define PHP_TDTINYURL_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define TORDEN_TINYURL_MODULE_VERSION "0.1"

#define TORDEN_TINYURL_API_VERSION 1

#define TORDEN_TINYURL_ID "__TORDEN_TINYURL_ID"
#define TORDEN_TINYURL_PREFIX_GRP_NO "__TORDEN_TINYURL_PREFIX_GRP_NO"
#define TORDEN_TINYURL_INGRP_NO "__TORDEN_TINYURL_INGRP_NO"

#define TORDEN_TINYURL_ID_LEN strlen(TORDEN_TINYURL_ID)
#define TORDEN_TINYURL_PREFIX_GRP_NO_LEN strlen(TORDEN_TINYURL_PREFIX_GRP_NO)+1
#define TORDEN_TINYURL_INGRP_NO_LEN strlen(TORDEN_TINYURL_INGRP_NO)+1

#define TORDEN_TINYURL_MSG_NOT_ENABLE "tdtinyurl module is not enable"
#define TORDEN_TINYURL_MSG_SYSTEM_FAILURE "tdtinyurl module is wrong working"

#define TORDEN_TINYURL_DOCS_HOME "https://bitbucket.org/torden/tdbucket"
#define TORDEN_TINYURL_DOCS2_HOME "https://bitbucket.org/torden/tdbucket/wiki/Home"

#define TORDEN_TINYURL_LOGO_IMG_URL "https://bitbucket-assetroot.s3.amazonaws.com/c/photos/2013/Dec/04/tdtinyurl-logo-3876131587-6_avatar.png"

#define PHP_TDTINYURL_GET_ONLY_KEY 3
#define PHP_TDTINYURL_LIVE         7

#define PHP_TDTINYURL_MODE_SECUER_TUNNELING 1
#define PHP_TDTINYURL_MODE_DEFAULT_REDIRECTION 0

#define TORDEN_TINYURL_REDIS_CON_MODE_UDS 1
#define TORDEN_TINYURL_REDIS_CON_MODE_TCP 2


#define TORDEN_TINYURL_SQL_BEGIN_TRANSACTION 1
#define TORDEN_TINYURL_SQL_COMMIT_TRANSACTION 2
#define TORDEN_TINYURL_SQL_ROLLBACK_TRANSACTION 4

#define TORDEN_TINYURL_SQL_INSET_TDTINYURL "INSERT INTO TDTINYURL (tinyurl,longurl,time,secure,hits) VALUES('%s', '%s', %ld,%ld,%d)"
#define TORDEN_TINYURL_SQL_INSET_TDTINYURL_LOG "INSERT INTO TDTINYURL_LOG (tinyurl,agent,ipaddr) VALUES('%s', '%s', '%s')"
#define TORDEN_TINYURL_SQL_UPDATE_TDTINYURL_HITS "UPDATE TDTINYURL SET hits = hits+1 WHERE tinyurl = '%s'"


PHP_MINIT_FUNCTION(tdtinyurl);
PHP_MSHUTDOWN_FUNCTION(tdtinyurl);
PHP_RINIT_FUNCTION(tdtinyurl);
PHP_RSHUTDOWN_FUNCTION(tdtinyurl);
PHP_MINFO_FUNCTION(tdtinyurl);

PHP_FUNCTION(tdtinyurl_set_url);
PHP_FUNCTION(tdtinyurl_set_rid);
PHP_FUNCTION(tdtinyurl_get_url);
PHP_FUNCTION(tdtinyurl_get_rid);
PHP_FUNCTION(tdtinyurl_get_sqlite3_db_path);

ZEND_BEGIN_MODULE_GLOBALS(tdtinyurl)
    zend_bool enable;
    zend_bool cli_support;
    zend_bool ignore_warning;
    char *redis_connect_mode;
    char *redis_uds_sock_path;
    char *redis_ip;
    char *redis_port;
    char *sqlite3_db_path;
	zend_bool sqlite3_with_mode_enable;
ZEND_END_MODULE_GLOBALS(tdtinyurl)

#ifdef ZTS
# define TINYURLG(v) TSRMG(tdtinyurl_globals_id, zend_tdtinyurl_globals *, v)
#else
# define TINYURLG(v) (tdtinyurl_globals.v)
#endif

#endif	/* PHP_TDTINYURL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
