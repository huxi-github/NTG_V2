/**
 * @file 	ntg_output_chain.c
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 16, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"


#if 0
#define NTG_SENDFILE_LIMIT  4096
#endif

/*
 * When DIRECTIO is enabled FreeBSD, Solaris, and MacOSX read directly
 * to an application memory from a device if parameters are aligned
 * to device sector boundary (512 bytes).  They fallback to usual read
 * operation if the parameters are not aligned.
 * Linux allows DIRECTIO only if the parameters are aligned to a filesystem
 * sector boundary, otherwise it returns EINVAL.  The sector size is
 * usually 512 bytes, however, on XFS it may be 4096 bytes.
 */

#define NTG_NONE            1


static ntg_inline ntg_int_t
    ntg_output_chain_as_is(ntg_output_chain_ctx_t *ctx, ntg_buf_t *buf);
#if (NTG_HAVE_AIO_SENDFILE)
static ntg_int_t ntg_output_chain_aio_setup(ntg_output_chain_ctx_t *ctx,
    ntg_file_t *file);
#endif
static ntg_int_t ntg_output_chain_add_copy(ntg_pool_t *pool,
    ntg_chain_t **chain, ntg_chain_t *in);
static ntg_int_t ntg_output_chain_align_file_buf(ntg_output_chain_ctx_t *ctx,
    off_t bsize);
static ntg_int_t ntg_output_chain_get_buf(ntg_output_chain_ctx_t *ctx,
    off_t bsize);
static ntg_int_t ntg_output_chain_copy_buf(ntg_output_chain_ctx_t *ctx);

/**
 * 输出数据链数据
 * @param[in] ctx 输出链上下文对象
 * @param[in] in 待发送的数据链
 * @return
 */
