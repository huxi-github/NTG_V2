/**
 * @file ntg_user_spline.h
 * @brief
 * @details
 * @author tzh
 * @date Mar 16, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_USER_SPLINE_H_
#define NTG_USER_SPLINE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#define		SPILINE_INPUT_MAX		24//样条输入数据个数，24小时
//#define		SPILINE_OUTPUT_MAX		1440//定义样条数据区间个数最多为50个
#define		SPILINE_OUTPUT_MAX		1440//定义样条数据区间个数最多为50个

typedef float* ntg_fouble_p;

/**
 * @name 三次样条对象
 */
typedef struct ntg_cubic_spline_s {
	unsigned N:8;//输入控制点数量 24个--->ctrl_n -1
	unsigned M:20;//输出的插入点数量 1440个---> out_n -1
	unsigned m_create:4;//
	/* 输入的控制点数组 X、Y*/
	float X[SPILINE_INPUT_MAX];
	float Y[SPILINE_INPUT_MAX];
	/* 输入的控制点数 Z、F*/
	float Z[SPILINE_OUTPUT_MAX];
	float F[SPILINE_OUTPUT_MAX];
	 /* 间距，缓存运算中间结果 */
	float H[SPILINE_INPUT_MAX];
	float A[SPILINE_INPUT_MAX];
	float B[SPILINE_INPUT_MAX];
	float C[SPILINE_INPUT_MAX];
	float D[SPILINE_INPUT_MAX];
} ntg_cubic_spline_t;

ntg_int_t ntg_cubic_spline_interpolation_init(ntg_cubic_spline_t *cs,
		float *ctrl_X, float *ctrl_y, int ctrl_n);
ntg_int_t ntg_cubic_spline_get_interpolation_points(ntg_cubic_spline_t *cs, int out_n);
ntg_int_t ntg_cubic_spline_get_y_by_x(ntg_cubic_spline_t *cs, const float *x,
		float *y);

#endif /* NTG_USER_SPLINE_H_ */
