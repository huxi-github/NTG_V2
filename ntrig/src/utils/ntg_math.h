/**
 * @file ntg_math.h
 * @brief
 * @details
 * @author tzh
 * @date Jun 6, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_MATH_H_
#define NTG_MATH_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

enum ntg_math_type_e {
	NTG_MATH_AVERAGE_E=0,
	NTG_MATH_LOGNORMAL_E,
	NTG_MATH_POSISSON_E,
	NTG_MATH_EXPONENTIAL_E,
	NTG_MATH_NORMAL_E,
	NTG_MATH_WEIBULL_E
};

float ntg_math2_average_random(float min, float max, int n);
//float ntg_math2_normal_random(float mu, float sigma);
//float ntg_math2_weibull_random(float a, float scale, float shape);
//float ntg_math2_lognormal_random(float a, float mu, float sigma);

float ntg_get_random(int type, float args[3]);

int ntg_math2_get_session_num();
float ntg_math2_get_session_interval();

int ntg_math2_get_clicks();
float ntg_math2_get_time();

int ntg_math2_get_lnline_num();

#endif /* NTG_MATH_H_ */
