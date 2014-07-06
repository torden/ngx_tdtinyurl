/**
 * Torden TinyURL
 *
 * @license : GPLv3, http://www.gnu.org/licenses/gpl.html
 * @author : torden <https://github.com/torden/>
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_crypt.h> 
#include <ngx_string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include "hiredis.h"
#include <kclangc.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <sqlite3.h>

// CONFIG : enable
#define NGX_TDTINYURL_OFF     0
#define NGX_TDTINYURL_ON      1
#define NGX_TDTINYURL_ALWAYS  2

// CONFIG : Working Mode
#define NGX_TDTINYURL_WORKING_MODE_LR 1
#define NGX_TDTINYURL_WORKING_MODE_RL 2

// CONFIG : redis connection mode
#define NGX_TDTINYURL_REDIS_CON_MODE_UDS 1
#define NGX_TDTINYURL_REDIS_CON_MODE_TCP 2

// CONFIG : defailt redis connection options
#define NGX_TDTINYURL_DEFAULT_REDIS_CON_MODE NGX_TDTINYURL_REDIS_CON_MODE_UDS //UDS
#define NGX_TDTINYURL_DEFAULT_REDIS_UDS_SOCK_PATH "/tmp/redis.sock" //Socket Path
#define NGX_TDTINYURL_DEFAULT_REDIS_TCP_IP_ADDT "127.0.0.1" //Localhost
#define NGX_TDTINYURL_DEFAULT_REDIS_PORT "6378" //Port

// GLOBAL DEFAULT SIZE
#define NGX_TDTINYURL_DEF_BUFSIZE 1024 * 256 //256k
#define NGX_TDTINYURL_CURL_RETBUFSIZE 1024 * 8 //8k
#define NGX_TDTINYURL_COOKIEVAL_SIZE 1024 //1k
#define NGX_TDTINYURL_URI_SIZE 8086 //8k , NOT USED

// URL Max Lenth
// http://www.w3.org/Protocols/rfc2616/rfc2616.html
#define NGX_TDTINYURL_URL_MAX_LEN 2048 

// TinyURL Key Max Length
#define NGX_TDTINYURL_KEY_MAX_LEN 128

// GLOBAL return value
#define NGX_TDTINYURL_RETCODE_OK    7
#define NGX_TDTINYURL_RETCODE_ALLOW 7
#define NGX_TDTINYURL_RETCODE_DENY 4

// HTTP User-Agent String
#define NGX_TDTINYURL_NAME "tdtinyurl"
#define NGX_TDTINYURL_VERSION "0.1"
#define NGX_TDTINYURL_AGENT NGX_TDTINYURL_NAME "/" NGX_TDTINYURL_VERSION

// Redirection HTML
#define NGX_TINYUTL_REDIRECT_BOXY_FIRST_TEMPL "<html><head><title>" NGX_TDTINYURL_NAME "</title></head><body><a href='"
#define NGX_TINYUTL_REDIRECT_BOXY_LAST_TEMPL "'>moved here</a></body></html>"

// fprintf macro
#define _DEBUGPRINT(...) { fprintf(stderr, "\t(%s:%d in %s): ", __FUNCTION__, __LINE__,__FILE__); fprintf(stderr, __VA_ARGS__); fflush(stderr); }

// SQLite SQL Query
#define NGX_TINYURL_SQL_UPDATE_TDTINYURL_HITS "UPDATE TDTINYURL SET hits = hits+1 WHERE tinyurl = '%s'"
#define NGX_TINYURL_SQL_INSET_TDTINYURL_LOG "INSERT INTO TDTINYURL_LOG (tinyurl,agent,ipaddr, referer) VALUES('%s', '%s', '%s', '%s')"

typedef struct {
    ngx_uint_t  enable;
    ngx_uint_t  redis_connnect_mode;
    u_char      *redis_uds_path;
    u_char      *redis_ip_addr;
    ngx_uint_t  redis_port;
    u_char      *work_mode;
    ngx_uint_t  work_mode_no;
    u_char      *dbm_path;
	ngx_uint_t  sqlite3_with_mode;
	u_char		*sqlite3_db_path;
} ngx_http_tdtinyurl_conf_t;

struct string {
  char *ptr;
  size_t len;
};

static ngx_http_request_t *global_r;
static ngx_int_t ngx_http_tdtinyurl_handler(ngx_http_request_t *r);
static void *ngx_http_tdtinyurl_create_conf(ngx_conf_t *cf);
static char *ngx_http_tdtinyurl_merge_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_tdtinyurl_init(ngx_conf_t *cf);
static char *ngx_http_conf_set_redis_uds_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_conf_set_redis_ip_addr(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_conf_set_tdtinyurl_work_mode(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_conf_set_tdtinyurl_dbm_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_conf_set_tdtinyurl_sqlite3_db_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static unsigned short ngx_http_tdtinyurl_check_vaild_ipaddr(u_char *ipaddr);
static int ngx_http_tdtinyurl_get_last_chr(u_char *pstr, unsigned int findchr);
static ngx_int_t ngx_http_tdtinyurl_redirection(ngx_http_request_t *r, u_char *predirecturl);
static ngx_int_t ngx_http_tdtinyurl_init_string(struct string *pstr, unsigned int defSize );
static void ngx_http_tdtinyurl_destroy_string(struct string *pstr);
static ngx_int_t ngx_http_tdtinyurl_printfout(ngx_http_request_t *r, struct string *pcurlretbuf, ngx_int_t httpstatus);
static size_t ngx_http_tdtinyurl_writefunc(void *ptr, size_t size, size_t nmemb, void *s);
static ngx_int_t ngx_http_tdtinyurl_redis_handler(ngx_http_request_t *r, ngx_http_tdtinyurl_conf_t *conf, u_char *ptdtinyurl_key, int ptdtinyurl_key_len);
static ngx_int_t ngx_http_tdtinyurl_dbm_handler(ngx_http_request_t *r, ngx_http_tdtinyurl_conf_t *conf, u_char *ptdtinyurl_key, int ptdtinyurl_key_len);
static ngx_int_t ngx_http_tdtinyurl_curl_client(ngx_http_request_t *r, const char *pcallurl, char *ppostdata, struct string *pcurlretbuf);
static ngx_int_t ngx_http_tdtinyurl_oldmalloc_to_ngxpool(ngx_http_request_t *r, struct string *pstr );

int ngx_http_tdtinyurl_mkpath(char *s);
int ngx_http_tdtinyurl_sqlite3_connect(ngx_log_t *log, sqlite3 **pdb, const char *ppath);
int ngx_http_tdtinyurl_sqlite3_close(ngx_log_t *log, sqlite3 *pdb);
int ngx_http_tdtinyurl_sqlite3_quick_exec(ngx_log_t *log, const char *ppath, const char *psql);
int ngx_http_tdtinyurl_sqlite3_quick_exists_table(ngx_log_t *log, const char *ppath, const char *ptablename);

//conf : tdtinyurl_enable
static ngx_conf_enum_t  ngx_tdtinyurl_enable_bounds[] = {
    { ngx_string("off"),        NGX_TDTINYURL_OFF },
    { ngx_string("on"),         NGX_TDTINYURL_ON },
    { ngx_string("always"),     NGX_TDTINYURL_ALWAYS },
    { ngx_null_string, 0 }
};

//conf : tdtinyurl_redis_connect_mode
static ngx_conf_enum_t ngx_http_tdtinyurl_redis_connnect_mode[] = {
    { ngx_string("uds"),   NGX_TDTINYURL_REDIS_CON_MODE_UDS},
    { ngx_string("tcp"),   NGX_TDTINYURL_REDIS_CON_MODE_TCP},
    { ngx_null_string, 0 }
};

//conf : tdtinyurl_redis_port
static ngx_conf_num_bounds_t  ngx_http_tdtinyurl_redis_port_bounds = {
    ngx_conf_check_num_bounds, 1, USHRT_MAX
};

//conf : tdtinyurl_sqlite3_with_mode
static ngx_conf_enum_t  ngx_tdtinyurl_sqlite3_with_mode_bounds[] = {
    { ngx_string("off"),        NGX_TDTINYURL_OFF },
    { ngx_string("on"),         NGX_TDTINYURL_ON },
    { ngx_null_string, 0 }
};

//init. tdtinyurl config directive
static ngx_command_t  ngx_http_tdtinyurl_commands[] = {

    { ngx_string("tdtinyurl"),
        NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_enum_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_tdtinyurl_conf_t, enable),
        &ngx_tdtinyurl_enable_bounds },

    { ngx_string("tdtinyurl_redis_connnect_mode"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_enum_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_tdtinyurl_conf_t, redis_connnect_mode),
        &ngx_http_tdtinyurl_redis_connnect_mode },

    { ngx_string("tdtinyurl_redis_uds_path"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_conf_set_redis_uds_path,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    { ngx_string("tdtinyurl_redis_ip_addr"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_conf_set_redis_ip_addr,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    { ngx_string("tdtinyurl_work_mode"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_conf_set_tdtinyurl_work_mode,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    { ngx_string("tdtinyurl_dbm_path"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_conf_set_tdtinyurl_dbm_path,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    { ngx_string("tdtinyurl_redis_port"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_tdtinyurl_conf_t, redis_port),
        &ngx_http_tdtinyurl_redis_port_bounds },

    { ngx_string("tdtinyurl_sqlite3_with_mode"),
        NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_enum_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_tdtinyurl_conf_t, sqlite3_with_mode),
        &ngx_tdtinyurl_sqlite3_with_mode_bounds },

    { ngx_string("tdtinyurl_sqlite3_db_path"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_conf_set_tdtinyurl_sqlite3_db_path,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL },

    ngx_null_command
};

//reg. config handler
static ngx_http_module_t  ngx_http_tdtinyurl_module_ctx = {
    NULL, /* preconfiguration */
    ngx_http_tdtinyurl_init, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_tdtinyurl_create_conf,   /* create location configuration */
    ngx_http_tdtinyurl_merge_conf     /* merge location configuration */
};

