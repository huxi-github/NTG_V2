/*
 * ntg_list.h
 *
 *  Created on: Jul 28, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_LIST_H_
#define CORE_NTG_LIST_H_

#include "ntg_palloc.h"
#include "../ntg_config.h"

typedef struct ntg_list_part_s  ntg_list_part_t;

// 参考：nginx源码分析—链表结构ntg_list_t http://blog.csdn.net/livelylittlefish/article/details/6599065

/*
ntg_list_t是nginx中使用的链表结构，但与我们常说的链表结构(例如`std::list`)不太一样，
它符合list类型数据结构的一些特点，比如可以添加元素，实现动态自增长，不会像数组类型的数据结构，受到初始设定的数组容量的限制，
不同点就在于它的节点，`std::list`每个节点只能存放一个元素，ntg_list_t的节点却是一个固定大小的数组，可以存放多个元素。

在初始化的时候，我们需要设定单个元素所需要占用的内存空间大小，以及每个节点数组的容量大小。
在添加元素到这个list里面的时候，会在最尾部的节点里的数组上添加元素，如果这个节点的数组存满了，就再增加一个新的节点到这个list里面去。
 */

// ntg_list_part_s是代表ntg_list_t链表的一个节点。
// 它自身包含了一个数组，用来存放最终的元素
struct ntg_list_part_s {
    void             *elts; //链表元素elts数组,数组申请的空间大小为size*nalloc
    ntg_uint_t        nelts; //当前已使用的elts个数，一定要小于等于nalloc
    ntg_list_part_t  *next; //指向ntg_list_t中的下个链表part
};

// ntg_list_t结构是一个链表，链表中每个节点是ntg_list_part_t结构。
// 而ntg_list_part_t中有个elts是一个数组，储存了任意大小固定的元素，它是由ntg_pool_t分配的连续空间
typedef struct {
    ntg_list_part_t  *last; //指向链表中最后一个元素，其作用相当于尾指针。插入新的节点时，从此开始。
    ntg_list_part_t   part; //链表中第一个元素，其作用相当于头指针。遍历时，从此开始。
    size_t            size; //链表中每个元素的大小
    ntg_uint_t        nalloc; //链表的每个ntg_list_part_t中elts数组的所能容纳的最大元素个数
    ntg_pool_t       *pool; //当前list数据存放的内存池
} ntg_list_t;

//ntg_list_create和ntg_list_init功能是一样的都是创建一个list，只是返回值不一样...
ntg_list_t *ntg_list_create(ntg_pool_t *pool, ntg_uint_t n, size_t size);

// ntg_list_init是初始化了一个已有的链表
ntg_int_t
ntg_list_init(ntg_list_t *list, ntg_pool_t *pool, ntg_uint_t n, size_t size);


void *ntg_list_push(ntg_list_t *list);



#endif /* CORE_NTG_LIST_H_ */
