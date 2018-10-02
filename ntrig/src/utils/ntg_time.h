/*
 * ntg_time.h
 *
 *  Created on: Jul 26, 2015
 *      Author: tzh
 */

#ifndef OS_NTG_TIME_H_
#define OS_NTG_TIME_H_

#include "../ntg_config.h"
//#include "../ntg_core.h"

#include "ntg_rbtree.h"


typedef ntg_rbtree_key_t      ntg_msec_t;
typedef ntg_rbtree_key_int_t  ntg_msec_int_t;

typedef struct tm             ntg_tm_t;

#define ntg_tm_sec            tm_sec
#define ntg_tm_min            tm_min
#define ntg_tm_hour           tm_hour
#define ntg_tm_mday           tm_mday
#define ntg_tm_mon            tm_mon
#define ntg_tm_year           tm_year
#define ntg_tm_wday           tm_wday
#define ntg_tm_isdst          tm_isdst

#define ntg_tm_sec_t          int
#define ntg_tm_min_t          int
#define ntg_tm_hour_t         int
#define ntg_tm_mday_t         int
#define ntg_tm_mon_t          int
#define ntg_tm_year_t         int
#define ntg_tm_wday_t         int

#if (NTG_HAVE_GMTOFF)
#define ntg_tm_gmtoff         tm_gmtoff
#define ntg_tm_zone           tm_zone
#endif

#define ngx_timezone(isdst) (- (isdst ? timezone + 3600 : timezone) / 60)

void ntg_timezone_update(void);
void ntg_localtime(time_t s, ntg_tm_t *tm);
void ntg_libc_localtime(time_t s, struct tm *tm);
void ntg_libc_gmtime(time_t s, struct tm *tm);

#define ntg_gettimeofday(tp)  (void) gettimeofday(tp, NULL);
#define ntg_msleep(ms)        (void) usleep(ms * 1000)
#define ntg_sleep(s)          (void) sleep(s)

#endif /* OS_NTG_TIME_H_ */
