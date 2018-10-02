/*
 * ntg_times.h
 *
 *  Created on: Aug 18, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_TIMES_H_
#define CORE_NTG_TIMES_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

typedef struct {
    time_t      sec;
    ntg_uint_t  msec;
    ntg_int_t   gmtoff;///< 精度为分
} ntg_time_t;

void ntg_time_init(void);

void ntg_time_update(void);

void ntg_time_sigsafe_update(void);

u_char *ntg_http_time(u_char *buf, time_t t);
u_char *ntg_http_cookie_time(u_char *buf, time_t t);

void ntg_gmtime(time_t t, ntg_tm_t *tp);

time_t ntg_next_time(time_t when);
#define ntg_next_time_n      "mktime()"


extern volatile ntg_time_t  *ntg_cached_time;

#define ntg_time()           ntg_cached_time->sec
#define ntg_timeofday()      (ntg_time_t *) ntg_cached_time

extern volatile ntg_str_t    ntg_cached_err_log_time;
extern volatile ntg_str_t    ntg_cached_http_time;
extern volatile ntg_str_t    ntg_cached_http_log_time;
extern volatile ntg_str_t    ntg_cached_http_log_iso8601;
extern volatile ntg_str_t    ntg_cached_syslog_time;
/*
 * milliseconds elapsed since epoch and truncated to ntg_msec_t,
 * used in event timers
 */
extern volatile ntg_msec_t  ntg_current_msec;

#endif /* CORE_NTG_TIMES_H_ */
