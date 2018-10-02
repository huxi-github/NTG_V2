/**
 * @file 	ntg_user.c
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 20, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_user_behavior.h"
#include "ntg_user.h"

#include "../utils/ntg_process_cycle.h"
#include "../http/ntg_http.h"

#include "../utils/ntg_channel.h"


#define DEFAULT_WORKER_USERS	1024
#define DEFAULT_REQUIST_NUM		10

static char *ntg_user_block(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_user_users(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_user_use(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_user_requests(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *
ntg_user_balance(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);

static void *ntg_user_core_create_conf(ntg_cycle_t *cycle);
static char *ntg_user_core_init_conf(ntg_cycle_t *cycle, void *conf);

//static ntg_int_t ntg_user_master_init(ntg_cycle_t *cycle);
static ntg_int_t ntg_user_module_init(ntg_cycle_t *cycle);
static ntg_int_t ntg_user_process_init(ntg_cycle_t *cycle);
static ntg_int_t ntg_user_process_init_in_virtual (ntg_cycle_t *cycle);
static ntg_int_t ntg_user_process_init_in_worker(ntg_cycle_t *cycle);

static ntg_uint_t ntg_user_get_key(void *data);
static ntg_uint_t ntg_user_cmp(void *d1, void *d2);

static ntg_int_t  ntg_user_message_traffic_processes(ntg_user_t *us);

//static ntg_int_t ntg_user_create_request(ntg_user_t *us);
//static ntg_http_request_t *ntg_user_get_request(ntg_user_t *u);
//static ntg_int_t ntg_user_reinit_request(ntg_user_t *us);
//static ntg_int_t ntg_user_process_header(ntg_user_t *us);
//static void ntg_user_abort_request(ntg_user_t *us);
//static void ntg_user_finalize_request(ntg_user_t *us, ntg_int_t uc);

static ntg_uint_t ntg_user_max_module;

/**
 * user模块命令集合
 */
static ntg_command_t ntg_users_commands[] = {
		{ ntg_string("users"),
		  NTG_MAIN_CONF | NTG_CONF_BLOCK | NTG_CONF_NOARGS,
		  ntg_user_block,
		  0,
		  0,
		  NULL
		},

		ntg_null_command
};

/**
 * user模块上下文
 */
static ntg_core_module_t ntg_users_module_ctx = {
		ntg_string("users"),
		NULL,
		NULL
};

/**
 * user模块定义
 */
ntg_module_t ntg_users_module = {
		NTG_MODULE_V1,
		&ntg_users_module_ctx, /**模块上下文*/
		ntg_users_commands, /**模块命令*/
		NTG_CORE_MODULE, /**模块类型*/
		NULL, /**初始化master*/
		NULL, /**初始化模块*/
		NULL, /**初始化进程*/
		NULL, /**初始化线程*/
		NULL, /**退出线程*/
		NULL, /**退出进程*/
		NULL, /**退出模块*/
		NTG_MODULE_V1_PADDING
};

static ntg_str_t user_core_name = ntg_string("user_core");

static ntg_command_t ntg_user_core_commands[] = {
		{ ntg_string("worker_users"),
		  NTG_USER_CONF | NTG_CONF_TAKE1,
		  ntg_user_users,
		  0,
		  0,
		  NULL
		},

		{ ntg_string("use"),
		  NTG_USER_CONF | NTG_CONF_TAKE1,
		  ntg_user_use,
		  0,
		  0,
		  NULL
		},

		{ ntg_string("req_num"),
		  NTG_USER_CONF | NTG_CONF_TAKE1,
		  ntg_user_requests,
		  0,
		  0,
		  NULL
		},

		{ ntg_string("balance"),
		  NTG_USER_CONF | NTG_CONF_TAKE1,
		  ntg_user_balance,
		  0,
		  0,
		  NULL
		},

		ntg_null_command
};

ntg_user_module_t ntg_user_core_module_ctx = {
		&user_core_name,
		ntg_user_core_create_conf,
		ntg_user_core_init_conf,

		{ NULL, NULL, NULL, NULL }
};

