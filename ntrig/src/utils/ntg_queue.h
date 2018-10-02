/*
 * ntg_queue.h
 *
 *  Created on: Jul 28, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_QUEUE_H_
#define CORE_NTG_QUEUE_H_

/*
`ntg_queue_t` 是Nginx提供的一个轻量级双向链表容器，它不负责分配内存来存放链表元素。
其具备下列特点：
- 可以高效的执行插入、删除、合并等操作
- 具有排序功能
- 支持两个链表间的合并
- 支持将一个链表一分为二的拆分动作
 */

typedef struct ntg_queue_s  ntg_queue_t;

//参考：
//http://blog.csdn.net/livelylittlefish/article/details/6607324
struct ntg_queue_s {
    ntg_queue_t  *prev;   //前一个
    ntg_queue_t  *next;   //下一个
};

//初始化队列
#define ntg_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q

//判断队列是否为空
#define ntg_queue_empty(h)                                                    \
    (h == (h)->prev)

//在头节点之后插入新节点
#define ntg_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x

#define ntg_queue_insert_after   ntg_queue_insert_head

//在尾节点之后插入新节点
#define ntg_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x

//头节点
#define ntg_queue_head(h)                                                     \
    (h)->next

//尾节点
#define ntg_queue_last(h)                                                     \
    (h)->prev

//标志节点
#define ntg_queue_sentinel(h)                                                 \
    (h)

//下一个节点
#define ntg_queue_next(q)                                                     \
    (q)->next

//上一个节点
#define ntg_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

//删除节点
#define ntg_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define ntg_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif

//分隔队列
#define ntg_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;

//链接队列
#define ntg_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;

//获取队列中节点数据， q是队列中的节点，type队列类型，link是队列类型中ntg_queue_t的元素名
#define ntg_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))

//队列的中间节点
ntg_queue_t *ntg_queue_middle(ntg_queue_t *queue);

void ntg_queue_sort(ntg_queue_t *queue,
    ntg_int_t (*cmp)(const ntg_queue_t *, const ntg_queue_t *));

#endif /* CORE_NTG_QUEUE_H_ */