//reg. hook
ngx_module_t  ngx_http_tdtinyurl_module = {
    NGX_MODULE_V1,
    &ngx_http_tdtinyurl_module_ctx,       /* module context */
    ngx_http_tdtinyurl_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

//set redis uds path to ngx_http_tdtinyurl_conf_t.redis_uds_path
static char *ngx_http_conf_set_redis_uds_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {

    ngx_http_tdtinyurl_conf_t  *loc_cf = conf;
    ngx_str_t   *conf_val;

    if(NULL != loc_cf->redis_uds_path) {
        return "is duplicated";
    }

    conf_val = cf->args->elts;
    if(NULL == conf_val) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.redis_uds_path is null", NGX_TDTINYURL_NAME);
        return NGX_CONF_ERROR;
    }

    if(0 != access((const char *)conf_val[1].data, F_OK | R_OK | W_OK)) {

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.redis_uds_path=%s, can not access or not exists UDS socket", NGX_TDTINYURL_NAME , conf_val[1].data);
        return NGX_CONF_ERROR;
    }

    loc_cf->redis_uds_path = (u_char *)ngx_pcalloc(cf->pool, 1025);

    ngx_cpystrn(loc_cf->redis_uds_path, conf_val[1].data, conf_val[1].len+1);

    return NGX_CONF_OK;
}

//set redis connectt ip address to ngx_http_tdtinyurl_conf_t.redis_ip_addr
static char *ngx_http_conf_set_redis_ip_addr(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {

    ngx_http_tdtinyurl_conf_t  *loc_cf = conf;
    ngx_str_t   *conf_val;

    if(NULL != loc_cf->redis_ip_addr) {
        return "is duplicated";
    }

    conf_val = cf->args->elts;
    if(NULL == conf_val) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.redis_ip_addr is null", NGX_TDTINYURL_NAME);
        return NGX_CONF_ERROR;
    }

    if(0 == ngx_http_tdtinyurl_check_vaild_ipaddr(conf_val[1].data)) {

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.redis_ip_addr=%s , can not access or not exists UDS socket", NGX_TDTINYURL_NAME , conf_val[1].data);
        return NGX_CONF_ERROR;
    }

    loc_cf->redis_ip_addr = (u_char *)ngx_pcalloc(cf->pool, 1025);

    ngx_cpystrn(loc_cf->redis_ip_addr, conf_val[1].data, conf_val[1].len+1);
    return NGX_CONF_OK;
}

