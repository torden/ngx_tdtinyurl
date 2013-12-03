/**
 * tdtinyurl php extension module
 * @license : GPLv3, http://www.gnu.org/licenses/gpl.html
 * @author : torden <https://github.com/torden/>
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_main.h"
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <uqid.h>
#include <hiredis/hiredis.h>
#include "php_tdtinyurl.h"
#include <mcheck.h>

ZEND_DECLARE_MODULE_GLOBALS(tdtinyurl)

ZEND_BEGIN_ARG_INFO_EX(arginfo_tdtinyurl_set_url, 0, 0, 1)
    ZEND_ARG_INFO(0, url)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tdtinyurl_set_rid, 0, 0, 1)
    ZEND_ARG_INFO(0, tinykey)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tdtinyurl_get_url, 0, 0, 1)
    ZEND_ARG_INFO(0, url)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_tdtinyurl_get_rid, 0, 0, 1)
    ZEND_ARG_INFO(0, url)
ZEND_END_ARG_INFO()

static redisContext *predis_con;
static short redis_conmod = 0;
static short redis_connected = 0;


const zend_function_entry tdtinyurl_functions[] = {
	PHP_FE(tdtinyurl_set_url,    	arginfo_tdtinyurl_set_url)
	PHP_FE(tdtinyurl_set_rid,       arginfo_tdtinyurl_set_rid)
	PHP_FE(tdtinyurl_get_url,    	arginfo_tdtinyurl_get_url)
	PHP_FE(tdtinyurl_get_rid,    	arginfo_tdtinyurl_get_rid)
	PHP_FE_END
};

zend_module_entry tdtinyurl_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"tdtinyurl",
	tdtinyurl_functions,
	PHP_MINIT(tdtinyurl),
	PHP_MSHUTDOWN(tdtinyurl),
	PHP_RINIT(tdtinyurl),		
	PHP_RSHUTDOWN(tdtinyurl),	
	PHP_MINFO(tdtinyurl),
#if ZEND_MODULE_API_NO >= 20010901
	TORDEN_TINYURL_MODULE_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_TDTINYURL
ZEND_GET_MODULE(tdtinyurl)
#endif

static void php_tdtinyurl_init_globals(zend_tdtinyurl_globals* tdtinyurl_globals TSRMLS_DC) {
    //for furture
}

PHP_INI_BEGIN()
STD_PHP_INI_BOOLEAN("tdtinyurl.enable",                          "1", PHP_INI_ALL, OnUpdateBool,   enable,               zend_tdtinyurl_globals, tdtinyurl_globals)
STD_PHP_INI_BOOLEAN("tdtinyurl.cli_support",                     "0", PHP_INI_ALL, OnUpdateBool,   cli_support,          zend_tdtinyurl_globals, tdtinyurl_globals)
STD_PHP_INI_BOOLEAN("tdtinyurl.ignore_warning",                  "0", PHP_INI_ALL, OnUpdateBool,   ignore_warning,       zend_tdtinyurl_globals, tdtinyurl_globals)
STD_PHP_INI_ENTRY("tdtinyurl.redis_connect_mode",              "uds", PHP_INI_ALL, OnUpdateString, redis_connect_mode,   zend_tdtinyurl_globals, tdtinyurl_globals)
STD_PHP_INI_ENTRY("tdtinyurl.redis_uds_sock_path", "/tmp/redis.sock", PHP_INI_ALL, OnUpdateString, redis_uds_sock_path,  zend_tdtinyurl_globals, tdtinyurl_globals)
STD_PHP_INI_ENTRY("tdtinyurl.redis_ip",                  "127.0.0.1", PHP_INI_ALL, OnUpdateString, redis_ip,             zend_tdtinyurl_globals, tdtinyurl_globals)
STD_PHP_INI_ENTRY("tdtinyurl.redis_port",                     "6379", PHP_INI_ALL, OnUpdateString, redis_port,           zend_tdtinyurl_globals, tdtinyurl_globals)
PHP_INI_END()

PHP_MINIT_FUNCTION(tdtinyurl)
{
    ZEND_INIT_MODULE_GLOBALS(tdtinyurl, php_tdtinyurl_init_globals, NULL);
    REGISTER_INI_ENTRIES();

/*
    if(0 != strcmp(sapi_module.name, "cli")) {
        TINYURLG(enable) = 0;
    } 
*/
    REGISTER_LONG_CONSTANT("TDTINYURL_GET_ONLY_KEY", PHP_TDTINYURL_GET_ONLY_KEY, CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("TDTINYURL_LIVE", PHP_TDTINYURL_LIVE, CONST_CS|CONST_PERSISTENT);
    
    REGISTER_LONG_CONSTANT("TDTINYURL_MODE_DEFAULT_REDIRECTION", PHP_TDTINYURL_MODE_DEFAULT_REDIRECTION, CONST_CS|CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("TDTINYURL_MODE_SECUER_TUNNELING", PHP_TDTINYURL_MODE_SECUER_TUNNELING, CONST_CS|CONST_PERSISTENT);

	return SUCCESS;
}
PHP_MSHUTDOWN_FUNCTION(tdtinyurl)
{
    UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
PHP_RINIT_FUNCTION(tdtinyurl)
{
	return SUCCESS;
}
PHP_RSHUTDOWN_FUNCTION(tdtinyurl)
{
/*
    efree(TINYURLG(redis_connect_mode));
    efree(TINYURLG(redis_uds_sock_path));
    efree(TINYURLG(redis_ip));
    efree(TINYURLG(redis_port));
*/
	return SUCCESS;
}

static int php_info_print(const char *str) {
    TSRMLS_FETCH();
    return php_output_write(str, strlen(str) TSRMLS_CC);
}

static int str_prefix_compare(const char *pstr1 , const char *pstr2) {

    while (*pstr1 == *pstr2++) {
        if (*pstr1 == 0 || *pstr1++ == 0 || *pstr2 == 0) {
            return 0;
        }

        if(*pstr1 != *pstr2 && *pstr1 == 0) {
            return 0;
        }
    }
    return -1;
}

static void tiny_url_redis_close(void) {

    redis_connected = 0;
    redisFree(predis_con);
}

static int tiny_url_redis_connect(const char *redis_connect_mode, const char *redis_uds_sock_path, const char *redis_ip, int redis_port) {

    //timeout 1.5
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;

    if(0 == redis_connected) {

        //UDS connection
        if(0 == strcmp(redis_connect_mode,"uds")) {
            redis_conmod = TORDEN_TINYURL_REDIS_CON_MODE_UDS;
            predis_con = redisConnectUnixWithTimeout(redis_uds_sock_path, timeout);
        } else { //TCP connectiong
            redis_conmod = TORDEN_TINYURL_REDIS_CON_MODE_TCP;
            predis_con = redisConnectWithTimeout(redis_ip, redis_port, timeout);
        }

        //check redis connection status
        if(NULL == predis_con || predis_con->err) {
            if(predis_con) {
                tiny_url_redis_close();
            }
            return -1;
        } else {

            redis_connected = 1;
            return 1;
        }
    } else {
        return 1;
    }
}

PHP_MINFO_FUNCTION(tdtinyurl)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "tdtinyurl support", "enable");
    php_info_print_table_row(2, "Development", "torden");
    php_info_print_table_row(2, "Version", TORDEN_TINYURL_MODULE_VERSION);
    php_info_print_table_row(2, "Main Doc Home", TORDEN_TINYURL_DOCS_HOME);
    php_info_print_table_row(2, "Sub Doc Home", TORDEN_TINYURL_DOCS2_HOME);
	php_info_print_table_end();
	php_info_print_hr();
    php_info_print_box_start(0);
    php_info_print("<center><h1>Torden's TinyURL Module!!</h1></center><br /><img src='");
    php_info_print(TORDEN_TINYURL_LOGO_IMG_URL);
    php_info_print("' style='float:none'>");
    php_info_print_box_end();

	DISPLAY_INI_ENTRIES();
}

