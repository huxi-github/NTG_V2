                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /**
 * @file 	ntg_http.c
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 27, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_http.h"
#include "ntg_http_request.h"

#include "ntg_tcp_socket.h"
#include "../user/ntg_user.h"
#include "../utils/ntg_process_cycle.h"
#include "../utils/ntg_database.h"

#define NTG_EVENT_ADD_ERROR 		1 ///添加事件错误
#define NTG_EVENT_DEL_ERROR 		2 ///删除事件错误
#define NTG_EVENT_TIMEOUT_ERROR 	3 ///超时错误
#define NTG_EVENT_NOT_WRITE_ERROR 	4 ///没有可写事件
#define NTG_EVENT_NOT_READ_ERROR 	5 ///没有可读事件

static ntg_int_t ntg_http_get_urls(ntg_http_request_t *rq);
static ntg_int_t ntg_http_get_next_url(ntg_http_request_t *rq);
static ntg_int_t ntg_http_connect_init(ntg_http_request_t *rq);
static ntg_int_t ntg_http_test_connect(ntg_http_request_t *rq);

static void ntg_http_write_cb(evutil_socket_t sock, short flags, void * args);
static void ntg_http_read_cb(evutil_socket_t sock, short flags, void * args);

static void ntg_http_handler_header(ntg_http_request_t *rq);

static ntg_int_t ntg_http_process_status_line(ntg_http_request_t *r);
static ntg_int_t ntg_http_process_header(ntg_http_request_t *r);

static ntg_int_t ntg_http_filter_init(ntg_http_request_t  *r);

#define NTG_HTTP_INTERNAL_FD 	1
#define NTG_HTTP_INTERNAL_URL 	2
#define NTG_HTTP_NOT_REQUEST 	3
#define NTG_HTTP_HOST_TOO_LONG 	4
#define NTG_HTTP_NOT_ADDR		5
#define NTG_HTTP_BUILD_FAILED	6


//printf("------------ntg_http_start--------start------\n");
//    char *urls[10]={
//    		"http://192.168.200.1/html/webs/web1/index.html",
//    		"http://192.168.200.1/html/webs/web2/index.html",
//    		"http://192.168.200.1/html/webs/web3/index.html",
//    		"http://192.168.200.1/html/a.txt",
//    		"http://192.168.200.1/html/b.txt",
//    		"http://192.168.200.1/html/c.txt",
//    		"http://192.168.200.1/html/index.html",
//    		"http://192.168.200.1/html/login.html",
//    		"http://192.168.200.1/html/equipment_message.html",
//    		"http://192.168.200.1/html/message_of_teachers.html"
//    };

/**
   		"http://pages.ntrig.webs.com/webs/web1/image_1.jpg",
		"http://pages.ntrig.webs.com/webs/web1/image_2.jpg",
		"http://pages.ntrig.webs.com/webs/web1/image_3.jpg",
		"http://pages.ntrig.webs.com/webs/web1/image_4.jpg"
 */
//    char *url = "http://pages.ntrig.webs.com/webs/web1/image_3.jpg";

//#define NTG_HTT_URL_MAX      22
static char *urls[NTG_HTT_URL_MAX] = {
		"http://pages.ntrig.webs.com/webs/web1/image_1.jpg",
		"http://pages.ntrig.webs.com/webs/web1/image_2.jpg",
		"http://pages.ntrig.webs.com/webs/web1/image_3.jpg",
		"http://pages.ntrig.webs.com/webs/web1/image_4.jpg",
		"http://pages.ntrig.webs.com/webs/web1/js_3.js",
		"http://pages.ntrig.webs.com/webs/web1/js_2.js",
		"http://pages.ntrig.webs.com/webs/web1/js_1.js",
		"http://pages.ntrig.webs.com/webs/web1/index.html",
		"http://pages.ntrig.webs.com/webs/web2/image_1.jpg",
		"http://pages.ntrig.webs.com/webs/web2/image_2.gif",
		"http://pages.ntrig.webs.com/webs/web2/image_3.jpg",
		"http://pages.ntrig.webs.com/webs/web2/image_4.jpg",
		"http://pages.ntrig.webs.com/webs/web2/js_3.js",
		"http://pages.ntrig.webs.com/webs/web2/js_2.js",
		"http://pages.ntrig.webs.com/webs/web2/js_1.js",
		"http://pages.ntrig.webs.com/webs/web2/index.html",
		"http://pages.ntrig.webs.com/webs/web3/image_1.jpg",
		"http://pages.ntrig.webs.com/webs/web3/image_2.jpg",
		"http://pages.ntrig.webs.com/webs/web3/image_3.jpg",
		"http://pages.ntrig.webs.com/webs/web3/image_4.gif",
		"http://pages.ntrig.webs.com/webs/web3/js_2.js",
		"http://pages.ntrig.webs.com/webs/web3/js_1.js"
};

