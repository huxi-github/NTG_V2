/*
 * ntg_parse.c
 *
 *  Created on: Sep 1, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

//解析大小
ssize_t ntg_parse_size(ntg_str_t *line) {
    u_char   unit;
    size_t   len;
    ssize_t  size, scale, max;

    len = line->len;
    unit = line->data[len - 1];//获取单位

    switch (unit) {
    case 'K':
    case 'k':
        len--;
        max = NTG_MAX_SIZE_T_VALUE / 1024;
        scale = 1024;
        break;

    case 'M':
    case 'm':
        len--;
        max = NTG_MAX_SIZE_T_VALUE / (1024 * 1024);
        scale = 1024 * 1024;
        break;

    default:
        max = NTG_MAX_SIZE_T_VALUE;
        scale = 1;
    }

    size = ntg_atosz(line->data, len);//转换大小
    if (size == NTG_ERROR || size > max) {//出错处理
        return NTG_ERROR;
    }

    size *= scale;

    return size;
}

//解析偏移
off_t ntg_parse_offset(ntg_str_t *line) {
    u_char  unit;
    off_t   offset, scale, max;
    size_t  len;

    len = line->len;
    unit = line->data[len - 1];

    switch (unit) {
    case 'K':
    case 'k':
        len--;
        max = NTG_MAX_OFF_T_VALUE / 1024;
        scale = 1024;
        break;

    case 'M':
    case 'm':
        len--;
        max = NTG_MAX_OFF_T_VALUE / (1024 * 1024);
        scale = 1024 * 1024;
        break;

    case 'G':
    case 'g':
        len--;
        max = NTG_MAX_OFF_T_VALUE / (1024 * 1024 * 1024);
        scale = 1024 * 1024 * 1024;
        break;

    default:
        max = NTG_MAX_OFF_T_VALUE;
        scale = 1;
    }

    offset = ntg_atoof(line->data, len);
    if (offset == NTG_ERROR || offset > max) {
        return NTG_ERROR;
    }

    offset *= scale;

    return offset;
}

/**
 * 解析时间
 * @param[in] line 输入时间字符串
 * @param[in] is_sec 表示是否以秒为单位
 * @return 合法返回对应的时间值, 否则返回NTG_ERROR
 */
ntg_int_t ntg_parse_time(ntg_str_t *line, ntg_uint_t is_sec) {
    u_char      *p, *last;
    ntg_int_t    value, total, scale;
    ntg_int_t    max, cutoff, cutlim;
    ntg_uint_t   valid;
    enum {
        st_start = 0,
        st_year,
        st_month,
        st_week,
        st_day,
        st_hour,
        st_min,
        st_sec,
        st_msec,
        st_last
    } step;

    valid = 0;
    value = 0;
    total = 0;
    cutoff = NTG_MAX_INT_T_VALUE / 10;
    cutlim = NTG_MAX_INT_T_VALUE % 10;
    step = is_sec ? st_start : st_month;

    p = line->data;
    last = p + line->len;

    while (p < last) {

        if (*p >= '0' && *p <= '9') {
            if (value >= cutoff && (value > cutoff || *p - '0' > cutlim)) {
                return NTG_ERROR;
            }

            value = value * 10 + (*p++ - '0');
            valid = 1;
            continue;
        }

        switch (*p++) {

        case 'y':
            if (step > st_start) {
                return NTG_ERROR;
            }
            step = st_year;
            max = NTG_MAX_INT_T_VALUE / (60 * 60 * 24 * 365);
            scale = 60 * 60 * 24 * 365;//转换为秒
            break;

        case 'M':
            if (step >= st_month) {
                return NTG_ERROR;
            }
            step = st_month;
            max = NTG_MAX_INT_T_VALUE / (60 * 60 * 24 * 30);
            scale = 60 * 60 * 24 * 30;
            break;

        case 'w':
            if (step >= st_week) {
                return NTG_ERROR;
            }
            step = st_week;
            max = NTG_MAX_INT_T_VALUE / (60 * 60 * 24 * 7);
            scale = 60 * 60 * 24 * 7;
            break;

        case 'd':
            if (step >= st_day) {
                return NTG_ERROR;
            }
            step = st_day;
            max = NTG_MAX_INT_T_VALUE / (60 * 60 * 24);
            scale = 60 * 60 * 24;
            break;

        case 'h':
            if (step >= st_hour) {
                return NTG_ERROR;
            }
            step = st_hour;
            max = NTG_MAX_INT_T_VALUE / (60 * 60);
            scale = 60 * 60;
            break;

        case 'm':
            if (*p == 's') {
                if (is_sec || step >= st_msec) {
                    return NTG_ERROR;
                }
                p++;
                step = st_msec;
                max = NTG_MAX_INT_T_VALUE;
                scale = 1;
                break;
            }

            if (step >= st_min) {
                return NTG_ERROR;
            }
            step = st_min;
            max = NTG_MAX_INT_T_VALUE / 60;
            scale = 60;
            break;

        case 's':
            if (step >= st_sec) {
                return NTG_ERROR;
            }
            step = st_sec;
            max = NTG_MAX_INT_T_VALUE;
            scale = 1;
            break;

        case ' ':
            if (step >= st_sec) {
                return NTG_ERROR;
            }
            step = st_last;
            max = NTG_MAX_INT_T_VALUE;
            scale = 1;
            break;

        default:
            return NTG_ERROR;
        }

        if (step != st_msec && !is_sec) {//转为毫秒
            scale *= 1000;
            max /= 1000;
        }

        if (value > max) {
            return NTG_ERROR;
        }

        value *= scale;

        if (total > NTG_MAX_INT_T_VALUE - value) {
            return NTG_ERROR;
        }

        total += value;

        value = 0;

        while (p < last && *p == ' ') {//忽略空格
            p++;
        }
    }

    if (!valid) {//不合法格式
        return NTG_ERROR;
    }

    if (!is_sec) {
        if (value > NTG_MAX_INT_T_VALUE / 1000) {
            return NTG_ERROR;
        }

        value *= 1000;
    }

    if (total > NTG_MAX_INT_T_VALUE - value) {
        return NTG_ERROR;
    }

    return total + value;
}
