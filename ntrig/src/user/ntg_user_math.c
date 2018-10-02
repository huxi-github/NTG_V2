/*
 * ntg_user_math.h
 *
 *  Created on: Dec 24, 2015
 *      Author: root
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include <stdio.h>
#include<math.h>
#define PI 3.1415
#define TIME 420
#define N 101

//#include <algorithm>
#define PI 3.1415
int k=0;

/**
 * @param[in] min 最小值
 * @param[in] max 最大值
 * @param[in] n 小数位数
 * @return 返回一个[min，max)范围内的平均随机数
 */
double ntg_math_average_random(double min, double max, int n) {

	double result;
	double c;
	int min_int, diff, rand_int;

	c = pow(10.0, (double) n);

	/* 将随机范围扩大c倍 */
	min_int = min * c ;//可能溢出
	diff = (max - min) *c ;//可能溢出

	/* 求大范围内的随机数 */
	rand_int = rand() % diff;
	result = rand_int + min_int;

	return result/c;
}

//double ntg_math_average_random(double min, double max, int n) {
//	double c = pow(10.0, (double) n);
//	int minInteger = (int) (min * 10000);
//	int maxInteger = (int) (max * 10000);
//	int randInteger = rand() * rand();
//	int diffInteger = maxInteger - minInteger;
//	int resultInteger = randInteger % diffInteger + minInteger;
//	return resultInteger / c;
//}

/**
 * 获取坐标x点的帕累托概率密度函数值
 * @param[in] x 坐标
 * @param[in] x_min 分布中的xmin参数
 * @param[in] k 分布中的k参数
 * @return 返回对于的
 * @note
 *     帕累托概率密度函数：
 *     其中x是任何一个大于xmin的数，xmin是X最小的可能值（正数），
 *     k是为正的参数，即x=x_min的值。
 *     1）if x < x_min  p(x) = 0
 *     2) if x >= x_min p(x) = k * (x_min)^k / (x)^(k+1)
 *
 */
double ntg_math_pareto(double x, double x_min, double k) {
	if (x < x_min) {
		return 0;
	} else if (x == x_min) {
		return k;
	}
	return k * pow(x_min, k) * pow(x, -(k + 1));
}
/**
 * 产生在[min, max]范围内满足帕累托分布的随机数
 * @param[in] x_min 帕累托分布参数
 * @param[in] k 帕累托分布参数
 * @param[in] min 最小值
 * @param[in] max 最大值
 * @return 满足帕累托分布的随机数
 */
double ntg_math_pareto_random(double x_min, double k, double min, double max) {
	double x;
	double dScope;
	double y;
	//srand((unsigned)time(NULL));
	do {
		x = ntg_math_average_random(min, max, 1);
		y = ntg_math_pareto(x, x_min, k);
		dScope = ntg_math_average_random(0, 0.5, 4);
	} while (dScope > y);
	return x;
}


/**
 * 正态分布概率密度函数
 * @param[in] x 随机变量x
 * @param[in] miu 正态分布参数平均值
 * @param[in] sigma 正态分布参数方差
 * @param[in] min 最小值
 * @param[in] max 最大值
 * @return 满足正态分布的随机数
 */
double ntg_math_normal(double x,double miu,double sigma)
{
	return 1.0/sqrt(2*PI*sigma) * exp(-1*(x-miu)*(x-miu)/(2*sigma*sigma));
}

/**
 * 产生在[min, max]范围内满足正态分布的随机数
 * @param[in] miu 正态分布参数平均值
 * @param[in] sigma 正态分布参数方差
 * @param min
 * @param max
 * @return
 */
double ntg_math_normal_random(double miu,double sigma,double min,double max)//²úÉúÖ¸¶¨·Ö²¼Ëæ»úÊý
{
	double x;
	double dScope;
	double y;
	do
	{
		x = ntg_math_average_random(min,max,1);
		y = ntg_math_normal(x, miu, sigma);
		dScope =ntg_math_average_random(0,  ntg_math_normal(miu,miu,sigma),4);
	}while( dScope > y);
	return x;
}

/**
 * 对数正态概率密度函数
 * @param[in] x 随机变量x
 * @param[in] miu 对数正态分布参数
 * @param[in] sigma 对数正态分布参数
 * @return 返回x对于的密度值
 */
