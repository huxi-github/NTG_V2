/**
 * @file ntg_table.h
 * @brief
 * @details
 * @author tzh
 * @date Jun 3, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_TABLE_H_
#define NTG_TABLE_H_

/**
 * @name 节点数据对象
 */
typedef struct ntg_hash_node_s {
	void *data;///节点数据
	struct ntg_hash_node_s *next;///下一个指针
} ntg_hash_node_t;

/**
 * 计算key的hash函数指针
 */
typedef ntg_uint_t (*ntg_hash_get_key_pt) (void *data);
/**
 * 比较函数指针
 * @param[in] d1 数据对象1
 * @param[in] d2 数据对象2
 * @return 如果 d1 == d2返回0；
 * 			d1 > d2 返回 1；
 * 			d1 < d2 返回 -1。
 */
typedef ntg_uint_t (*ntg_hash_compare_pt)(void *d1, void *d2);

/**
 * @name hash表对象
 */
typedef struct ntg_hash_table_s{
    ntg_hash_node_t  		**buckets;///hash表槽
    ntg_uint_t       		size;///hash表大小

    ntg_hash_get_key_pt   	key;///为计算散列值用的函数指针
    ntg_hash_compare_pt 	cmp;///比较函数

    /* 节点池 */
	ntg_hash_node_t 		*nodes;///节点数组
	ntg_uint_t        		node_n;///允许的最大节点数
	ntg_hash_node_t		    *free_nodes;///可用节点集合
	ntg_int_t				free_node_n;///可用节点数

//    char             *name;///名称
    ntg_pool_t       *pool;///内存池

} ntg_hash_table_t;

ntg_int_t ntg_hash_table_init(ntg_hash_table_t *hash);
ntg_hash_node_t *ntg_hash_find_node(ntg_hash_table_t *table, void *data);
ntg_int_t ntg_hash_insert_node(ntg_hash_table_t *table, void *data);
void* ntg_hash_delete_node(ntg_hash_table_t *table, void *tmp_d);


#endif /* NTG_TABLE_H_ */
