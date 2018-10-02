/**
 * @file 	ntg_http_request.c
 * @brief
 * @details
 * @author	tzh
 * @date	Jan 16, 2016	
 * @version		V0.1
 * @copyright	tzh 
 */
#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../user/ntg_user.h"

#include "../utils/ntg_process_cycle.h"
#include "ntg_http_request.h"

static ntg_int_t ntg_http_process_content_length(ntg_http_request_t *r,
    ntg_table_elt_t *h, ntg_uint_t offset);
static ntg_int_t
ntg_http_process_connection(ntg_http_request_t *r, ntg_table_elt_t *h,
    ntg_uint_t offset);
static ntg_int_t
ntg_http_process_transfer_encoding(ntg_http_request_t *r,
    ntg_table_elt_t *h, ntg_uint_t offset);

/**
 * 需要处理的输入头部
 */
ntg_http_header_t  ntg_http_headers_in[] = {

    { ntg_string("content-length"),
                 ntg_http_process_content_length, 0
    },

    { ntg_string("Connection"),
                 ntg_http_process_connection, 0
    },

    { ntg_string("Transfer-Encoding"),
                 ntg_http_process_transfer_encoding, 0
    },

    { ntg_null_string, NULL, 0 }
};



/**
 * 获取一个request对象
 * @param cycle
 * @param user
 * @return 存在返回一个可用的request对象, 否则返回NULL
 * @note 已对返回的对象进行了初始化处理
 */
ntg_http_request_t *ntg_http_get_request(ntg_cycle_t *cycle) {
    ntg_http_request_t *rq;
    ntg_pool_t 			*pool;
    ntg_http_upstream_state_t       *hus;

	/* 获取一个http_request对象 */
	rq = cycle->free_reqs;
	if(rq == NULL){
        ntg_log_error(NTG_LOG_ALERT, cycle->log, 0,
                      "%ui requests are not enough free=%ui",
                      cycle->request_n, cycle->free_req_n );
        printf("requests===========================NULL\n");
        return NULL;
	}
#define NTG_REQUEST_SIZE  3*1024
	pool = ntg_create_pool(NTG_REQUEST_SIZE, cycle->log);
    if (pool == NULL) {
        ntg_log_error(NTG_LOG_EMERG, cycle->log, 0,
                      "could not create pool");
        return NULL;
    }

    hus = ntg_palloc(pool,  sizeof(ntg_http_upstream_state_t));
    if (hus == NULL) {
        ntg_log_error(NTG_LOG_EMERG, cycle->log, 0,
                      "could not create ntg_http_upstream_state_t");
        return NULL;
    }


	cycle->free_reqs = rq->data;
	cycle->free_req_n--;

	ntg_memzero(rq, sizeof(ntg_http_request_t));
	ntg_memzero(hus, sizeof(ntg_http_upstream_state_t));
	rq->pool = pool;
	rq->upstream_state = hus;

	/*TODO 暂时直接设定 */
	rq->out_size = NTG_HTTP_OUT_BUFFER_SIZE;
	rq->in_size = NTG_HTTP_IN_BUFFER_SIZE;

	/* 超时时间 */
	rq->timeout.tv_sec = 120;
	rq->timeout.tv_usec = 0;

	rq->cycle = cycle;
	rq->base = cycle->base;
	rq->dns = cycle->dns;
	rq->log = cycle->log;


	rq->sock = -1;

	return rq;
}

/**
 * 重用request对象
 * @param rq
 */
void ntg_http_reuse_request(ntg_http_request_t *rq){

	rq->timeout.tv_sec = 120;
	rq->timeout.tv_usec = 0;

	rq->req_state = NTG_REQ_STATE_START;
	rq->length = 0;


	ntg_memzero(&rq->status , sizeof(ntg_http_status_t));
	ntg_memzero(&rq->chunked , sizeof(ntg_http_chunked_t));
	ntg_memzero(&rq->headers_in , sizeof(ntg_http_headers_in_t));
	ntg_memzero(&rq->status , sizeof(ntg_http_status_t));
	ntg_memzero(rq->upstream_state , sizeof(ntg_http_upstream_state_t));


	rq->read_event_handler = NULL;
	rq->process_header = NULL;
	rq->identical=0;
	rq->invalid_header = 0;
}

/**
 * 释放一个结束的request对象
 * @param rq
 * @param flag
 * @note 根据不同的flag进行日志的记录
 * @note TODO flag 暂时没有定义
 * @note 向记录进程发送记录消息
 */
void ntg_http_free_request(ntg_http_request_t *rq, int flag) {
	ntg_cycle_t *cycle;

//	printf("+++++++++++++++sucess to send record>>>>>>%d>>>>>>>>>>\n", ++i);
	cycle = rq->cycle;
	if(rq->uri != NULL){
		evhttp_uri_free(rq->uri);
		rq->uri=NULL;
	}

	if(rq->cn != NULL){
		evhttp_connection_free(rq->cn);//中已经是否请求
		rq->cn = NULL;

	}else if(rq->req != NULL){
		evhttp_request_free(rq->req);
		rq->req = NULL;
	}
	if(rq->sock >= 0){
		close(rq->sock);
		rq->sock = -1;
	}
	if(rq->ev_read != NULL){
		event_free(rq->ev_read);
		rq->ev_read = NULL;
	}

	if(rq->ev_write != NULL){
		event_free(rq->ev_write);
		rq->ev_write = NULL;
	}

	if(rq->pool != NULL ){
		ntg_destroy_pool(rq->pool);
		rq->pool = NULL;
	}
	//TODO 释放data的资源
	rq->data = cycle->free_reqs;
	cycle->free_reqs = rq;
	cycle->free_req_n++;

	//printf("+++++++++++++++cycle->free_req_n>>>>>%d>>>>>>>>>>\n", (int)cycle->free_req_n);
    //2.2 以通道方式发送命令
}