ntg_module_t ntg_user_core_module = {

		NTG_MODULE_V1, &ntg_user_core_module_ctx, /**模块上下文*/
		ntg_user_core_commands, /**模块命令*/
		NTG_USER_MODULE, /**模块类型*/
		NULL, /**初始化master*/
		ntg_user_module_init, /**初始化模块*/
		ntg_user_process_init, /**初始化进程*/
		NULL, /**初始化线程*/
		NULL, /**退出线程*/
		NULL, /**退出进程*/
		NULL, /**退出模块*/
		NTG_MODULE_V1_PADDING
};

/**
 * user配置块解析函数
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 配置存储位置
 * @return 成功返回NTG_CONF_OK,重复返回"is duplicate" , 否则返回NTG_CONF_ERROR
 */
static char*
ntg_user_block(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	char *rv;
	void ***ctx;
	ntg_uint_t i;
	ntg_conf_t pcf;
	ntg_user_module_t *m;

	//检查是否存在
	if (*(void **) conf) {
		return "is duplicate";
	}

	//索引化事件模块
	ntg_user_max_module = 0;
	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->type != NTG_USER_MODULE) {
			continue;
		}

		ntg_modules[i]->ctx_index = ntg_user_max_module++;
	}

	//分配好配置存储空间
	ctx = ntg_pcalloc(cf->pool, sizeof(void *));
	if (ctx == NULL) {
		return NTG_CONF_ERROR;
	}

	*ctx = ntg_pcalloc(cf->pool, ntg_user_max_module * sizeof(void *));
	if (*ctx == NULL) {
		return NTG_CONF_ERROR;
	}

	*(void **) conf = ctx;

	//调用所有USER模块组的配置创建函数
	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->type != NTG_USER_MODULE) {
			continue;
		}

		m = ntg_modules[i]->ctx;

		if (m->create_conf) {
			(*ctx)[ntg_modules[i]->ctx_index] = m->create_conf(cf->cycle);
			if ((*ctx)[ntg_modules[i]->ctx_index] == NULL) {
				return NTG_CONF_ERROR;
			}
		}
	}

	//切换配置
	pcf = *cf;

	cf->ctx = ctx;
	cf->module_type = NTG_USER_MODULE;
	cf->cmd_type = NTG_USER_CONF;

	rv = ntg_conf_parse(cf, NULL);//解析events配置块
	//恢复配置
	*cf = pcf;

	if (rv != NTG_CONF_OK)
		return rv;

	//调用所有USER模块组的初始化配置函数
	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->type != NTG_USER_MODULE) {
			continue;
		}

		m = ntg_modules[i]->ctx;

		if (m->init_conf) {
			rv = m->init_conf(cf->cycle, (*ctx)[ntg_modules[i]->ctx_index]);
			if (rv != NTG_CONF_OK) {
				return rv;
			}
		}
	}

	return NTG_CONF_OK;
}

/**
 * users配置命令解析函数
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 配置指针
 * @return 配置成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 */
static char *
ntg_user_users(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_user_conf_t *ucf = conf;

	ntg_str_t *value;

	if (ucf->worker_users != NTG_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = cf->args->elts;
	ucf->worker_users = ntg_atoi(value[1].data, value[1].len);
	if (ucf->worker_users == (ntg_uint_t) NTG_ERROR) {
		ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, "invalid number \"%V\"",
				&value[1]);

		return NTG_CONF_ERROR;
	}

//	cf->cycle->user_n = ucf->worker_users;

	return NTG_CONF_OK;
}

/**
 * 用户的默认行为配置函数
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 配置指针
 * @return 配置成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 */
