/*
 * ntg_rbtree.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_RBTREE_H_
#define CORE_NTG_RBTREE_H_

#include "../ntg_config.h"
//#include "../ntg_core.h"

typedef ntg_uint_t  ntg_rbtree_key_t;
typedef ntg_int_t   ntg_rbtree_key_int_t;


typedef struct ntg_rbtree_node_s  ntg_rbtree_node_t;

struct ntg_rbtree_node_s {
    ntg_rbtree_key_t       key;
    ntg_rbtree_node_t     *left;
    ntg_rbtree_node_t     *right;
    ntg_rbtree_node_t     *parent;
    u_char                 color;
    u_char                 data;
};


typedef struct ntg_rbtree_s  ntg_rbtree_t;

typedef void (*ntg_rbtree_insert_pt) (ntg_rbtree_node_t *root,
    ntg_rbtree_node_t *node, ntg_rbtree_node_t *sentinel);

struct ntg_rbtree_s {
    ntg_rbtree_node_t     *root;
    ntg_rbtree_node_t     *sentinel;
    ntg_rbtree_insert_pt   insert;
};


#define ntg_rbtree_init(tree, s, i)                                           \
    ntg_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i

#define ntg_rbtree_is_empty(tree)											  \
	(((tree)->root) == ((tree)->sentinel))

void ntg_rbtree_insert(ntg_rbtree_t *tree, ntg_rbtree_node_t *node);
void ntg_rbtree_delete(ntg_rbtree_t *tree, ntg_rbtree_node_t *node);
void ntg_rbtree_insert_value(ntg_rbtree_node_t *root, ntg_rbtree_node_t *node,
    ntg_rbtree_node_t *sentinel);
void ntg_rbtree_insert_timer_value(ntg_rbtree_node_t *root,
    ntg_rbtree_node_t *node, ntg_rbtree_node_t *sentinel);


#define ntg_rbt_red(node)               ((node)->color = 1)
#define ntg_rbt_black(node)             ((node)->color = 0)
#define ntg_rbt_is_red(node)            ((node)->color)
#define ntg_rbt_is_black(node)          (!ntg_rbt_is_red(node))
#define ntg_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define ntg_rbtree_sentinel_init(node)  ntg_rbt_black(node)


static inline ntg_rbtree_node_t *
ntg_rbtree_min(ntg_rbtree_node_t *node, ntg_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}
#endif /* CORE_NTG_RBTREE_H_ */
