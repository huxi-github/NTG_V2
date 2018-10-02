/*
 * ntg_times.c
 *
 *  Created on: Aug 18, 2015
 *      Author: tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_times.h"
#include "ntg_atomic.h"



/*
 * The time may be updated by signal handler or by several threads.
 * The time update operations are rare and require to hold the ntg_time_lock.
 * The time read operations are frequent, so they are lock-free and get time
 * values and strings from the current slot.  Thus thread may get the corrupted
 * values only if it is preempted while copying and then it is not scheduled
 * to run more than NTG_TIME_SLOTS seconds.
 */

#define NTG_TIME_SLOTS   64

static ntg_uint_t        slot;
static ntg_atomic_t      ntg_time_lock;

volatile ntg_msec_t      ntg_current_msec;///当前的ms值
volatile ntg_time_t     *ntg_cached_time;
volatile ntg_str_t       ntg_cached_err_log_time;
volatile ntg_str_t       ntg_cached_http_time;
volatile ntg_str_t       ntg_cached_http_log_time;
volatile ntg_str_t       ntg_cached_http_log_iso8601;
volatile ntg_str_t       ntg_cached_syslog_time;

#if !(NTG_WIN32)

/*
 * localtime() and localtime_r() are not Async-Signal-Safe functions, therefore,
 * they must not be called by a signal handler, so we use the cached
 * GMT offset value. Fortunately the value is changed only two times a year.
 */

static ntg_int_t         cached_gmtoff;
#endif

static ntg_time_t        cached_time[NTG_TIME_SLOTS];
static u_char            cached_err_log_time[NTG_TIME_SLOTS]
                                    [sizeof("1970/09/28 12:00:00")];
static u_char            cached_http_time[NTG_TIME_SLOTS]
                                    [sizeof("Mon, 28 Sep 1970 06:00:00 GMT")];
static u_char            cached_http_log_time[NTG_TIME_SLOTS]
                                    [sizeof("28/Sep/1970:12:00:00 +0600")];
static u_char            cached_http_log_iso8601[NTG_TIME_SLOTS]
                                    [sizeof("1970-09-28T12:00:00+06:00")];
static u_char            cached_syslog_time[NTG_TIME_SLOTS]
                                    [sizeof("Sep 28 12:00:00")];


static char  *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
/**
 * 时间缓存初始化
 */
void
ntg_time_init(void)
{
    ntg_cached_err_log_time.len = sizeof("1970/09/28 12:00:00") - 1;
    ntg_cached_http_time.len = sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1;
    ntg_cached_http_log_time.len = sizeof("28/Sep/1970:12:00:00 +0600") - 1;
    ntg_cached_http_log_iso8601.len = sizeof("1970-09-28T12:00:00+06:00") - 1;
    ntg_cached_syslog_time.len = sizeof("Sep 28 12:00:00") - 1;

    ntg_cached_time = &cached_time[0];

    ntg_time_update();
}

/**
 * 更新缓存时间
 * @note 更新的属性有:
 * 		ntg_current_msec
 *	    ntg_cached_time
 *	    ntg_cached_http_time
 *	    ntg_cached_err_log_time
 *	    ntg_cached_http_log_time
 *	    ntg_cached_http_log_iso8601
 *	    ntg_cached_syslog_time
 */