/**
 * http处理入口
 * @param[in] cycle 循环体对象
 * @param[in] user 用户对象
 * @param[in] url url字符串
 * @return 成功返回NTG_OK,否则返回对于的错误码
 * @note 当所有的准备就绪后才取获取request对象
 */
int ntg_http_start(ntg_cycle_t *cycle, ntg_message_t *msg)
{

    ntg_http_request_t  *rq;

    struct timeval tv;

    ntg_request_message_t *rqt;
    if(msg->type != NTG_REQUEST_TYPE){
    	printf("-----------> NO NTG_REQUEST_TYPE\n");
    	return NTG_HTTP_INTERNAL_FD;
    }
    rqt = &msg->body.rqt;

    /* 获取请求对象 */
    rq = ntg_http_get_request(cycle);
    if(rq == NULL) {
    	return NTG_HTTP_NOT_REQUEST;
    }

    /* 设置请求属性 */
	rq->csm_id = rqt->csm_id;
	rq->gp_id = rqt->gp_id;
	rq->user_id = rqt->user_id;
	rq->url_id = 0;

	/* 获取目标url集 */
	if(ntg_http_get_urls(rq) != NTG_OK){
    	rq->req_state = NTG_REQ_STATE_DONE;
    	ntg_http_final_process(rq);
		return NTG_HTTP_NOT_ADDR;
	}

	/* 获取首页url */
	ntg_int_t ret = ntg_http_get_next_url(rq);

	if(ret != NTG_OK){
		//结束，释放资源
    	rq->req_state = NTG_REQ_STATE_DONE;
    	ntg_http_final_process(rq);
		return NTG_ERROR;
	}
//	 printf("------------ntg_http_start--------3------\n");
    /* 构造请求 */
    if ( ntg_http_build_request(rq) != NTG_OK){
    	rq->req_state = NTG_REQ_STATE_DONE;
    	ntg_http_final_process(rq);
    	return NTG_HTTP_BUILD_FAILED;
    }

	/* 设置统计时间 */
	if (evutil_gettimeofday(&tv, NULL) < 0){
		printf("------------evutil_gettimeofday-----------\n");
    	rq->req_state = NTG_REQ_STATE_DONE;
    	ntg_http_final_process(rq);
		return NTG_ERROR;
	}

	/* 统计响应时间 TODO 开始时间还没有设置 */
	rq->upstream_state->response_tv.tv_sec = tv.tv_sec;
	rq->upstream_state->response_tv.tv_usec = tv.tv_usec;

//    u_char *ip = (u_char*)&rq->addr.sin_addr.s_addr;
//    printf("ip address: %d.%d.%d.%d:port-->%d\n",ip[0], ip[1], ip[2], ip[3],
//    		ntohs(rq->addr.sin_port));
    /* 建立链接 */

	if(ntg_http_connect_init(rq) != NTG_OK){
		//释放资源，结束请求
    	rq->req_state = NTG_REQ_STATE_DONE;
    	ntg_http_final_process(rq);
		return NTG_ERROR;
	}
//    printf("------------ntg_http_start--------end------\n");
    return NTG_OK;
}