//set tdtinyurl working mode to ngx_http_tdtinyurl_conf_t.work_mode
static char *ngx_http_conf_set_tdtinyurl_work_mode(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {

    ngx_http_tdtinyurl_conf_t  *loc_cf = conf;
    ngx_str_t   *conf_val;
    short ck_lr = 0;
    short ck_rl = 0;

    if(NULL != loc_cf->work_mode) {
        return "is duplicated";
    }

    conf_val = cf->args->elts;
    if(NULL == conf_val) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.work_mode is null", NGX_TDTINYURL_NAME);
        return NGX_CONF_ERROR;
    }

    ck_lr = ngx_strcmp(conf_val[1].data, "lr");
    ck_rl = ngx_strcmp(conf_val[1].data, "rl");

    if(0 != ck_lr && 0 != ck_rl) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.work_mode not support your config value, only support lr,rl", NGX_TDTINYURL_NAME);
        return NGX_CONF_ERROR;
    }

    if(0 == ck_lr) {
        loc_cf->work_mode_no = NGX_TDTINYURL_WORKING_MODE_LR;
    } else if(0 == ck_rl) {
        loc_cf->work_mode_no = NGX_TDTINYURL_WORKING_MODE_RL;
    } else {
        loc_cf->work_mode_no = NGX_TDTINYURL_WORKING_MODE_LR;
    }

    loc_cf->work_mode = (u_char *)ngx_pcalloc(cf->pool, 1025);
    ngx_cpystrn(loc_cf->work_mode, conf_val[1].data, conf_val[1].len+1);

    return NGX_CONF_OK;
}

//set kyotocabinet dbm path to ngx_http_tdtinyurl_conf_t.dbm_path
static char *ngx_http_conf_set_tdtinyurl_dbm_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {

    ngx_http_tdtinyurl_conf_t  *loc_cf = conf;
    ngx_str_t   *conf_val;
    KCDB *dbm_dummy = kcdbnew();
    KCDB *dbm = kcdbnew();
    int32_t retval = 0;
    int mkpath_retval = 0;
    char *dbm_path;
    char *dbm_file_path;

    if(NULL != loc_cf->dbm_path) {
        return "is duplicated";
    }

    conf_val = cf->args->elts;
    if(NULL == conf_val) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path is null", NGX_TDTINYURL_NAME);
        return NGX_CONF_ERROR;
    }

    if(0 != access((const char *)conf_val[1].data, F_OK | R_OK | W_OK)) {

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, can not access or not exists dbm file", NGX_TDTINYURL_NAME , conf_val[1].data);
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, make a dummy dbm file", NGX_TDTINYURL_NAME , conf_val[1].data);


		//make a dummy
        dbm_file_path = strdup((const char *)conf_val[1].data);
        dbm_path = dirname(dbm_file_path);

        if(0 != access(dbm_path, F_OK)) {
            mkpath_retval = ngx_http_tdtinyurl_mkpath(dbm_path);
            if(0 != mkpath_retval) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, can not mkdir (%d)", NGX_TDTINYURL_NAME , conf_val[1].data, dbm_path);
            	return NGX_CONF_ERROR;
            } else {
               ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, mkdir %s", NGX_TDTINYURL_NAME , conf_val[1].data, dbm_path);
            }
        }

		retval = kcdbopen(dbm_dummy, (const char *)conf_val[1].data, KCOWRITER | KCOCREATE | KCOAUTOSYNC);
    	if(1 != retval) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, can not make a dummy dbm file (%s) : %s", NGX_TDTINYURL_NAME , conf_val[1].data, conf_val[1].data, kcdbemsg(dbm_dummy));
        	return NGX_CONF_ERROR;
    	}

        if(!kcdbclose(dbm_dummy)) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, can not close the dummy dbm file", NGX_TDTINYURL_NAME , conf_val[1].data);
        	return NGX_CONF_ERROR;
		}
    }

    retval = kcdbopen(dbm, (const char *)conf_val[1].data, KCOREADER);
    kcdbclose(dbm);
    if(1 != retval) {

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.dbm_path=%s, can not read dbm file", NGX_TDTINYURL_NAME , conf_val[1].data);
        return NGX_CONF_ERROR;
    }

    loc_cf->dbm_path = (u_char *)ngx_pcalloc(cf->pool, 1025);

    ngx_cpystrn(loc_cf->dbm_path, conf_val[1].data, conf_val[1].len+1);

    return NGX_CONF_OK;
}

//set sqlite3 db file path to ngx_http_tdtinyurl_conf_t.sqlite3_db_path
static char *ngx_http_conf_set_tdtinyurl_sqlite3_db_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {

    ngx_http_tdtinyurl_conf_t  *loc_cf = conf;
    ngx_str_t   *conf_val;
	const char *ptinyurl_tbname = "TDTINYURL";
	const char *ptinyurl_tbname_log = "TDTINYURL_LOG";

    if(NULL != loc_cf->sqlite3_db_path) {
        return "is duplicated";
    }

    conf_val = cf->args->elts;
    if(NULL == conf_val) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.sqlite3_db_path is null", NGX_TDTINYURL_NAME);
        return NGX_CONF_ERROR;
    }

    if(0 != access((const char *)conf_val[1].data, F_OK | R_OK | W_OK)) {

		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.sqlite3_db_path=%s, can not access db file", NGX_TDTINYURL_NAME , conf_val[1].data);
        return NGX_CONF_ERROR;
    }


	if(SQLITE_OK != ngx_http_tdtinyurl_sqlite3_quick_exists_table(cf->log, (const char *)conf_val[1].data, ptinyurl_tbname)) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.sqlite3_db_path=%s, not valid db , not found %s table in db", NGX_TDTINYURL_NAME , conf_val[1].data, ptinyurl_tbname);
        return NGX_CONF_ERROR;
	}

	if(SQLITE_OK != ngx_http_tdtinyurl_sqlite3_quick_exists_table(cf->log, (const char *)conf_val[1].data, ptinyurl_tbname_log)) {
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%s.sqlite3_db_path=%s, not valid db , not found %s table in db", NGX_TDTINYURL_NAME , conf_val[1].data, ptinyurl_tbname_log);
        return NGX_CONF_ERROR;
	}

    loc_cf->sqlite3_db_path = (u_char *)ngx_pcalloc(cf->pool, 1025);

    ngx_cpystrn(loc_cf->sqlite3_db_path, conf_val[1].data, conf_val[1].len+1);

    return NGX_CONF_OK;
}

//check ipaddres pattern.
static unsigned short ngx_http_tdtinyurl_check_vaild_ipaddr(u_char *ipaddr) {

    struct sockaddr_in sa;
    return inet_pton(AF_INET, (const char *)ipaddr, &(sa.sin_addr));
}

//check white space char.
inline unsigned short ngx_http_tdtinyurl_is_white_space(register char c) {

    if(32 < c && 127 != c) {
        return 0;
    } else {
        return -1;
    }
}

