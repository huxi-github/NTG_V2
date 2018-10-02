/**
 * @file 	ntg_http_request.h
 * @brief
 * @details
 * @author	tzh
 * @date	Jan 16, 2016	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef SRC_HTTP_NTG_HTTP_REQUEST_H_
#define SRC_HTTP_NTG_HTTP_REQUEST_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../utils/ntg_hash.h"

#define NTG_HTTP_MAX_URI_CHANGES           10
#define NTG_HTTP_MAX_SUBREQUESTS           200

/* must be 2^n */
#define NTG_HTTP_LC_HEADER_LEN             32


#define NTG_HTTP_DISCARD_BUFFER_SIZE       4096
#define NTG_HTTP_LINGERING_BUFFER_SIZE     4096

#define NTG_HTTP_OUT_BUFFER_SIZE       		512
#define NTG_HTTP_IN_BUFFER_SIZE     		2560///1024*2 + 512

#define NTG_HTTP_VERSION_9                 9
#define NTG_HTTP_VERSION_10                1000
#define NTG_HTTP_VERSION_11                1001

#define NTG_HTTP_UNKNOWN                   0x0001
#define NTG_HTTP_GET                       0x0002
#define NTG_HTTP_HEAD                      0x0004
#define NTG_HTTP_POST                      0x0008
#define NTG_HTTP_PUT                       0x0010
#define NTG_HTTP_DELETE                    0x0020
#define NTG_HTTP_MKCOL                     0x0040
#define NTG_HTTP_COPY                      0x0080
#define NTG_HTTP_MOVE                      0x0100
#define NTG_HTTP_OPTIONS                   0x0200
#define NTG_HTTP_PROPFIND                  0x0400
#define NTG_HTTP_PROPPATCH                 0x0800
#define NTG_HTTP_LOCK                      0x1000
#define NTG_HTTP_UNLOCK                    0x2000
#define NTG_HTTP_PATCH                     0x4000
#define NTG_HTTP_TRACE                     0x8000

#define NTG_HTTP_CONNECTION_CLOSE          1
#define NTG_HTTP_CONNECTION_KEEP_ALIVE     2


#define NTG_NONE                           1


#define NTG_HTTP_PARSE_HEADER_DONE         1

#define NTG_HTTP_CLIENT_ERROR              10
#define NTG_HTTP_PARSE_INVALID_METHOD      10
#define NTG_HTTP_PARSE_INVALID_REQUEST     11
#define NTG_HTTP_PARSE_INVALID_09_METHOD   12

#define NTG_HTTP_PARSE_INVALID_HEADER      13


/* unused                                  1 */
#define NTG_HTTP_SUBREQUEST_IN_MEMORY      2
#define NTG_HTTP_SUBREQUEST_WAITED         4
#define NTG_HTTP_LOG_UNSAFE                8


#define NTG_HTTP_CONTINUE                  100
#define NTG_HTTP_SWITCHING_PROTOCOLS       101
#define NTG_HTTP_PROCESSING                102

#define NTG_HTTP_OK                        200
#define NTG_HTTP_CREATED                   201
#define NTG_HTTP_ACCEPTED                  202
#define NTG_HTTP_NO_CONTENT                204
#define NTG_HTTP_PARTIAL_CONTENT           206

#define NTG_HTTP_SPECIAL_RESPONSE          300
#define NTG_HTTP_MOVED_PERMANENTLY         301
#define NTG_HTTP_MOVED_TEMPORARILY         302
#define NTG_HTTP_SEE_OTHER                 303
#define NTG_HTTP_NOT_MODIFIED              304
#define NTG_HTTP_TEMPORARY_REDIRECT        307

#define NTG_HTTP_BAD_REQUEST               400
#define NTG_HTTP_UNAUTHORIZED              401
#define NTG_HTTP_FORBIDDEN                 403
#define NTG_HTTP_NOT_FOUND                 404
#define NTG_HTTP_NOT_ALLOWED               405
#define NTG_HTTP_REQUEST_TIME_OUT          408
#define NTG_HTTP_CONFLICT                  409
#define NTG_HTTP_LENGTH_REQUIRED           411
#define NTG_HTTP_PRECONDITION_FAILED       412
#define NTG_HTTP_REQUEST_ENTITY_TOO_LARGE  413
#define NTG_HTTP_REQUEST_URI_TOO_LARGE     414
#define NTG_HTTP_UNSUPPORTED_MEDIA_TYPE    415
#define NTG_HTTP_RANGE_NOT_SATISFIABLE     416


#define NTG_HTTP_MAX_HOSTNAME			   128


#define NTG_EVENT_ADD_ERROR 		1 ///添加事件错误
#define NTG_EVENT_DEL_ERROR 		2 ///删除事件错误
#define NTG_EVENT_TIMEOUT_ERROR 	3 ///超时错误
#define NTG_EVENT_NOT_WRITE_ERROR 	4 ///没有可写事件
#define NTG_EVENT_NOT_READ_ERROR 	5 ///没有可读事件


typedef ntg_int_t (*ntg_http_header_handler_pt)(ntg_http_request_t *r,
    ntg_table_elt_t *h, ntg_uint_t offset);


