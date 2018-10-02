/**
 * @file ntg_user_group.c
 * @brief
 * @details
 * @author tzh
 * @date Jan 28, 2016
 * @version V0.1
 * @copyright tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_user_behavior.h"
#include "ntg_user.h"


static ntg_int_t ntg_group_enable_user(ntg_user_group_t *gp, ntg_user_t *u, ntg_int_t flag);
static void ntg_group_user_control_cb(evutil_socket_t fd, short what, void *arg);

/**
 *
 * @param gp
 * @return
 */
ntg_int_t ntg_user_group_reuse(ntg_user_group_t *gp){

	ntg_int_t i;

	for(i=0; i < gp->users_n; i++){
//		if(gp->gp_users[i] != NULL){
//
//		}
	}

	return NTG_OK;
}


/* 在组结束时，必须将占有的用户归还给用户池，这样用户对象才能被重用 */



/**
 * 初始化用户组对象
 * @details 从gp关联的管理对象中获取指定数目的用户，并初始化组内用户池
 * @param[in] gp 组对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 在对gp进行初始化前必须对其初始化用到的参数进行初始化
 * data-->管理的用户管理对象， gp_id， consumer，gp_users,
 * users_n
 */
ntg_int_t ntg_user_group_init(ntg_user_group_t *gp){
	ntg_int_t i;
	ntg_user_manage_t *mg;
	ntg_user_t  *u;

	static float x[24] = {
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
			11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
			21, 22, 23};

	mg = (ntg_user_manage_t *)gp->manage;

	/* 判断可用用户数是否够 */
	if(mg->free_user_n < gp->users_n){
		//记录日志
		return NTG_ERROR;
	}

	/* 初始化队列 */
	ntg_queue_init(&gp->free_q);
	ntg_queue_init(&gp->live_q);

	/* 组内用户获取与用户池初始化 */
	i = gp->users_n;
	do{
		i--;
		u = mg->free_users;
		mg->free_users = u->data;
		ntg_queue_insert_head(&gp->free_q, &u->qele);
	}while(i);

	mg->free_user_n -= gp->users_n;
	gp->free_n = gp->users_n;

	gp->next_atv = 0;
	gp->data = NULL;

	if(gp->is_scene){//是否采用场景控制
		printf("=====采用场景控制=====\n");
		/* 初始化样条对象 */
		if ( ntg_cubic_spline_interpolation_init(&gp->cs,x,gp->scene, SPILINE_INPUT_MAX)!= NTG_OK ) {
			//TODO 记录日志
			return NTG_ERROR;
		}
		/* 获取插入值 */
		if ( ntg_cubic_spline_get_interpolation_points(&gp->cs, SPILINE_OUTPUT_MAX) != NTG_OK){//24*60 = 2440
			//TODO 记录日志
			return NTG_ERROR;
		}

		gp->st_users = (ntg_int_t)(gp->cs.F[gp->point++] / 100.0 * gp->users_n );//获取开始点用户数
		gp->point = gp->point > gp->cs.M ? 0 : gp->point;

		/* 初始化事件 */
		gp->event =  event_new(gp->manage->cycle->base, -1, EV_TIMEOUT|EV_PERSIST,
				ntg_group_user_control_cb, gp);//TODO 要跟换
		if(gp->event == NULL){
			printf("event_new   ----> failed\n");
			return NTG_ERROR;
		}

		gp->time.tv_sec = 1;
		gp->time.tv_usec = 0;
	}

	return NTG_OK;
}
/**
 * 用户组释放
 * @param gp 用户组对象
 * @return 成功返回NTG_OK
 */
