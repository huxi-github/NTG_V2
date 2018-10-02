/**
 * @file 	ntg_user_random_module.c
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 22, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_user.h"

static ntg_int_t ntg_user_random_get_time(ntg_user_t *user, void *args);
static ntg_int_t ntg_user_random_get_url(ntg_user_t *user, void *args);
static ntg_int_t ntg_user_random_init(ntg_cycle_t *cycle, void *args);
static ntg_int_t ntg_user_random_done(ntg_cycle_t *cycle);
static void * ntg_user_random_create_conf(ntg_cycle_t *cycle);
static char *ntg_user_random_init_conf(ntg_cycle_t *cycle, void *conf);

typedef struct {
	ntg_uint_t	epoll;
} ntg_user_random_conf_t;

static ntg_str_t 	random_name = ntg_string("random");

static ntg_command_t	ntg_user_random_commands[] = {

};


ntg_user_module_t	ntg_user_random_module_ctx = {
		&random_name,
		ntg_user_random_create_conf,
		ntg_user_random_init_conf,
		{
			ntg_user_random_get_time,
			ntg_user_random_get_url,
			ntg_user_random_init,
			ntg_user_random_done,
		}
};

ntg_module_t  ntg_user_random_module = {
    NTG_MODULE_V1,
    &ntg_user_random_module_ctx,        /* module context */
	ntg_user_random_commands,           /* module directives */
    NTG_USER_MODULE,                    /* module type */
    NULL,                                /* init master */
    NULL,                                /* init module */
    NULL,                                /* init process */
    NULL,                                /* init thread */
    NULL,                                /* exit thread */
    NULL,                                /* exit process */
    NULL,                                /* exit master */
    NTG_MODULE_V1_PADDING
};

/**
 * 获取定时时间
 * @param[in] user 用户对象
 * @param[in] args 关联的参数
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
static ntg_int_t ntg_user_random_get_time(ntg_user_t *user, void *args){

	return NTG_OK;
}
/**
 * 获取目标URL
 * @param[in] user 用户对象
 * @param[in] args 关联的参数
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
static ntg_int_t ntg_user_random_get_url(ntg_user_t *user, void *args){

	return NTG_OK;
}

/**
 * 行为模块的初始化函数
 * @param[in] cycle 循环对象
 * @param[in] args 关联的参数
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
static ntg_int_t ntg_user_random_init(ntg_cycle_t *cycle, void *args){

	return NTG_OK;
}
/**
 * 获取目标URL
 * @param[in] user 用户对象
 * @param[in] args 关联的参数
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
/**
 * 用户行为模块进程结束时的资源释放函数
 * @param[in] cycle 循环对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
static ntg_int_t ntg_user_random_done(ntg_cycle_t *cycle){

	return NTG_OK;
}


/*
 * 创建配置
 */
/**
 * 创建用户行为模块配置对象
 * @param[in] cycle 循环对象
 * @return 成功返回用户行为配置对象指针,否则返回NTG_CONF_ERROR
 */
static void *
ntg_user_random_create_conf(ntg_cycle_t *cycle)
{
    ntg_user_random_conf_t  *rcf;

    rcf = ntg_palloc(cycle->pool, sizeof(ntg_user_random_conf_t));
    if (rcf == NULL) {
        return NULL;
    }
//
//    epcf->events = NTG_CONF_UNSET;
//    epcf->aio_requests = NTG_CONF_UNSET;

    return rcf;
}


/*
 * 初始化配置
 */
/**
 *
 * @param[in] cycle 循环对象
 * @param[in] conf 配置对象指针
 * @return 成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 */
static char *
ntg_user_random_init_conf(ntg_cycle_t *cycle, void *conf)
{
//    ntg_epoll_conf_t *epcf = conf;
//
//    ntg_conf_init_uint_value(epcf->events, 512);
//    ntg_conf_init_uint_value(epcf->aio_requests, 32);

    return NTG_CONF_OK;
}