static char *
ntg_user_use(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_user_conf_t *ucf = conf;

	ntg_int_t m;
	ntg_str_t *value;
	ntg_user_module_t *module;

	if (ucf->use != NTG_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = cf->args->elts;

	//设置用户的行为模块
	for (m = 0; ntg_modules[m]; m++) {
		if (ntg_modules[m]->type != NTG_USER_MODULE) {
			continue;
		}

		module = ntg_modules[m]->ctx;
		if (module->name->len == value[1].len) {
			if (ntg_strcmp(module->name->data, value[1].data) == 0) {
				ucf->use = ntg_modules[m]->ctx_index;
				ucf->name = module->name->data;

				return NTG_CONF_OK;
			}
		}
	}

	ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, "invalid event type \"%V\"",
			&value[1]);

	return NTG_CONF_ERROR;
}

/**
 * 每个用户的最大请求数配置函数
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 配置指针
 * @return 配置成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 */
static char *
ntg_user_requests(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_user_conf_t *ucf = conf;

	ntg_str_t *value;

	if (ucf->req_num != NTG_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	value = cf->args->elts;
	ucf->req_num = ntg_atoi(value[1].data, value[1].len);
	if (ucf->req_num == (ntg_uint_t) NTG_ERROR) {
		ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, "invalid number \"%V\"",
				&value[1]);

		return NTG_CONF_ERROR;
	}

	return NTG_CONF_OK;
}

static char *
ntg_user_balance(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{

	ntg_user_conf_t *ucf = conf;

	ntg_str_t *value;

	if (ucf->blc_type != NTG_CONF_UNSET_UINT) {
		return "is duplicate";
	}

    value = cf->args->elts;

    if (ntg_strcasecmp(value[1].data, (u_char *) "dynamic") == 0) {
    	ucf->blc_type  = 1;

    } else if (ntg_strcasecmp(value[1].data, (u_char *) "polling") == 0) {
    	ucf->blc_type  = 0;

    } else {
        ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                     "invalid value \"%s\" in \"%s\" directive, "
                     "it must be \"on\" or \"off\"",
                     value[1].data, cmd->name.data);
        return NTG_CONF_ERROR;
    }


    return NTG_CONF_OK;
}

/**
 * 创建配置的存储结构
 * @param[in] cycle 循环体
 */
static void *
ntg_user_core_create_conf(ntg_cycle_t *cycle) {
	ntg_user_conf_t *ucf;

	ucf = ntg_palloc(cycle->pool, sizeof(ntg_user_conf_t));
	if (ucf == NULL) {
		return NULL;
	}

	ucf->worker_users = NTG_CONF_UNSET_UINT;
	ucf->use = NTG_CONF_UNSET_UINT;
	ucf->req_num = NTG_CONF_UNSET_UINT;
	ucf->name = (void *) NTG_CONF_UNSET;
	ucf->blc_type = NTG_CONF_UNSET_UINT;

	return ucf;
}

/**
 * 用户核心模块的初始化配置
 * @param[in] cycle 循环体
 * @param[in] conf 配置存储指针
 * @return 成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 */
static char *
ntg_user_core_init_conf(ntg_cycle_t *cycle, void *conf) {
	ntg_user_conf_t *ucf = conf;
	ntg_int_t i;
	ntg_module_t *module;
	ntg_user_module_t *user_module;

	ntg_conf_init_uint_value(ucf->worker_users, DEFAULT_WORKER_USERS);
//	cycle->user_n = ucf->worker_users;

	module = NULL;

	//查看配置文件的模块是否匹配
	if ((ucf->name != (void *) NTG_CONF_UNSET) || (ucf->use
			!= NTG_CONF_UNSET_UINT)) {

		for (i = 0; ntg_modules[i]; i++) {

			if (ntg_modules[i]->type != NTG_USER_MODULE) {
				continue;
			}

			user_module = ntg_modules[i]->ctx;

			if (ntg_strcmp(user_module->name->data, user_core_name.data) == 0) {
				continue;
			}

			if (ntg_strcmp(user_module->name->data, ucf->name) == 0) {
				module = ntg_modules[i];
				break;
			}
		}
		ntg_log_error(NTG_LOG_WARN, cycle->log, 0, "illegal settings, no matching %s module", ucf->name);
	}

	//默认使用第一个用户行为模块
	if (module == NULL) {
		for (i = 0; ntg_modules[i]; i++) {

			if (ntg_modules[i]->type != NTG_USER_MODULE) {
				continue;
			}

			user_module = ntg_modules[i]->ctx;

			if (ntg_strcmp(user_module->name->data, user_core_name.data) == 0) {
				continue;
			}

			module = ntg_modules[i];
			break;
		}
	}

	if (module == NULL) {
		ntg_log_error(NTG_LOG_EMERG, cycle->log, 0, "no users module found");
		return NTG_CONF_ERROR;
	}

	ntg_conf_init_uint_value(ucf->use, module->ctx_index);

	user_module = module->ctx;
	ntg_conf_init_ptr_value(ucf->name, user_module->name->data);

	ntg_conf_init_value(ucf->req_num, DEFAULT_REQUIST_NUM);

	ntg_conf_init_value(ucf->blc_type, 0);

	return NTG_CONF_OK;
}