double ntg_math_lognormal(double x,double miu,double sigma)
{
	return 1.0/(sqrt(2*PI*sigma)*x*sigma) * exp(-1*(log(x)-miu)*(log(x)-miu)/(2*sigma*sigma));
}

/**
 * 产生在[min, max]范围内满足对数正态分布的随机数
 * @param[in] miu 对数正态分布参数
 * @param[in] sigma 对数正态分布参数
 * @param[in] min 最小值
 * @param[in] max 最大值
 * @return 满足对数正态分布的随机数
 */
double ntg_math_lognormal_random(double miu,double sigma,double min,double max)
{
	double x;
	double dScope;
	double y;
	do
	{
		x = ntg_math_average_random(min,max,1); //µÃµ½Ò»¸ö·¶Î§ÄÚµÄËæ»úÖµ
		y = ntg_math_lognormal(x, miu, sigma);//µÃµ½È¡¸ÃÖµµÄ¸ÅÂÊ
		dScope = ntg_math_average_random(0, ntg_math_lognormal(exp(miu),miu,sigma),4);
		//Normal(miu,miu,sigma)ÎªyÖá×î´óÖµ ÔÚxÖáÎª¸ÃËæ»úÖµÊ± ²úÉúÒ»¸ö0µ½yÖá×î´óÖµµÄµÄËæ»úÊý£¬Èç¹ûÔÚËæ»úÊýÐ¡ÓÚÔÚ¸ÄµãµÄ¸ÅÂÊÔòÈ¡¸Ãµã£¬ÕâÑùÔÚ¸ÅÂÊ´óµÄµãÈ¡µÄ¸ÅÂÊ´ó
	}while( dScope > y);
	return x;
}


/**
 * weibull分布概率密度函数
 * @param[in] x 随机变量
 * @param[in] scale 比例参数
 * @param[in] shape 形状参数
 * @return 返回在x点的概率密度值
 */
double ntg_math_weibull(double x,double scale,double shape)
{
	return (shape/scale)*pow(x/scale,shape-1)*exp(-pow(x/scale,shape));
}
/**
 * 生成符合weibull分布的随机数
 * @param[in] scale 比例参数
 * @param[in] shape 形状参数
 * @param[in] min 最小值
 * @param[in] max 最大值
 * @return 返回一个在[min,max]范围的符合weibull分布的随机数
 */
double ntg_math_weibull_random(double scale,double shape,double min,double max) {
	double x;
	double dScope;
	double y;
	do
	{
		x = ntg_math_average_random(min,max,1);
		y =ntg_math_weibull(x, scale, shape);
		dScope = ntg_math_average_random(0,0.5,4);
	}while( dScope > y);
	return x;

}



double ntg_math_weibull_c(double a, double b, double c){
	return a + b*pow(-log(ntg_math_average_random(0,1,4)),1/c);
}

/**
 *
 * @param name
 * @return
 */
int ntg_math_get_type(char* name)
{
	if (strcmp(name,"Pareto")==0)
	{
		return 1;
	}
	else if (strcmp(name,"Normal")==0)
	{
		return 2;
	}
	else if (strcmp(name,"LogNormal")==0)
	{
		return 3;
	}
	else if (strcmp(name,"Weibull")==0)
	{
		return 4;
	}
	else if (strcmp(name,"Uniform")==0)
	{
		return 5;
	}

	return 0;
}

/**
 * 根据分布名称获取指定范围内的随机时间
 * @param[in] name 分布名
 * @param[in] value1 分布参数1
 * @param[in] value2 分布参数2
 * @param[in] value3 分布参数3
 * @param[in] min 最小值
 * @param[in] max 最大值
 * @return 返回随机时间值
 */
int ntg_math_get_random_time(char* name,double value1,double value2,double value3,double min,double max)
{
	int type= ntg_math_get_type(name);
	switch(type)
	{
	case 1:
//		return ntg_math_pareto_random(value1,value2,value3,min,max);
		break;
	case 2:
		return ntg_math_normal_random(value1,value2,min, max);
		break;
	case 3:
		return ntg_math_lognormal_random(value1,value2,min, max);
		break;
	case 4:
		return ntg_math_weibull_random(value1,value2,min,max);
		break;
	case 5:
		return ntg_math_average_random(min,max,0); //ntg_math_weibull_random(value1,value2,value3, value4);
		break;
	}
	return -1;
}



int get_time(){
	double time;

	time = ntg_math_weibull_c(30, 1.46, 0.38);

	return ceil(time);
}



