/*
 * ntg_array.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_ARRAY_H_
#define CORE_NTG_ARRAY_H_



#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_palloc.h"

typedef struct ntg_array_s ntg_array_t;
// 动态数组
struct ntg_array_s {
    // elts指向数组的首地址elements
    void        *elts;
    // nelts是数组中已经使用的元素个数
    ntg_uint_t   nelts;
    // 每个数组元素占用的内存大小
    size_t       size;
    // 当前数组中能够容纳元素个数的总大小
    ntg_uint_t   nalloc;
    // 内存池对象
    ntg_pool_t  *pool;
};

/*
从内存池中创建n个元素的数组，元素大小为size
创建一个新的数组对象，并返回这个对象。

p:	数组分配内存使用的内存池；
n:	数组的初始容量大小，即在不扩容的情况下最多可以容纳的元素个数。
size:	单个元素的大小，单位是字节。

注意事项: 由于使用ntg_palloc分配内存，数组在扩容时，旧的内存不会被释放，会造成内存的浪费。
因此，最好能提前规划好数组的容量，在创建或者初始化的时候一次搞定，避免多次扩容，造成内存浪费。
 */
ntg_array_t *ntg_array_create(ntg_pool_t *p, ntg_uint_t n, size_t size);

// 销毁该数组对象，并释放其分配的内存回内存池。
void ntg_array_destroy(ntg_array_t *a);

// 在数组a上新追加一个元素，并返回指向新元素的指针。
// 需要把返回的指针使用类型转换，转换为具体的类型，然后再给新元素本身或者是各字段（如果数组的元素是复杂类型）赋值。
// 如果数组已满，则重新分配两倍（nalloc*size)的内存空间，且nalloc更新为2*nalloc
void *ntg_array_push(ntg_array_t *a);

// 返回将要添加n个元素到数组中其首个元素的地址
void *ntg_array_push_n(ntg_array_t *a, ntg_uint_t n);


// 如果一个数组对象是被分配在堆上的，那么当调用ntg_array_destroy销毁以后，如果想再次使用，就可以调用此函数。
// 如果一个数组对象是被分配在栈上的，那么就需要调用此函数，进行初始化的工作以后，才可以使用。
ntg_int_t
ntg_array_init(ntg_array_t *array, ntg_pool_t *pool, ntg_uint_t n, size_t size);



#endif /* CORE_NTG_ARRAY_H_ */
