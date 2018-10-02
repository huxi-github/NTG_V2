/*
 * ntg_list.c
 *
 *  Created on: Jul 28, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_list.h"


//该函数创建一个ntg_list_t类型的对象,并对该list的第一个节点分配存放元素的内存空间。
//
//pool:	分配内存使用的pool。
//n:	每个节点(ntg_list_part_t)最多可以存放的元素个数
//size:	每个元素所占用的内存大小
ntg_list_t *ntg_list_create(ntg_pool_t *pool, ntg_uint_t n, size_t size) {
    ntg_list_t  *list;

    // 先创建一个ntg_list_t指针
    list = ntg_palloc(pool, sizeof(ntg_list_t));
    if (list == NULL) {
        return NULL;
    }

    list->part.elts = ntg_palloc(pool, n * size);//一个数组空间
    if (list->part.elts == NULL) {
        return NULL;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return list;
}

/*
ntg_list_create的使用示例：

ntg_list_t *testlist = ntg_list_create(r->pool, 4, sizeof(ntg_str_t));
if (testlist == NULL) {
    return NGX_ERROR;
}
*/


// 往链表l中添加新的元素，返回的是新分配的元素的首地址
void * ntg_list_push(ntg_list_t *l){
    void             *elt;
    ntg_list_part_t  *last;

    last = l->last;

    if (last->nelts == l->nalloc) {
        // 这里是last节点已经使用了nlloc的elts，所以必须新建一个last节点
        /* the last part is full, allocate a new list part */

        last = ntg_palloc(l->pool, sizeof(ntg_list_part_t));
        if (last == NULL) {
            return NULL;
        }

        last->elts = ntg_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}



ntg_int_t
ntg_list_init(ntg_list_t *list, ntg_pool_t *pool, ntg_uint_t n, size_t size)
{
    list->part.elts = ntg_palloc(pool, n * size); //从内存池申请空间后，让elts指向可用空间
    if (list->part.elts == NULL) {
        return NTG_ERROR;
    }

    list->part.nelts = 0; //刚分配下来，还没使用，所以为0
    list->part.next = NULL;
    list->last = &list->part; //last开始的时候指向首节点
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NTG_OK;
}