PHP_FUNCTION(tdtinyurl_set_url)
{
    unsigned char *purl = NULL;
    int url_len = 0;
    long int seed_num = 0;
    char *ptdtinyurl_key = NULL;
    long working_mode = PHP_TDTINYURL_LIVE;
    long action_mode = -1;
    long expire_time_sec = 0;
    char nowtime[12] = {0x00,};
    char *pnowtime = nowtime;
    struct timeval current_time;
    redisReply *predis_reply;
    
    if(!TINYURLG(enable)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, TORDEN_TINYURL_MSG_NOT_ENABLE);
        RETURN_FALSE;
    }

    if(FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|ll", &purl, &url_len, &action_mode, &expire_time_sec, &working_mode)) {
        RETURN_FALSE;
    }

    if(1 > url_len ) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects at least 1 parameter, 0 given");
        RETURN_FALSE;
    }


    if(-1 == str_prefix_compare("http://", purl) && -1 == str_prefix_compare("https://", purl)) {

        php_error_docref(NULL TSRMLS_CC, E_WARNING, "wrong url format, only support http and https");
        RETURN_FALSE;
    }


    if(PHP_TDTINYURL_MODE_DEFAULT_REDIRECTION != action_mode && PHP_TDTINYURL_MODE_SECUER_TUNNELING != action_mode) {

        php_error_docref(NULL TSRMLS_CC, E_WARNING, "second parameteris wrong(%d), please use the TDTINYURL_MODE_DEFAULT_REDIRECTION or TDTINYURL_MODE_SECUER_TUNNELING ", action_mode);
        RETURN_FALSE;
    }

    if(0 > expire_time_sec) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects at least 3 parameter, 0 given or minus given (%ld)", expire_time_sec);
        RETURN_FALSE;
    }


    //get uqid generating
    ptdtinyurl_key = (char *)safe_emalloc(sizeof(char), UQID_MALLOC_LEN, 1);
    seed_num = uqid_get_str_to_number(purl);
    if(NULL == uqid_get_str(ptdtinyurl_key, UQID_MAX_LEN, seed_num)) {

        efree(ptdtinyurl_key);
        php_error_docref(NULL TSRMLS_CC, E_WARNING, TORDEN_TINYURL_MSG_SYSTEM_FAILURE);
        RETURN_FALSE;
    }
    
    switch(working_mode) {

        case PHP_TDTINYURL_LIVE:

            if(-1 == tiny_url_redis_connect(TINYURLG(redis_connect_mode), TINYURLG(redis_uds_sock_path), TINYURLG(redis_ip), atoi(TINYURLG(redis_port)))) {

                efree(ptdtinyurl_key);
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "redis connection fail");
                RETURN_FALSE;
            }

            //get generating time
            gettimeofday(&current_time, NULL);
            snprintf(pnowtime, 11, "%ld", current_time.tv_sec);

            //make redis hsetnx command string
            predis_reply = redisCommand(predis_con, "HSETNX %s time %s", ptdtinyurl_key, pnowtime);
            if(NULL == predis_reply || 1 != predis_reply->integer) {
                freeReplyObject(predis_reply);
                efree(ptdtinyurl_key);
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "redis error : can not insert");
                tiny_url_redis_close();
                RETURN_FALSE;
            } else {

                freeReplyObject(predis_reply);

                //overwrite more data
                predis_reply = redisCommand(predis_con, "HMSET %s time %s rid -1 url %s api %d secure %d", ptdtinyurl_key, pnowtime, purl, TORDEN_TINYURL_API_VERSION, action_mode);
                if(REDIS_REPLY_STATUS != predis_reply->type) {

                    freeReplyObject(predis_reply);
                    predis_reply = redisCommand(predis_con, "DEL %s", ptdtinyurl_key);
                    efree(ptdtinyurl_key);
                    php_error_docref(NULL TSRMLS_CC, E_WARNING, "redis HMSET command error : can not insert tdtinyurl (%s)", ptdtinyurl_key);
                    freeReplyObject(predis_reply);
                    tiny_url_redis_close();
                    RETURN_FALSE;
                }

                freeReplyObject(predis_reply);

                //set expire time
                if(0 < expire_time_sec) {

                    predis_reply = redisCommand(predis_con, "EXPIRE %s %ld", ptdtinyurl_key, expire_time_sec);

                    if(REDIS_REPLY_INTEGER != predis_reply->type && 1 != predis_reply->integer) {

                        //rollback
                        freeReplyObject(predis_reply);
                        predis_reply = redisCommand(predis_con, "DEL %s", ptdtinyurl_key);
                        efree(ptdtinyurl_key);
                        freeReplyObject(predis_reply);
                        php_error_docref(NULL TSRMLS_CC, E_WARNING, "redis EXPIRE command error : can not insert tdtinyurl (%s)", ptdtinyurl_key);
                        tiny_url_redis_close();
                        RETURN_FALSE;
                    }

                    freeReplyObject(predis_reply);
                }
            }
            
            break;

        case PHP_TDTINYURL_GET_ONLY_KEY:
        default:
            break;
    }


    tiny_url_redis_close();
    RETURN_STRING(ptdtinyurl_key,0);
    efree(ptdtinyurl_key);
}

