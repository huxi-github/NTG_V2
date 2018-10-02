/**
 * @file ntg_user_manage.h
 * @brief
 * @details
 * @author tzh
 * @date Jan 27, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_USER_MANAGE_H_
#define NTG_USER_MANAGE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user.h"
#include "ntg_user_balance.h"
#include "../message/ntg_message.h"

typedef void ntg_user_manage_handler(ntg_user_manage_t *manage);

typedef void (*ntg_task_process_pt)(ntg_user_manage_t *mg);
/**
 * @name 虚拟用户层管理对象
 * @note 提供私有的pool
 */
struct ntg_user_manage_s {
	ntg_cycle_t				*cycle;///循环体对象
	ntg_log_t				*log;///日志对象
	ntg_pool_t  			*pool;///用户内部内存池

	/* 用户池 */
	ntg_user_t 	*users;///用户对象资源
	ntg_uint_t	user_n;///总的用户数
	ntg_user_t	*free_users;///可用用户
	ntg_uint_t	free_user_n;///可用用户数

	/* 消费者池 Consumer */
	ntg_user_consumer_t *consumers;///消费者对象池
	ntg_uint_t          csm_n;
	ntg_user_consumer_t *free_csms;///可用消费者
	ntg_uint_t          free_csm_n;///可用消费者数

	ntg_hash_table_t *csm_table;///消费者hash表

	/* 任务处理队列 */
	ntg_uint_t factor;///每次最多添加的用户数目因子
	event_callback_fn   task_pt;///任务处理指针
	void *tasks;///任务链表
	ntg_uint_t task_n;///任务数
    struct event * ev_task;///任务处理事件

	/* 负载均衡 */
	ntg_user_balance_t *blc;///TODO 还没有添加

	unsigned task_flag:1;///任务标志，1表示已启动任务处理，0表示没有启动任务处理

};

ntg_int_t ntg_user_manage_init(ntg_user_manage_t *mg);
ntg_int_t ntg_user_message_handler(ntg_user_manage_t *mg, ntg_message_t *msg);

#endif /* NTG_USER_MANAGE_H_ */