ntg_int_t ntg_user_group_free(ntg_user_group_t *gp){
	ntg_user_manage_t *mg;
	ntg_user_group_t *pre;
	ntg_queue_t *q, *next;
	ntg_user_t *u;
	ntg_int_t count = 0;
	printf(".........ntg_user_group_free......1.....\n");
	mg = gp->manage;
	/* 从消费者中删除 */
	gp->consumer->gps[gp->gp_id] = NULL;

	/* 是否在任务队列 */
	if(gp->active){//在任务队列中
		/* 从队列中删除 */
		printf(".........ntg_user_group_free......2.....\n");
		pre = (ntg_user_group_t *)mg->tasks;
		printf(".........ntg_user_group_free......2..0...\n");
		if(gp == pre){
			printf(".........ntg_user_group_free......2....1.\n");
			mg->tasks = gp->data;
			gp->data = NULL;
		}else{
			printf(".........ntg_user_group_free......2....2.\n");
			while(gp != (ntg_user_group_t *)pre->data){
				pre = pre->data;
			}
			printf(".........ntg_user_group_free......2....4.\n");
			if(pre->data){
				pre->data = gp->data;
				gp->data = NULL;
			}
		}
		printf(".........ntg_user_group_free......2...3.\n");
	}
	printf(".........ntg_user_group_free......3.....\n");
	/* 组内事件释放 */
	if(gp->event){
		event_free(gp->event);
		gp->event = NULL;
	}

	/* 组内用户释放 */
	//空闲用户队列
	for (q = ntg_queue_head(&(gp->free_q)); q != ntg_queue_sentinel(&(gp->free_q)); q = next) {
		next = ntg_queue_next(q);
		ntg_queue_remove(q);

		u = ntg_queue_data(q,ntg_user_t, qele);
		if(u->event){
			event_del(u->event);
			//u->event = NULL;
		}

		printf(".......free..............\n");
		u->data = mg->free_users;
		mg->free_users = u;
		count++;
	}
	// 活跃用户队列
	for (q = ntg_queue_head(&(gp->live_q)); q != ntg_queue_sentinel(&(gp->live_q)); q = next) {
		next = ntg_queue_next(q);
		ntg_queue_remove(q);

		u = ntg_queue_data(q,ntg_user_t, qele);
		if(u->event){
			event_del(u->event);
			//u->event = NULL;
		}
		printf(".......live..............\n");
		u->data = mg->free_users;
		mg->free_users = u;
		count++;
	}
	mg->free_user_n += count;

	printf(".........ntg_user_group_free.....5.users=%d....\n", gp->users_n);
	ntg_free(gp);
	return NTG_OK;
}

/**
 * 激活组内的用户
 * @param[in] gp 用户组对象
 * @param[in] num 本次允许激活的最大用户数
 * @return 成功返回NTG_OK,需要继续返回NTG_AGAIN, 否则返回NTG_ERROR
 * @note 还没进行错误处理
 * @see 在ntg_user_manage_process_task 中被调用
 */
ntg_int_t ntg_user_activate_group_users(ntg_user_group_t *gp, ntg_int_t num) {
	ntg_cycle_t *cycle;
	ntg_user_manage_t *mg;
	ntg_user_t *u;
	ntg_int_t n;
	ntg_queue_t *q;

	mg = (ntg_user_manage_t *) gp->manage;
	cycle = mg->cycle;

	if (gp->is_scene) {
		n = ntg_min(gp->st_users - gp->live_n, num);
	} else {
		n = ntg_min(gp->users_n - gp->live_n, num);
	}
//	printf("---------ntg_user_activate_group_users----n===%d---start=%d ==live==%d---\n", n, gp->st_users, gp->live_n);
	/* 激活组内用户 */
	while(n--) {
		q = ntg_queue_head(&gp->free_q);
		ntg_queue_remove(q);

		u = ntg_queue_data(q,ntg_user_t, qele);

		if(ntg_group_enable_user(gp, u, 0) != NTG_OK){
			//TODO 记录日志
			printf("----------------ntg_group_enable_user(gp, u, 0) != NTG_OK------\n");
			ntg_queue_insert_tail(&gp->free_q, q);
			continue;
		}

		ntg_queue_insert_head(&gp->live_q, q);
		u->live = 1;
		gp->live_n++;
	}

	if (gp->is_scene && gp->live_n == gp->st_users) {
		printf("----------is scene------------\n");
		//启动场景控制,任务队列中脱离
		struct timeval time;
		time.tv_sec = 30;
		time.tv_usec = 0;
		if (evtimer_add(gp->event, &time) < 0) {
			//TODO 纪录日志
//			event_free(gp->event);
			printf("evtimer_add   ----> failed\n");
//			gp->active = 0;//需要退出
//			return NTG_ERROR;
		}

		return NTG_OK;
	} else if (gp->live_n == gp->users_n) {
		return NTG_OK;
	}

	return NTG_AGAIN;

}

