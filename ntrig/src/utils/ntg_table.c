/**
 * @file ntg_table.c
 * @brief
 * @details
 * @author tzh
 * @date Jun 3, 2016
 * @version V0.1
 * @copyright tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_string.h"
#include "ntg_palloc.h"

#include "ntg_table.h"

static ntg_hash_node_t *ntg_hash_get_node(ntg_hash_table_t *table);
static ntg_hash_node_t *ntg_hash_free_node(ntg_hash_table_t *table,
		ntg_hash_node_t *node);
/**
 * hash表的初始化
 * @param[in] hash 待初始化的hash表
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note hash的相关属性的事先准备
 *  size、key、cmp、node_n、pool
 */
ntg_int_t ntg_hash_table_init(ntg_hash_table_t *hash){
	void *p;
	ntg_uint_t i;
	ntg_hash_node_t *n, *n_next;
	ntg_uint_t len;

	/* 分配资源 */
//	printf("-------------ntg_hash_table_init----------------1------\n");
	//计算大小
	len = hash->size * sizeof(ntg_hash_node_t **) +
			hash->node_n * sizeof(ntg_hash_node_t);

	//分配内存
	p = ntg_palloc(hash->pool, len);
	if(p == NULL){
		return NTG_ERROR;
	}


	ntg_memzero(p,hash->size * sizeof(ntg_hash_node_t **));
	hash->buckets = (ntg_hash_node_t**)p;

	p +=  hash->size * sizeof(ntg_hash_node_t **);
	hash->nodes = (ntg_hash_node_t*)p;

	/* 节点池 */
	n = hash->nodes;
	i = hash->node_n;

	n_next = NULL;

	do{
		i--;
		n[i].next = n_next;
		n_next = &n[i];
	}while(i);

	hash->free_nodes = n_next;
	hash->free_node_n = hash->node_n;
	printf("-------------ntg_hash_table_init----------------2------\n");
	return NTG_OK;
}

/**
 * 从hash表中查找指定的对象
 * @param[in] table hash表
 * @param[in] data 数据对象
 * @return 存在返回对应的节点对象， 否则返回NULL
 * @note data可以时实际的关联数据也可以是临时数据
 */
ntg_hash_node_t *ntg_hash_find_node(ntg_hash_table_t *table, void *data){
	ntg_hash_node_t *node;
	ntg_uint_t key;

	if(table == NULL){
		return NULL;
	}
	key = table->key(data);
	printf("-------------ntg_hash_find_node---------------key===%d-----\n", (int)key);
	node = table->buckets[key%table->size];
	printf("-------------ntg_hash_find_node--------------3---index==%d--\n", key%table->size);
	while(node != NULL){
		printf("-------------ntg_hash_find_node----------------6------\n");
		if(table->cmp(node->data, data) == 0){
//			printf("-------------ntg_hash_find_node----------------5------\n");
			return node;
		}
		node = node->next;
	}
	printf("-------------ntg_hash_find_node----------------4------\n");
	return NULL;
}

/**
 * 向hash表中插入一个节点
 * @param[in] table hash表
 * @param[in] data 节点管理的数据
 * @return 插入成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 如果hash表中已存在该节点返回NTG_OK
 */
ntg_int_t ntg_hash_insert_node(ntg_hash_table_t *table, void *data){
	ntg_hash_node_t *node;
	ntg_uint_t key;
	ntg_uint_t index;

	if(table == NULL){
		return NTG_ERROR;
	}
	/* 判断是否已存在 */
	if((ntg_hash_find_node(table, data)) != NULL){
		//已存在,记录日志
		return NTG_OK;
	}
	printf(".............ntg_hash_insert_node........1.\n");
	/* 插入接点 */
	node = ntg_hash_get_node(table);
	if(node == NULL){
		//没有可用节点，记录日志
		return NTG_ERROR;
	}

	node->data = data;
	key = table->key(data);
	index = (table->key(data))%table->size;
	node->next = table->buckets[index];
	table->buckets[index] = node;
	printf("ntg_hash_insert_node.......index= %d..key=%d..ok....\n", index, key);
	return NTG_OK;
}

/**
 * 通过一个临时数据对象,删除对应的节点
 * @param[in] table hash表对象
 * @param[in] tmp_d 临时数据对象
 * @return 成功返回被删除节点关联的数据，否则返回NULL
 * @note tmp_d 为临时对象，需提供用于计算key和cmp的相关数据
 */
void* ntg_hash_delete_node(ntg_hash_table_t *table, void *tmp_d){
	ntg_hash_node_t *node, *pre_node, *next_node;
	ntg_int_t index;
	void *data;

	index = (table->key(tmp_d))%table->size;

	if((table == NULL  ) || (table->buckets[index] == NULL)){
		return NULL;
	}

	pre_node = table->buckets[index];

	/* 首节点判断 */
	if(table->cmp(pre_node->data, tmp_d) == 0){
		table->buckets[index] = NULL;
		data = pre_node->data;
		pre_node->data = NULL;
		ntg_hash_free_node(table, pre_node);
		return data;
	}

	node = pre_node->next;

	while(node){
		if(table->cmp(node->data, tmp_d) == 0){
			break;
		}
		next_node = node->next;
		pre_node = node;
		node = next_node;
	}

	if(node == NULL){
		return NULL;
	}

	pre_node->next = node->next;
	data = node->data;
	node->data = NULL;
	ntg_hash_free_node(table, node);

	return data;
}

/**
 * 从节点池中获取一个节点
 * @param[in] table hash表对象
 * @return 若存在可用节点返回一个节点对象，否则返回NULL.
 */
static ntg_hash_node_t *ntg_hash_get_node(ntg_hash_table_t *table){
	ntg_hash_node_t *node;

	node = table->free_nodes;
	if(node == NULL){
		//没有可用，记录日志
		return NULL;
	}

	table->free_nodes = node->next;
	table->free_node_n--;

	node->next = NULL;

	return node;
}

/**
 * 回收节点对象
 * @param[in] table hash表对象
 * @return 成功返回NTG_OK，否则返回NTG_ERROR.
 * @note 在回收节点前，必须处理其关联的资源
 */
static ntg_hash_node_t *ntg_hash_free_node(ntg_hash_table_t *table,
		ntg_hash_node_t *node){

	if(node->data){
		node->data = NULL;
	}

	node->next = table->free_nodes;

	table->free_nodes = node;
	table->free_node_n++;

	return NTG_OK;
}