#define NTG_FEEDBACK_INTERVAL 20
void ntg_http_final_process(ntg_http_request_t *rq) {
	ntg_int_t ret;
	struct timeval tv;
	ntg_message_t msg;
	ntg_record_message_t *rcd;
	ntg_feedback_message_t *fb;


	static int count = 0;

	if (rq->cycle->blc_type) {
		count++;
		if (count >= NTG_FEEDBACK_INTERVAL) {
			//printf("+++++++++++NTG_FEEDBACK_INTERVAL>>>>>>>%d>>>>>>>>>\n", (int)ntg_process_slot);
			count = 0;
			/* 清空发下命令 */ntg_memzero(&msg, sizeof(ntg_message_t));
			msg.type = NTG_FEEDBACK_TYPE;
			fb = &msg.body.fb;
			fb->proc_slot = ntg_process_slot;
			fb->reqs = (int) (rq->cycle->request_n - rq->cycle->free_req_n);

			if (ntg_write_channel(ntg_processes[0].channel[0], &msg,
					sizeof(msg.type) + sizeof(*fb), rq->log) != NTG_OK) {
				//管道已关闭
				printf("+++++++++++++++failed to send feedback>>>>>>>>>>>>>>>>\n");
				ntg_processes[1].exiting = 1;
				ntg_log_error(NTG_LOG_CRIT, rq->log, 0,
						"failed to send a message to the (%P) process.", ntg_processes[1].pid);
			}
		}
	}

	/* 清空发下命令 */
	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_RECORD_TPYE;
	rcd = &msg.body.rcd;

	/* 设置 csm_id, gp_id, url_id*/
	rcd->csm_id = rq->csm_id;
	rcd->gp_id = rq->gp_id;
	rcd->user_id = rq->user_id;
	rcd->url_id = rq->url_id;
	rcd->state_code = rq->upstream_state->status;
	rcd->send = rq->out_buffer.pos - rq->out_buffer.start;
	rcd->receive = rq->upstream_state->response_length;

	if ( ntg_db_mysql_insert(rq->cycle->db, rcd) != NTG_OK){
		//日志记录
		printf("ntg_db_mysql_insert-----------\n");
	}

	//printf("---keep alive =  %d======response--length----%d-\n",rq->keepalive, (long int)rq->upstream_state->response_length);
//    if (ntg_write_channel(ntg_processes[1].channel[0],&msg,
//    		sizeof(msg.type) + sizeof(*rcd), rq->log) != NTG_OK)
//     {
//    	//管道已关闭
//    	printf("+++++++++++++++failed to send record>>>>>>>>>>>>>>>>\n");
//    	ntg_processes[1].exiting = 1;
//    	ntg_log_error(NTG_LOG_CRIT, rq->log, 0,
//                   "failed to send a message to the (%P) process.", ntg_processes[1].pid);
//     }

	static int num=0;
	static long long max=0;
	static long long min=0;
	static long long avg=0;

	num++;

		///printf("---{{{{{{-rq->urln---%ld------\n", (long int)rq->url_n);
		long int ms = (long int)rq->upstream_state->header_tv.tv_sec * 1000 +
				(long int)rq->upstream_state->header_tv.tv_usec/1000;
		avg = (avg*(num-1) + ms)/num;
		if(min > ms){
			min = ms;
		}
		if(max < ms){
			max = ms;
		}
		if(num%1000==0){
			printf("-response-time--min-%lld--avg--%lld--max--%lld (ms)---\n",
					min, avg, max);
		}


//	}

	if(rq->req_state == NTG_REQ_STATE_DONE){

		ntg_http_free_request(rq, 0);
	}else if (rq->req_state == NTG_REQ_STATE_NEXT) {

		for (;;) {
			//重用请求对象
			ntg_http_reuse_request(rq);

			//获取下一个url
			ret = ntg_http_get_next_url(rq);
			if (ret == NTG_DONE) {
				//结束，释放资源
				ntg_http_free_request(rq, 0);
				return;
			} else if (ret != NTG_OK) {
				//记录日志
				continue;
			}

			/* 构造请求 */
			if ( ntg_http_build_request(rq) != NTG_OK){
				//记录日志
				continue;
			}

			/* 设置统计时间 */
			if (evutil_gettimeofday(&tv, NULL) < 0){
				continue ;
			}

			/* 统计响应时间 TODO 开始时间还没有设置 */
			rq->upstream_state->response_tv.tv_sec = tv.tv_sec;
			rq->upstream_state->response_tv.tv_usec = tv.tv_usec;

		    /* 建立链接 */
			if(ntg_http_connect_init(rq) != NTG_OK){
				//释放资源，结束请求
				continue;
			}
			break;
		}
	}else{
		ntg_http_free_request(rq, 0);
	}

}
/**
 * 获取urls
 * @param[in] rq 请求对象
 * @return 成功返回返回NTG_OK,否则返回TNG_ERROR
 * @note 只进行错误日志处理
 */
