/**
 * @file ntg_remote.h
 * @brief 远程管理模块
 * @details 负责监听前端的请求，处理前端发送过来的消息
 * @author tzh
 * @date May 31, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_REMOTE_H_
#define NTG_REMOTE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../message/ntg_message.h"
#include "ntg_table.h"

#define NTG_REMOTE_FROMAT_ERROR		100	//前端格式错误
#define NTG_REMOTE_EXIST_ERROR		101 //已存在错误
#define NTG_REMOTE_TOO_CSM_ERROR	102 //太多的用户数
#define NTG_REMOTE_END_ERROR	103 //太多的用户数
#define NTG_REMOTE_SUCCESS	200 //处理成功

/**
 * @name 前端链接对象
 */
typedef struct {
	ntg_socket_t fd;///socket
	ntg_int_t id;///消费者id
	struct bufferevent *bev;///链接缓存对象
	ntg_remote_t *rmt;
	unsigned live:1;///活跃标志
} ntg_connect_t;

/**
 * 远程管理对象
 */
struct ntg_remote_s {
	struct evconnlistener *listen;///前端监听
	ntg_hash_table_t *cnt_table;///链接hash表
	ntg_pool_t 	*pool;
	ntg_cycle_t	 *cycle;
};

ntg_int_t ntg_remote_init(ntg_cycle_t *cycle);
ntg_int_t ntg_remote_result_handler(ntg_remote_t *rmt, ntg_result_message_t *rm);
#endif /* NTG_REMOTE_H_ */