ntg_int_t
ntg_output_chain(ntg_output_chain_ctx_t *ctx, ntg_chain_t *in)
{
    off_t         bsize;
    ntg_int_t     rc, last;
    ntg_chain_t  *cl, *out, **last_out;

    if (ctx->in == NULL && ctx->busy == NULL
#if (NTG_HAVE_FILE_AIO || NTG_THREADS)
        && !ctx->aio
#endif
       )
    {
        /*
         * the short path for the case when the ctx->in and ctx->busy chains
         * are empty, the incoming chain is empty too or has the single buf
         * that does not require the copy
         */

        if (in == NULL) {
            return ctx->output_filter(ctx->filter_ctx, in);
        }
        //这里说明只有一个chain，并且它的buf不需要复制
        if (in->next == NULL
#if (NTG_SENDFILE_LIMIT)
            && !(in->buf->in_file && in->buf->file_last > NTG_SENDFILE_LIMIT)
#endif
            && ntg_output_chain_as_is(ctx, in->buf))
        {
            return ctx->output_filter(ctx->filter_ctx, in);
        }
    }

    /* add the incoming buf to the chain ctx->in */
    //需要复制

    if (in) {
        if (ntg_output_chain_add_copy(ctx->pool, &ctx->in, in) == NTG_ERROR) {
            return NTG_ERROR;
        }
    }

    out = NULL;
    last_out = &out;
    last = NTG_NONE;

    for ( ;; ) {

#if (NTG_HAVE_FILE_AIO || NTG_THREADS)
        if (ctx->aio) {
            return NTG_AGAIN;
        }
#endif
        //开始遍历chain
        while (ctx->in) {

            /*
             * cycle while there are the ctx->in bufs
             * and there are the free output bufs to copy in
             */
        	//取得当前chain的buf大小
            bsize = ntg_buf_size(ctx->in->buf);

            //跳过bsize为0的buf
            if (bsize == 0 && !ntg_buf_special(ctx->in->buf)) {

                ntg_log_error(NTG_LOG_ALERT, ctx->pool->log, 0,
                              "zero size buf in output "
                              "t:%d r:%d f:%d %p %p-%p %p %O-%O",
                              ctx->in->buf->temporary,
                              ctx->in->buf->recycled,
                              ctx->in->buf->in_file,
                              ctx->in->buf->start,
                              ctx->in->buf->pos,
                              ctx->in->buf->last,
                              ctx->in->buf->file,
                              ctx->in->buf->file_pos,
                              ctx->in->buf->file_last);

                ntg_debug_point();

                ctx->in = ctx->in->next;

                continue;
            }
            //判断是否需要复制buf
            if (ntg_output_chain_as_is(ctx, ctx->in->buf)) {
            	//如果不需要复制，则直接链接chain到out，然后继续循环
                /* move the chain link to the output chain */

                cl = ctx->in;
                ctx->in = cl->next;

                *last_out = cl;
                last_out = &cl->next;
                cl->next = NULL;

                continue;
            }
            //到达这里，说明我们需要拷贝buf，这里buf最终都会被拷贝进ctx->buf中，
            //因此这里先判断ctx->buf是否为空
            if (ctx->buf == NULL) {

                rc = ntg_output_chain_align_file_buf(ctx, bsize);

                if (rc == NTG_ERROR) {
                    return NTG_ERROR;
                }
                //大部分情况下，都会落入这个分支
                if (rc != NTG_OK) {

                    if (ctx->free) {

                        /* get the free buf */

                        cl = ctx->free;
                        ctx->buf = cl->buf;
                        ctx->free = cl->next;

                        ntg_free_chain(ctx->pool, cl);

                    } else if (out || ctx->allocated == ctx->bufs.num) {

                        break;

                    } else if (ntg_output_chain_get_buf(ctx, bsize) != NTG_OK) {
                        return NTG_ERROR;
                    }
                }
            }

            rc = ntg_output_chain_copy_buf(ctx);

            if (rc == NTG_ERROR) {
                return rc;
            }

            if (rc == NTG_AGAIN) {
                if (out) {
                    break;
                }

                return rc;
            }

            /* delete the completed buf from the ctx->in chain */

            if (ntg_buf_size(ctx->in->buf) == 0) {
                ctx->in = ctx->in->next;
            }

            cl = ntg_alloc_chain_link(ctx->pool);
            if (cl == NULL) {
                return NTG_ERROR;
            }

            cl->buf = ctx->buf;
            cl->next = NULL;
            *last_out = cl;
            last_out = &cl->next;
            ctx->buf = NULL;
        }

        if (out == NULL && last != NTG_NONE) {

            if (ctx->in) {
                return NTG_AGAIN;
            }

            return last;
        }

        last = ctx->output_filter(ctx->filter_ctx, out);

        if (last == NTG_ERROR || last == NTG_DONE) {
            return last;
        }

        ntg_chain_update_chains(ctx->pool, &ctx->free, &ctx->busy, &out,
                                ctx->tag);
        last_out = &out;
    }
}


static ntg_inline ntg_int_t
ntg_output_chain_as_is(ntg_output_chain_ctx_t *ctx, ntg_buf_t *buf)
{
    ntg_uint_t  sendfile;

    if (ntg_buf_special(buf)) {
        return 1;
    }

#if (NTG_THREADS)
    if (buf->in_file) {
        buf->file->thread_handler = ctx->thread_handler;
        buf->file->thread_ctx = ctx->filter_ctx;
    }
#endif

    if (buf->in_file && buf->file->directio) {
        return 0;
    }

    sendfile = ctx->sendfile;

#if (NTG_SENDFILE_LIMIT)

    if (buf->in_file && buf->file_pos >= NTG_SENDFILE_LIMIT) {
        sendfile = 0;
    }

#endif

    if (!sendfile) {

        if (!ntg_buf_in_memory(buf)) {
            return 0;
        }

        buf->in_file = 0;
    }

#if (NTG_HAVE_AIO_SENDFILE)
    if (ctx->aio_preload && buf->in_file) {
        (void) ntg_output_chain_aio_setup(ctx, buf->file);
    }
#endif

    if (ctx->need_in_memory && !ntg_buf_in_memory(buf)) {
        return 0;
    }

    if (ctx->need_in_temp && (buf->memory || buf->mmap)) {
        return 0;
    }

    return 1;
}


#if (NTG_HAVE_AIO_SENDFILE)