void
ntg_time_update(void)
{
    u_char          *p0, *p1, *p2, *p3, *p4;
    ntg_tm_t         tm, gmt;
    time_t           sec;
    ntg_uint_t       msec;
    ntg_time_t      *tp;
    struct timeval   tv;

    if (!ntg_trylock(&ntg_time_lock)) {
        return;
    }

    ntg_gettimeofday(&tv);

    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    ntg_current_msec = (ntg_msec_t) sec * 1000 + msec;

    tp = &cached_time[slot];
    //在同一1秒中内
    if (tp->sec == sec) {
        tp->msec = msec;
        ntg_unlock(&ntg_time_lock);
        return;
    }
    //下移槽位
    if (slot == NTG_TIME_SLOTS - 1) {
        slot = 0;
    } else {
        slot++;
    }
    //更新时间
    tp = &cached_time[slot];

    tp->sec = sec;
    tp->msec = msec;

    //获取统一分解时
    ntg_gmtime(sec, &gmt);

    //更新http格式时间
    p0 = &cached_http_time[slot][0];

    (void) ntg_sprintf(p0, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                       week[gmt.ntg_tm_wday], gmt.ntg_tm_mday,
                       months[gmt.ntg_tm_mon - 1], gmt.ntg_tm_year,
                       gmt.ntg_tm_hour, gmt.ntg_tm_min, gmt.ntg_tm_sec);

#if (NTG_HAVE_GETTIMEZONE)

    tp->gmtoff = ntg_gettimezone();
    ntg_gmtime(sec + tp->gmtoff * 60, &tm);

#elif (NTG_HAVE_GMTOFF)
    //获取本地分解时
    ntg_localtime(sec, &tm);
    cached_gmtoff = (ntg_int_t) (tm.ntg_tm_gmtoff / 60);
    tp->gmtoff = cached_gmtoff;

#else

    ntg_localtime(sec, &tm);
    cached_gmtoff = ntg_timezone(tm.ntg_tm_isdst);
    tp->gmtoff = cached_gmtoff;

#endif

    //更新err日志格式时间
    p1 = &cached_err_log_time[slot][0];

    (void) ntg_sprintf(p1, "%4d/%02d/%02d %02d:%02d:%02d",
                       tm.ntg_tm_year, tm.ntg_tm_mon,
                       tm.ntg_tm_mday, tm.ntg_tm_hour,
                       tm.ntg_tm_min, tm.ntg_tm_sec);

    //更新http日志格式时间
    p2 = &cached_http_log_time[slot][0];

    (void) ntg_sprintf(p2, "%02d/%s/%d:%02d:%02d:%02d %c%02d%02d",
                       tm.ntg_tm_mday, months[tm.ntg_tm_mon - 1],
                       tm.ntg_tm_year, tm.ntg_tm_hour,
                       tm.ntg_tm_min, tm.ntg_tm_sec,
                       tp->gmtoff < 0 ? '-' : '+',
                       ntg_abs(tp->gmtoff / 60), ntg_abs(tp->gmtoff % 60));
    //更新http日志iso8601格式时间
    p3 = &cached_http_log_iso8601[slot][0];

    (void) ntg_sprintf(p3, "%4d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
                       tm.ntg_tm_year, tm.ntg_tm_mon,
                       tm.ntg_tm_mday, tm.ntg_tm_hour,
                       tm.ntg_tm_min, tm.ntg_tm_sec,
                       tp->gmtoff < 0 ? '-' : '+',
                       ntg_abs(tp->gmtoff / 60), ntg_abs(tp->gmtoff % 60));
    //更新系统日志格式时间
    p4 = &cached_syslog_time[slot][0];

    (void) ntg_sprintf(p4, "%s %2d %02d:%02d:%02d",
                       months[tm.ntg_tm_mon - 1], tm.ntg_tm_mday,
                       tm.ntg_tm_hour, tm.ntg_tm_min, tm.ntg_tm_sec);

    ntg_memory_barrier();//进行同步

    ntg_cached_time = tp;
    ntg_cached_http_time.data = p0;
    ntg_cached_err_log_time.data = p1;
    ntg_cached_http_log_time.data = p2;
    ntg_cached_http_log_iso8601.data = p3;
    ntg_cached_syslog_time.data = p4;

    ntg_unlock(&ntg_time_lock);
}


#if !(NTG_WIN32)

void
ntg_time_sigsafe_update(void)
{
    u_char          *p, *p2;
    ntg_tm_t         tm;
    time_t           sec;
    ntg_time_t      *tp;
    struct timeval   tv;

    if (!ntg_trylock(&ntg_time_lock)) {
        return;
    }

    ntg_gettimeofday(&tv);

    sec = tv.tv_sec;

    tp = &cached_time[slot];

    if (tp->sec == sec) {
        ntg_unlock(&ntg_time_lock);
        return;
    }

    if (slot == NTG_TIME_SLOTS - 1) {
        slot = 0;
    } else {
        slot++;
    }

    tp = &cached_time[slot];

    tp->sec = 0;

    ntg_gmtime(sec + cached_gmtoff * 60, &tm);

    p = &cached_err_log_time[slot][0];

    (void) ntg_sprintf(p, "%4d/%02d/%02d %02d:%02d:%02d",
                       tm.ntg_tm_year, tm.ntg_tm_mon,
                       tm.ntg_tm_mday, tm.ntg_tm_hour,
                       tm.ntg_tm_min, tm.ntg_tm_sec);

    p2 = &cached_syslog_time[slot][0];

    (void) ntg_sprintf(p2, "%s %2d %02d:%02d:%02d",
                       months[tm.ntg_tm_mon - 1], tm.ntg_tm_mday,
                       tm.ntg_tm_hour, tm.ntg_tm_min, tm.ntg_tm_sec);

    ntg_memory_barrier();

    ntg_cached_err_log_time.data = p;
    ntg_cached_syslog_time.data = p2;

    ntg_unlock(&ntg_time_lock);
}