//make recusive directory path
int ngx_http_tdtinyurl_mkpath(char *pPath) { 

    char bufpath[1025];
    char *pbufpath = bufpath;

    struct stat st;
    unsigned int i=0;
    int status=0;
    unsigned int pPathlen = strlen(pPath);
    memset(&bufpath, 0x00, 1025);

    umask(0);

    for(;i<pPathlen;i++) {
        if( '/' == pPath[i] && 0 < i ) {

            if( 0 == lstat(pbufpath, &st) ) {
                if( !S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode) ) {
                    perror("can not make a path : ");
                    return -1;
                } 
            } else {

                status = mkdir( pbufpath, 0755);
                if(0 > status) {
                    if( 0 != stat(pbufpath, &st) ) {
                        return -1;
                    }
                }
            }
        }
        pbufpath[i] = pPath[i];
    }

    if( '/' != pPath[pPathlen-1]) {
        status = mkdir( pbufpath, 0755);
        if(0 > status) {
            if( 0 != stat(pbufpath, &st) ) {
                return -1;
            }
        }
    }
    return 0;

}

//connect to sqlite3 file (open)
int ngx_http_tdtinyurl_sqlite3_connect(ngx_log_t *log, sqlite3 **pdb, const char *ppath) {

	long flags = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
	int errcode = 0;

	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	sqlite3_enable_shared_cache(1);
	sqlite3_initialize();

	errcode = sqlite3_open_v2(ppath, pdb, flags, NULL);
	if(SQLITE_OK != errcode) {

        ngx_log_error(NGX_LOG_ALERT, log, 0, "can not open sqlite3(%s)", ppath);
		return errcode;
	}

	return SQLITE_OK;
}

//disconnect to sqlite3 file (close)
int ngx_http_tdtinyurl_sqlite3_close(ngx_log_t *log, sqlite3 *pdb) {

	int errcode = 0;

	errcode = sqlite3_close(pdb);
	if(SQLITE_OK != errcode) {

        ngx_log_error(NGX_LOG_ALERT, log, 0, "can not close sqlite3 : %s", sqlite3_errmsg(pdb));
		return errcode;
	}
	return SQLITE_OK;
}

//SQL : incresing hits field of SQLite3 db by tinyurl
int ngx_http_tdtinyurl_sqlite3_hit_logging(ngx_http_request_t *r, const char *ppath, const char *ptinyurl) {

	sqlite3 *pdb;
	int retcode = 0;
	char *perrmsg = NULL;
	char sqlbuf[501] = {0x00,};
	char *psqlbuf = sqlbuf;
	u_char *preferer = NULL;
	u_char *puser_agent = NULL;
	u_char *pip_addr = NULL;

	retcode = ngx_http_tdtinyurl_sqlite3_connect(r->connection->log, &(pdb), ppath);
	if(SQLITE_OK != retcode) {

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not open sqlite3 : %s", sqlite3_errmsg(pdb));
		return retcode;
	}


	retcode = sqlite3_exec(pdb, "BEGIN", NULL, NULL, &perrmsg);
	if(SQLITE_OK != retcode) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not start transaction of sqlite3 : %s", sqlite3_errmsg(pdb));
		return retcode;
	}

	snprintf(psqlbuf, 500, NGX_TINYURL_SQL_UPDATE_TDTINYURL_HITS, ptinyurl);
	retcode = sqlite3_exec(pdb, psqlbuf, NULL, NULL, &perrmsg);
	if(SQLITE_OK != retcode) {

		sqlite3_exec(pdb, "ROLLBACK", NULL, NULL, &perrmsg);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not update hits: %s , %s", sqlite3_errmsg(pdb), psqlbuf);
		return retcode;
	}

	//set
    if(NULL != r->headers_in.referer) {
		preferer = (u_char *)ngx_pcalloc(r->pool, r->headers_in.referer->value.len+1);
		ngx_memcpy(preferer, r->headers_in.referer->value.data, r->headers_in.referer->value.len);
    }
    if(NULL != r->headers_in.user_agent) {
		puser_agent = (u_char *)ngx_pcalloc(r->pool, r->headers_in.user_agent->value.len+1);
		ngx_memcpy(puser_agent, r->headers_in.user_agent->value.data, r->headers_in.user_agent->value.len);
    }
    if(NULL != r->connection->addr_text.data) {
		pip_addr = (u_char *)ngx_pcalloc(r->pool, r->connection->addr_text.len+1);
		ngx_memcpy(pip_addr, r->connection->addr_text.data, r->connection->addr_text.len);
    }

	memset(psqlbuf,0x00,500);
	snprintf(psqlbuf, 500, NGX_TINYURL_SQL_INSET_TDTINYURL_LOG, ptinyurl, (const char *)puser_agent, (const char *)pip_addr, (const char *)preferer);
	retcode = sqlite3_exec(pdb, psqlbuf, NULL, NULL, &perrmsg);
	if(SQLITE_OK != retcode) {

		sqlite3_exec(pdb, "ROLLBACK", NULL, NULL, &perrmsg);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not insert log: %s , %s", sqlite3_errmsg(pdb), psqlbuf);
		return retcode;
	}

	retcode = sqlite3_exec(pdb, "COMMIT", NULL, NULL, &perrmsg);
	if(SQLITE_OK != retcode) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not commit transaction of sqlite3 : %s", sqlite3_errmsg(pdb));
		return retcode;
	}

	retcode = ngx_http_tdtinyurl_sqlite3_close(r->connection->log, pdb);
	if(SQLITE_OK != retcode) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not close sqlite3 : %s", sqlite3_errmsg(pdb));
		return retcode;
	}

	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "complete update hit of %s", ptinyurl);
	return SQLITE_OK;
}

//single execution on sqlite3
int ngx_http_tdtinyurl_sqlite3_quick_exec(ngx_log_t *log, const char *ppath, const char *psql) {

	sqlite3 *pdb;
	char *perrmsg = NULL;
	int errcode = 0;

	errcode = ngx_http_tdtinyurl_sqlite3_connect(log, &(pdb), ppath);
	if(SQLITE_OK != errcode) {
		return errcode;
	}

	errcode = sqlite3_exec(pdb, psql, NULL, NULL, &perrmsg);
	if(SQLITE_OK != errcode) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "can not execute %s : %s", psql, sqlite3_errmsg(pdb));
		sqlite3_free(perrmsg);
		sqlite3_close(pdb);
		return errcode;
	}

	errcode = ngx_http_tdtinyurl_sqlite3_close(log, pdb);
	if(SQLITE_OK != errcode) {
		return errcode;
	}

	return SQLITE_OK;
}

