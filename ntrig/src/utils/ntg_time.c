/*
 * ntg_time.c
 *
 *  Created on: Jul 26, 2015
 *      Author: tzh
 */

#include "ntg_time.h"

/**
 * 时区更新
 */
void ntg_timezone_update(void){
	time_t s;
	struct tm *t;
	char buf[4];

	s = time(0);//获取日历时间
	t = localtime(&s);
	strftime(buf, 4, "%H", t);//strftime将触发/etc/localtime的改变
}


/**
 * 将日历时间转为日常分解时
 * @param[in] s 日历时间
 * @param[out] tm 分解时间
 */
void ntg_localtime(time_t s, ntg_tm_t *tm){
	ntg_tm_t *t;

	t = localtime(&s);
	*tm = *t;
	tm->tm_mon++;
	tm->tm_year += 1900;
}
/**
 * 将日历时间转为本地分解时
 * @param[in] s	日历时间
 * @param[out] tm 分解时间
 */
void ntg_libc_localtime(time_t s, struct tm *tm){
	struct tm *t;

	t = localtime(&s);
	*tm = *t;
}
/**
 * 将日历时间转为统一分解时
 * @param[in] s	日历时间
 * @param[out] tm 分解时间
 */
void ntg_libc_gmtime(time_t s, struct tm *tm){
	struct tm  *t;

	t = gmtime(&s);
	*tm = *t;
}