/**
 * 构造请求数据包
 * @param[in] rq 请求对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 内部不处理失败时的资源释放，只记录日志
 */
ntg_int_t
ntg_http_build_request(ntg_http_request_t *rq) {

	u_char *p;
	const char *query;
	const char *path;
	size_t len;
	char *headers =
			"User-Agent: Mozilla/5.0\r\n"
					"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
					"Accept_Charset: en-us,en;q=0.5\r\n"
					"Accept_Encoding: gzip,deflate\r\n"
					"Accept_Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
					"Connection: keep alive\r\n"
					"\r\n";


	/* 分配空间 */
	if(rq->out_buffer.start == NULL){
		rq->out_buffer.start = ntg_palloc(rq->pool, rq->out_size);

		if (rq->out_buffer.start == NULL) {
			/* 结束请求 */
			return NTG_ERROR;
		}
	}
	rq->out_buffer.end = rq->out_buffer.start + rq->out_size;
	rq->out_buffer.pos = rq->out_buffer.last = rq->out_buffer.start;


	path = evhttp_uri_get_path(rq->uri);
	query = evhttp_uri_get_query(rq->uri);

	/* 计算长度 */
	if (path == NULL || strlen(path) == 0) {
		len = 1;
	} else {
		if (query == NULL || strlen(query) == 0) {
			len = strlen(path);
		} else {
			len = strlen(path) + 1 + strlen(query);
		}
	}
	//GET %s HTTP/1.1\r\nHost: %s\r\nheaders
	len = 3 + len + 16 + strlen(rq->hostname) + 2 + strlen(headers);
	if(len > rq->out_size){
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
				"reqest message is too long(%d)", len);
		return NTG_ERROR;
	}

	//printf("host= %s /path==%s== %s\n", rq->hostname, path, query);
	//	printf("__________________33____________________\n");
	/* 构造消息 */
	p = rq->out_buffer.pos;

	if (path == NULL || strlen(path) == 0) {
		p = ntg_sprintf(p, "GET %s HTTP/1.1\r\n", "/");
	} else {
		if (query == NULL || strlen(query) == 0) {
			p = ntg_sprintf(p, "GET %s HTTP/1.1\r\n", path);
		} else {
			p = ntg_sprintf(p, "GET %s?%s HTTP/1.1\r\n", path, query);
		}
	}

	p = ntg_sprintf(p, "Host: %s\r\n", rq->hostname);
	p = ntg_sprintf(p, "%s", headers);


	rq->out_buffer.last = p;

	//printf("------ntg_http_build_request---end----\n");
	return NTG_OK;
}




/**
 * 请求状态机处理
 * @param urls
 */
void ntg_http_request_process(evutil_socket_t sock, short flags,
		void * args){

    enum state{
        sw_start = 0,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_ignore_line,
        sw_almost_done,
        sw_header_almost_done//头域将要结束
    } ;//头域解析状态机


}

/**
 * content_length头处理函数
 * @param[in] r 请求对象
 * @param[in] h 表元素对象
 * @param[in] offset 偏移量
 * @return 返回NTG_OK
 * @note 设置http响应实体长度
 */
static ntg_int_t
ntg_http_process_content_length(ntg_http_request_t *r,
    ntg_table_elt_t *h, ntg_uint_t offset)
{

//    r->headers_in.content_length = h;
    r->headers_in.content_length_n = ntg_atoof(h->value.data, h->value.len);

    return NTG_OK;
}
/**
 * connection头处理函数
 * @param[in] r 请求对象
 * @param[in] h 表元素对象
 * @param[in] offset 偏移量
 * @return 返回NTG_OK
 * @note 设置是否为链接关闭标志
 */
static ntg_int_t
ntg_http_process_connection(ntg_http_request_t *r, ntg_table_elt_t *h,
    ntg_uint_t offset)
{
//    r->headers_in.connection = h;
//	printf("---connection---%s------\n", h->value.data);

    if (ntg_strlcasestrn(h->value.data, h->value.data + h->value.len,
                         (u_char *) "close", 5 - 1)
        != NULL)
    {
//    	printf("close-------\n");
        r->headers_in.connection_close = 1;
    }

    return NTG_OK;
}
/**
 * transfer_encoding头处理函数
 * @param[in] r 请求对象
 * @param[in] h 表元素对象
 * @param[in] offset 偏移量
 * @return 返回NTG_OK
 * @note 设置是否为chunked编码标志
 */
static ntg_int_t
ntg_http_process_transfer_encoding(ntg_http_request_t *r,
    ntg_table_elt_t *h, ntg_uint_t offset)
{
//    r->headers_in.transfer_encoding = h;

    if (ntg_strlcasestrn(h->value.data, h->value.data + h->value.len,
                         (u_char *) "chunked", 7 - 1)
        != NULL)
    {
        r->headers_in.chunked = 1;
    }

    return NTG_OK;
}
