/**
 * @file ntg_record.h
 * @brief
 * @details
 * @author tzh
 * @date Mar 5, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_RECORD_H_
#define NTG_RECORD_H_

#include "../ntg_config.h"
#include "../ntg_core.h"


/**
 * @name 记录对象
 */
struct ntg_record_s {
	unsigned   csm_id:8;///消费者id
	unsigned   gp_id:4;///组id
	unsigned   state_code:4;///组id
	unsigned   user_id;///目标url id
	unsigned   url_id;///目标url id
	time_t     time;///时间戳
	ntg_uint_t send;///发送大小
	ntg_uint_t receive;///接收大小
};

/**
 * @name 记录管理对象
 * @note 提供私有的pool
 */
struct ntg_record_manage_s {
	ntg_cycle_t				*cycle;///循环体对象
	ntg_log_t				*log;///日志对象
	ntg_pool_t  			*pool;///内部内存池

	/* 记录池 */
	ntg_record_t 	*records;///记录对象资源
	ntg_uint_t	record_n;///总的记录数
	unsigned int dequeue_count;
	unsigned int enqueue_count;

	ntg_record_t	*free_records;///可用记录
	ntg_uint_t	free_record_n;///可用记录数

	/* 数据库对象 */

};

ntg_int_t ntg_record_init(ntg_cycle_t *cycle);
ntg_int_t ntg_record_manage_init(ntg_record_manage_t *mg);
ntg_record_t* ntg_get_record(ntg_record_manage_t *mg);

#endif /* NTG_RECORD_H_ */
