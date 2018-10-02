/*
 * ntg_buf.c
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

/*
 * 创建一个临时buf
 */
ntg_buf_t *
ntg_create_temp_buf(ntg_pool_t *pool, size_t size)
{
    ntg_buf_t *b;

    b = ntg_calloc_buf(pool);
    if (b == NULL) {
        return NULL;
    }

    b->start = ntg_palloc(pool, size);
    if (b->start == NULL) {
        return NULL;
    }

    /*
     * set by ntg_calloc_buf():
     *
     *     b->file_pos = 0;
     *     b->file_last = 0;
     *     b->file = NULL;
     *     b->shadow = NULL;
     *     b->tag = 0;
     *     and flags
     */

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;

    return b;
}

/*
 * 分配一个链
 */
ntg_chain_t *
ntg_alloc_chain_link(ntg_pool_t *pool)
{
    ntg_chain_t  *cl;

    cl = pool->chain;

    if (cl) {
        pool->chain = cl->next;
        return cl;
    }

    cl = ntg_palloc(pool, sizeof(ntg_chain_t));
    if (cl == NULL) {
        return NULL;
    }

    return cl;
}

/*
 * 通过bufs创建一个链
 */
ntg_chain_t *
ntg_create_chain_of_bufs(ntg_pool_t *pool, ntg_bufs_t *bufs)
{
    u_char       *p;
    ntg_int_t     i;
    ntg_buf_t    *b;
    ntg_chain_t  *chain, *cl, **ll;

    p = ntg_palloc(pool, bufs->num * bufs->size);
    if (p == NULL) {
        return NULL;
    }

    ll = &chain;

    for (i = 0; i < bufs->num; i++) {

        b = ntg_calloc_buf(pool);
        if (b == NULL) {
            return NULL;
        }

        /*
         * set by ntg_calloc_buf():
         *
         *     b->file_pos = 0;
         *     b->file_last = 0;
         *     b->file = NULL;
         *     b->shadow = NULL;
         *     b->tag = 0;
         *     and flags
         *
         */

        b->pos = p;
        b->last = p;
        b->temporary = 1;

        b->start = p;
        p += bufs->size;
        b->end = p;

        cl = ntg_alloc_chain_link(pool);
        if (cl == NULL) {
            return NULL;
        }

        cl->buf = b;
        *ll = cl;
        ll = &cl->next;
    }

    *ll = NULL;

    return chain;
}


ntg_int_t
ntg_chain_add_copy(ntg_pool_t *pool, ntg_chain_t **chain, ntg_chain_t *in)
{
    ntg_chain_t  *cl, **ll;

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) {
        ll = &cl->next;
    }

    while (in) {
        cl = ntg_alloc_chain_link(pool);
        if (cl == NULL) {
            return NTG_ERROR;
        }

        cl->buf = in->buf;
        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = NULL;

    return NTG_OK;
}

/*
 * 获取一个可用链
 * 首先从free中获取,如果满足返回,否则额外分配一个
 */
ntg_chain_t *
ntg_chain_get_free_buf(ntg_pool_t *p, ntg_chain_t **free)
{
    ntg_chain_t  *cl;

    if (*free) {
        cl = *free;
        *free = cl->next;
        cl->next = NULL;
        return cl;
    }

    cl = ntg_alloc_chain_link(p);
    if (cl == NULL) {
        return NULL;
    }

    cl->buf = ntg_calloc_buf(p);
    if (cl->buf == NULL) {
        return NULL;
    }

    cl->next = NULL;

    return cl;
}


void
ntg_chain_update_chains(ntg_pool_t *p, ntg_chain_t **free, ntg_chain_t **busy,
    ntg_chain_t **out, ntg_buf_tag_t tag)
{
    ntg_chain_t  *cl;

    if (*busy == NULL) {
        *busy = *out;

    } else {
        for (cl = *busy; cl->next; cl = cl->next) { /* void */ }

        cl->next = *out;
    }

    *out = NULL;

    while (*busy) {
        cl = *busy;

        if (ntg_buf_size(cl->buf) != 0) {
            break;
        }

        if (cl->buf->tag != tag) {
            *busy = cl->next;
            ntg_free_chain(p, cl);
            continue;
        }

        cl->buf->pos = cl->buf->start;
        cl->buf->last = cl->buf->start;

        *busy = cl->next;
        cl->next = *free;
        *free = cl;
    }
}


off_t
ntg_chain_coalesce_file(ntg_chain_t **in, off_t limit)
{
    off_t         total, size, aligned, fprev;
    ntg_fd_t      fd;
    ntg_chain_t  *cl;

    total = 0;

    cl = *in;
    fd = cl->buf->file->fd;

    do {
        size = cl->buf->file_last - cl->buf->file_pos;

        if (size > limit - total) {
            size = limit - total;

            aligned = (cl->buf->file_pos + size + ntg_pagesize - 1)
                       & ~((off_t) ntg_pagesize - 1);

            if (aligned <= cl->buf->file_last) {
                size = aligned - cl->buf->file_pos;
            }
        }

        total += size;
        fprev = cl->buf->file_pos + size;
        cl = cl->next;

    } while (cl
             && cl->buf->in_file
             && total < limit
             && fd == cl->buf->file->fd
             && fprev == cl->buf->file_pos);

    *in = cl;

    return total;
}


ntg_chain_t *
ntg_chain_update_sent(ntg_chain_t *in, off_t sent)
{
    off_t  size;

    for ( /* void */ ; in; in = in->next) {

        if (ntg_buf_special(in->buf)) {
            continue;
        }

        if (sent == 0) {
            break;
        }

        size = ntg_buf_size(in->buf);

        if (sent >= size) {
            sent -= size;

            if (ntg_buf_in_memory(in->buf)) {
                in->buf->pos = in->buf->last;
            }

            if (in->buf->in_file) {
                in->buf->file_pos = in->buf->file_last;
            }

            continue;
        }

        if (ntg_buf_in_memory(in->buf)) {
            in->buf->pos += (size_t) sent;
        }

        if (in->buf->in_file) {
            in->buf->file_pos += sent;
        }

        break;
    }

    return in;
}
