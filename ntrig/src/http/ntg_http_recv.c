/**
 * @file 	ntg_http_recv.c
 * @brief
 * @details
 * @author	tzh
 * @date	Jan 16, 2016	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_http_request.h"

/**
 *
 * @param c
 * @param buf
 * @param size
 * @return 成功返回字节数, 继续返回NTG_AGAIN, 否则返回NTG_ERROR
 * @note 只负责数据的接收,和日志记录,事件处理交由调用者
 */
ssize_t
ntg_http_recv(ntg_http_request_t *r, u_char *buf, size_t size)
{
    ssize_t       n;
    ntg_err_t     err;


    do {
        n = recv(r->sock, buf, size, 0);

        ntg_log_debug3(NTG_LOG_DEBUG_EVENT, r->log, 0,
                       "recv: fd:%d %d of %d", r->sock, n, size);
        if (n >= 0) {
//        	printf("headers===\n%s",buf);
            return n;
        }

        err = ntg_socket_errno;

        if (err == NTG_EAGAIN || err == NTG_EINTR) {
            ntg_log_debug0(NTG_LOG_DEBUG_EVENT, r->log, err,
                           "recv() not ready");
            n = NTG_AGAIN;

        } else {//其他错误

        	ntg_log_error(NTG_LOG_ALERT, r->log, err, "recv() not ready");
            break;
        }

    } while (err == NTG_EINTR);

    //其他错误

    if (n == NTG_AGAIN) {
    	return n;
    }

    return NTG_ERROR;
}