//check table eixsts in sqlite3 db
int ngx_http_tdtinyurl_sqlite3_quick_exists_table(ngx_log_t *log, const char *ppath, const char *ptablename) {

	sqlite3 *pdb;
	char *perrmsg = NULL;
	int errcode = 0;
	int sqllen = 0;
	int retcode = 0;
	int returnval = 0;
	sqlite3_stmt *pstmt;
	char sqlbuf[200] = {0x00,};
	char *psqlbuf = sqlbuf;

	errcode = ngx_http_tdtinyurl_sqlite3_connect(log, &(pdb), ppath);
	if(SQLITE_OK != errcode) {
		return errcode;
	}

	snprintf(psqlbuf, 200, "SELECT count(*) as cnt FROM SQLITE_MASTER WHERE TYPE='table' AND NAME = '%s'", ptablename);
	sqllen = strlen(psqlbuf);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "SQL : %s", psqlbuf);

	errcode = sqlite3_prepare_v2(pdb, psqlbuf, sqllen, &pstmt, NULL);
	if(SQLITE_OK != errcode) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "can not prepare %s : %s", psqlbuf, sqlite3_errmsg(pdb));
		sqlite3_free(perrmsg);
		sqlite3_close(pdb);
		return errcode;
	}

	retcode = sqlite3_step(pstmt);
	switch(retcode) {

		case SQLITE_ROW:
			if(1 > sqlite3_data_count(pstmt)) {

        		ngx_log_error(NGX_LOG_ALERT, log, 0, "count zero error %s : %s", psqlbuf, sqlite3_errmsg(pdb));
				returnval = -1;
			} else {
				ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "column of result : %s", sqlite3_column_name(pstmt, 0));
				ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "value of result : %s", sqlite3_column_text(pstmt, 0));

				returnval = sqlite3_column_int(pstmt, 0);
				if(0 < returnval) {
					returnval = SQLITE_OK;
				} else {
					returnval = SQLITE_MISMATCH;
				}
			}
			break;
		case SQLITE_DONE:
			returnval = SQLITE_EMPTY;
			break;
		default:
			
        	ngx_log_error(NGX_LOG_ALERT, log, 0, "can not run step func %s : %s", psqlbuf, sqlite3_errmsg(pdb));
			sqlite3_free(perrmsg);
			break;
	}

	sqlite3_finalize(pstmt);
	errcode = ngx_http_tdtinyurl_sqlite3_close(log, pdb);
	if(SQLITE_OK != errcode) {
		return errcode;
	}

	return returnval;
}

//strip whitespace from a string
void ngx_http_tdtinyurl_str_trim(register char *pStr) {

    register char *pStrLeft;
    register char *pStrRight;

    for(pStrLeft=pStr; ngx_http_tdtinyurl_is_white_space(*pStrLeft); pStrLeft++ ) { }
    for(pStrRight=pStr; *pStrLeft; pStrLeft++, pStrRight++) {
        *pStrRight = *pStrLeft;
    }
    *pStrRight-- = 0x00;

    while ( pStrRight > pStr && ngx_http_tdtinyurl_is_white_space(*pStrRight) ) {
        *pStrRight-- = 0x00;
    }
}

//init(allocation) tdtinyurl.string 
static ngx_int_t ngx_http_tdtinyurl_init_string(struct string *pstr, unsigned int defSize ) {

    pstr->len = defSize;
    pstr->ptr = malloc(pstr->len+1);
    if(NULL == pstr->ptr) {
        return NGX_ERROR;
    }
    memset(pstr->ptr,0x00,  pstr->len);
    return NGX_OK;
}

//destroy tdtinyurl.string
static void ngx_http_tdtinyurl_destroy_string(struct string *pstr) {

    pstr->len = 0;
    free(pstr->ptr);
}

//ngx cant support realloca.
static size_t ngx_http_tdtinyurl_writefunc(void *ptr, size_t size, size_t nmemb, void *s) {

    struct string *pstr = s;

    size_t new_len = pstr->len + size*nmemb;
    pstr->ptr = realloc(pstr->ptr, new_len+1);
    if(NULL == pstr->ptr) {
        return NGX_ERROR;
    }

    memcpy(pstr->ptr+pstr->len, ptr, size*nmemb);
    pstr->ptr[new_len] = '\0';
    pstr->len = new_len;
    return size*nmemb;
}

//convert tdtinyurl.string to ngx pool
static ngx_int_t ngx_http_tdtinyurl_oldmalloc_to_ngxpool(ngx_http_request_t *r, struct string *pstr ) {

    char *pbuf_str = NULL;

    if(NULL == pstr->ptr || 1 > pstr->len) {
        return NGX_ERROR;
    } 

    pbuf_str = ngx_pcalloc(r->pool, pstr->len);
    if(NULL == pbuf_str) {
        return NGX_ERROR;
    }
    ngx_memcpy(pbuf_str, pstr->ptr, pstr->len);
    free(pstr->ptr);

    pstr->ptr = pbuf_str;

    return NGX_OK;
}

//check tdtinyurl enable with tdtinyurl related information logging
static short ngx_http_tdtinyurl_check_enable(ngx_http_request_t *r) {

    ngx_http_tdtinyurl_conf_t  *conf;
    conf = ngx_http_get_module_loc_conf(r, ngx_http_tdtinyurl_module);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "=====================================================");
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "Config Information");
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->unparsed_uri : %s", (const char *)r->unparsed_uri.data);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->uri : %s", (const char *)r->uri.data);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->request_line : %s", (const char *)r->request_line.data);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->enable : %d", (int)conf->enable);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.user_agent : %s", (const char *)r->headers_in.user_agent->value.data);

	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.msie : %d", r->headers_in.msie);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.msie6 : %d", r->headers_in.msie6);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.gecko : %d", r->headers_in.gecko);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.chrome : %d", r->headers_in.chrome);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.safari : %d", r->headers_in.safari);
	ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.konqueror : %d", r->headers_in.konqueror);


    if(1 == (int)conf->enable) { 
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->redis_connnect_mode : %d", (int)conf->redis_connnect_mode);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->redis_uds_path : %s", (const char *)conf->redis_uds_path);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->redis_ip_addr : %s", (const char *)conf->redis_ip_addr);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->redis_port: %d", conf->redis_port);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->work_mode: %s", (const char *)conf->work_mode);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->work_mode_no : %d", conf->work_mode_no);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "conf->dbm_path: %s", (const char *)conf->dbm_path);
    }

    if(0 == (int)conf->enable) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "tdtinyurl off : %d", (int)conf->enable);
        return -1;
    }

    return 1;
}