#define NTG_USER_DEFAULT_HASH_TABLE_SIZE 32

/**
 * master进程初始化函数
 * @param[in] cycle 全局循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 初始化虚拟用户层的一些资源
 * @note 在子进程初始化结束后调用，因为这不是与子进程共用的
 * @note 如果master初始化失败，应该先通知多有的子进程结束
 * ，然后自己也结束。可以通过设置结束标志完成。
 */
//static ntg_int_t ntg_user_master_init(ntg_cycle_t *cycle){
//	ntg_uint_t m;
//	ntg_user_manage_t *manage;
//
//	ntg_core_conf_t * ccf;
//	ntg_user_conf_t *ucf;
//	ntg_user_module_t *module;
//
//	ntg_pool_t *pool;
//
//
//	ucf = (ntg_user_conf_t *) ntg_user_get_conf(cycle->conf_ctx, ntg_user_core_module);
//	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);
//
//	pool = ntg_create_pool(NTG_DEFAULT_POOL_SIZE, cycle->log);//私有化自己的内存池
//    if (pool == NULL) {
//        return NTG_ERROR;
//    }
//
//	/* 用户管理对象 */
//	manage = ntg_palloc(cycle->pool, sizeof(ntg_user_manage_t));
//	if(manage == NULL){
//		return NTG_ERROR;
//	}
//	cycle->manage = manage;
//
//	manage->cycle = cycle;
//	manage->log = cycle->log;
//	manage->pool = pool;
//	manage->user_n = ucf->worker_users * ccf->worker_processes;//总共能够模拟用户数
//
//	/* 初始化管理对象 */
//	if( ntg_user_manage_init(manage) != NTG_OK){
//		printf("-------------ntg_user_manage_init----------------error------\n");
//		return NTG_ERROR;
//	}
//
//	/* 用户行为函数的初始化 TODO 暂时没有实现 */
//	for (m = 0; ntg_modules[m]; m++) {
//		if (ntg_modules[m]->type != NTG_USER_MODULE) {
//			continue;
//		}
//
//		if (ntg_modules[m]->ctx_index != ucf->use) {
//			continue;
//		}
//
//		module = ntg_modules[m]->ctx;
//
//		if (module->actions.init(cycle, NULL) != NTG_OK) {
//			/* fatal */
//			return NTG_ERROR;
//		}
//
//		break;
//	}
//
//	return NTG_OK;
//}


/**
 * 获取用户对象的key
 * @param data
 * @return
 */
static ntg_uint_t ntg_user_get_key(void *data){
	ntg_user_t *user;

	user = (ntg_user_t*)data;
	return user->user_id;
}
/**
 * 比较函数
 * @param d1
 * @param d2
 * @return
 */
static ntg_uint_t ntg_user_cmp(void *d1, void *d2){
	ntg_user_t *user1;
	ntg_user_t *user2;

	user1 = (ntg_user_t*)d1;
	user2 = (ntg_user_t*)d2;

	return user1->user_id - user2->user_id;
}
/**
 * 配置之后进行用户模块初始化
 * @param[in] cycle 全局循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note TODO 没有做实质性的事
 * @note init_cycle中被调用
 */
