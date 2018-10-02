/**
 * @file 	ntg_http_discard.c
 * @brief
 * @details
 * @author	tzh
 * @date	Jan 14, 2016	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_http.h"
#include "ntg_http_request.h"

static  void ntg_http_discarded_request_body_handler(ntg_http_request_t *r);
static ntg_int_t ntg_http_read_discarded_request_body(ntg_http_request_t *r);
static ntg_int_t ntg_http_discard_request_body_filter(ntg_http_request_t *r, ntg_buf_t *b);

/**
 * 丢弃响应包体入口函数
 * @param r
 * @return 调用成功返回NTG_OK,
 * @note 调用成功并不表示已丢弃完,如果数据没有丢弃完将会注册回调函数,继续丢弃数据
 * @note 不管成功还是失败都交由调用者处理
 */
ntg_int_t ntg_http_discard_response_body(ntg_http_request_t *r)
{
    ssize_t       size;
    ntg_int_t     rc;

    //TODO 如果以下子接收到很长的数据包含头域的,已沾满buf怎么处理
    ntg_log_debug0(NTG_LOG_DEBUG_HTTP, r->log, 0, "http set discard body");

    //printf("--------------ntg_http_discard_response_body---------1------\n");
    if (r->headers_in.content_length_n <= 0 && !r->headers_in.chunked) {
    	//没有需要丢弃的数据
    	/* TODO 需要结束链接或转到下一个url */
    	//printf("--------------ntg_http_discard_response_body---------1------\n");
    	if(r->index==1){
    		r->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		r->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(r);
        return NTG_OK;
    }

    /* 处理已有的数据 */
    size = r->in_buffer.last - r->in_buffer.pos;//获取已接收到的大小

    if (size || r->headers_in.chunked) {
        rc = ntg_http_discard_request_body_filter(r, &r->in_buffer);//进行一次过滤处理

        if (r->headers_in.content_length_n == 0) {//处理完
        	/* TODO 需要结束链接或转到下一个url */
        	//printf("--------------ntg_http_discard_response_body-------finish------\n");
        	r->req_state = NTG_REQ_STATE_NEXT;
        	ntg_http_final_process(r);
            return NTG_OK;
        }

        if(rc != NTG_OK && rc != NTG_AGAIN){
        	/* 出错情况 */
        	printf("--------------ntg_http_discard_response_body---------3------\n");
        	if(r->index==1){
        		r->req_state = NTG_REQ_STATE_DONE;
        	}else{
        		r->req_state = NTG_REQ_STATE_NEXT;
        	}
        	ntg_http_final_process(r);
            return rc;
        }
    }

    /* 读取并丢弃新的数据 */
    rc = ntg_http_read_discarded_request_body(r);

    if (rc == NTG_OK) {
    	//printf("--------------ntg_http_discard_response_body----4-----finish------\n");
    	/* TODO 需要结束链接或转到下一个url */
    	//printf(">>>>>>>>>>>>time---%ld->\n", (long int)r->upstream_state->header_sec);

    	r->req_state = NTG_REQ_STATE_NEXT;
    	ntg_http_final_process(r);
        return NTG_OK;
    }

    if (rc >= NTG_HTTP_SPECIAL_RESPONSE) {//出错
    	printf("--------------ntg_http_discard_response_body---------5------\n");

    	r->req_state = NTG_REQ_STATE_NEXT;
    	ntg_http_final_process(r);
        return rc;
    }

    /* rc == NTG_AGAIN 本次读取的数据已处理完 */

    r->read_event_handler = ntg_http_discarded_request_body_handler;

    /* 重新注册事件 */
	if( event_add((struct event*)r->ev_read, &r->timeout) < 0){
		/* 错误处理 */
    	if(r->index==1){
    		r->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		r->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(r);
		return NTG_ERROR;
	}

    return NTG_OK;
}

static
void
ntg_http_discarded_request_body_handler(ntg_http_request_t *r)
{
    ntg_int_t                  rc;
    //ntg_msec_t                 timer;
    //printf("------ntg_http_discarded_request_body_handler----start----\n");
    rc = ntg_http_read_discarded_request_body(r);

    if (rc == NTG_OK) {
//    	printf("--------------ntg_http_discarded_request_body_handler---------1---finish---\n");
    	r->req_state = NTG_REQ_STATE_NEXT;
    	ntg_http_final_process(r);
        return;
    }

    if (rc >= NTG_HTTP_SPECIAL_RESPONSE) {
    	printf("--------------ntg_http_discarded_request_body_handler---------2------\n");

    	r->req_state = NTG_REQ_STATE_NEXT;
    	ntg_http_final_process(r);
        return;
    }

    /* rc == NTG_AGAIN */
	if( event_add((struct event*)r->ev_read, &r->timeout) < 0){
		/* 错误处理 */
    	r->req_state = NTG_REQ_STATE_NEXT;
    	ntg_http_final_process(r);
		return;
	}
}

/**
 * 读取并丢弃数据
 * @param r
 * @return 数据丢弃完返回NTG_OK, 继续返回NTG_AGAIN, 否则返回其他值
 * @note 出错或处理完的资源释放问题交由调用处理
 */
static ntg_int_t
ntg_http_read_discarded_request_body(ntg_http_request_t *r)
{
    size_t     size;
    ssize_t    n;
    ntg_int_t  rc;
    //ntg_buf_t  b;
//    u_char     buffer[NTG_HTTP_DISCARD_BUFFER_SIZE];

    ntg_log_debug0(NTG_LOG_DEBUG_HTTP, r->log, 0,
                   "http read discarded body");

//    ntg_memzero(&b, sizeof(ntg_buf_t));

//    b.temporary = 1;

    for ( ;; ) {
        if (r->headers_in.content_length_n == 0) {
            return NTG_OK;
        }

        r->in_buffer.pos = r->in_buffer.last = r->in_buffer.start;

        size = (size_t) ntg_min(r->headers_in.content_length_n,
                                r->in_size);

        n = ntg_http_recv(r, r->in_buffer.last, size);

        if (n == NTG_AGAIN) {
            return NTG_AGAIN;
        }

        if ( n == 0 || n == NTG_ERROR) {
            return NTG_OK;
        }

        r->upstream_state->response_length += n;

        r->in_buffer.last += n;

        rc = ntg_http_discard_request_body_filter(r, &r->in_buffer);

        if (rc != NTG_OK) {
            return rc;
        }
    }
}

/**
 * 丢弃包体过滤函数
 * @param r
 * @param b
 * @return 成功返回NTG_OK, 继续返回NTG_AGAIN, 否则返回NTG_ERROR
 * @note 保证对所有读取的数据都进行了过滤处理
 */
static ntg_int_t
ntg_http_discard_request_body_filter(ntg_http_request_t *r, ntg_buf_t *b)
{
    size_t                    size;
    ntg_int_t                 rc;

    if (r->headers_in.chunked) {
    	/* chunked 数据过滤*/

        for ( ;; ) {

            rc = ntg_http_parse_chunked(r, b, &r->chunked);

            if (rc == NTG_OK) {

                /* a chunk has been parsed successfully */

                size = b->last - b->pos;

                if ((off_t) size > r->chunked.size) {
                    b->pos += (size_t) r->chunked.size;
                    r->chunked.size = 0;

                } else {
                    r->chunked.size -= size;
                    b->pos = b->last;
                }

                continue;
            }

            if (rc == NTG_DONE) {

                /* a whole response has been parsed successfully */
            	r->keepalive = !r->headers_in.connection_close;
                r->headers_in.content_length_n = 0;
                break;
            }

            if (rc == NTG_AGAIN) {

                /* set amount of data we want to see next time */
                r->headers_in.content_length_n = r->chunked.length;
                break;
            }

            /* invalid */

            ntg_log_error(NTG_LOG_ERR, r->log, 0,
                          "client sent invalid chunked body");

            return NTG_ERROR;
        }

    } else {
    	/* 一般数据过滤 */
        size = b->last - b->pos;

        if ((off_t) size > r->headers_in.content_length_n) {
            b->pos += (size_t) r->headers_in.content_length_n;
            r->headers_in.content_length_n = 0;

            r->keepalive = !r->headers_in.connection_close;
        } else {
            b->pos = b->last;
            r->headers_in.content_length_n -= size;
        }
    }

    return NTG_OK;
}

