/**
 * @file ntg_record.c
 * @brief
 * @details
 * @author tzh
 * @date Mar 5, 2016
 * @version V0.1
 * @copyright tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_record.h"

static int ntg_get_records(ntg_record_manage_t *mg, ntg_record_t *cd);

/**
 * master进程初始化函数
 * @param[in] cycle 全局循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 初始化虚拟用户层的一些资源
 * @note 在子进程初始化结束后调用，因为这不是与子进程共用的
 * @note 如果master初始化失败，应该先通知多有的子进程结束
 * ，然后自己也结束。可以通过设置结束标志完成。
 */
#define NTG_RECORD_NUM 1024
ntg_int_t ntg_record_init(ntg_cycle_t *cycle) {

	ntg_record_manage_t *manage;

	ntg_pool_t *pool;

	//私有化自己的内存池
	pool = ntg_create_pool(NTG_DEFAULT_POOL_SIZE, cycle->log);
	if (pool == NULL) {
		return NTG_ERROR;
	}

	/* 管理对象 */
	manage = ntg_palloc(cycle->pool, sizeof(ntg_record_manage_t));
	if (manage == NULL) {
		return NTG_ERROR;
	}
	cycle->record_mg = manage;

	manage->cycle = cycle;
	manage->log = cycle->log;
	manage->pool = pool;
	manage->record_n = NTG_RECORD_NUM;

	/* 初始化管理对象 */
	if (ntg_record_manage_init(manage) != NTG_OK) {
		printf("-------------ntg_user_manage_init----------------error------n");
		return NTG_ERROR;
	}

	/* 用户行为函数的初始化 TODO 暂时没有实现 */

	return NTG_OK;
}

ntg_record_t* ntg_get_record(ntg_record_manage_t *mg) {
	ntg_record_t *rd=NULL;

	if ((mg->enqueue_count - mg->dequeue_count) >= (mg->record_n - 1)) {
		return rd;
	} else {
		unsigned int enqueue_count = mg->enqueue_count;
		unsigned int i = enqueue_count & (mg->record_n - 1);
		rd = &mg->records[i];
		mg->enqueue_count = enqueue_count + 1;
	}
	return rd;
}

static int ntg_get_records(ntg_record_manage_t *mg, ntg_record_t *cd) {
	int result;

	if (mg->enqueue_count == mg->dequeue_count) {
		result = -1;
	} else {
		unsigned int dequeue_count = mg->dequeue_count;
		unsigned int i = dequeue_count & (mg->record_n - 1);
		ntg_record_t temp = mg->records[i];
		*cd = temp;
		mg->dequeue_count = dequeue_count + 1;
		result = 0;
	}

	return result;
}