static ntg_int_t ntg_user_module_init(ntg_cycle_t *cycle) {
	void ***cf;
	ntg_core_conf_t *ccf;
	ntg_user_conf_t *ucf;

	cf = ntg_get_conf(cycle->conf_ctx, ntg_users_module);
	ucf = (*cf)[ntg_user_core_module.ctx_index];

	if (!ntg_test_config && ntg_process <= NTG_PROCESS_MASTER) {
		ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0,
				"using the \"%s\" user's behaver method", ucf->name);
	}

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	if (ccf->master == 0) {
		//单进程工作方式
		return NTG_OK;
	}

	return NTG_OK;
}

/**
 * 用户模块的工作进程初始化
 * @param[in] cycle 循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 用户模块的进程初始化比较特殊，它在不同进程中进行不同的初始化
 * @note 当前只实现了虚拟用户进程与流量产生进程的初始化
 */
static ntg_int_t ntg_user_process_init(ntg_cycle_t *cycle) {

	ntg_int_t ret;

	switch(ntg_processes[ntg_process_slot].type){
	case NTG_VIRTUAL_PROCESS:

		ret = ntg_user_process_init_in_virtual(cycle);
		printf("........ntg_user_process_init_in_virtual.......\n");
		break;

	case NTG_TRAFFIC_PROCESS:

		ret = ntg_user_process_init_in_worker(cycle);
		break;
	default:
		ret = NTG_OK;
	}

	return ret;

}

/**
 * 用户模块在虚拟用户进程中的初始化
 * @param[in] cycle 全局循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 初始化虚拟用户层的一些资源
 * @note 如果初始化失败，应该先通知多有的子进程结束
 * ，然后自己也结束。可以通过设置结束标志完成。
 * TODO 并没有完全实现
 */
static ntg_int_t ntg_user_process_init_in_virtual (ntg_cycle_t *cycle) {
	ntg_uint_t m;
	ntg_user_manage_t *manage;

	ntg_core_conf_t * ccf;
	ntg_user_conf_t *ucf;
	ntg_user_module_t *module;

	ntg_pool_t *pool;

	ucf = (ntg_user_conf_t *) ntg_user_get_conf(cycle->conf_ctx, ntg_user_core_module);
	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	cycle->blc_type = ucf->blc_type;
	/* 私有化自己的内存池 */
	pool = ntg_create_pool(NTG_DEFAULT_POOL_SIZE, cycle->log);
    if (pool == NULL) {
        return NTG_ERROR;
    }

	/* 用户管理对象 */
	manage = ntg_palloc(cycle->pool, sizeof(ntg_user_manage_t));
	if(manage == NULL){
		printf("ntg_user_process_init_in_virtual............failed\n");
		return NTG_ERROR;
	}
	cycle->manage = manage;

	manage->cycle = cycle;
	manage->log = cycle->log;
	manage->pool = pool;
	manage->user_n = ucf->worker_users * ccf->worker_processes;//总共能够模拟用户数

	/* 初始化管理对象 */
	if( ntg_user_manage_init(manage) != NTG_OK){
		printf("-------------ntg_user_manage_init----------------error------\n");
		return NTG_ERROR;
	}

	/* 用户行为函数的初始化 TODO 暂时没有实现 */
	for (m = 0; ntg_modules[m]; m++) {
		if (ntg_modules[m]->type != NTG_USER_MODULE) {
			continue;
		}

		if (ntg_modules[m]->ctx_index != ucf->use) {
			continue;
		}

		module = ntg_modules[m]->ctx;

		if (module->actions.init(cycle, NULL) != NTG_OK) {
			/* fatal */
			return NTG_ERROR;
		}

		break;
	}

	return NTG_OK;
}

/**
 * 用户模块在流量产生进程中的初始化
 * @param[in] cycle 循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 对用户的free_request链表进行了初始化
 */
