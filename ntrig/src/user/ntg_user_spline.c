/**
 * @file ntg_user_spline.c
 * @brief
 * @details
 * @author tzh
 * @date Mar 16, 2016
 * @version V0.1
 * @copyright tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user_spline.h"

#include <math.h>

static ntg_int_t ntg_cubic_spline(ntg_cubic_spline_t *cs);
static ntg_int_t ntg_cs_interpolation(ntg_cubic_spline_t *cs);

/**
 * 三次样条曲线初始化
 * @param[in] cs 样条对象
 * @param ctrl_x 控制点x数组
 * @param ctrl_y 控制点y数组
 * @param ctrl_n 控制点数
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t ntg_cubic_spline_interpolation_init(ntg_cubic_spline_t *cs,
		float *ctrl_x, float *ctrl_y, int ctrl_n) {
	ntg_int_t nDataCount;
	ntg_memzero(cs, sizeof(ntg_cubic_spline_t));

	if (NULL == ctrl_x || NULL == ctrl_y || ctrl_n < 3 || ctrl_n
			> 24) {
		cs->m_create = 0;
		return NTG_ERROR;
	}
	cs->N = ctrl_n - 1;

	nDataCount = cs->N + 1;

	ntg_memcpy(cs->X, ctrl_x, nDataCount * sizeof(float));
	ntg_memcpy(cs->Y, ctrl_y, nDataCount * sizeof(float));

	return ntg_cubic_spline(cs);
}
/**
 * 获取指定数目的插入值
 * @param[in] cs 样条对象
 * @param[in] out_n 输出点数
 * @return 成功返回NTG_OK，否则返回NTG_ERROR
 * @note 输出数据保存在Z、F中
 */
ntg_int_t ntg_cubic_spline_get_interpolation_points(ntg_cubic_spline_t *cs, int out_n) {
	if (!cs->m_create) {//0不成立

		return NTG_ERROR;
	}

	if(out_n > SPILINE_OUTPUT_MAX || out_n < 1){
		return NTG_ERROR;
	}

	cs->M = out_n - 1;

//	printf("----3---\n");
	return ntg_cs_interpolation(cs);

}

/**
 * 计算三次样条曲线数据
 * @param cs 样条对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t ntg_cubic_spline(ntg_cubic_spline_t *cs) {
	int i, P, L;

	int N; //输入控制点数量
	int M; //输出的插入点数量
	ntg_fouble_p X, Y; //输入的控制点数组
	ntg_fouble_p H, A, B, C, D; //间距，缓存运算中间结果。

	N = cs->N;
	M = cs->M;
	X = cs->X;
	Y = cs->Y;
	H = cs->H;
	A = cs->A;
	B = cs->B;
	C = cs->C;
	D = cs->D;

	//计算步长
	for (i = 1; i <= N; i++) {
		H[i - 1] = X[i] - X[i - 1];
	}

	L = N - 1;
	for (i = 1; i <= L; i++) {
		A[i] = H[i - 1] / (H[i - 1] + H[i]);
		B[i] = 3 * ((1 - A[i]) * (Y[i] - Y[i - 1]) / H[i - 1] + A[i]
				* (Y[i + 1] - Y[i]) / H[i]);
	}
	A[0] = 1;
	A[N] = 0;
	B[0] = 3 * (Y[1] - Y[0]) / H[0];
	B[N] = 3 * (Y[N] - Y[N - 1]) / H[N - 1];

	for (i = 0; i <= N; i++) {
		D[i] = 2;
	}

	for (i = 0; i <= N; i++) {
		C[i] = 1 - A[i];
	}

	P = N;
	for (i = 1; i <= P; i++) {

		if (fabs(D[i]) <= 0.000001) {
			cs->m_create = 0;
			return NTG_ERROR;
		}
		A[i - 1] = A[i - 1] / D[i - 1];
		B[i - 1] = B[i - 1] / D[i - 1];
		D[i] = A[i - 1] * (-C[i]) + D[i];
		B[i] = -C[i] * B[i - 1] + B[i];
	}
	B[P] = B[P] / D[P];
	for (i = 1; i <= P; i++) {
		B[P - i] = B[P - i] - A[P - i] * B[P - i + 1];
	}
	cs->m_create = 1;
	return NTG_OK;
}

/**
 * 插入点
 * @param[in] cs 样条对象
 * @return 成功返回NTG_OK， 否则返回NTG_ERROR
 */
static ntg_int_t ntg_cs_interpolation(ntg_cubic_spline_t *cs) {

	int N; //输入控制点数量
	int M; //输出的插入点数量
	ntg_fouble_p X, Y; //输入的控制点数组
	ntg_fouble_p Z, F; //输出的控制点数组
	ntg_fouble_p H, A, B, C, D; //间距，缓存运算中间结果。
//	printf("----1---\n");
	N = cs->N;
	M = cs->M;
	X = cs->X;
	Y = cs->Y;
	Z = cs->Z;
	F = cs->F;
	H = cs->H;
	A = cs->A;
	B = cs->B;
	C = cs->C;
	D = cs->D;

	float dbStep = (X[N] - X[0]) / (M);
	int i;
	for (i = 0; i <= M; ++i) {
		Z[i] = X[0] + dbStep * i;
	}

	for (i = 1; i <= M; i++) {
		if (ntg_cubic_spline_get_y_by_x(cs, &Z[i], &F[i]) != NTG_OK) {
			printf("----2---\n");
			return NTG_ERROR;
		}

	}

	F[0] = Y[0];

	return NTG_OK;
}
/**
 * 获取某个x点的值
 * @param[in] cs 样条对象
 * @param[in] x 横坐标
 * @param[out] y 输出纵坐标值
 * @return 成功返回NTG_OK，否则返回NTG_ERROR
 */
ntg_int_t ntg_cubic_spline_get_y_by_x(ntg_cubic_spline_t *cs, const float *x,
		float *y) {
	int N; //输入控制点数量
	int M; //输出的插入点数量
	ntg_fouble_p X, Y; //输入的控制点数组
	ntg_fouble_p Z, F; //输出的控制点数组
	ntg_fouble_p H, A, B, C, D; //间距，缓存运算中间结果。

	N = cs->N;
	M = cs->M;
	X = cs->X;
	Y = cs->Y;
	Z = cs->Z;
	F = cs->F;
	H = cs->H;
	A = cs->A;
	B = cs->B;
	C = cs->C;
	D = cs->D;

	if (!cs->m_create) {
		return NTG_ERROR;
	}

	float E, E1, K, K1, H1;
	int j;

	if (*x < X[0]) {
		j = 0;

	} else if (*x > X[N]) {
		j = N - 1;
	} else {
		for (j = 1; j <= N; j++) {
			if (*x <= X[j]) {
				j = j - 1;

				break;
			}
		}

	}

	//////////////////////////////////////////////////////////////////////////
	E = X[j + 1] - *x;
	E1 = E * E;
	K = *x - X[j];
	K1 = K * K;
	H1 = H[j] * H[j];

	*y = (3 * E1 - 2 * E1 * E / H[j]) * Y[j]
			+ (3 * K1 - 2 * K1 * K / H[j]) * Y[j + 1];
	*y = *y + (H[j] * E1 - E1 * E) * B[j] - (H[j] * K1 - K1 * K)
			* B[j + 1];
	*y = *y / H1;

	return NTG_OK;
}

