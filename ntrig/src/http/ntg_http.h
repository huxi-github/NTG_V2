/**
 * @file 	ntg_http.h
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 22, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef HTTP_NTG_HTTP_H_
#define HTTP_NTG_HTTP_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
//#include "../utils/ntg_hash.h"
#include "../utils/ntg_channel.h"

#include "ntg_http_request.h"

int ntg_http_start(ntg_cycle_t *cycle, ntg_message_t *ch);
void ntg_http_final_process(ntg_http_request_t *rq);

ssize_t
ntg_http_recv(ntg_http_request_t *r, u_char *buf, size_t size);

ntg_int_t
ntg_http_parse_status_line(ntg_http_request_t *r, ntg_buf_t *b,
    ntg_http_status_t *status);
ntg_int_t
ntg_http_parse_header_line(ntg_http_request_t *r, ntg_buf_t *b,
    ntg_uint_t allow_underscores);
ntg_int_t
ntg_http_parse_chunked(ntg_http_request_t *r, ntg_buf_t *b,
    ntg_http_chunked_t *ctx);

ntg_int_t ntg_http_discard_response_body(ntg_http_request_t *r);


#endif /* HTTP_NTG_HTTP_H_ */
