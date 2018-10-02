/**
 * @file 	ntg_user.h
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 20, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef USER_NTG_USER_H_
#define USER_NTG_USER_H_


#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../http/ntg_http.h"

#include "../utils/ntg_table.h"
#include "../utils/ntg_math.h"

#include "ntg_user_manage.h"
#include "ntg_user_consumer.h"
#include "ntg_user_group.h"

/* 用户模块魔术 */
#define NTG_USER_MODULE		0x52455355
#define NTG_USER_CONF		0x02000000

/**
 * @name 用户对象
 */
struct ntg_user_s {
	void 					*data;
	ntg_uint_t	 			user_id;///用户id
	ntg_user_group_t		*group;

	struct event 			*event;///用户定时事件对象
	struct timeval 			time;///超时时间

	ntg_queue_t				qele;///队列元素

	ntg_log_t				*log;///日志对象
//	ntg_pool_t  			*pool;///用户内部内存池

	/* 模型参数 */
	ntg_uint_t 				clicks;///点击数
	ntg_uint_t	            sessions;///会话数

	ntg_int_t				request_n;///允许的最大请求数
	ntg_int_t				have_rqs;///已有的reqest

	unsigned 				live:1;///活跃标记
	unsigned				enable:1;///event已初始化
};



/**
 * 用户行为函数
 */
typedef struct {
	ntg_int_t (*get_time)(ntg_user_t *user, void *args);
	ntg_int_t (*get_url)(ntg_user_t *user, void *args);
	ntg_int_t (*init)(ntg_cycle_t *cycle, void *args);
	ntg_int_t (*done)(ntg_cycle_t *cycle);
} ntg_user_actions_t;

/**
 * @name 用户模块配置对象
 */
typedef struct {
	ntg_uint_t 	worker_users;/// 每个工作进程中最多模拟的用户数
	ntg_uint_t 	use;/// 选用的用户行为模块所在的序列号
	ntg_uint_t	req_num;/// 每个用户最多产生的请求数
	ntg_uint_t  blc_type;
	u_char 		*name;/// 底层模块名称
} ntg_user_conf_t;

/**
 * @name 用户模块定义
 */
typedef struct {
	ntg_str_t			*name;///模块名称
    // 在解析配置项前，这个回调方法用于创建存储配置项参数的结构体
    void                *(*create_conf)(ntg_cycle_t *cycle);
    // 在解析配置项完成后，init_conf方法会被调用，用于综合处理当前事件模块感兴趣的全部配置项。
    char                *(*init_conf)(ntg_cycle_t *cycle, void *conf);
    ntg_user_actions_t	actions;
} ntg_user_module_t;




extern ntg_module_t ntg_users_module;
extern ntg_module_t	ntg_user_core_modules;

#define ntg_user_get_conf(conf_ctx, module)                                  \
             (*(ntg_get_conf(conf_ctx, ntg_users_module))) [module.ctx_index];

ntg_user_t * ntg_get_user(ntg_cycle_t *cycle, ntg_log_t *log);
void ntg_user_timeout_cb(evutil_socket_t fd, short what, void *arg);

//void ntg_user_timeout_handler(ntg_event_t *ev);

#endif /* USER_NTG_USER_H_ */