//HTTP client via curl library
static ngx_int_t ngx_http_tdtinyurl_curl_client(ngx_http_request_t *r, const char *pcallurl, char *ppostdata, struct string *pcurlretbuf) {


    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(NULL != curl) {

        curl_easy_setopt(curl, CURLOPT_URL, pcallurl);

        if(NULL != ppostdata) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ppostdata);
        }
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 800);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

        if(NULL == r->headers_in.referer) {
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.referer->value.data is NULL");
        } else {
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->headers_in.referer->value.data :%s", (const char *)r->headers_in.referer->value.data);
            curl_easy_setopt(curl, CURLOPT_REFERER, (const char *)r->headers_in.referer->value.data);
        }

        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 1);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 2);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        //curl_easy_setopt(curl, CURLOPT_USERAGENT, NGX_TDTINYURL_AGENT);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, (const char *)r->headers_in.user_agent->value.data);

        curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ngx_http_tdtinyurl_writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, pcurlretbuf);

        res = curl_easy_perform(curl);
        if(CURLE_OK != res) {
            ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "failure tdtinyurl api call : %s", curl_easy_strerror(res));
            return NGX_ERROR;
        }
    }

    curl_easy_cleanup(curl);
    return NGX_OK;
}

//fetch mode : web cralwing
static ngx_int_t ngx_http_tdtinyurl_printfout(ngx_http_request_t *r, struct string *pcurlretbuf, ngx_int_t httpstatus) {

    ngx_int_t       rc;
    ngx_buf_t       *buf;
    ngx_chain_t     out;

    rc = ngx_http_discard_request_body(r);
    if(NGX_OK != rc) {
        return rc;
    }
 
    ngx_str_set(&r->headers_out.content_type, "text/html");
 
    if(NGX_HTTP_HEAD == r->method) {
        r->headers_out.status = httpstatus;
        r->headers_out.content_length_n = pcurlretbuf->len;
        return ngx_http_send_header(r);
    }
 
    buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if(NULL == buf) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf->pos = (u_char *)pcurlretbuf->ptr;
    buf->last =  buf->pos + pcurlretbuf->len-1;
    buf->memory = 1;    
    buf->last_buf = 1;  
    buf->flush = 1;

    r->headers_out.status = httpstatus;
    r->headers_out.content_length_n = pcurlretbuf->len - 1;

    rc = ngx_http_send_header(r);

    if(NGX_ERROR == rc) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "rc == NGX_ERROR");
        return rc;
    }

    if(NGX_OK < rc) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "rc > NGX_OK : %d", rc);
        return rc;
    }

    if(r->header_only) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "r->header_only", rc);
        return rc;
    }

    out.buf = buf;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

//normal mode : http redirection to target url (long tail url)
static ngx_int_t ngx_http_tdtinyurl_redirection(ngx_http_request_t *r, u_char *predirecturl) {

    //ngx_int_t    rc;
    u_char *last, *location;
    ngx_table_elt_t  *pnewheaders;
    ngx_table_elt_t  *cc, **ccp, *expires;
    size_t expires_len = 0;
    size_t bodyhtml_len = 0;
    size_t predirecturlLen = ngx_strlen(predirecturl);
    u_char *pbodyhtml;

    ngx_http_clear_location(r);
    r->headers_out.location = ngx_palloc(r->pool, sizeof(ngx_table_elt_t));
    if(NULL == r->headers_out.location) {
        return NGX_DECLINED;
    }

    location = ngx_pnalloc(r->pool, predirecturlLen);
    if(NULL == location) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    last = ngx_copy(location, predirecturl, predirecturlLen+1);

    //r->headers_out.status = NGX_HTTP_MOVED_PERMANENTLY;
    //r->headers_out.content_length_n = 0;

    pnewheaders = ngx_list_push(&r->headers_out.headers);
    if(NULL == pnewheaders) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    pnewheaders->hash = 1;
    ngx_str_set(&pnewheaders->key, "Location");

    pnewheaders->value.data = location;
    pnewheaders->value.len = predirecturlLen;
    r->headers_out.location = pnewheaders;
    r->headers_out.last_modified_time = -1;
    r->headers_out.last_modified = NULL;

    //set Cache-control
    ccp = r->headers_out.cache_control.elts;
    if(NULL == ccp) {
        if(NGX_OK != ngx_array_init(&r->headers_out.cache_control, r->pool, 1, sizeof(ngx_table_elt_t *))) {
            return NGX_ERROR;
        }
    }
            
    ccp = ngx_array_push(&r->headers_out.cache_control);
    if(NULL == ccp) {
        return NGX_ERROR;
    }

    cc = ngx_list_push(&r->headers_out.headers);
    if(NULL == cc) {
        return NGX_ERROR;
    }

    cc->hash = 1;
    ngx_str_set(&cc->key, "Cache-Control");
    ngx_str_set(&cc->value, "private, max-age=0");
    *ccp = cc;

    //set Expire
    expires = r->headers_out.expires;
    if(NULL == expires) {
    
        expires = ngx_list_push(&r->headers_out.headers);
        if(NULL == expires) {
            return NGX_ERROR;
        }
    
        r->headers_out.expires = expires;
        expires->hash = 1;
        ngx_str_set(&expires->key, "Expires");
    }

    expires_len = 30;
    expires->value.len = expires_len - 1;
    expires->value.data = (u_char *) "Thu, 01 Jan 1970 00:00:01 GMT";

    expires->value.data = ngx_pnalloc(r->pool, expires_len);
    if(NULL == expires->value.data) {
        return NGX_ERROR;
    }
    ngx_memcpy(expires->value.data, ngx_cached_http_time.data, ngx_cached_http_time.len + 1);

    bodyhtml_len = sizeof(NGX_TINYUTL_REDIRECT_BOXY_FIRST_TEMPL);
    bodyhtml_len += sizeof(NGX_TINYUTL_REDIRECT_BOXY_LAST_TEMPL);
    bodyhtml_len += predirecturlLen;
    
    pbodyhtml = (u_char *)ngx_pcalloc(r->pool, bodyhtml_len+1);

    ngx_snprintf(pbodyhtml, bodyhtml_len, "%s%s%s", NGX_TINYUTL_REDIRECT_BOXY_FIRST_TEMPL, predirecturl, NGX_TINYUTL_REDIRECT_BOXY_LAST_TEMPL);

    struct string bodybuf;
    struct string *pbodybuf = &bodybuf;
    pbodybuf->ptr = (char *)pbodyhtml;
    pbodybuf->len = bodyhtml_len;

    return ngx_http_tdtinyurl_printfout(r, pbodybuf, NGX_HTTP_MOVED_PERMANENTLY);
}

//string url : get last char
static int ngx_http_tdtinyurl_get_last_chr(u_char *pstr, unsigned int findchr) {

    register unsigned int chr = 0;
    register unsigned int i = 0;
    register unsigned int len = 0;
    register u_char *porgstr = pstr;

    while(0 != *pstr) {
        chr = *pstr++;
        if(0 == chr) {
            break;
        }
        i++;
    }
    len = i;

    while(len) {
        chr = *pstr--;
        if(chr == findchr) {
            break;
        }
        len--;
    }

    pstr = porgstr;
    return len+1;
}

