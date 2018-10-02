/*
 * ntg_buf.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_BUF_H_
#define CORE_NTG_BUF_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_file.h"

typedef void *            ntg_buf_tag_t;

// ntg_buf_s是nginx用于处理大数据的关键数据结构
// 它既应用于内存数据，也应用于磁盘数据。

struct ntg_buf_s {
    // 处理内存数据
    u_char          *pos;       //告知需要处理的内存数据的起始位置
    u_char          *last;      //告知需要处理的内存数据的结束位置，即希望处理的数据为[pos,last)

    // 处理文件数据
    off_t            file_pos;  //告知需要处理的文件数据的起始位置
    off_t            file_last; //告知需要处理的文件数据的结束位置

    //数据的起始与终止位置
    u_char          *start;      //当一整块内存被包含在多个buf中的时候，那么这些buf里面的start和end都指向这块内存的起始位置和终止位置，和pos不同，pos会大于等于start
    u_char          *end;        //见start分析，和last不同，last会小于等于end

    ntg_buf_tag_t    tag;        //当前缓冲区的类型。例如由哪个模块使用，就指向这个模块ntg_module_t变量的地址
    ntg_file_t      *file;       //文件数据所引用的文件

    // 当前缓冲区的影子缓冲区，这个成员很少使用到。
	//当一个buf完整的copy另一buf的所有字段的时候，那么这两个buf指向的实际上是同一个内存或者同一个文件。
	//此时的两个buf的shadow是相互指向对方的，那么对于这样的两个buf在释放的时候需要特别小心。
    ntg_buf_t       *shadow;

    /* the buf's content could be changed */
    //临时内存标志位，1表示数据在临时内存中，且这段数据可以修改
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    //内存标志位，1表示数据在内存中，且这段数据不能被修改
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
    // 标志位，1表示这段内存是用mmap系统调用映射过来的，不可以被修改
    unsigned         mmap:1;

	//标志位，1表示可以被回收，通常配合shadow字段一起使用；
	//当使用ntg_create_temp_buf函数创建的buf同时也是另一个buf的shadow的时候，表示这个buf是可释放的
    unsigned         recycled:1;


    unsigned         in_file:1;     //标志位，1表示是处理文件数据，而不是内存数据


	//遇到有flush字段被设置为1的buf的chain，则该chain的数据即使不是最后结束的数据也会进行输出；
	//不会受postpone_output配置的限制。
    unsigned         flush:1;

    //标志位，对于操作这个缓冲区时是否使用同步方式，需要谨慎考虑。
    //这有可能会阻塞nginx进程，nginx中所有操作几乎都是异步的。
    unsigned         sync:1;

    // 标志位，是否是最后一块缓冲区。nginx_buf_t可以由ntg_chain_t链表串联起来
    // 1代表是最后一块待处理的缓冲区
    unsigned         last_buf:1;

    //标志位，是否是ntg_chain_t中的最后一块缓冲区
    unsigned         last_in_chain:1;

    //标志位，是否是最后一个影子缓冲区，与shadow配合使用，通常不建议使用；
	//在创建一个buf的shadow的时候，通常将新创建的一个buf的last_shadow设置为1.
    unsigned         last_shadow:1;

    //标志位，是否属于临时文件
	//由于受到内存使用的限制，有时候一些buf的内容需要被写到磁盘上的临时文件。
    unsigned         temp_file:1;

    /* STUB */ int   num;
};

//buf链
struct ntg_chain_s {
    ntg_buf_t    *buf; //链表对应的buffer
    ntg_chain_t  *next; //链表下一个元素
};

typedef struct{
	ntg_int_t 	num;//
	size_t		size;
}ntg_bufs_t;


typedef struct ntg_output_chain_ctx_s  ntg_output_chain_ctx_t;

typedef ntg_int_t (*ntg_output_chain_filter_pt)(void *ctx, ntg_chain_t *in);


//输出链上下文
struct ntg_output_chain_ctx_s {
    ntg_buf_t                   *buf;///临时buf
    ntg_chain_t                 *in;///保存将要发送的chain
    ntg_chain_t                 *free;///保存了已经发送完毕的chain，以便于重复利用
    ntg_chain_t                 *busy;////保存了还未发送的chain

    unsigned                     sendfile:1;
    unsigned                     directio:1;
#if (NTG_HAVE_ALIGNED_DIRECTIO)
    unsigned                     unaligned:1;
#endif
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;

    off_t                        alignment;

    ntg_pool_t                  *pool;
    ntg_int_t                    allocated;///已经allocated的大小
    ntg_bufs_t                   bufs;
    ntg_buf_tag_t                tag;

    ntg_output_chain_filter_pt   output_filter;//输出过滤函数
    void                        *filter_ctx;//输出过滤上下文
};

//链的输出上下文
typedef struct {
    ntg_chain_t                 *out;
    ntg_chain_t                **last;
    ntg_pool_t                  *pool;
    off_t                        limit;
} ntg_chain_writer_ctx_t;


#define NTG_CHAIN_ERROR     (ntg_chain_t *) NTG_ERROR


#define ntg_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ntg_buf_in_memory_only(b)   (ntg_buf_in_memory(b) && !b->in_file)

#define ntg_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ntg_buf_in_memory(b) && !b->in_file)

#define ntg_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ntg_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define ntg_buf_size(b)                                                      \
    (ntg_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ntg_buf_t *ntg_create_temp_buf(ntg_pool_t *pool, size_t size);
ntg_chain_t *ntg_create_chain_of_bufs(ntg_pool_t *pool, ntg_bufs_t *bufs);


#define ntg_alloc_buf(pool)  ntg_palloc(pool, sizeof(ntg_buf_t))
#define ntg_calloc_buf(pool) ntg_pcalloc(pool, sizeof(ntg_buf_t))

ntg_chain_t *ntg_alloc_chain_link(ntg_pool_t *pool);
#define ntg_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ntg_int_t ntg_output_chain(ntg_output_chain_ctx_t *ctx, ntg_chain_t *in);
ntg_int_t ntg_chain_writer(void *ctx, ntg_chain_t *in);

ntg_int_t ntg_chain_add_copy(ntg_pool_t *pool, ntg_chain_t **chain,
    ntg_chain_t *in);
ntg_chain_t *ntg_chain_get_free_buf(ntg_pool_t *p, ntg_chain_t **free);
void ntg_chain_update_chains(ntg_pool_t *p, ntg_chain_t **free,
    ntg_chain_t **busy, ntg_chain_t **out, ntg_buf_tag_t tag);

off_t ntg_chain_coalesce_file(ntg_chain_t **in, off_t limit);

ntg_chain_t *ntg_chain_update_sent(ntg_chain_t *in, off_t sent);
#endif /* CORE_NTG_BUF_H_ */
