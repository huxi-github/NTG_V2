/**
 * @file ntg_user_group.h
 * @brief
 * @details
 * @author tzh
 * @date Jan 28, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_USER_GROUP_H_
#define NTG_USER_GROUP_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user.h"
#include "ntg_user_spline.h"

/**
 * @name 用户组对象
 */
struct ntg_user_group_s {
	void		 *data;///作为next指针
	ntg_int_t    gp_id;///组id，由前台计算得到
	ntg_str_t    gp_name;///组名

	ntg_user_manage_t   *manage;///用户管理对象
	ntg_user_consumer_t *consumer;///所属的消费者

	/* 组内事件 */
	struct event 			*event;///定时事件对象
	struct timeval 			time;///超时时间

	/* 组内用户 */
//	ntg_user_t **gp_users;///组内用户，需要自行释放
	ntg_int_t  users_n;///总的用户数
	ntg_int_t  next_atv;///下一个激活序号

	ntg_queue_t  free_q;///空闲队列
	ntg_uint_t   free_n;//
	ntg_queue_t  live_q;///活跃队列
	ntg_uint_t  live_n;///活跃用户数

	/* 场景控制 */
	ntg_int_t   point;///模拟位置1440中的一个
	ntg_int_t   st_users;///开始用户数
	float       scene[24];///场景参数
	unsigned ss_num_type:8;///会话数分布函数类型
	unsigned ss_itv_type:8;///会话间隔分布函数类型
	unsigned clk_num_type:8;///点击数分布函数类型
	unsigned clk_itv_type:8;///点击间隔分布函数类型
	float ss_num[3];///会话数
	float ss_itv[3];///会话间隔
	float clk_num[3];///点击数
	float clk_itv[3];///点击间隔

	ntg_cubic_spline_t cs;///样条对象
	unsigned 	active:1;///在任务队列中
	unsigned	type:8;///用户模型类别
	unsigned    is_scene:1;///场景标志

};

ntg_int_t ntg_user_group_init(ntg_user_group_t *gp);
ntg_int_t ntg_user_group_free(ntg_user_group_t *gp);
ntg_int_t ntg_user_activate_group_users(ntg_user_group_t *gp, ntg_int_t num);

#endif /* NTG_USER_GROUP_H_ */