/**
 * 组内用户控制回调函数
 * @param[in] fd 描述对象，对于定时事件没有意义
 * @param[in] what 触发事件标志
 * @param[in] arg 传入的参数
 */
static void ntg_group_user_control_cb(evutil_socket_t fd, short what, void *arg) {
	ntg_user_group_t *gp;
	ntg_cycle_t *cycle;
	ntg_queue_t *q;
	ntg_user_t *u;
	ntg_int_t n, count;
	printf("ntg_group_user_control_cb\n");
	gp = (ntg_user_group_t*) arg;
	cycle = gp->manage->cycle;

	n = (ntg_int_t)(gp->cs.F[gp->point++] / 100.0 * gp->users_n);
	gp->point = gp->point > gp->cs.M ? 0 : gp->point;

	count = gp->live_n - n;
	printf("=-------live-%d----point=%d--n-%d----count--(%d)------------\n", gp->live_n, gp->point,n, count);
	if(count < 0){//需要增加用户
		count = (-count);
		while(count--){
			q = ntg_queue_head(&gp->free_q);
			ntg_queue_remove(q);

			u = ntg_queue_data(q,ntg_user_t, qele);

			if(ntg_group_enable_user(gp, u, 1) != NTG_OK){
				//TODO 记录日志
				printf("-------ntg_group_enable_user-----failed---------\n");
				ntg_queue_insert_tail(&gp->free_q, q);
				continue;
			}

			ntg_queue_insert_head(&gp->live_q, q);
			u->live=1;
			gp->live_n++;
		}

	} else {//需要减少用户

		while(count--){
			q = ntg_queue_last(&gp->live_q);
			ntg_queue_remove(q);

			u = ntg_queue_data(q, ntg_user_t, qele);
			u->live = 0;//TODO 回调函数要做处理

			ntg_queue_insert_tail(&gp->free_q, q);//插入到空闲队列
			gp->live_n--;
		}
	}
}


/**
 * 激活单个用户
 * @param[in] gp 用户组对象
 * @param[in] u 待激活的用户
 * @param[in] flag 阶段标志（0，表示启动阶段，1表示控制阶段）
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 通过enable标志，判断用户是否被激活过
 */
static ntg_int_t ntg_group_enable_user(ntg_user_group_t *gp, ntg_user_t *u, ntg_int_t flag){
	ntg_cycle_t *cycle;
	ntg_user_manage_t *mg;
	float time;

	mg = (ntg_user_manage_t *)gp->manage;
	cycle = mg->cycle;

	if (!u->enable) {
		u->user_id = gp->next_atv;
		u->group = gp;
		u->log = mg->log;
		u->data = mg;

		if(u->event != NULL){
			event_free(u->event);
			u->event = NULL;
		}

		u->event = event_new(cycle->base, -1, EV_TIMEOUT, ntg_user_timeout_cb, u);//TODO 要跟换
		if (u->event == NULL) {
			printf("event_new   ----> failed\n");
			return NTG_ERROR;
		}

		u->enable = 1;
		gp->next_atv++;
	}

	if(flag){
		time = ntg_user_get_time(u->group);
	}else{
		time = ntg_math2_average_random(0.0, 35.0, 4);//进行用户的分散处理
	}

	u->sessions = ntg_user_get_session_num(u->group);
	u->clicks = ntg_user_get_clicks(u->group);


	u->time.tv_sec = (long int)time;
	u->time.tv_usec = (long int)((time - (long int)u->time.tv_sec) * 1000) * 1000;

	if ( evtimer_add(u->event, &u->time) < 0 ){
			//TODO 纪录日志
//			event_free(u->event);
			printf("evtimer_add   ----> failed\n");
			return NTG_ERROR;
	}

	return NTG_OK;
}