static ntg_int_t ntg_user_process_init_in_worker(ntg_cycle_t *cycle) {
	ntg_uint_t i;
	ntg_http_request_t *r, *r_next;
	ntg_user_conf_t *ucf;

	ucf = ntg_user_get_conf(cycle->conf_ctx, ntg_user_core_module);

	cycle->blc_type = ucf->blc_type;
	/* 进程请求池的资源处理 */
	cycle->request_n = ucf->req_num * ucf->worker_users;
	cycle->requests = ntg_palloc(cycle->pool, sizeof(ntg_http_request_t) * cycle->request_n);
	if (cycle->requests == NULL) {
		printf("}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{\n");
		return NTG_ERROR;
	}

	r = cycle->requests;
	i = cycle->request_n;
	r_next = NULL;

	do {
		i--;
		r[i].data = r_next;
		r_next = &r[i];
	} while (i);

	cycle->free_reqs = r_next;
	cycle->free_req_n = cycle->request_n;

	return NTG_OK;
}




/**
 * 从free_user队列中获取一个用户对象
 * @param[in] cycle 循环对象
 * @param[in] log 错误日志对象
 * @return 成功返回用户对象指针，否则返回NULL
 */
ntg_user_t * ntg_get_user(ntg_cycle_t *cycle, ntg_log_t *log) {

	ntg_user_t *u;
	ntg_user_conf_t *ucf;

	ntg_user_manage_t *manage;

	ucf = ntg_user_get_conf(cycle->conf_ctx, ntg_user_core_module);

	manage = cycle->manage;

	u = manage->free_users;

	if (u == NULL) {
		ntg_log_error(NTG_LOG_ALERT, log, 0,
				"%ui worker_users are not enough",
				manage->user_n);

		return NULL;
	}

	manage->free_users = u->data;
	manage->free_user_n--;

	u->data = NULL;

	//ntg_memzero(u, sizeof(ntg_user_t));
	u->request_n = ucf->req_num;
//	u->manage = manage;

	//TODO 可以根据配置创建一个日志对象
	u->log = manage->log;

	u->live = 1;

//	req = u->requests;


//	ntg_memzero(req, sizeof(ntg_http_request_t) * ucf->req_num);
//
//	i = ucf->req_num;
//	req_next = NULL;
//	do {
//		i--;
//		req[i].data = req_next;
//		req_next = &req[i];
//	} while (i);
//
//	u->free_requests = req_next;
//	u->free_request_n = ucf->req_num;
//	u->request_n = ucf->req_num;

	return u;
}



/**
 * 用户添加回调函数
 * @param fd
 * @param what
 * @param arg
 */
#define NTG_ADD_USERS 100
void ntg_user_add_cb(evutil_socket_t fd, short what, void *arg){
	int i;
	ntg_user_manage_t *mg;
	ntg_cycle_t *cycle;
	ntg_user_t  *us;

	mg = (ntg_user_manage_t*) arg;

	cycle = mg->cycle;

	for (i = 0; i < NTG_ADD_USERS ; i++) {
		us = ntg_get_user(cycle, cycle->log);
		if (us == NULL) {
			printf("worker_users are not enough ******(%d)*********\n", mg->user_n);
		}
		us->event = event_new(cycle->base, -1, EV_TIMEOUT | EV_PERSIST,
				ntg_user_timeout_cb, us);

		us->time.tv_sec = ntg_user_get_time(us->group);
		evtimer_add(us->event, &us->time);
	}

}

///**
// *
// * @param us
// * @return
// */
//ntg_int_t ntg_user_enable(ntg_user_t *us){
//
//}

/**
 * 用户定时事件回调函数
 * @param[in] fd 描述对象，对于定时事件没有意义
 * @param[in] what 触发事件标志
 * @param[in] arg 传入的参数
 */
