/*
 * ntg_palloc.c
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */




#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_palloc.h"
//块内存
static void *ntg_palloc_block(ntg_pool_t *pool, size_t size);
//大块内存
static void *ntg_palloc_large(ntg_pool_t *pool, size_t size);


/**
 * 创建内存池
 * @param size 分配的空间大小
 * @param log 对应的日志对象
 * @return 成功,饭后内存池对象指针, 否则返回NULL
 *
 */
ntg_pool_t * ntg_create_pool(size_t size, ntg_log_t *log){
    ntg_pool_t  *p;

    p = ntg_memalign(NTG_POOL_ALIGNMENT, size, log); // 分配内存函数，uinx,windows分开走
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(ntg_pool_t); //初始指向 ntg_pool_t 结构体后面
    p->d.end = (u_char *) p + size; //整个结构的结尾后面
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ntg_pool_t);
    p->max = (size < NTG_MAX_ALLOC_FROM_POOL) ? size : NTG_MAX_ALLOC_FROM_POOL; //最大不超过 NTG_MAX_ALLOC_FROM_POOL,也就是getpagesize()-1 大小

    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}


/**
 * 销毁内存池
 * @param[in] pool 内存池
 */
void
ntg_destroy_pool(ntg_pool_t *pool)
{
    ntg_pool_t          *p, *n;
    ntg_pool_large_t    *l;
    ntg_pool_cleanup_t  *c;
    //1) 相关的清除链表
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ntg_log_debug1(NTG_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }
    //2) 清除大内存块
    for (l = pool->large; l; l = l->next) {

        ntg_log_debug1(NTG_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);

        if (l->alloc) {
            ntg_free(l->alloc);
        }
    }

#if (NTG_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ntg_log_debug2(NTG_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif
    //TODO
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ntg_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void
ntg_reset_pool(ntg_pool_t *pool)
{
    ntg_pool_t        *p;
    ntg_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ntg_free(l->alloc);
        }
    }

    pool->large = NULL;

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ntg_pool_t);
    }
}


void * ntg_palloc(ntg_pool_t *pool, size_t size){
    u_char      *m;
    ntg_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = ntg_align_ptr(p->d.last, NTG_ALIGNMENT); // 对齐内存指针，加快存取速度

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return ntg_palloc_block(pool, size);
    }

    return ntg_palloc_large(pool, size);
}


void * ntg_pnalloc(ntg_pool_t *pool, size_t size){
    u_char      *m;
    ntg_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = p->d.last;

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return ntg_palloc_block(pool, size);
    }

    return ntg_palloc_large(pool, size);
}


static void * ntg_palloc_block(ntg_pool_t *pool, size_t size){
    u_char      *m;
    size_t       psize;
    ntg_pool_t  *p, *new, *current;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = ntg_memalign(NTG_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (ntg_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ntg_pool_data_t);
    m = ntg_align_ptr(m, NTG_ALIGNMENT);
    new->d.last = m + size;

    current = pool->current;

    for (p = current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            current = p->d.next;
        }
    }

    p->d.next = new;

    pool->current = current ? current : new;

    return m;
}

//控制大块内存的申请
static void *
ntg_palloc_large(ntg_pool_t *pool, size_t size)
{
    void              *p;
    ntg_uint_t         n;
    ntg_pool_large_t  *large;

    p = ntg_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = ntg_palloc(pool, sizeof(ntg_pool_large_t));
    if (large == NULL) {
        ntg_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
ntg_pmemalign(ntg_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ntg_pool_large_t  *large;

    p = ntg_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ntg_palloc(pool, sizeof(ntg_pool_large_t));
    if (large == NULL) {
        ntg_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

//控制大块内存的释放。注意，这个函数只会释放大内存，不会释放其对应的头部结构，遗留下来的头部结构会做下一次申请大内存之用
ntg_int_t
ntg_pfree(ntg_pool_t *pool, void *p)
{
    ntg_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ntg_log_debug1(NTG_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ntg_free(l->alloc);
            l->alloc = NULL;

            return NTG_OK;
        }
    }

    return NTG_DECLINED;
}


void *
ntg_pcalloc(ntg_pool_t *pool, size_t size)
{
    void *p;

    p = ntg_palloc(pool, size);
    if (p) {
        ntg_memzero(p, size);
    }

    return p;
}

//注册cleanup回叫函数（结构体）
ntg_pool_cleanup_t *
ntg_pool_cleanup_add(ntg_pool_t *p, size_t size)
{
    ntg_pool_cleanup_t  *c;

    c = ntg_palloc(p, sizeof(ntg_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ntg_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ntg_log_debug1(NTG_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}


void
ntg_pool_run_cleanup_file(ntg_pool_t *p, ntg_fd_t fd)
{
    ntg_pool_cleanup_t       *c;
    ntg_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ntg_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}


void
ntg_pool_cleanup_file(void *data)
{
    ntg_pool_cleanup_file_t  *c = data;

    ntg_log_debug1(NTG_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ntg_close_file(c->fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, c->log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", c->name);
    }
}


void
ntg_pool_delete_file(void *data)
{
    ntg_pool_cleanup_file_t  *c = data;

    ntg_err_t  err;

    ntg_log_debug2(NTG_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ntg_delete_file(c->name) == NTG_FILE_ERROR) {
        err = ntg_errno;

        if (err != NTG_ENOENT) {
            ntg_log_error(NTG_LOG_CRIT, c->log, err,
                          ntg_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ntg_close_file(c->fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, c->log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ntg_get_cached_block(size_t size)
{
    void                     *p;
    ntg_cached_block_slot_t  *slot;

    if (ntg_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ntg_cycle->cache[(size + ntg_pagesize - 1) / ntg_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
