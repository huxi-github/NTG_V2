/**
 * @file 	ntg_tcp_socket.c
 * @brief
 * @details
 * @author	tzh
 * @date	Jan 16, 2016	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_http.h"
#include "../utils/ntg_errno.h"
#include "../utils/ntg_log.h"

/**
 * 获取一个tcp socket
 * @return
 * @note 套接字被设置为非阻塞
 * @note 错误处理记录交由调用者
 */
evutil_socket_t ntg_make_tcp_socket(ntg_http_request_t *rq)
{
	evutil_socket_t sock;
    ntg_err_t     err;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1) {
        err = ntg_socket_errno;
        ntg_log_error(NTG_LOG_ALERT, rq->log, err, "socket() create socket is failed");
		return NTG_ERROR;
	}

//	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &on, sizeof(on))
//			== -1) {
//		return NTG_ERROR;
//	}

	if ((evutil_make_socket_nonblocking(sock)) == -1) {
        err = ntg_socket_errno;
        ntg_log_error(NTG_LOG_ALERT, rq->log, err, "evutil_make_socket_nonblocking() is failed");
		return NTG_ERROR;
	}

	return sock;
}


/**
 * 通过域名获取对应的地址
 * @param hostname
 * @param port
 * @param[out] addr 保存获取的地址
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t
ntg_get_ipv4_for_host(const char *hostname, ev_uint16_t port, struct sockaddr *addr){
	char port_buf[6];
	struct evutil_addrinfo hints;
	struct evutil_addrinfo *answer = NULL;
	int err;

//	evutil_socket_t sock;
	evutil_snprintf(port_buf, sizeof(port_buf), "%d", (int)port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = EVUTIL_AI_ADDRCONFIG;

	err = evutil_getaddrinfo(hostname, port_buf, &hints, &answer);
	if(err < 0){
		fprintf(stderr, "error while resolving '%s': '%s'\n",
				hostname, evutil_gai_strerror(err));
		return NTG_ERROR;
	}

	if(answer == NULL){
		return NTG_ERROR;
	}

	*addr = *answer->ai_addr;

	return NTG_OK;
}

