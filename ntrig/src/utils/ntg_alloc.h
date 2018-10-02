/*
 * ntg_alloc.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef OS_NTG_ALLOC_H_
#define OS_NTG_ALLOC_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

//使用malloc分配内存空间
void *ntg_alloc(size_t size, ntg_log_t *log);

//使用malloc分配内存空间，并且将空间内容初始化为0
void *ntg_calloc(size_t size, ntg_log_t *log);

#define ntg_free          free

void *ntg_memalign(size_t alignment, size_t size, ntg_log_t *log);


extern uint  ntg_pagesize;
extern uint  ntg_pagesize_shift;
extern uint  ntg_cacheline_size;


#endif /* OS_NTG_ALLOC_H_ */