void ntg_user_timeout_cb(evutil_socket_t fd, short what, void *arg) {
	ntg_int_t ret;
	ntg_user_t *us;
	ntg_cycle_t *cycle;
	struct event *ev;
	ntg_user_conf_t *ucf;
	struct timeval  time;
	float n;

	us = (ntg_user_t*) arg;
	cycle = us->group->manage->cycle;

	static long count = 0;
	count++;
	//	printf("+++++++++ntg_user_timeout_cb+++++((%ld))++++\n" ,count);

	ucf = ntg_user_get_conf(cycle->conf_ctx, ntg_user_core_module);

	ev = us->event;

	if(ev == NULL){
		printf("+++++++++ntg_user_timeout_cb++++ failed+++++++++\n" );
	}

	if (us->live == 0) {//非活跃用户
		if (!us->group->is_scene) {//没有设置场景
			us->live = 1;
			n = ntg_user_get_session_interval(us->group);

			time.tv_sec = (long int)n;
			time.tv_usec = (long int)((n - (long int)time.tv_sec) * 1000) * 1000;

			us->sessions = ntg_user_get_session_num(us->group);//设置本次会话的数
			us->clicks = ntg_user_get_clicks(us->group);//设置第一次访问中的点击数
			event_add(ev, &time);
		} else {//设置了场景控制
			event_del(us->event);

			/* 将用户插入到空闲队列中 */
			ntg_queue_t *q = &us->qele;
			ntg_queue_remove(q);
			ntg_queue_insert_tail(&us->group->free_q, q);//插入到空闲队列
		}
	} else { //活跃用户
		/* 向流量产生进程发送消息 */
		ret = ntg_user_message_traffic_processes(us);
		if (ret == NTG_ERROR) {
			printf("+++++++++ntg_user_message_traffic_processes failed++++++++++\n");
		}

		ntg_requests++;

		us->clicks--;
		if (us->clicks <= 0) {//本次会话结束
			us->sessions--;
			if (us->sessions > 0) {//还有其他的页面需要访问
				us->clicks = ntg_user_get_clicks(us->group);//下一次会话点击数
			} else {
				us->live = 0;//在下一次回调时进行处理
			}
			n = ntg_user_get_session_interval(us->group);//会话间隔

		} else {
			n = ntg_user_get_time(us->group);//思考时间
		}
		time.tv_sec = (long int)n;
		time.tv_usec = (long int)((n - (long int)time.tv_sec) * 1000) * 1000;
		if (event_add(ev, &time) < 0){
			printf("+++++++++ntg_user_timeout_cb++++ failed+++++++++\n" );
		}
	}
}


/**
 * 向流量产生进程发送请求类消息
 * @param[in] us 用户对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t  ntg_user_message_traffic_processes(ntg_user_t *us){

	ntg_user_manage_t *mg;
	ntg_message_t msg;
	ntg_request_message_t *rqt;

	mg = us->group->manage;

	/* 清空发下命令 */
	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_REQUEST_TYPE;

	rqt = &msg.body.rqt;
	/* 设置 csm_id, gp_id, url_id*/
	rqt->csm_id = us->group->consumer->id;
	rqt->gp_id = us->group->gp_id;
	rqt->user_id = us->user_id;
	//ch.url_id = 0;//TODO 有待实现

	if ( mg->blc->blance(mg, &msg) != NTG_OK){//blance 已进行错误日志处理

		return NTG_ERROR;
	}

    return NTG_OK;
}

///**
// * 重新初始化请求
// * @param[in] us 用户对象
// * @return 成功返回NTG_OK,否则返回NTG_ERROR
// */
//static ntg_int_t ntg_user_reinit_request(ntg_user_t *us) {
//	return NTG_OK;
//}
///**
// *
// * @param us
// * @return
// */
//static ntg_int_t ntg_user_process_header(ntg_user_t *us) {
//	return NTG_OK;
//}
///**
// *
// * @param us
// */
//static void ntg_user_abort_request(ntg_user_t *us) {
//	return;
//}
//
//static void ntg_user_finalize_request(ntg_user_t *us, ntg_int_t uc) {
//	return;
//}