static ntg_int_t ntg_http_get_urls(ntg_http_request_t *rq){
	char **us;
	//建立数据库链接

	//获取urls
	if(rq->cycle->isurl){
		us = rq->cycle->ntg_urls;
		//printf("-------ntg_urls--------------------123123---\n");
	} else {
		us = urls;
		//printf("-------ntg_urls--------------------urls---\n");
	}

	//设置请求的urls
	if(us == NULL){
		//记录日志
		rq->req_state =NTG_REQ_STATE_DONE;
		return NTG_ERROR;
	}

	rq->urls = us;
	rq->url_n = ntg_math2_get_lnline_num()+1;//%(NTG_HTT_URL_MAX-1) +1;
//	printf("------url_n-----%d----------\n", rq->url_n);

	rq->index = 0;
	rq->req_state = NTG_REQ_STATE_URLS;
	return NTG_OK;
}

/**
 * 获取下一个url，并进行DNS解析
 * @param[in] rq 请求对象
 * @return 获取成功返回NTG_OK，没有url返回NTG_DONE, 否则返回NTG_ERROR
 */
static ntg_int_t ntg_http_get_next_url(ntg_http_request_t *rq){
	ntg_int_t ret;
	struct evhttp_uri *uri;
	const char *host;
	int port;

	if (rq->index >= rq->url_n){
		return NTG_DONE;
	}

	//获取下一个url
	uri = evhttp_uri_parse(rq->urls[rand()%rq->cycle->url_n]);
	rq->index++;
	if ( uri == NULL) {
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
				"evhttp_uri_parse is failed");
		return NTG_HTTP_INTERNAL_URL;
	}

	/* host处理 */
	host = evhttp_uri_get_host(uri);
	if( host == NULL || strcmp(host, "") == 0){
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
				"host is not exit");
		evhttp_uri_free(uri);
		return NTG_HTTP_INTERNAL_URL;
	}

	if(strlen(host) > NTG_HTTP_MAX_HOSTNAME -1){
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
				"host is long than default (%d)", NTG_HTTP_MAX_HOSTNAME);
		evhttp_uri_free(uri);
		return NTG_HTTP_HOST_TOO_LONG;
	}

	/* hostname比较 */
	if(ntg_strncmp((u_char*)rq->hostname, (u_char*)host, strlen(host)) != 0){
		rq->identical = 0;

		ntg_cpystrn((u_char*)rq->hostname, (u_char*)host, strlen(host)+1);
		//printf("hsot name = %s==sock==%d\n", rq->hostname, s);
		/* 端口处理 */
		if((port = evhttp_uri_get_port(uri)) == -1 ){
			port = 80;
		}

		/* 设置地址属性 */
	    rq->addr.sin_family = AF_INET;
	    rq->addr.sin_port = htons(port);
//	    inet_pton(AF_INET,"192.168.10.2", &rq->addr.sin_addr.s_addr);

	    /* 阻塞方式获取地址 TODO 后期换成非阻塞实现 */
	    ret = ntg_get_ipv4_for_host(rq->hostname, port, (struct sockaddr *)&rq->addr);
	    if(ret != NTG_OK) {
			ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
					"error while resolving");
			return NTG_HTTP_NOT_ADDR;
	    }
	} else {
		rq->identical = 1;
	}

	if(rq->uri != NULL){
		evhttp_uri_free(rq->uri);
	}
	rq->uri = uri;

	return NTG_OK;
}

/**
 * 进行链接初始化
 * @param rq
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 * @note 只进行了错误日志记录，资源释放交由调用者
 * @note 初始化了链接的读写事件，并注册了读事件
 */
static ntg_int_t ntg_http_connect_init(ntg_http_request_t *rq){
	ntg_int_t rt;
	 int        err;
	 ntg_int_t level;


	 if(rq->identical==0 || rq->keepalive == 0 || (ntg_http_test_connect(rq) != NTG_OK)){
	 	 /* 创建socket */
		if(rq->sock >= 0){
			close(rq->sock);
		}
		/* 创建套接字 */
		rq->sock = ntg_make_tcp_socket(rq);

		if( rq->sock == (evutil_socket_t )-1) {
			ntg_log_error(NTG_LOG_ALERT, rq->log, 0,"ntg_make_tcp_socket is failed");
			return NTG_HTTP_INTERNAL_FD;
		}

	    /* 建立链接 */
		rt =  connect(rq->sock, (struct sockaddr*) &rq->addr, sizeof(rq->addr));
		if (rt == -1) {
			err = ntg_socket_errno;

			if (err != NTG_EINPROGRESS
					) {
				if (err == NTG_ECONNREFUSED
	#if (NTG_LINUX)
						/*
						 * Linux returns EAGAIN instead of ECONNREFUSED
						 * for unix sockets if listen queue is full
						 */
						|| err == NTG_EAGAIN
	#endif
						|| err == NTG_ECONNRESET || err == NTG_ENETDOWN
						|| err == NTG_ENETUNREACH || err == NTG_EHOSTDOWN
						|| err == NTG_EHOSTUNREACH) {
					level = NTG_LOG_ERR;

				} else {
					level = NTG_LOG_CRIT;
				}

				ntg_log_error(level, rq->log, err, "connect() to %s failed",
						rq->hostname);

				return NTG_DECLINED;
			}

		}

		ntg_connections +=1;
		/* 注册读写回调函数 */
		if(rq->ev_write != NULL)
			event_free(rq->ev_write);
		if(rq->ev_read != NULL)
			event_free(rq->ev_read);

		rq->ev_write = event_new(rq->base, rq->sock, EV_WRITE, ntg_http_write_cb, (void*)rq);
		rq->ev_read = event_new(rq->base, rq->sock, EV_READ , ntg_http_read_cb, (void*)rq);
	}

    event_add(rq->ev_write, &rq->timeout);

	return NTG_OK;
}



