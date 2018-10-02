/*
 * ntg_palloc.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_PALLOC_H_
#define CORE_NTG_PALLOC_H_

#include "ntg_alloc.h"
#include "ntg_log.h"
#include "ntg_files.h"

#include "../ntg_config.h"
#include "../ntg_core.h"
/*
 * ntg_MAX_ALLOC_FROM_POOL should be (ntg_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NTG_MAX_ALLOC_FROM_POOL  (ntg_pagesize - 1)

//原来为16* 1024
#define NTG_DEFAULT_POOL_SIZE    (4 * 1024)

#define NTG_POOL_ALIGNMENT       16
#define NTG_MIN_POOL_SIZE                                                     \
    ntg_align((sizeof(ntg_pool_t) + 2 * sizeof(ntg_pool_large_t)),            \
              NTG_POOL_ALIGNMENT)

//typedef struct ntg_pool_s ntg_pool_t;
typedef void (*ntg_pool_cleanup_pt)(void *data);


typedef struct ntg_pool_cleanup_s  ntg_pool_cleanup_t;

struct ntg_pool_cleanup_s {
    ntg_pool_cleanup_pt   handler;//清除关联文件的回调函数
    void                 *data;//关联的数据ntg_pool_cleanup_file_t
    ntg_pool_cleanup_t   *next;
};


typedef struct ntg_pool_large_s  ntg_pool_large_t;
//大内存结构
struct ntg_pool_large_s {
    ntg_pool_large_t     *next; //下一个大块内存
    void                 *alloc;//nginx分配的大块内存空间
};

//小块内存
//该结构用来维护内存池的数据块，供用户分配之用
typedef struct {
    u_char               *last;  //当前内存分配结束位置，即下一段可分配内存的起始位置
    u_char               *end;   //内存池结束位置
    ntg_pool_t           *next;  //链接到下一个内存池
    ntg_uint_t            failed;//统计该内存池不能满足分配请求的次数
} ntg_pool_data_t;

//该结构维护整个内存池的头部信息
struct ntg_pool_s {
    ntg_pool_data_t       d;       //小块数据块
    size_t                max;     //数据块大小，即小块内存的最大值
    ntg_pool_t           *current; //记录后续内存分配的起始节点,它的移动更据data中的failed字段
    ntg_chain_t          *chain;   //可以挂一个chain结构
    ntg_pool_large_t     *large;   //分配大块内存用，即超过max的内存请求
    ntg_pool_cleanup_t   *cleanup; //挂载一些内存池释放的时候，同时释放的资源
    ntg_log_t            *log;
};


typedef struct {
    ntg_fd_t              fd;
    u_char               *name;
    ntg_log_t            *log;
} ntg_pool_cleanup_file_t;

//使用malloc分配内存空间
void *ntg_alloc(size_t size, ntg_log_t *log);

//使用malloc分配内存空间，并且将空间内容初始化为0
void *ntg_calloc(size_t size, ntg_log_t *log);

//创建内存池
ntg_pool_t *ntg_create_pool(size_t size, ntg_log_t *log);
void ntg_destroy_pool(ntg_pool_t *pool);
void ntg_reset_pool(ntg_pool_t *pool);

void *ntg_palloc(ntg_pool_t *pool, size_t size);    //palloc取得的内存是对齐的
void *ntg_pnalloc(ntg_pool_t *pool, size_t size);   //pnalloc取得的内存是不对齐的
void *ntg_pcalloc(ntg_pool_t *pool, size_t size);   //pcalloc直接调用palloc分配好内存，然后进行一次0初始化操作
void *ntg_pmemalign(ntg_pool_t *pool, size_t size, size_t alignment); //在分配size大小的内存，并按照alignment对齐，然后挂到large字段下
ntg_int_t ntg_pfree(ntg_pool_t *pool, void *p);


ntg_pool_cleanup_t *ntg_pool_cleanup_add(ntg_pool_t *p, size_t size);
void ntg_pool_run_cleanup_file(ntg_pool_t *p, ntg_fd_t fd);
void ntg_pool_cleanup_file(void *data);
void ntg_pool_delete_file(void *data);


#endif /* CORE_NTG_PALLOC_H_ */