//kyotocabinet dbm handler
static ngx_int_t ngx_http_tdtinyurl_dbm_handler(ngx_http_request_t *r, ngx_http_tdtinyurl_conf_t *conf, u_char *ptdtinyurl_key, int ptdtinyurl_key_len) {

    KCDB *dbm = kcdbnew();
    int32_t retval = 0;
    uid_t user_id;
    struct passwd *user_pw;
    const char *ppath = (const char *)conf->dbm_path;
    char *pdbm_val = NULL;
    u_char *ptdtinyurl_dest_url = NULL;
    size_t sp;
    int secure_mode = -1;
    struct string curlretbuf;
    struct string *pcurlretbuf = &curlretbuf;

    user_id = getuid();
    user_pw = getpwuid(user_id);

    if(0 != access(ppath, F_OK | R_OK)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not access dbm file : %s", ppath);
        return NGX_ERROR;
    }

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "**************************************************");
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "dbm path : %s", (const char *)conf->dbm_path);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "dbm retval : %d", retval);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "current user_pw->pw_name : %s", user_pw->pw_name);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "current user_pw->pw_uid : %d", user_pw->pw_uid);

    retval = kcdbopen(dbm, ppath, KCOREADER);
    if(1 != retval) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not open dbm file : %s", ppath);
        return NGX_ERROR;
    }


    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ptdtinyurl_key : %s", ptdtinyurl_key);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ptdtinyurl_key_len : %d", ptdtinyurl_key_len);
    pdbm_val = kcdbget(dbm, (const char *)ptdtinyurl_key, ptdtinyurl_key_len, &sp);
    if(NULL != pdbm_val) {

        secure_mode = (int)pdbm_val[0] - 48;
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "return value from dbm : %s", pdbm_val);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "return value from dbm : %d", secure_mode);

        kcdbclose(dbm);

        ptdtinyurl_dest_url = (u_char *)ngx_pcalloc(r->pool, NGX_TDTINYURL_URL_MAX_LEN+1);
        if(NULL == ptdtinyurl_dest_url) {

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "not allocate memory for tdtinyurl to dest url");
            return NGX_ERROR;
        }

        ngx_memcpy(ptdtinyurl_dest_url, pdbm_val+2, strlen(pdbm_val));
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "SET ptdtinyurl_dest_url : %s", ptdtinyurl_dest_url);

        // Secure mode
        if(1 == secure_mode) {

            //get web page
            ngx_http_tdtinyurl_init_string(pcurlretbuf,0);
            if(NGX_OK != ngx_http_tdtinyurl_curl_client(r, (const char *)ptdtinyurl_dest_url, NULL, pcurlretbuf) ) {

                ngx_http_tdtinyurl_destroy_string(pcurlretbuf);
                return NGX_HTTP_GATEWAY_TIME_OUT;
                //return NGX_HTTP_NOT_FOUND;
            } else {

                ngx_http_tdtinyurl_str_trim(pcurlretbuf->ptr);
                ngx_http_tdtinyurl_oldmalloc_to_ngxpool(r, pcurlretbuf);

                return ngx_http_tdtinyurl_printfout(r, pcurlretbuf, NGX_HTTP_OK);
            }

        } else { // none secure mode

            return ngx_http_tdtinyurl_redirection(r, ptdtinyurl_dest_url);
        }

    } else {

        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "can not found data (%s) in dbm", ptdtinyurl_key);

        kcdbclose(dbm);
        return NGX_HTTP_NOT_FOUND;
    }
}

