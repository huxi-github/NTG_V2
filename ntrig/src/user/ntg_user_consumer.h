/**
 * @file ntg_user_consumer.h
 * @brief
 * @details
 * @author tzh
 * @date Jan 27, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_USER_CONSUMER_H_
#define NTG_USER_CONSUMER_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user.h"


#define NTG_USER_GROUP_SIZE 10
/**
 * @name 消费者对象
 */
struct ntg_user_consumer_s {
	void      *data;///
	ntg_int_t id;///消费者id
	ntg_str_t name;///消费者名
	/* 组资源 */
	ntg_user_group_t *gps[NTG_USER_GROUP_SIZE];//TODO 每个消费者最大可支持创建10用户组，用户组时独立分配内存的

};

ntg_uint_t ntg_user_consumer_get_key(void *data);
ntg_uint_t ntg_user_consumer_compare(void *d1, void *d2);

#endif /* NTG_USER_CONSUMER_H_ */