PHP_FUNCTION(tdtinyurl_get_url)
{
    unsigned char *ptinykey = NULL;
    int tinykey_len = 0;
    redisReply *predis_reply;

    if(!TINYURLG(enable)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, TORDEN_TINYURL_MSG_NOT_ENABLE);
        RETURN_FALSE;
    }

    if(FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ptinykey, &tinykey_len)) {
        return;
    }

    if(1 > tinykey_len) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects at least 1 parameter, 0 given");
        RETURN_FALSE;
    }


    if(-1 == tiny_url_redis_connect(TINYURLG(redis_connect_mode), TINYURLG(redis_uds_sock_path), TINYURLG(redis_ip), atoi(TINYURLG(redis_port)))) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "redis connection fail");
        RETURN_FALSE;
    }

    //make redis hsetnx command string
    predis_reply = redisCommand(predis_con, "HGET %s url", ptinykey);
    if(NULL == predis_reply->str && REDIS_REPLY_STRING != predis_reply->type) {
        freeReplyObject(predis_reply);
        tiny_url_redis_close();
        RETURN_FALSE;
    } else {

        tiny_url_redis_close();
        RETURN_STRING(predis_reply->str,1);
        freeReplyObject(predis_reply);
    }
}
 
PHP_FUNCTION(tdtinyurl_get_rid)
{
    unsigned char *ptinykey = NULL;
    int tinykey_len = 0;
    redisReply *predis_reply;

    if(!TINYURLG(enable)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, TORDEN_TINYURL_MSG_NOT_ENABLE);
        RETURN_FALSE;
    }

    if(FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ptinykey, &tinykey_len)) {
        return;
    }

    if(1 > tinykey_len) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects at least 1 parameter, 0 given");
        RETURN_FALSE;
    }


    if(-1 == tiny_url_redis_connect(TINYURLG(redis_connect_mode), TINYURLG(redis_uds_sock_path), TINYURLG(redis_ip), atoi(TINYURLG(redis_port)))) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "redis connection fail");
        RETURN_FALSE;
    }

    //make redis hsetnx command string
    predis_reply = redisCommand(predis_con, "HGET %s rid", ptinykey);
    if(NULL == predis_reply->str && REDIS_REPLY_STRING != predis_reply->type) {

        freeReplyObject(predis_reply);
        tiny_url_redis_close();
        RETURN_FALSE;
    } else {
        tiny_url_redis_close();
        RETURN_STRING(predis_reply->str,1);
        freeReplyObject(predis_reply);
    }
}
 
