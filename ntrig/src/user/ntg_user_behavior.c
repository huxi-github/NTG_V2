/**
 * @file ntg_user_behavior.c
 * @brief
 * @details
 * @author tzh
 * @date Mar 20, 2016
 * @version V0.1
 * @copyright tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user.h"
#include "../utils/ntg_math.h"
#include "ntg_user_behavior.h"

/**
 * 获取会话数
 * @param[in] gp 用户组对象
 * @return
 */
int ntg_user_get_session_num(ntg_user_group_t *gp){
	float ret;
	ret = ntg_get_random(gp->ss_num_type, gp->ss_num);

	return ceil(ret);
}

/**
 * 会话间隔时间
 * @param[in] gp 用户组对象
 * @return
 */
float ntg_user_get_session_interval(ntg_user_group_t *gp){
	float ret;
	do{
		ret = ntg_get_random(gp->ss_itv_type, gp->ss_itv);
	}while(ret > 1800);//30*60s

	return ret;
}

/**
 * 获取点击数
 * @param[in] gp 用户组对象
 * @return
 */
int ntg_user_get_clicks(ntg_user_group_t *gp){
	float ret;
	ret = ntg_get_random(gp->clk_num_type, gp->clk_itv);

	return ceil(ret);
}
/**
 * 获取用户思考时间（点击间隔）
 * @param[in] gp 用户组对象
 * @return
 */
float ntg_user_get_time(ntg_user_group_t *gp){
	float ret;
	do{
		ret = ntg_get_random(gp->clk_itv_type, gp->clk_itv);
	}while(ret > 600);//10*60s

	return ret;
}