static ntg_int_t
ntg_output_chain_aio_setup(ntg_output_chain_ctx_t *ctx, ntg_file_t *file)
{
    ntg_event_aio_t  *aio;

    if (file->aio == NULL && ntg_file_aio_init(file, ctx->pool) != NTG_OK) {
        return NTG_ERROR;
    }

    aio = file->aio;

    aio->data = ctx->filter_ctx;
    aio->preload_handler = ctx->aio_preload;

    return NTG_OK;
}

#endif


static ntg_int_t
ntg_output_chain_add_copy(ntg_pool_t *pool, ntg_chain_t **chain,
    ntg_chain_t *in)
{
    ntg_chain_t  *cl, **ll;
#if (NTG_SENDFILE_LIMIT)
    ntg_buf_t    *b, *buf;
#endif

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) {
        ll = &cl->next;
    }

    while (in) {

        cl = ntg_alloc_chain_link(pool);
        if (cl == NULL) {
            return NTG_ERROR;
        }

#if (NTG_SENDFILE_LIMIT)

        buf = in->buf;

        if (buf->in_file
            && buf->file_pos < NTG_SENDFILE_LIMIT
            && buf->file_last > NTG_SENDFILE_LIMIT)
        {
            /* split a file buf on two bufs by the sendfile limit */

            b = ntg_calloc_buf(pool);
            if (b == NULL) {
                return NTG_ERROR;
            }

            ntg_memcpy(b, buf, sizeof(ntg_buf_t));

            if (ntg_buf_in_memory(buf)) {
                buf->pos += (ssize_t) (NTG_SENDFILE_LIMIT - buf->file_pos);
                b->last = buf->pos;
            }

            buf->file_pos = NTG_SENDFILE_LIMIT;
            b->file_last = NTG_SENDFILE_LIMIT;

            cl->buf = b;

        } else {
            cl->buf = buf;
            in = in->next;
        }

#else
        cl->buf = in->buf;
        in = in->next;

#endif

        cl->next = NULL;
        *ll = cl;
        ll = &cl->next;
    }

    return NTG_OK;
}


static ntg_int_t
ntg_output_chain_align_file_buf(ntg_output_chain_ctx_t *ctx, off_t bsize)
{
    size_t      size;
    ntg_buf_t  *in;

    in = ctx->in->buf;

    if (in->file == NULL || !in->file->directio) {
        return NTG_DECLINED;
    }

    ctx->directio = 1;

    size = (size_t) (in->file_pos - (in->file_pos & ~(ctx->alignment - 1)));

    if (size == 0) {

        if (bsize >= (off_t) ctx->bufs.size) {
            return NTG_DECLINED;
        }

        size = (size_t) bsize;

    } else {
        size = (size_t) ctx->alignment - size;

        if ((off_t) size > bsize) {
            size = (size_t) bsize;
        }
    }

    ctx->buf = ntg_create_temp_buf(ctx->pool, size);
    if (ctx->buf == NULL) {
        return NTG_ERROR;
    }

    /*
     * we do not set ctx->buf->tag, because we do not want
     * to reuse the buf via ctx->free list
     */

#if (NTG_HAVE_ALIGNED_DIRECTIO)
    ctx->unaligned = 1;
#endif

    return NTG_OK;
}


static ntg_int_t
ntg_output_chain_get_buf(ntg_output_chain_ctx_t *ctx, off_t bsize)
{
    size_t       size;
    ntg_buf_t   *b, *in;
    ntg_uint_t   recycled;

    in = ctx->in->buf;
    size = ctx->bufs.size;
    recycled = 1;

    if (in->last_in_chain) {

        if (bsize < (off_t) size) {

            /*
             * allocate a small temp buf for a small last buf
             * or its small last part
             */

            size = (size_t) bsize;
            recycled = 0;

        } else if (!ctx->directio
                   && ctx->bufs.num == 1
                   && (bsize < (off_t) (size + size / 4)))
        {
            /*
             * allocate a temp buf that equals to a last buf,
             * if there is no directio, the last buf size is lesser
             * than 1.25 of bufs.size and the temp buf is single
             */

            size = (size_t) bsize;
            recycled = 0;
        }
    }

    b = ntg_calloc_buf(ctx->pool);
    if (b == NULL) {
        return NTG_ERROR;
    }

    if (ctx->directio) {

        /*
         * allocate block aligned to a disk sector size to enable
         * userland buffer direct usage conjunctly with directio
         */

        b->start = ntg_pmemalign(ctx->pool, size, (size_t) ctx->alignment);
        if (b->start == NULL) {
            return NTG_ERROR;
        }

    } else {
        b->start = ntg_palloc(ctx->pool, size);
        if (b->start == NULL) {
            return NTG_ERROR;
        }
    }

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;
    b->tag = ctx->tag;
    b->recycled = recycled;

    ctx->buf = b;
    ctx->allocated++;

    return NTG_OK;
}