#endif


u_char *
ntg_http_time(u_char *buf, time_t t)
{
    ntg_tm_t  tm;

    ntg_gmtime(t, &tm);

    return ntg_sprintf(buf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
                       week[tm.ntg_tm_wday],
                       tm.ntg_tm_mday,
                       months[tm.ntg_tm_mon - 1],
                       tm.ntg_tm_year,
                       tm.ntg_tm_hour,
                       tm.ntg_tm_min,
                       tm.ntg_tm_sec);
}


u_char *
ntg_http_cookie_time(u_char *buf, time_t t)
{
    ntg_tm_t  tm;

    ntg_gmtime(t, &tm);

    /*
     * Netscape 3.x does not understand 4-digit years at all and
     * 2-digit years more than "37"
     */

    return ntg_sprintf(buf,
                       (tm.ntg_tm_year > 2037) ?
                                         "%s, %02d-%s-%d %02d:%02d:%02d GMT":
                                         "%s, %02d-%s-%02d %02d:%02d:%02d GMT",
                       week[tm.ntg_tm_wday],
                       tm.ntg_tm_mday,
                       months[tm.ntg_tm_mon - 1],
                       (tm.ntg_tm_year > 2037) ? tm.ntg_tm_year:
                                                 tm.ntg_tm_year % 100,
                       tm.ntg_tm_hour,
                       tm.ntg_tm_min,
                       tm.ntg_tm_sec);
}


void
ntg_gmtime(time_t t, ntg_tm_t *tp)
{
    ntg_int_t   yday;
    ntg_uint_t  n, sec, min, hour, mday, mon, year, wday, days, leap;

    /* the calculation is valid for positive time_t only */

    n = (ntg_uint_t) t;

    days = n / 86400;

    /* January 1, 1970 was Thursday */

    wday = (4 + days) % 7;

    n %= 86400;
    hour = n / 3600;
    n %= 3600;
    min = n / 60;
    sec = n % 60;

    /*
     * the algorithm based on Gauss' formula,
     * see src/http/ntg_http_parse_time.c
     */

    /* days since March 1, 1 BC */
    days = days - (31 + 28) + 719527;

    /*
     * The "days" should be adjusted to 1 only, however, some March 1st's go
     * to previous year, so we adjust them to 2.  This causes also shift of the
     * last February days to next year, but we catch the case when "yday"
     * becomes negative.
     */

    year = (days + 2) * 400 / (365 * 400 + 100 - 4 + 1);

    yday = days - (365 * year + year / 4 - year / 100 + year / 400);

    if (yday < 0) {
        leap = (year % 4 == 0) && (year % 100 || (year % 400 == 0));
        yday = 365 + leap + yday;
        year--;
    }

    /*
     * The empirical formula that maps "yday" to month.
     * There are at least 10 variants, some of them are:
     *     mon = (yday + 31) * 15 / 459
     *     mon = (yday + 31) * 17 / 520
     *     mon = (yday + 31) * 20 / 612
     */

    mon = (yday + 31) * 10 / 306;

    /* the Gauss' formula that evaluates days before the month */

    mday = yday - (367 * mon / 12 - 30) + 1;

    if (yday >= 306) {

        year++;
        mon -= 10;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday -= 306;
         */

    } else {

        mon += 2;

        /*
         * there is no "yday" in Win32 SYSTEMTIME
         *
         * yday += 31 + 28 + leap;
         */
    }

    tp->ntg_tm_sec = (ntg_tm_sec_t) sec;
    tp->ntg_tm_min = (ntg_tm_min_t) min;
    tp->ntg_tm_hour = (ntg_tm_hour_t) hour;
    tp->ntg_tm_mday = (ntg_tm_mday_t) mday;
    tp->ntg_tm_mon = (ntg_tm_mon_t) mon;
    tp->ntg_tm_year = (ntg_tm_year_t) year;
    tp->ntg_tm_wday = (ntg_tm_wday_t) wday;
}


time_t
ntg_next_time(time_t when)
{
    time_t     now, next;
    struct tm  tm;

    now = ntg_time();

    ntg_libc_localtime(now, &tm);

    tm.tm_hour = (int) (when / 3600);
    when %= 3600;
    tm.tm_min = (int) (when / 60);
    tm.tm_sec = (int) (when % 60);

    next = mktime(&tm);

    if (next == -1) {
        return -1;
    }

    if (next - now > 0) {
        return next;
    }

    tm.tm_mday++;

    /* mktime() should normalize a date (Jan 32, etc) */

    next = mktime(&tm);

    if (next != -1) {
        return next;
    }

    return -1;
}

