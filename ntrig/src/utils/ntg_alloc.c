/*
 * ntg_alloc.c
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_string.h"
#include "ntg_alloc.h"


/**
 * @name 页面大小属性
 * @{
 */
uint  ntg_pagesize;///< 系统页面大小
uint  ntg_pagesize_shift;///< 偏移大小
uint  ntg_cacheline_size;///< 缓存行大小
/**@}*/
/**
 * 分配内存
 * @param size 分配内存大小
 * @param log	日志对象
 * @return 分配的内存首地址
 * @note 带错误日志记录, 调用者需要自行判断是否分配成功
 */
void * ntg_alloc(size_t size, ntg_log_t *log){
	void *p;
	p = malloc(size);
	if (p == NULL) { //出错记录日志文件

		ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
				"malloc(%uz) failed",size);
	}

	ntg_log_debug2(NTG_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

	return p;
}


/**
 * 分配内存空间，并且将空间内容初始化为0
 * @param size 分配内存大小
 * @param log 日志对象
 * @return 分配的内存首地址
 * @note 带错误日志记录, 调用者需要自行判断是否分配成功
 */
void *ntg_calloc(size_t size, ntg_log_t *log){
	void *p;

	p = ntg_alloc(size, log);
	if(p){
		ntg_memzero(p, size);
	}
	return p;
}


/**
 * 对齐内存分配
 * @param alignment 对齐的大小
 * @param size	分配的内存大小
 * @param log 日志对象
 * @return 分配的内存首地址
 * @note 带错误日志记录, 调用者需要自行判断是否分配成功
 */
void *ntg_memalign(size_t alignment, size_t size, ntg_log_t *log){
	void *p;
	int err;

	err = posix_memalign(&p, alignment, size);

	if(err){
        ntg_log_error(NTG_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
	}
    ntg_log_debug3(NTG_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

	return p;
}