//redis handler
static ngx_int_t ngx_http_tdtinyurl_redis_handler(ngx_http_request_t *r, ngx_http_tdtinyurl_conf_t *conf, u_char *ptdtinyurl_key, int ptdtinyurl_key_len) {

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;
    redisContext *predis_con;
    u_char *ptdtinyurl_dest_url = NULL;
    redisReply *predis_reply = NULL;
    struct string curlretbuf;
    struct string *pcurlretbuf = &curlretbuf;

    //redis connect
    switch(conf->redis_connnect_mode) {

        case NGX_TDTINYURL_REDIS_CON_MODE_UDS: 
        default:
            predis_con = redisConnectUnixWithTimeout((const char *)conf->redis_uds_path, timeout);
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "redis uds connection : %s", conf->redis_uds_path);
            break;

        case NGX_TDTINYURL_REDIS_CON_MODE_TCP :
            predis_con = redisConnectWithTimeout((const char *)conf->redis_ip_addr, conf->redis_port, timeout);
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "redis tcp connection : %s:%d", conf->redis_ip_addr, conf->redis_port);
            break;
    }

    if(NULL == predis_con || predis_con->err) {

        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "Redis : Connecting Failure");
        if(predis_con) {
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "Redis error : %s", predis_con->errstr);
            redisFree(predis_con);
        }
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    } else {

        predis_reply = redisCommand(predis_con, "HMGET %s url secure", ptdtinyurl_key);
        if(REDIS_REPLY_ARRAY != predis_reply->type) {
            freeReplyObject(predis_reply);
            redisFree(predis_con);
            return NGX_DECLINED;
        }

        if(REDIS_REPLY_NIL == predis_reply->element[0]->type) {
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "not exists url of %s in redis", ptdtinyurl_key);
            freeReplyObject(predis_reply);
            redisFree(predis_con);
            return NGX_DECLINED;
        }

        if(REDIS_REPLY_NIL == predis_reply->element[1]->type) {
            ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "not exists secure of %s in redis", ptdtinyurl_key);
            freeReplyObject(predis_reply);
            redisFree(predis_con);
            return NGX_DECLINED;
        }

        ngx_pfree(r->pool, ptdtinyurl_key);

        ptdtinyurl_dest_url = (u_char *)ngx_pcalloc(r->pool, NGX_TDTINYURL_URL_MAX_LEN+1);
        if(NULL == ptdtinyurl_dest_url) {

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "not allocate memory for tdtinyurl to dest url");
            freeReplyObject(predis_reply);
            redisFree(predis_con);
            return NGX_ERROR;
        }

        ngx_memcpy(ptdtinyurl_dest_url, predis_reply->element[0]->str, strlen(predis_reply->element[0]->str)+1);
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "ptdtinyurl_dest_url : %s", ptdtinyurl_dest_url);


        // Secure mode
        if(1 == atoi(predis_reply->element[1]->str)) {

            //get web page
            ngx_http_tdtinyurl_init_string(pcurlretbuf,0);
            if(NGX_OK != ngx_http_tdtinyurl_curl_client(r, (const char *)ptdtinyurl_dest_url, NULL, pcurlretbuf) ) {

                ngx_http_tdtinyurl_destroy_string(pcurlretbuf);

                return NGX_HTTP_GATEWAY_TIME_OUT;
                //return NGX_HTTP_NOT_FOUND;
            } else {
                ngx_http_tdtinyurl_str_trim(pcurlretbuf->ptr);
                ngx_http_tdtinyurl_oldmalloc_to_ngxpool(r, pcurlretbuf);

                freeReplyObject(predis_reply);
                redisFree(predis_con);

                return ngx_http_tdtinyurl_printfout(r,pcurlretbuf, NGX_HTTP_OK);
            }
        } else { // none secure mode

            //set redirection to dest url
            freeReplyObject(predis_reply);
            redisFree(predis_con);

            return ngx_http_tdtinyurl_redirection(r, ptdtinyurl_dest_url);
        }
    }
}
// MAIN
static ngx_int_t ngx_http_tdtinyurl_handler(ngx_http_request_t *r) {

    ngx_http_tdtinyurl_conf_t *conf;
    u_char *ptdtinyurl_key_buf = NULL;
    u_char *ptdtinyurl_key = NULL;
    int pos_last_chr = 0;
    ngx_int_t retval = NGX_OK;
    int ptdtinyurl_key_len = 0;

    global_r = r;

    // pass
    if(NULL != r->exten.data) {
        return NGX_DECLINED;
    }

    // check "tdtinyurl on" directive
    if(-1 == ngx_http_tdtinyurl_check_enable(r)) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "DISABLED tdtinyurl");
        return NGX_DECLINED;
    }
    
    // only allow GET/HEAD
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }
 
    //load config
    conf = ngx_http_get_module_loc_conf(r, ngx_http_tdtinyurl_module);
    if (conf == NULL) {

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "can not config loading");
        return NGX_ERROR;
    }

    //set tiny key buffer
    ptdtinyurl_key_buf = (u_char *)ngx_pcalloc(r->pool, NGX_TDTINYURL_KEY_MAX_LEN+1);
    if(NULL == ptdtinyurl_key_buf) {

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "not allocate memory for tdtinyurl key");
        return NGX_ERROR;
    }

    ptdtinyurl_key = (u_char *)ngx_pcalloc(r->pool, NGX_TDTINYURL_KEY_MAX_LEN+1);
    if(NULL == ptdtinyurl_key) {

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "not allocate memory for tdtinyurl key");
        return NGX_ERROR;
    }

    //get tdtinyurl key in request_uri
    ngx_memcpy(ptdtinyurl_key_buf, r->uri.data, r->uri.len);
    pos_last_chr = ngx_http_tdtinyurl_get_last_chr(ptdtinyurl_key_buf, '/');
    if(1 > pos_last_chr) {
        pos_last_chr = 0;
    }

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "pos_last_chr : %d", pos_last_chr);


    ptdtinyurl_key_len = r->uri.len-pos_last_chr;
    ngx_memcpy(ptdtinyurl_key, ptdtinyurl_key_buf+pos_last_chr, ptdtinyurl_key_len);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "tdtinyurl key str : %s", ptdtinyurl_key);

    //empty check
    if(0 == ptdtinyurl_key[0]) {

        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "tdtinyurl key str is empty");
        ngx_pfree(r->pool, ptdtinyurl_key_buf);
    } else {

		//logging to sqlite3
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "sqlite3_with_mode working mode : %d", conf->sqlite3_with_mode);
		if(1 == (int)conf->sqlite3_with_mode) {
			ngx_http_tdtinyurl_sqlite3_hit_logging(r, (const char *)conf->sqlite3_db_path, (const char *)ptdtinyurl_key);
		}

        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "working mode : %d", conf->work_mode_no);
        switch(conf->work_mode_no) {

            default:
            case NGX_TDTINYURL_WORKING_MODE_LR:
                retval = ngx_http_tdtinyurl_dbm_handler(r, conf, ptdtinyurl_key, ptdtinyurl_key_len);

                if(NGX_ERROR == retval || NGX_HTTP_NOT_FOUND == retval) {

                    ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "LR mode  , but L not working");
                    retval = ngx_http_tdtinyurl_redis_handler(r, conf, ptdtinyurl_key, ptdtinyurl_key_len);
                }
                break;

            case NGX_TDTINYURL_WORKING_MODE_RL:
                retval = ngx_http_tdtinyurl_redis_handler(r, conf, ptdtinyurl_key, ptdtinyurl_key_len);
                if(NGX_ERROR == retval || NGX_HTTP_NOT_FOUND == retval) {

                    ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "RL mode  , but R not working");
                    retval = ngx_http_tdtinyurl_dbm_handler(r, conf, ptdtinyurl_key, ptdtinyurl_key_len);
                }
                break;
        }

        ngx_pfree(r->pool, ptdtinyurl_key_buf);
        return retval;
    }

    return NGX_DECLINED;
}

//init. tdtinyurl config
static void *ngx_http_tdtinyurl_create_conf(ngx_conf_t *cf) {

    ngx_http_tdtinyurl_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tdtinyurl_conf_t));
    if (NULL == conf) {
        return NULL;
    }

    conf->enable                = NGX_CONF_UNSET_UINT;
    conf->redis_connnect_mode   = NGX_CONF_UNSET_UINT;
    conf->redis_port            = NGX_CONF_UNSET_UINT;
    conf->redis_uds_path        = NULL;
    conf->redis_ip_addr         = NULL;
    conf->work_mode             = NULL;
    conf->work_mode_no          = NGX_CONF_UNSET_UINT;
    conf->dbm_path              = NULL;
	conf->sqlite3_with_mode		= NGX_CONF_UNSET_UINT;
    conf->sqlite3_db_path		= NULL;

    return conf;
}

//merging config
static char *ngx_http_tdtinyurl_merge_conf(ngx_conf_t *cf, void *parent, void *child) {

    ngx_http_tdtinyurl_conf_t  *prev = parent;
    ngx_http_tdtinyurl_conf_t  *conf = child;

    ngx_conf_merge_uint_value(conf->enable, prev->enable, NGX_TDTINYURL_OFF);
    ngx_conf_merge_uint_value(conf->redis_connnect_mode, prev->redis_connnect_mode, NGX_TDTINYURL_REDIS_CON_MODE_UDS);

    return NGX_CONF_OK;
}

//ini tdtinyurl ngx module
static ngx_int_t ngx_http_tdtinyurl_init(ngx_conf_t *cf) {

    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);

    if(NULL == h) {
        return NGX_ERROR;
    }

    *h = ngx_http_tdtinyurl_handler;

    return NGX_OK;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
