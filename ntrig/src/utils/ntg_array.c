/*
 * ntg_array.c
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "ntg_palloc.h"
#include "ntg_array.h"


ntg_array_t *
ntg_array_create(ntg_pool_t *p, ntg_uint_t n, size_t size) {
	ntg_array_t *a;

	// 分配ntg_array_t数组管理结构的内存
	a = ntg_palloc(p, sizeof(ntg_array_t));
	if (a == NULL) {
		return NULL;
	}

	// 分配存放n个元素，单个元素大小为size的内存空间
	a->elts = ntg_palloc(p, n * size);
	if (a->elts == NULL) {
		return NULL;
	}

	a->nelts = 0;
	a->size = size;	// 元素大小
	a->nalloc = n;	// 数组容量
	a->pool = p;

	return a;
}

void
ntg_array_destroy(ntg_array_t *a) {
	ntg_pool_t *p;

	p = a->pool;

	// 若内存池未使用内存地址等于数组最后元素的地址，则释放数组内存到内存池
	if ((u_char *) a->elts + a->size * a->nalloc == p->d.last) {
		p->d.last -= a->size * a->nalloc;
	}

	if ((u_char *) a + sizeof(ntg_array_t) == p->d.last) {
		p->d.last = (u_char *) a;
	}
}


/**
 * 在数组a中获取一个空的元素位置
 * @param[int] a 数组对象
 * @return 成功返回元素的起始地址,否则返回NULL
 * @note
 *首先判断　nalloc是否和nelts相等，即数组预先分配的空间已经满了.
 *如果没满则计算地址直接返回指针;
 *如果已经满了则先判断是否我们的pool中的当前链表节点还有剩余的空间;
 *如果有则直接在当前的pool链表节点中分配内存，并返回;
 *如果当前链表节点没有足够的空间则使用ntg_palloc重新分配一个2倍于之前数组空间大小的数组，
 *然后将数据转移过来，并返回新地址的指针.
 */
void *
ntg_array_push(ntg_array_t *a) {
	void *elt, *new;
	size_t size;
	ntg_pool_t *p;

	if (a->nelts == a->nalloc) {

		/* the array is full */

		size = a->size * a->nalloc;

		p = a->pool;

		if ((u_char *) a->elts + size == p->d.last
				&& p->d.last + a->size <= p->d.end) {
			//数组已满，内存节点有剩余空间
			/*
			 * the array allocation is the last in the pool
			 * and there is space for new allocation
			 */

			p->d.last += a->size;
			a->nalloc++;

		} else {
			/* allocate a new array */

			new = ntg_palloc(p, 2 * size);
			if (new == NULL) {
				return NULL;
			}

			ntg_memcpy(new, a->elts, size);
			a->elts = new;
			a->nalloc *= 2;
		}
	}

	elt = (u_char *) a->elts + a->size * a->nelts;
	a->nelts++;

	return elt;
}

void *
ntg_array_push_n(ntg_array_t *a, ntg_uint_t n) {
	void *elt, *new;
	size_t size;
	ntg_uint_t nalloc;
	ntg_pool_t *p;

	size = n * a->size;

	if (a->nelts + n > a->nalloc) {

		/* the array is full */

		p = a->pool;

		if ((u_char *) a->elts + a->size * a->nalloc == p->d.last
				&& p->d.last + size <= p->d.end) {
			/*
			 * the array allocation is the last in the pool
			 * and there is space for new allocation
			 */

			p->d.last += size;
			a->nalloc += n;

		} else {
			/* allocate a new array */

			nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

			new = ntg_palloc(p, nalloc * a->size);
			if (new == NULL) {
				return NULL;
			}

			ntg_memcpy(new, a->elts, a->nelts * a->size);
			a->elts = new;
			a->nalloc = nalloc;
		}
	}

	elt = (u_char *) a->elts + a->size * a->nelts;
	a->nelts += n;

	return elt;
}

/**
 * 数组初始化
 * @param array	数组对象
 * @param pool	内存池
 * @param n		分配的元素个数
 * @param size	元素的大小
 * @return	成功返回NTG_OK,否则返回NTG_ERROR
 *
 */
ntg_int_t
ntg_array_init(ntg_array_t *array, ntg_pool_t *pool, ntg_uint_t n, size_t size)
{
    /*
     * set "array->nelts" before "array->elts", otherwise MSVC thinks
     * that "array->nelts" may be used without having been initialized
     */

    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->pool = pool;

    array->elts = ntg_palloc(pool, n * size);
    if (array->elts == NULL) {
        return NTG_ERROR;
    }

    return NTG_OK;
}
