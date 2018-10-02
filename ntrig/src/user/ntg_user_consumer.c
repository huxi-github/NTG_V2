/**
 * @file ntg_user_consumer.c
 * @brief
 * @details
 * @author tzh
 * @date Jan 27, 2016
 * @version V0.1
 * @copyright tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user_consumer.h"
#include "ntg_user.h"



/**
 * 计算消费者对象的key
 * @param[in] data 数据指针
 * @return 返回对象的key
 */
ntg_uint_t ntg_user_consumer_get_key(void *data){
	ntg_uint_t key;
	ntg_user_consumer_t *csm;

	csm = (ntg_user_consumer_t *)data;
	key = csm->id;

	return key;
}

/**
 * 比较两个消费者对象
 * @param[in] d1 消费者对象1
 * @param[in] d2 消费者对象2
 * @return 如果两个对象的id相等返回0
 */
ntg_uint_t ntg_user_consumer_compare(void *d1, void *d2){

	ntg_user_consumer_t *csm1, *csm2;

	csm1 = (ntg_user_consumer_t *)d1;
	csm2 = (ntg_user_consumer_t *)d2;

	return (csm1->id - csm2->id);
}