static ntg_int_t
ntg_output_chain_copy_buf(ntg_output_chain_ctx_t *ctx)
{
    off_t        size;
    ssize_t      n;
    ntg_buf_t   *src, *dst;
    ntg_uint_t   sendfile;

    src = ctx->in->buf;
    dst = ctx->buf;

    size = ntg_buf_size(src);
    size = ntg_min(size, dst->end - dst->pos);

    sendfile = ctx->sendfile & !ctx->directio;

#if (NTG_SENDFILE_LIMIT)

    if (src->in_file && src->file_pos >= NTG_SENDFILE_LIMIT) {
        sendfile = 0;
    }

#endif

    if (ntg_buf_in_memory(src)) {
        ntg_memcpy(dst->pos, src->pos, (size_t) size);
        src->pos += (size_t) size;
        dst->last += (size_t) size;

        if (src->in_file) {

            if (sendfile) {
                dst->in_file = 1;
                dst->file = src->file;
                dst->file_pos = src->file_pos;
                dst->file_last = src->file_pos + size;

            } else {
                dst->in_file = 0;
            }

            src->file_pos += size;

        } else {
            dst->in_file = 0;
        }

        if (src->pos == src->last) {
            dst->flush = src->flush;
            dst->last_buf = src->last_buf;
            dst->last_in_chain = src->last_in_chain;
        }

    } else {

#if (NTG_HAVE_ALIGNED_DIRECTIO)

        if (ctx->unaligned) {
            if (ntg_directio_off(src->file->fd) == NTG_FILE_ERROR) {
                ntg_log_error(NTG_LOG_ALERT, ctx->pool->log, ntg_errno,
                              ntg_directio_off_n " \"%s\" failed",
                              src->file->name.data);
            }
        }

#endif

#if (NTG_HAVE_FILE_AIO)
        if (ctx->aio_handler) {
            n = ntg_file_aio_read(src->file, dst->pos, (size_t) size,
                                  src->file_pos, ctx->pool);
            if (n == NTG_AGAIN) {
                ctx->aio_handler(ctx, src->file);
                return NTG_AGAIN;
            }

        } else
#endif
#if (NTG_THREADS)
        if (src->file->thread_handler) {
            n = ntg_thread_read(&ctx->thread_task, src->file, dst->pos,
                                (size_t) size, src->file_pos, ctx->pool);
            if (n == NTG_AGAIN) {
                ctx->aio = 1;
                return NTG_AGAIN;
            }

        } else
#endif
        {
            n = ntg_read_file(src->file, dst->pos, (size_t) size,
                              src->file_pos);
        }

#if (NTG_HAVE_ALIGNED_DIRECTIO)

        if (ctx->unaligned) {
            ntg_err_t  err;

            err = ntg_errno;

            if (ntg_directio_on(src->file->fd) == NTG_FILE_ERROR) {
                ntg_log_error(NTG_LOG_ALERT, ctx->pool->log, ntg_errno,
                              ntg_directio_on_n " \"%s\" failed",
                              src->file->name.data);
            }

            ntg_set_errno(err);

            ctx->unaligned = 0;
        }

#endif

        if (n == NTG_ERROR) {
            return (ntg_int_t) n;
        }

        if (n != size) {
            ntg_log_error(NTG_LOG_ALERT, ctx->pool->log, 0,
                          ntg_read_file_n " read only %z of %O from \"%s\"",
                          n, size, src->file->name.data);
            return NTG_ERROR;
        }

        dst->last += n;

        if (sendfile) {
            dst->in_file = 1;
            dst->file = src->file;
            dst->file_pos = src->file_pos;
            dst->file_last = src->file_pos + n;

        } else {
            dst->in_file = 0;
        }

        src->file_pos += n;

        if (src->file_pos == src->file_last) {
            dst->flush = src->flush;
            dst->last_buf = src->last_buf;
            dst->last_in_chain = src->last_in_chain;
        }
    }

    return NTG_OK;
}

