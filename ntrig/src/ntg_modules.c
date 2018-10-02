/*
 * ntg_modules.c
 *
 *  Created on: Oct 30, 2015
 *      Author: tzh
 */

#include "ntg_config.h"
#include "ntg_core.h"

#include "ntrig.h"

extern ntg_module_t  ntg_core_module;
extern ntg_module_t  ntg_errlog_module;
extern ntg_module_t  ntg_conf_module;

//extern ntg_module_t  ntg_events_module;
//extern ntg_module_t  ntg_event_core_module;
//extern ntg_module_t  ntg_epoll_module;

//extern ntg_module_t  ntg_regex_module;

extern ntg_module_t	 ntg_users_module;
extern ntg_module_t	 ntg_user_core_module;
extern ntg_module_t	 ntg_user_random_module;

//extern ntg_module_t  ntg_http_module;
//extern ntg_module_t  ntg_http_core_module;
//extern ntg_module_t  ntg_http_log_module;
//extern ntg_module_t  ntg_http_upstream_module;
//extern ntg_module_t  ntg_http_discard_module;
//extern ntg_module_t  ntg_http_static_module;
//extern ntg_module_t  ntg_http_autoindex_module;
//extern ntg_module_t  ntg_http_index_module;
//extern ntg_module_t  ntg_http_auth_basic_module;
//extern ntg_module_t  ntg_http_access_module;
//extern ntg_module_t  ntg_http_limit_conn_module;
//extern ntg_module_t  ntg_http_limit_req_module;
//extern ntg_module_t  ntg_http_geo_module;
//extern ntg_module_t  ntg_http_map_module;
//extern ntg_module_t  ntg_http_split_clients_module;
//extern ntg_module_t  ntg_http_referer_module;
//extern ntg_module_t  ntg_http_rewrite_module;
//extern ntg_module_t  ntg_http_proxy_module;
//extern ntg_module_t  ntg_http_fastcgi_module;
//extern ntg_module_t  ntg_http_uwsgi_module;
//extern ntg_module_t  ntg_http_scgi_module;
//extern ntg_module_t  ntg_http_memcached_module;
//extern ntg_module_t  ntg_http_empty_gif_module;
//extern ntg_module_t  ntg_http_browser_module;
//extern ntg_module_t  ntg_http_upstream_hash_module;
//extern ntg_module_t  ntg_http_upstream_ip_hash_module;
//extern ntg_module_t  ntg_http_upstream_least_conn_module;
//extern ntg_module_t  ntg_http_upstream_keepalive_module;
//extern ntg_module_t  ntg_http_write_filter_module;
//extern ntg_module_t  ntg_http_header_filter_module;
//extern ntg_module_t  ntg_http_chunked_filter_module;
//extern ntg_module_t  ntg_http_range_header_filter_module;
//extern ntg_module_t  ntg_http_gzip_filter_module;
//extern ntg_module_t  ntg_http_postpone_filter_module;
//extern ntg_module_t  ntg_http_ssi_filter_module;
//extern ntg_module_t  ntg_http_charset_filter_module;
//extern ntg_module_t  ntg_http_userid_filter_module;
//extern ntg_module_t  ntg_http_headers_filter_module;
//extern ntg_module_t  ntg_http_copy_filter_module;
//extern ntg_module_t  ntg_http_range_body_filter_module;
//extern ntg_module_t  ntg_http_not_modified_filter_module;

ntg_module_t *ntg_modules[] = {
    &ntg_core_module,
    &ntg_errlog_module,
    &ntg_conf_module,
//    &ntg_events_module,
//    &ntg_event_core_module,
//    &ntg_epoll_module,
//    &ntg_regex_module,
	&ntg_users_module,
	&ntg_user_core_module,
	&ntg_user_random_module,
//    &ntg_http_module,
//    &ntg_http_core_module,
//    &ntg_http_log_module,
//    &ntg_http_upstream_module,
//	&ntg_http_discard_module,
//    &ntg_http_static_module,
//    &ntg_http_autoindex_module,
//    &ntg_http_index_module,
//    &ntg_http_auth_basic_module,
//    &ntg_http_access_module,
//    &ntg_http_limit_conn_module,
//    &ntg_http_limit_req_module,
//    &ntg_http_geo_module,
//    &ntg_http_map_module,
//    &ntg_http_split_clients_module,
//    &ntg_http_referer_module,
//    &ntg_http_rewrite_module,
//    &ntg_http_proxy_module,
//    &ntg_http_fastcgi_module,
//    &ntg_http_uwsgi_module,
//    &ntg_http_scgi_module,
//    &ntg_http_memcached_module,
//    &ntg_http_empty_gif_module,
//    &ntg_http_browser_module,
//    &ntg_http_upstream_hash_module,
//    &ntg_http_upstream_ip_hash_module,
//    &ntg_http_upstream_least_conn_module,
//    &ntg_http_upstream_keepalive_module,
//    &ntg_http_write_filter_module,
//    &ntg_http_header_filter_module,
//    &ntg_http_chunked_filter_module,
//    &ntg_http_range_header_filter_module,
//    &ntg_http_gzip_filter_module,
//    &ntg_http_postpone_filter_module,
//    &ntg_http_ssi_filter_module,
//    &ntg_http_charset_filter_module,
//    &ntg_http_userid_filter_module,
//    &ntg_http_headers_filter_module,
//    &ntg_http_copy_filter_module,
//    &ntg_http_range_body_filter_module,
//    &ntg_http_not_modified_filter_module,
    NULL
};
