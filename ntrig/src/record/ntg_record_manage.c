/**
 * @file ntg_record_manage.c
 * @brief
 * @details
 * @author tzh
 * @date Mar 5, 2016
 * @version V0.1
 * @copyright tzh
 */

#include "ntg_record.h"

/**
 * 初始化记录管理对象
 * @param[in] mg 记录管理对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 在master进程初始化中被调用
 */
ntg_int_t ntg_record_manage_init(ntg_record_manage_t *mg){
	ntg_pool_t *pool;
	ntg_record_t *records;

//	%printf("-------------ntg_record_manage_init----------------1------\n");
	pool = mg->pool;

	records = ntg_palloc(pool, sizeof(ntg_record_t) * mg->record_n);
	if (records == NULL) {
		return NTG_ERROR;
	}

//	printf("-------------ntg_record_manage_init----------------2-----\n");
	mg->records = records;
	mg->dequeue_count=0;
	mg->enqueue_count=0;

	/* 将用户链表化为用户池 */
//	r = mg->records;
//	i = mg->record_n;
//	r_next = NULL;
//
//	do {
//		i--;
//		r[i].data = u_next;
//		r_next = &r[i];
//	} while (i);
//
//	printf("-------------ntg_record_manage_init----------------3------\n");
//	mg->free_records = r_next;
//	mg->free_record_n = mg->record_n;

	/* TODO 数据库初始化 */

	return NTG_OK;
}