/**
 * 测试链接是否有效
 * @param[in] rq 请求对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
static ntg_int_t ntg_http_test_connect(ntg_http_request_t *rq) {
	int err;
	socklen_t len;

	err = 0;
	len = sizeof(int);

	if(rq->sock < 0){
		return NTG_ERROR;
	}
	/*
	 * BSDs and Linux return 0 and set a pending error in err
	 * Solaris returns -1 and sets errno
	 */

	if (getsockopt(rq->sock, SOL_SOCKET, SO_ERROR, (void *) &err, &len) == -1) {
		err = ntg_errno;
	}

	if (err) {
//		rq->log->action = "connecting to web server";
//		ntg_log_error(level, rq->log, err, "connect() to %s failed", rq->hostname);
		return NTG_ERROR;
	}

	return NTG_OK;
}



/**
 * 链接可写回调函数
 * @param[in] sock 套接字
 * @param[in] flags 事件标志
 * @param[in] args 出入的参数
 */
static void ntg_http_write_cb(evutil_socket_t sock, short flags, void * args)
{
//	int do_write = 1;
    ssize_t       n, size;
    ntg_err_t err;
	ntg_http_request_t *rq = (ntg_http_request_t *)args;
	/* 超时释放所有的资源 */
	if(!(flags & EV_WRITE)){
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
				"not to write event");
		printf("ntg_http_write_cb---------error----\n");
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	if((flags & EV_TIMEOUT)){
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0,
				"conncte is timeout");
		printf("ntg_http_write_cb---------timeout----\n");
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}


	ntg_log_debug1(NTG_LOG_DEBUG_HTTP, rq->log, 0, "connecte to %s", rq->hostname);

    for ( ;; ) {
    	size = rq->out_buffer.last - rq->out_buffer.pos;

        if(size == 0){
        	/* 发送成功 */
        	goto sucess;
        }

        n = send(rq->sock, rq->out_buffer.pos, size , 0);

        ntg_log_debug3(NTG_LOG_DEBUG_EVENT, rq->log, 0,
                       "send: fd:%d %d of %d", rq->sock, n, size);

        if (n > 0) {

            rq->out_buffer.pos += n;
            continue;
        }

        err = ntg_socket_errno;

        if (n == 0) {
            ntg_log_error(NTG_LOG_ALERT, rq->log, err, "send() returned zero");
        }

        if (err == NTG_EAGAIN || err == NTG_EINTR) {

            ntg_log_debug0(NTG_LOG_DEBUG_EVENT, rq->log, err,
                           "send() not ready");
            /* 重新注册事件 */
            if (event_add(rq->ev_write, &rq->timeout) < 0){///TODO 添加超时时间
            	if(rq->index==1){
            		rq->req_state = NTG_REQ_STATE_DONE;
            	}else{
            		rq->req_state = NTG_REQ_STATE_NEXT;
            	}
            	ntg_http_final_process(rq);
            }
            return;

        } else {
        	if(rq->index==1){
        		rq->req_state = NTG_REQ_STATE_DONE;
        	}else{
        		rq->req_state = NTG_REQ_STATE_NEXT;
        	}
        	ntg_http_final_process(rq);
            return;
        }
    }// end of    for ( ;; )

	sucess:
