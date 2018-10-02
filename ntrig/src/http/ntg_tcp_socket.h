/**
 * @file 	ntg_tcp_socket.h
 * @brief
 * @details
 * @author	tzh
 * @date	Jan 16, 2016	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef SRC_HTTP_NTG_TCP_SOCKET_H_
#define SRC_HTTP_NTG_TCP_SOCKET_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_http_request.h"

evutil_socket_t ntg_make_tcp_socket(ntg_http_request_t *rq);
ntg_int_t ntg_get_ipv4_for_host(const char *hostname, ev_uint16_t port, struct sockaddr *addr);

#endif /* SRC_HTTP_NTG_TCP_SOCKET_H_ */
