/**
 * @file 	ntg_hash.h
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 15, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef UTILS_NTG_HASH_H_
#define UTILS_NTG_HASH_H_


#include "../ntg_config.h"
#include "../ntg_core.h"


typedef struct {
    void             *value;
    u_short           len;
    u_char            name[1];
} ntg_hash_elt_t;


typedef struct {
    ntg_hash_elt_t  **buckets;
    ntg_uint_t        size;
} ntg_hash_t;


typedef struct {
    ntg_hash_t        hash;
    void             *value;
} ntg_hash_wildcard_t;


typedef struct {
    ntg_str_t         key;
    ntg_uint_t        key_hash;
    void             *value;
} ntg_hash_key_t;


typedef ntg_uint_t (*ntg_hash_key_pt) (u_char *data, size_t len);


typedef struct {
    ntg_hash_t            hash;
    ntg_hash_wildcard_t  *wc_head;
    ntg_hash_wildcard_t  *wc_tail;
} ntg_hash_combined_t;

/**
 * hash初始化对象
 */
typedef struct {
    ntg_hash_t       *hash;///需要初始化的hash指针
    ntg_hash_key_pt   key;///为计算散列值用的函数指针

    ntg_uint_t        max_size;///允许的最大bucket数量
    ntg_uint_t        bucket_size; ///为每个bucket允许占用的最大空间

    char             *name;///名称
    ntg_pool_t       *pool;///内存池
    ntg_pool_t       *temp_pool;///临时内存池
} ntg_hash_init_t;


#define NTG_HASH_SMALL            1
#define NTG_HASH_LARGE            2

#define NTG_HASH_LARGE_ASIZE      16384
#define NTG_HASH_LARGE_HSIZE      10007

#define NTG_HASH_WILDCARD_KEY     1
#define NTG_HASH_READONLY_KEY     2

/**
 * @name hash关键字数组
 */
typedef struct {
    ntg_uint_t        hsize;///大小

    ntg_pool_t       *pool;
    ntg_pool_t       *temp_pool;

    ntg_array_t       keys;///关键字
    ntg_array_t      *keys_hash;///hash值

    ntg_array_t       dns_wc_head;
    ntg_array_t      *dns_wc_head_hash;

    ntg_array_t       dns_wc_tail;
    ntg_array_t      *dns_wc_tail_hash;
} ntg_hash_keys_arrays_t;

/**
 * 表元素结构
 */
typedef struct {
    ntg_uint_t        hash;
    ntg_str_t         key;
    ntg_str_t         value;
    u_char           *lowcase_key;
} ntg_table_elt_t;


void *ntg_hash_find(ntg_hash_t *hash, ntg_uint_t key, u_char *name, size_t len);
void *ntg_hash_find_wc_head(ntg_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ntg_hash_find_wc_tail(ntg_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ntg_hash_find_combined(ntg_hash_combined_t *hash, ntg_uint_t key,
    u_char *name, size_t len);

ntg_int_t ntg_hash_init(ntg_hash_init_t *hinit, ntg_hash_key_t *names,
    ntg_uint_t nelts);
ntg_int_t ntg_hash_wildcard_init(ntg_hash_init_t *hinit, ntg_hash_key_t *names,
    ntg_uint_t nelts);

#define ntg_hash(key, c)   ((ntg_uint_t) key * 31 + c)
ntg_uint_t ntg_hash_key(u_char *data, size_t len);
ntg_uint_t ntg_hash_key_lc(u_char *data, size_t len);
ntg_uint_t ntg_hash_strlow(u_char *dst, u_char *src, size_t n);


ntg_int_t ntg_hash_keys_array_init(ntg_hash_keys_arrays_t *ha, ntg_uint_t type);
ntg_int_t ntg_hash_add_key(ntg_hash_keys_arrays_t *ha, ntg_str_t *key,
    void *value, ntg_uint_t flags);


#endif /* UTILS_NTG_HASH_H_ */