//	printf("ntg_http_write_cb-----finish-------\n");
	/* 分配接收缓存 */
	/* 分配空间 */
	if(rq->in_buffer.start == NULL){
		rq->in_buffer.start = ntg_palloc(rq->pool, rq->in_size);
			if (rq->in_buffer.start == NULL) {
				/* 结束请求 */
		    	if(rq->index==1){
		    		rq->req_state = NTG_REQ_STATE_DONE;
		    	}else{
		    		rq->req_state = NTG_REQ_STATE_NEXT;
		    	}
		    	ntg_http_final_process(rq);
				return;
			}
	}

	rq->in_buffer.end = rq->in_buffer.start + rq->in_size;
	rq->in_buffer.pos = rq->in_buffer.last = rq->in_buffer.start;

	rq->state = 0;
	rq->read_event_handler = ntg_http_handler_header;
	rq->process_header = ntg_http_process_status_line;//头部处理函数

	if (event_del(rq->ev_write) < 0){
		/* 错误处理 */
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	if( event_add((struct event*)rq->ev_read, &rq->timeout) < 0){
		/* 错误处理 */
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}
}

/**
 * 链接可读回调函数
 * @param[in] sock 套接字
 * @param[in] flags 事件标志
 * @param[in] args 出入的参数
 */