/* 请求状态机 */
enum ntg_req_state_e {
	NTG_REQ_STATE_START,
	NTG_REQ_STATE_URLS,
	NTG_REQ_STATE_DNS_BEFFORE,
	NTG_REQ_STATE_DNS,
	NTG_REQ_STATE_BUILD,
	NTG_REQ_STATE_CONN,
	NTG_REQ_STATE_SEND_START,
	NTG_REQ_STATE_SEND,
	NTG_REQ_STATE_RECV_START,
	NTG_REQ_STATE_RECV,
	NTG_REQ_STATE_AGAIN,
	NTG_REQ_STATE_NEXT,
	NTG_REQ_STATE_DONE
};

/**
 * http状态对象
 */
typedef struct {
    ntg_uint_t           http_version;///http版本
    ntg_uint_t           code;///状态编码
    ntg_uint_t           count;///<状态码长度计数
    u_char              *start;///<状态码的开始
    u_char              *end;///<文本描述的结束处
} ntg_http_status_t;

typedef struct ntg_http_chunked_s ntg_http_chunked_t;
/**
 * http的chunked对象
 */
struct ntg_http_chunked_s {
    ntg_uint_t           state;
    off_t                size;//chunk的大小
    off_t                length;
};

/**
 * http请求状态对象
 */
typedef struct {
    ntg_msec_t                       bl_time;
    ntg_uint_t                       bl_state;

    ntg_uint_t                       status;//

    struct timeval					 response_tv;
    struct timeval					 header_tv;

    off_t                            response_length;///记录响应包体的长度

} ntg_http_upstream_state_t;


/**
 * http头部对象
 */
typedef struct {
    ntg_str_t                         name;
    ntg_http_header_handler_pt        handler;
    ntg_uint_t                        offset;
} ntg_http_header_t;

/**
 * 需要保留的http头部输入对象
 */
typedef struct {
    ntg_list_t                       headers;

    ntg_uint_t                       status_n;///状态码
    ntg_str_t                        status_line;


    ntg_table_elt_t                 *connection;
    ntg_table_elt_t                 *content_length;
    ntg_table_elt_t                 *transfer_encoding;

    off_t                            content_length_n;

    unsigned                         connection_close:1;
    unsigned                         chunked:1;
} ntg_http_headers_in_t;


/**
 * @name http请求对象
 */
struct ntg_http_request_s {
	void *data;///请求关联的数据url
	struct event_base			*base;///事件驱动
	struct evdns_base			*dns;///dns对象
	struct evhttp_uri			*uri;///请求的url对象
	struct evhttp_connection 	*cn;///请求对应的链接对象
	struct evhttp_request		*req;///请求对象

	evutil_socket_t sock;///关联的sock
	char   hostname[NTG_HTTP_MAX_HOSTNAME];///主机名
	struct sockaddr_in addr;///关联的地址

    struct event * ev_write;
    struct event * ev_read;
    struct timeval timeout;

	ntg_int_t   csm_id;///消费者id
	ntg_int_t   gp_id;///组id
	ntg_int_t 	user_id;///用户id
	ntg_int_t   url_id;///目标url id

	ntg_cycle_t	*cycle;
//	ntg_user_t	*user;

	enum ntg_req_state_e		req_state;///全局状态机变量

	/* 输出缓存区 */
	ntg_buf_t				out_buffer;
	off_t					out_size;///输出缓存大小

	/* 输入缓冲区 */
    ntg_buf_t                        in_buffer;///接收数据的buf
    off_t							 in_size;///接收缓存大小
    off_t                            length;

    /* 上游输入 */
    ntg_http_status_t              status;
    ntg_http_chunked_t             chunked;

    ntg_http_upstream_state_t       *upstream_state;//上游统计信息
    ntg_http_headers_in_t   		headers_in;//输入头域

    /* 相关处理函数 */
    void					       (*read_event_handler)(ntg_http_request_t *r);///读事件处理函数
    ntg_int_t                      (*process_header)(ntg_http_request_t *r);///头部处理函数

	/* 用于头部的解析 */
    ntg_uint_t                        state;///记录当前的解析状态
    ntg_uint_t                        header_hash;///头部hash值
    /* 用来计算头域名的hash值 */
    ntg_uint_t                        lowcase_index;///小写索引
    u_char                            lowcase_header[NTG_HTTP_LC_HEADER_LEN];///小写的头名
    u_char                           *header_name_start;///头域名的开始
    u_char                           *header_name_end;///头域名的结束
    u_char                           *header_start;///值的开始
    u_char                           *header_end;///值的结束

    ntg_uint_t                        http_version; //http的版本
    unsigned                          keepalive:1;
    unsigned                          invalid_header:1;///非法头部标志
    unsigned						  identical:1;///域名相同

    unsigned                          http_minor:16;
    unsigned                          http_major:16;

//	ntg_msec_t	timeout;///超时时间,由超时算法决定
	char **urls;///用户访问的url
	ntg_uint_t url_n;///url数
	ntg_uint_t index;///当前索引


	ntg_pool_t	*pool;
	ntg_log_t	*log;///错去日志对象
};

extern ntg_http_header_t  ntg_http_headers_in[];

ntg_http_request_t *ntg_http_get_request(ntg_cycle_t *cycle);
void ntg_http_reuse_request(ntg_http_request_t *rq);
void ntg_http_free_request(ntg_http_request_t *rq, int flag);

ntg_int_t ntg_http_build_request(ntg_http_request_t *rq);

#endif /* SRC_HTTP_NTG_HTTP_REQUEST_H_ */
