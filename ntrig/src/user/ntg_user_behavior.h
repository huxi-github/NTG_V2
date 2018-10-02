/**
 * @file ntg_user_behavior.h
 * @brief
 * @details
 * @author tzh
 * @date Mar 20, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_USER_BEHAVIOR_H_
#define NTG_USER_BEHAVIOR_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_user.h"

#define NTG_MAC_ARGS   6
typedef float (*dist_process)(ntg_user_group_t *gp);
typedef struct {
	unsigned id:16;///分布编号id
	unsigned arg_n:16;///参数个数
	float    args[NTG_MAC_ARGS];///参数数组
} ntg_distribution_t;

//ntg_distribution_t ntg_dists[10] ={
//		{},
//};
//typedef struct {
//
//} ntg_user_behavior_t;

int ntg_user_get_session_num(ntg_user_group_t *gp);
float ntg_user_get_session_interval(ntg_user_group_t *gp);
int ntg_user_get_clicks(ntg_user_group_t *gp);
float ntg_user_get_time(ntg_user_group_t *gp);


#endif /* NTG_USER_BEHAVIOR_H_ */