static void ntg_http_read_cb(evutil_socket_t sock, short flags, void * args) {
	ntg_http_request_t *rq;

	rq = (ntg_http_request_t *) args;

	/* 超时释放所有的资源 */
	if ((flags & EV_TIMEOUT)) {
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0, "conncte is timeout");
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	if (!(flags & EV_READ)) {
		ntg_log_error(NTG_LOG_ALERT, rq->log, 0, "conncte is timeout");
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	rq->read_event_handler(rq);

	//printf("--------------ntg_http_read_cb---------end------\n");

}

/**
 * 处理http头的读事件回调函数
 * @param[in] rq 请求对象
 * @note 负责整个http头的接收和处理.在处理过程中回进行process_header函数指针的切换
 * @note 如果出错,自行处理,对于读事件回调函数是透明的
 * @note 如果头部处理完毕,回跳转到处理响应包体
 */
static void ntg_http_handler_header(ntg_http_request_t *rq){

	ssize_t n;
	struct timeval tv;
	ntg_int_t rc;

	//printf("--------------ntg_http_handler_header---------start------\n");
    ntg_log_debug0(NTG_LOG_DEBUG_HTTP, rq->log, 0,
                   "http  handler header");
    rq->log->action = "reading response header";

	if ((rq->in_buffer.start == NULL) && rq->req_state == NTG_REQ_STATE_RECV_START) {
		rq->in_buffer.start = ntg_palloc(rq->pool, rq->in_size);
		printf("--------------ntg_http_handler_header--------9------\n");
		if (rq->in_buffer.start == NULL) {
			/* 结束请求 */
	    	if(rq->index==1){
	    		rq->req_state = NTG_REQ_STATE_DONE;
	    	}else{
	    		rq->req_state = NTG_REQ_STATE_NEXT;
	    	}
	    	ntg_http_final_process(rq);
			return;
		}

		rq->in_buffer.end = rq->in_buffer.start + rq->in_size;
		rq->in_buffer.pos = rq->in_buffer.last = rq->in_buffer.start;
		rq->req_state = NTG_REQ_STATE_RECV;
		rq->in_buffer.temporary = 1;

		/* 分配头部接收链表 */
	}

	/* 必须进行循环的读,直到读完, NTG_AGAIN, 或 NTG_ERROR */
	for (;;) {
		/* 读取包体时,进行接收缓存的剩余空间的判断 */

		/* 读取网络数据 */
		n = ntg_http_recv(rq, rq->in_buffer.last,
				rq->in_buffer.end - rq->in_buffer.last);

		/* 本次事件数据已接收完毕 */
		if (n == NTG_AGAIN) {

			/* 重新注册事件 */
			if (event_add((struct event*) rq->ev_read, &rq->timeout) < 0) {
				/* 错误处理 */
				printf("--------------ntg_http_handler_header---------2------\n");
		    	if(rq->index==1){
		    		rq->req_state = NTG_REQ_STATE_DONE;
		    	}else{
		    		rq->req_state = NTG_REQ_STATE_NEXT;
		    	}
		    	ntg_http_final_process(rq);
				return;
			}

			return;
		}

		if (n == 0) {
			//printf("--------------ntg_http_handler_header---------8------\n");
			ntg_log_error(NTG_LOG_ERR, rq->log, 0,
					"upstream  closed connection");
		}
		/* 出错或数据已接收完(被上游主动关闭) */
		if (n == NTG_ERROR || n == 0) {
			/* TODO 判断是否有下一个 这里暂时直接结束 */

	    	if(rq->index==1){
				printf("--------------ntg_http_handler_header---------3------\n");
	    		rq->req_state = NTG_REQ_STATE_DONE;
	    	}else{

	    		rq->req_state = NTG_REQ_STATE_NEXT;
	    	}
	    	ntg_http_final_process(rq);
			return;
		}

		/* 数据处理 */
		rq->in_buffer.last += n;
		rq->upstream_state->response_length += n;

		/**
		 * process_header内部回进行函数指针的切换,如开始是解析状态行的函数,而后是解析头域的函数.
		 * 只有整个http头都解析完毕后,才会返回NTG_OK
		 */
		rc = rq->process_header(rq);

		if (rc == NTG_AGAIN) {

			if (rq->in_buffer.last == rq->in_buffer.end) {
				//已经分配了比较大的接收缓存,如果整个头部都不能接收完,说明头部过大
				ntg_log_error(NTG_LOG_ERR, rq->log, 0,
						"upstream sent too big header");
//				ntg_http_upstream_next(rq, u, NTG_HTTP_UPSTREAM_FT_INVALID_HEADER);
				/* TODO 判断是否有下一个 这里暂时直接结束 */
				printf("--------------ntg_http_handler_header---------4------\n");
		    	if(rq->index==1){
		    		rq->req_state = NTG_REQ_STATE_DONE;
		    	}else{
		    		rq->req_state = NTG_REQ_STATE_NEXT;
		    	}
		    	ntg_http_final_process(rq);
				return;
			}

			continue;
		}
		break;
	}
	//printf("--------------ntg_http_handler_header---------5------\n");
	/* 其他出错情况 */
	if (rc == NTG_HTTP_PARSE_INVALID_HEADER) {
		printf("--------------ntg_http_handler_header---------7------\n");
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	if (rc == NTG_ERROR) {
		printf("--------------ntg_http_handler_header---------6------\n");
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	/* rc == NTG_OK */
	if (evutil_gettimeofday(&tv, NULL) < 0){
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}
	/* 统计响应时间 */
	rq->upstream_state->header_tv.tv_sec =
			tv.tv_sec - rq->upstream_state->response_tv.tv_sec;
	rq->upstream_state->header_tv.tv_usec =
			tv.tv_usec - rq->upstream_state->response_tv.tv_usec;

	/* 没有响应实体的情况 */
	if (rq->headers_in.status_n >= NTG_HTTP_SPECIAL_RESPONSE) {

		/* 下一个处理 */
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	/* request content in memory */

	if ( ntg_http_filter_init(rq) == NTG_ERROR) {
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);
		return;
	}

	if (rq->length == 0) {//rq->length在filter的初始化函数中被设置
    	if(rq->index==1){
    		rq->req_state = NTG_REQ_STATE_DONE;
    	}else{
    		rq->req_state = NTG_REQ_STATE_NEXT;
    	}
    	ntg_http_final_process(rq);;
		return;
	}

	/* 进行包体丢弃处理 */
	//printf("----------ntg_http_discard_response_body--------start-----\n");
	ntg_http_discard_response_body(rq);

}


/**
 * 处理状态行
 * @param[in] r 请求对象
 * @return 出错返回NTG_ERROR, 继续返回NTG_AGAIN, 状态行解析完返回解析头域的结果
 * @note 在状态行解析成功后会修改process_header处理函数,跳转到处理头域
 */
static ntg_int_t ntg_http_process_status_line(ntg_http_request_t *r) {
	size_t len;
	ntg_int_t rc;

	rc = ntg_http_parse_status_line(r, &r->in_buffer, &r->status);

	if (rc == NTG_AGAIN) {
		return rc;
	}

	if (rc == NTG_ERROR) {

		ntg_log_error(NTG_LOG_ERR, r->log, 0,
				"upstream sent no valid HTTP/1.0 header");

		r->http_version = NTG_HTTP_VERSION_9;
		r->upstream_state->status = NTG_HTTP_OK;
		r->headers_in.connection_close = 1;

		return NTG_OK;
	}

	/* NTG_OK */

	if (r->upstream_state && r->upstream_state->status == 0) {
		r->upstream_state->status = r->status.code;
	}


    len = r->status.end - r->status.start;
    r->headers_in.status_line.len = len;

    r->headers_in.status_line.data = ntg_pnalloc(r->pool, len);
    if (r->headers_in.status_line.data == NULL) {
        return NTG_ERROR;
    }

    ntg_memcpy(r->headers_in.status_line.data, r->status.start, len);

	ntg_log_debug2(NTG_LOG_DEBUG_HTTP, r->log, 0,
			"http  status %ui \"%V\"", r->status.code,
			&r->headers_in.status_line);

	if (r->status.http_version < NTG_HTTP_VERSION_11) {
		r->headers_in.connection_close = 1;
	}

	r->process_header = ntg_http_process_header;

	return ntg_http_process_header(r);
}

/**
 * 头域处理函数
 * @param[in] r 请求对象
 * @return 头域全部成功解析返回NTG_OK, 再次解析返回NTG_AGAIN,
 * 出错返回NTG_ERROR,  非法头域返回NTG_HTTP_PARSE_INVALID_HEADER
 */
static ntg_int_t
ntg_http_process_header(ntg_http_request_t *r)
{
    ntg_int_t                       rc;
    ntg_table_elt_t                 t, *h;
    int i;

    h = &t;

    for ( ;; ) {

        rc = ntg_http_parse_header_line(r, &r->in_buffer, 0);

        if (rc == NTG_OK) {

            /* a header line has been parsed successfully */
            h->hash = r->header_hash;

            h->key.len = r->header_name_end - r->header_name_start;
            h->value.len = r->header_end - r->header_start;

            h->key.data = r->header_name_start;
            h->value.data = r->header_start;

            i=0;
            while(ntg_http_headers_in[i].name.len != 0 ){
            	if(ntg_strncasecmp(r->lowcase_header,
            			ntg_http_headers_in[i].name.data,
						ntg_http_headers_in[i].name.len) == 0){
            		if(ntg_http_headers_in[i].handler(r, h,
            				ntg_http_headers_in[i].offset) != NTG_OK){
                        return NTG_ERROR;
            		}
            	}
            	i++;
            }

            ntg_log_debug2(NTG_LOG_DEBUG_HTTP, r->log, 0,
                           "http header: \"%V: %V\"",
                           &h->key, &h->value);

            continue;
        }

        //printf("--------head---%d------\n%s", r->headers_in.connection_close, r->in_buffer.start);
        if (rc == NTG_HTTP_PARSE_HEADER_DONE) {

            /* a whole header has been parsed successfully */

            ntg_log_debug0(NTG_LOG_DEBUG_HTTP, r->log, 0,
                           "http header done");
            /* clear content length if response is chunked */

            if (r->headers_in.chunked) {
                r->headers_in.content_length_n = -1;
            }

            if (r->headers_in.status_n == NTG_HTTP_SWITCHING_PROTOCOLS) {
                r->keepalive = 0;

            } else {
            	r->keepalive = !r->headers_in.connection_close;
            }

            return NTG_OK;
        }

        if (rc == NTG_AGAIN) {
            return NTG_AGAIN;
        }

        /* there was error while a header line parsing */

        ntg_log_error(NTG_LOG_ERR, r->log, 0,
                      "upstream sent invalid header");

        return NTG_HTTP_PARSE_INVALID_HEADER;
    }
}
/**
 * http输入过滤初始化
 * @param[in] r 请求对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t
ntg_http_filter_init(ntg_http_request_t  *r)
{

    ntg_log_debug3(NTG_LOG_DEBUG_HTTP, r->log, 0,
                   "http filter init s:%d c:%d l:%O",
                   r->headers_in.status_n,  r->headers_in.chunked,
                   r->headers_in.content_length_n);

    /* as per RFC2616, 4.4 Message Length */

    if (r->headers_in.status_n == NTG_HTTP_NO_CONTENT
        || r->headers_in.status_n == NTG_HTTP_NOT_MODIFIED )
    {
        /* 1xx, 204, and 304 and replies to HEAD requests */
        /* no 1xx since we don't send Expect and Upgrade */
        r->length = 0;
        r->keepalive = !r->headers_in.connection_close;

    } else if (r->headers_in.chunked) {
        /* chunked */
        r->length = 1;

    } else if (r->headers_in.content_length_n == 0) {
        /* empty body: special case as filter won't be called */
        r->length = 0;
        r->keepalive = !r->headers_in.connection_close;

    } else {
        /* content length or connection close */
        r->length = r->headers_in.content_length_n;
    }

    return NTG_OK;
}




