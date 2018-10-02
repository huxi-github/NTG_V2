/*
 * ntg_queue.c
 *
 *  Created on: Jul 28, 2015
 *      Author: tzh
 */


/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_queue.h"

/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */

ntg_queue_t *
ntg_queue_middle(ntg_queue_t *queue) {
	ntg_queue_t *middle, *next;

	middle = ntg_queue_head(queue);
	//头尾相等，只有一个元素或为空
	if (middle == ntg_queue_last(queue)) {
		return middle;
	}

	next = ntg_queue_head(queue);
	//使用快慢指针的技巧
	for (;;) {
		middle = ntg_queue_next(middle);

		next = ntg_queue_next(next);

		if (next == ntg_queue_last(queue)) {
			return middle;
		}

		next = ntg_queue_next(next);

		if (next == ntg_queue_last(queue)) {
			return middle;
		}
	}
	return middle;
}

/* the stable insertion sort */

void ntg_queue_sort(ntg_queue_t *queue,
		ntg_int_t (*cmp)(const ntg_queue_t *, const ntg_queue_t *)) {

	ntg_queue_t *q, *prev, *next;

	q = ntg_queue_head(queue);

	if (q == ntg_queue_last(queue)) {
		return;
	}

	for (q = ntg_queue_next(q); q != ntg_queue_sentinel(queue); q = next) {

		prev = ntg_queue_prev(q);
		next = ntg_queue_next(q);

		ntg_queue_remove(q);

		do {
			if (cmp(prev, q) <= 0) {
				break;
			}

			prev = ntg_queue_prev(prev);

		} while (prev != ntg_queue_sentinel(queue));

		ntg_queue_insert_after(prev, q);
	}
}
