/**
 * @file ntg_user_balance.h
 * @brief
 * @details
 * @author tzh
 * @date Mar 13, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_USER_BALANCE_H_
#define NTG_USER_BALANCE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user.h"
#include "../message/ntg_message.h"
/**
 * @name 负载均衡对象
 */
struct ntg_user_balance_s {
	ntg_user_manage_t *mg;
	ntg_uint_t min_reqs;///< 最小请求数
	ntg_uint_t cur_slot;///< min_reqs对应的进程槽
	ntg_uint_t count;///< 计数器
	ntg_uint_t n;///< 流量产生进程数
	ntg_process_t *pros;///< 进程槽

	ntg_int_t (*blance)(ntg_user_manage_t *mg, ntg_message_t *msg);///< 均衡处理函数
};

ntg_int_t ntg_user_balance_init(ntg_user_manage_t *mg);

#endif /* NTG_USER_BALANCE_H_ */
