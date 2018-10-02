/**
 * @file ntg_math.c
 * @brief
 * @details
 * @author tzh
 * @date Jun 6, 2016
 * @version V0.1
 * @copyright tzh
 */
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_math.h"


static float ntg_math2_lognormal_random(float a, float mu, float sigma);
static float ntg_math2_posisson_random(float mu);
static float ntg_math2_exponential_random(float a, float b);
static float ntg_math2_normal_random(float mu, float sigma);
static float ntg_math2_weibull_random(float a, float scale, float shape);

/**
 * 获取指定类型的随机数
 * @param type 随机函数类型
 * @param args 需要输入的参数
 * @return 返回对应的随机数
 * @note 必须保证输入参数与对应的随机函数相匹配
 */
float ntg_get_random(int type, float args[3]){
	float ret;

	switch(type){
	case NTG_MATH_LOGNORMAL_E :
		ret = ntg_math2_lognormal_random(args[0],args[1],args[2]);
		break;
	case NTG_MATH_POSISSON_E :
		ret = ntg_math2_posisson_random(args[0]);
		break;
	case NTG_MATH_EXPONENTIAL_E :
		ret = ntg_math2_exponential_random(args[0], args[1]);
		break;
	case NTG_MATH_NORMAL_E :
		ret = ntg_math2_normal_random(args[0], args[1]);
		break;
	case NTG_MATH_WEIBULL_E :
		ret = ntg_math2_weibull_random(args[0], args[1], args[2]);
		break;
	default:
		//TODO
		if (args[0] < args[1]){
			ret = ntg_math2_average_random(args[0], args[1], 2);
		} else {
			ret = ntg_math2_average_random(args[1], args[0], 2);
		}
		break;
	}

	return ret;
}
/**
 * 获取用户的浏览时间
 * @return 返回浏览的时间
 * @note 返回的时间单位为s
 * @note 以weibull分布实现
 */
float ntg_math2_get_time(){
	float time;

	do{
		time = ntg_math2_lognormal_random(35, 0.49, 2.78);
	}while(time>600);//10*60

	return time;//返回最接近的整数
}


int ntg_math2_get_clicks(){
	float clicks;

	clicks = ntg_math2_lognormal_random(1.5, 0.47, 0.69);

	return ceil(clicks);//返回最接近的整数
}
float ntg_math2_get_session_interval(){
	float time;
	do{
		time = ntg_math2_weibull_random(10, 0.93, 1.67);
	}while(time>1800);//30*60

	return time;//返回最接近的整数
}

int ntg_math2_get_session_num(){
	float count;
	count = ntg_math2_posisson_random(0.39);

	return ceil(count);
}

int ntg_math2_get_lnline_num(){
	float num;
	num = ntg_math2_exponential_random(1,31.93);

	int n = ceil(num);
	return n%32;
}


/**
 * 获取指定范围[min, max)内的平均随机数
 * @param[in] min 范围的下限
 * @param[in] max 范围的上限
 * @param[in] n 保留的小数位
 * @return 返回对应的随机值
 * @note 产生原理：先根据小数位进行范围的扩大，然后在其扩展的
 * 范围上取一随机值，最后再将其缩小到指定范围
 * @note 精度不能太大，会溢出
 */
float ntg_math2_average_random(float min, float max, int n) {

	float result;
	float c;
	int min_int, diff, rand_int;

	//srand((unsigned) time(NULL));

	c = pow(10.0, (float) n);

	/* 将随机范围扩大c倍 */
	min_int = min * c ;//可能溢出
	diff = (max - min) *c ;//可能溢出

	/* 求大范围内的随机数 */
	rand_int = rand() % diff;
	result = rand_int + min_int;

	return result/c;
}

/**
 * 获取服从正态分布的随机数
 * @param[in] mu 正态分布参数
 * @param[in] sigma 正态分布参数
 * @return 服从正态分布的随机数
 * @note 采用累计分布函数的反函数计算随机数
 */
static float ntg_math2_normal_random(float mu, float sigma){
	float p, p1, p2;
	do{
		p1 = ntg_math2_average_random(-1 , 1, 4);
		p2 = ntg_math2_average_random(-1 , 1, 4);
		p = p1*p1 + p2*p2;
	}while(p >= 1);

	return mu + sigma*p1*sqrt(-2 * log(p)/p);
}


/**
 * 获取服从weibull分布的随机数
 * @param[in] a x轴开始位置
 * @param[in] scale 尺度参数
 * @param[in] shape 形状参数
 * @return 服从weibull分布的随机数
 * @note 采用累计分布函数的反函数计算随机数
 */
static float ntg_math2_weibull_random(float a, float scale, float shape){
	return a + scale*pow(-log(ntg_math2_average_random(0,1,4)),1/shape);
}

/**
 * 获取服从对数正态分布的随机数
 * @param[in] a x轴开始位置
 * @param[in] mu 正态分布参数
 * @param[in] sigma 正态分布参数
 * @return 服从对数正态分布的随机数
 * @note 采用累计分布函数的反函数计算随机数
 */
static float ntg_math2_lognormal_random(float a, float mu, float sigma){
	return a + exp(ntg_math2_normal_random(mu, sigma));
}

static float ntg_math2_posisson_random(float mu){
	float b = 1;
	int i;
	for(i=0; b >=exp(-mu); i++){
		b *= ntg_math2_average_random(0,1, 4);
	}

	return i -1;
}

/**
 *
 * @param a 开始位置
 * @param b 1/r=E(x)
 * @return
 */
static float ntg_math2_exponential_random(float a, float b){
	return a - b * log(ntg_math2_average_random(0,1, 4));
}