/**
 * 默认的数据链输出
 * @param[in] data 输出链的过滤上下文
 * @param[in] in 输出链表
 * @return
 * @note 通过调用链接对象的send_chain将数据发送出去
 */
//ntg_int_t
//ntg_chain_writer(void *data, ntg_chain_t *in)
//{
//    ntg_chain_writer_ctx_t *ctx = data;
//
//    off_t              size;
//    ntg_chain_t       *cl, *ln, *chain;
//
//
//    /*将所有要发送的数据映射到数据过滤上下文*/
//    for (size = 0; in; in = in->next) {
//
//#if 1
//        if (ntg_buf_size(in->buf) == 0 && !ntg_buf_special(in->buf)) {
//
//            ntg_log_error(NTG_LOG_ALERT, ctx->pool->log, 0,
//                          "zero size buf in chain writer "
//                          "t:%d r:%d f:%d %p %p-%p %p %O-%O",
//                          in->buf->temporary,
//                          in->buf->recycled,
//                          in->buf->in_file,
//                          in->buf->start,
//                          in->buf->pos,
//                          in->buf->last,
//                          in->buf->file,
//                          in->buf->file_pos,
//                          in->buf->file_last);
//
//            ntg_debug_point();
//
//            continue;
//        }
//#endif
//
//        size += ntg_buf_size(in->buf);
//
////        ntg_log_debug2(NTG_LOG_DEBUG_CORE, c->log, 0,
////                       "chain writer buf fl:%d s:%uO",
////                       in->buf->flush, ntg_buf_size(in->buf));
//
//        cl = ntg_alloc_chain_link(ctx->pool);
//        if (cl == NULL) {
//            return NTG_ERROR;
//        }
//
//        cl->buf = in->buf;
//        cl->next = NULL;
//        *ctx->last = cl;
//        ctx->last = &cl->next;
//    }
//
////    ntg_log_debug1(NTG_LOG_DEBUG_CORE, c->log, 0,
////                   "chain writer in: %p", ctx->out);
//
//    //原来剩余的数据
//    for (cl = ctx->out; cl; cl = cl->next) {
//
//#if 1
//        if (ntg_buf_size(cl->buf) == 0 && !ntg_buf_special(cl->buf)) {
//
//            ntg_log_error(NTG_LOG_ALERT, ctx->pool->log, 0,
//                          "zero size buf in chain writer "
//                          "t:%d r:%d f:%d %p %p-%p %p %O-%O",
//                          cl->buf->temporary,
//                          cl->buf->recycled,
//                          cl->buf->in_file,
//                          cl->buf->start,
//                          cl->buf->pos,
//                          cl->buf->last,
//                          cl->buf->file,
//                          cl->buf->file_pos,
//                          cl->buf->file_last);
//
//            ntg_debug_point();
//
//            continue;
//        }
//#endif
//
//        size += ntg_buf_size(cl->buf);
//    }
//
//    if (size == 0 && !c->buffered) {
//        return NTG_OK;
//    }
//
//    chain = c->send_chain(c, ctx->out, ctx->limit);//返回下一个待发的数据链
//
//    ntg_log_debug1(NTG_LOG_DEBUG_CORE, c->log, 0,
//                   "chain writer out: %p", chain);
//
//    if (chain == NTG_CHAIN_ERROR) {
//        return NTG_ERROR;
//    }
//
//    //释放所有已发送的数据
//    for (cl = ctx->out; cl && cl != chain; /* void */) {
//        ln = cl;
//        cl = cl->next;
//        ntg_free_chain(ctx->pool, ln);//并没有真正释放空间,而是回收到pool的chain中
//    }
//
//    ctx->out = chain;
//
//    if (ctx->out == NULL) {
//        ctx->last = &ctx->out;
//
//        if (!c->buffered) {
//        	//链接没有缓存
//            return NTG_OK;
//        }
//    }
//
//    return NTG_AGAIN;
//}