PHP_FUNCTION(tdtinyurl_set_rid)
{
    unsigned char *ptinykey = NULL;
    int tinykey_len = 0;
    long rid = 0;
    redisReply *predis_reply;

    if(!TINYURLG(enable)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, TORDEN_TINYURL_MSG_NOT_ENABLE);
        RETURN_FALSE;
    }

    if(FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &ptinykey, &tinykey_len, &rid)) {
        return;
    }

    if(1 > tinykey_len) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "expects at least 1 parameter, 0 given");
        RETURN_FALSE;
    }


    if(-1 == tiny_url_redis_connect(TINYURLG(redis_connect_mode), TINYURLG(redis_uds_sock_path), TINYURLG(redis_ip), atoi(TINYURLG(redis_port)))) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "redis connection fail");
        RETURN_FALSE;
    }

    predis_reply = redisCommand(predis_con, "HEXISTS %s rid", ptinykey);
    if(NULL == predis_reply || 1 != predis_reply->integer) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "not exists %s tiny key in redis", ptinykey);
        freeReplyObject(predis_reply);
        tiny_url_redis_close();
        RETURN_FALSE;
    } else {
        predis_reply = redisCommand(predis_con, "HSET %s rid %d", ptinykey, rid);
        if(NULL == predis_reply && REDIS_REPLY_STATUS != predis_reply->type) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "can not update rid about %s", ptinykey);

            freeReplyObject(predis_reply);
            tiny_url_redis_close();
            RETURN_FALSE;
        } else {

            freeReplyObject(predis_reply);
            tiny_url_redis_close();
            RETURN_TRUE;
        }
    }
}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
