/*
 * ntg_string.c
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_time.h"


static u_char *ntg_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,
    u_char zero, ntg_uint_t hexadecimal, ntg_uint_t width);
static ntg_int_t ntg_decode_base64_internal(ntg_str_t *dst, ntg_str_t *src,
    const u_char *basis);

/**
 * 以小写方式将源字符串src拷贝到目标字符串dst
 * @param[in] dst 目标字符串
 * @param[in] src 源字符串
 * @param n 源字符串的长度
 */
void ntg_strlow(u_char *dst, u_char *src, size_t n){
    while (n) {
        *dst = ntg_tolower(*src);
        dst++;
        src++;
        n--;

    }
}

/**
 * 将str的n字符拷贝到dst中返回dst中下一个存储位置,即'\0'位置
 * @param dst 目标字符串
 * @param src 源字符串
 * @param n 最大的拷贝字符数
 * @return 返回下一个存储位置的指针
 * @note
 * 	返回的字符串必定以'\0'结束.
 * 	n是包含字符串结束符'\0'的,如果要将src的整个字符串拷贝到dst中
 * 	n = strlen(src) + 1.
 */
u_char * ntg_cpystrn(u_char *dst, u_char *src, size_t n){
    if (n == 0) {
        return dst;
    }
    while (--n) {
        *dst = *src;
        if (*dst == '\0') {
            return dst;
        }
        dst++;
        src++;
    }
    *dst = '\0';
    return dst;
}


/**
 * 将字符串拷贝到指定的内存池中
 * @param[in] pool 内存池指针
 * @param[in] src 字符串
 * @return 成功返回字符串在内存池pool中的首地址,否则返回NULL
 * @note p表示带pool参数,且仅仅拷贝数据不带字符串结束符
 */
u_char *ntg_pstrdup(ntg_pool_t *pool, ntg_str_t *src){
    u_char  *dst;

    dst = ntg_pnalloc(pool, src->len);
    if (dst == NULL) {
        return NULL;
    }
    ntg_memcpy(dst, src->data, src->len);

    return dst;
}


/*
 * supported formats:
 *    %[0][width][x][X]O        off_t
 *    %[0][width]T              time_t
 *    %[0][width][u][x|X]z      ssize_t/size_t
 *    %[0][width][u][x|X]d      int/u_int
 *    %[0][width][u][x|X]l      long
 *    %[0][width|m][u][x|X]i    ntg_int_t/ntg_uint_t
 *    %[0][width][u][x|X]D      int32_t/uint32_t
 *    %[0][width][u][x|X]L      int64_t/uint64_t
 *    %[0][width|m][u][x|X]A    ntg_atomic_int_t/ntg_atomic_uint_t
 *    %[0][width][.width]f      double, max valid number fits to %18.15f
 *    %P                        ntg_pid_t
 *    %M                        ntg_msec_t
 *    %r                        rlim_t
 *    %p                        void *
 *    %V                        ntg_str_t *
 *    %v                        ntg_variable_value_t *
 *    %s                        null-terminated string
 *    %*s                       length and string
 *    %Z                        '\0'
 *    %N                        '\n'
 *    %c                        char
 *    %%                        %
 *
 *  reserved:
 *    %t                        ptrdiff_t
 *    %S                        null-terminated wchar string
 *    %C                        wchar
 */


u_char * ntg_sprintf(u_char *buf, const char *fmt, ...){
    u_char   *p;
    va_list   args;

    va_start(args, fmt);
    p = ntg_vslprintf(buf, (void *) -1, fmt, args);
    va_end(args);

    return p;
}

/**
 * 带大小的格式化输出到buf中
 * @param[in] buf	输出缓冲区
 * @param[in] max	缓冲区最大大小
 * @param[in] fmt	fmt格式化字符串
 * @return 输出字符串的结尾的下一个指针
 * @note
 * s表示以字符串输出到buf中
 * n表示带大小参数
 */
u_char *ntg_snprintf(u_char *buf, size_t max, const char *fmt, ...){
    u_char   *p;
    va_list   args;

    va_start(args, fmt);
    p = ntg_vslprintf(buf, buf + max, fmt, args);
    va_end(args);

    return p;
}


/**
 * 带last的格式化输出到buf中
 * @param[in] buf	输出缓冲区
 * @param[in] last 	溢出标志
 * @param[in] fmt	fmt格式化字符串
 * @return 输出字符串的结尾的下一个指针
 * @note
 * s表示以字符串输出到buf中
 * n表示带大小参数
 */
u_char * ntg_slprintf(u_char *buf, u_char *last, const char *fmt, ...){
    u_char   *p;
    va_list   args;

    va_start(args, fmt);
    p = ntg_vslprintf(buf, last, fmt, args);
    va_end(args);

    return p;
}

/**
 * 格式化输出到buf中
 * @param[in] buf 保存输出的字符串
 * @param[in] last 溢出标志
 * @param[in] fmt 格式化字符串
 * @param[in] args 输出参数列表
 * @return 输出字符串的结尾的下一个指针
 * @note
 * v表示带va_list参数
 * s表示以字符串输出到buf中
 * l表示带last参数
 */
u_char *ntg_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args) {
    u_char                *p, zero;
    int                    d;
    double                 f, scale;
    size_t                 len, slen;
    int64_t                i64;
    uint64_t               ui64;
    ntg_msec_t             ms;
    ntg_uint_t             width, sign, hex, max_width, frac_width, n;
    ntg_str_t             *v;
    ntg_variable_value_t  *vv;

    while (*fmt && buf < last) {

        /*
         * "buf < last" means that we could copy at least one character:
         * the plain character, "%%", "%c", and minus without the checking
         */

        if (*fmt == '%') {

            i64 = 0;
            ui64 = 0;

            zero = (u_char) ((*++fmt == '0') ? '0' : ' ');//前导0

            width = 0;//宽度
            sign = 1;
            hex = 0;//大写还是小写16进制
            max_width = 0;
            frac_width = 0;//小数点宽度
            slen = (size_t) -1;

            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt++ - '0';
            }


            for ( ;; ) {
                switch (*fmt) {

                case 'u':
                    sign = 0;
                    fmt++;
                    continue;

                case 'm':
                    max_width = 1;
                    fmt++;
                    continue;

                case 'X':
                    hex = 2;
                    sign = 0;
                    fmt++;
                    continue;

                case 'x':
                    hex = 1;
                    sign = 0;
                    fmt++;
                    continue;

                case '.':
                    fmt++;

                    while (*fmt >= '0' && *fmt <= '9') {
                        frac_width = frac_width * 10 + *fmt++ - '0';
                    }

                    break;

                case '*':
                    slen = va_arg(args, size_t);
                    fmt++;
                    continue;

                default:
                    break;
                }

                break;
            }


            switch (*fmt) {

            case 'V':
                v = va_arg(args, ntg_str_t *);

                len = ntg_min(((size_t) (last - buf)), v->len);
                buf = ntg_cpymem(buf, v->data, len);
                fmt++;

                continue;

            case 'v':
                vv = va_arg(args, ntg_variable_value_t *);

                len = ntg_min(((size_t) (last - buf)), vv->len);
                buf = ntg_cpymem(buf, vv->data, len);
                fmt++;

                continue;

            case 's':
                p = va_arg(args, u_char *);

                if (slen == (size_t) -1) {
                    while (*p && buf < last) {
                        *buf++ = *p++;
                    }

                } else {
                    len = ntg_min(((size_t) (last - buf)), slen);
                    buf = ntg_cpymem(buf, p, len);
                }

                fmt++;

                continue;

            case 'O':
                i64 = (int64_t) va_arg(args, off_t);
                sign = 1;
                break;

            case 'P':
            	//TODO
                i64 = (int64_t) va_arg(args, ntg_pid_t);
                sign = 1;
                break;

            case 'T':
                i64 = (int64_t) va_arg(args, time_t);
                sign = 1;
                break;

            case 'M':
                ms = (ntg_msec_t) va_arg(args, ntg_msec_t);
                if ((ntg_msec_int_t) ms == -1) {
                    sign = 1;
                    i64 = -1;
                } else {
                    sign = 0;
                    ui64 = (uint64_t) ms;
                }
                break;

            case 'z':
                if (sign) {
                    i64 = (int64_t) va_arg(args, ssize_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, size_t);
                }
                break;

            case 'i':
                if (sign) {
                    i64 = (int64_t) va_arg(args, ntg_int_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, ntg_uint_t);
                }

                if (max_width) {
                    width = NTG_INT_T_LEN;
                }

                break;

            case 'd':
                if (sign) {
                    i64 = (int64_t) va_arg(args, int);
                } else {
                    ui64 = (uint64_t) va_arg(args, u_int);
                }
                break;

            case 'l':
                if (sign) {
                    i64 = (int64_t) va_arg(args, long);
                } else {
                    ui64 = (uint64_t) va_arg(args, u_long);
                }
                break;

            case 'D':
                if (sign) {
                    i64 = (int64_t) va_arg(args, int32_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, uint32_t);
                }
                break;

            case 'L':
                if (sign) {
                    i64 = va_arg(args, int64_t);
                } else {
                    ui64 = va_arg(args, uint64_t);
                }
                break;

            case 'A'://原子变量
            	//TODO
//                if (sign) {
//                    i64 = (int64_t) va_arg(args, ntg_atomic_int_t);
//                } else {
//                    ui64 = (uint64_t) va_arg(args, ntg_atomic_uint_t);
//                }
//
//                if (max_width) {
//                    width = NTG_ATOMIC_T_LEN;
//                }
//
                break;

            case 'f':
                f = va_arg(args, double);

                if (f < 0) {
                    *buf++ = '-';
                    f = -f;
                }

                ui64 = (int64_t) f;

                buf = ntg_sprintf_num(buf, last, ui64, zero, 0, width);

                if (frac_width) {

                    if (buf < last) {
                        *buf++ = '.';
                    }

                    scale = 1.0;

                    for (n = frac_width; n; n--) {
                        scale *= 10.0;
                    }

                    /*
                     * (int64_t) cast is required for msvc6:
                     * it cannot convert uint64_t to double
                     */
                    ui64 = (uint64_t) ((f - (int64_t) ui64) * scale + 0.5);

                    buf = ntg_sprintf_num(buf, last, ui64, '0', 0, frac_width);
                }

                fmt++;

                continue;

            case 'r':
                i64 = (int64_t) va_arg(args, rlim_t);
                sign = 1;
                break;

            case 'p':
            	//TODO
//                ui64 = (uintptr_t) va_arg(args, void *);
//                hex = 2;
//                sign = 0;
//                zero = '0';
//                width = NTG_PTR_SIZE * 2;
                break;

            case 'c':
                d = va_arg(args, int);
                *buf++ = (u_char) (d & 0xff);
                fmt++;

                continue;

            case 'Z':
                *buf++ = '\0';
                fmt++;

                continue;

            case 'N':
                *buf++ = LF;
                fmt++;

                continue;

            case '%':
                *buf++ = '%';
                fmt++;

                continue;

            default:
                *buf++ = *fmt++;

                continue;
            }

            if (sign) {
                if (i64 < 0) {
                    *buf++ = '-';
                    ui64 = (uint64_t) -i64;

                } else {
                    ui64 = (uint64_t) i64;
                }
            }

            buf = ntg_sprintf_num(buf, last, ui64, zero, hex, width);

            fmt++;

        } else {
            *buf++ = *fmt++;
        }
    }

    return buf;
}


static u_char *
ntg_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero,ntg_uint_t hexadecimal, ntg_uint_t width){

    u_char         *p, temp[NTG_INT64_LEN + 1];
                       /*
                        * we need temp[NTG_INT64_LEN] only,
                        * but icc issues the warning
                        */
    size_t          len;
    uint32_t        ui32;
    static u_char   hex[] = "0123456789abcdef";
    static u_char   HEX[] = "0123456789ABCDEF";

    p = temp + NTG_INT64_LEN;

    if (hexadecimal == 0) {

        if (ui64 <= NTG_MAX_UINT32_VALUE) {

            /*
             * To divide 64-bit numbers and to find remainders
             * on the x86 platform gcc and icc call the libc functions
             * [u]divdi3() and [u]moddi3(), they call another function
             * in its turn.  On FreeBSD it is the qdivrem() function,
             * its source code is about 170 lines of the code.
             * The glibc counterpart is about 150 lines of the code.
             *
             * For 32-bit numbers and some divisors gcc and icc use
             * a inlined multiplication and shifts.  For example,
             * unsigned "i32 / 10" is compiled to
             *
             *     (i32 * 0xCCCCCCCD) >> 35
             */

            ui32 = (uint32_t) ui64;

            do {
                *--p = (u_char) (ui32 % 10 + '0');
            } while (ui32 /= 10);

        } else {
            do {
                *--p = (u_char) (ui64 % 10 + '0');
            } while (ui64 /= 10);
        }

    } else if (hexadecimal == 1) {

        do {

            /* the "(uint32_t)" cast disables the BCC's warning */
            *--p = hex[(uint32_t) (ui64 & 0xf)];

        } while (ui64 >>= 4);

    } else { /* hexadecimal == 2 */

        do {

            /* the "(uint32_t)" cast disables the BCC's warning */
            *--p = HEX[(uint32_t) (ui64 & 0xf)];

        } while (ui64 >>= 4);
    }

    /* zero or space padding */

    len = (temp + NTG_INT64_LEN) - p;

    while (len++ < width && buf < last) {
        *buf++ = zero;
    }

    /* number safe copy */

    len = (temp + NTG_INT64_LEN) - p;

    if (buf + len > last) {
        len = last - buf;
    }

    return ntg_cpymem(buf, p, len);
}


/*
 * We use ntg_strcasecmp()/ntg_strncasecmp() for 7-bit ASCII strings only,
 * and implement our own ntg_strcasecmp()/ntg_strncasecmp()
 * to avoid libc locale overhead.  Besides, we use the ntg_uint_t's
 * instead of the u_char's, because they are slightly faster.
 */

/*
 * 不去分大小的字符串比较.
 * 注:s1的长度 >= s2的长度
 *
 */
ntg_int_t ntg_strcasecmp(u_char *s1, u_char *s2){
    ntg_uint_t  c1, c2;

    for ( ;; ) {
        c1 = (ntg_uint_t) *s1++;
        c2 = (ntg_uint_t) *s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {
            if (c1) {//c1不为'\0'
                continue;
            }
            return 0;
        }
        return c1 - c2;
    }
}

/*
 * 不区分大小写的最多n个字符的比较
 * 注:s1的长度 >= s2的长度
 */
ntg_int_t ntg_strncasecmp(u_char *s1, u_char *s2, size_t n) {
    ntg_uint_t  c1, c2;

    while (n) {
        c1 = (ntg_uint_t) *s1++;
        c2 = (ntg_uint_t) *s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {
            if (c1) {
                n--;
                continue;
            }
            return 0;
        }
        return c1 - c2;
    }
    return 0;
}

//注 s2 不能为空, s2的长度对len
u_char * ntg_strnstr(u_char *s1, char *s2, size_t len){
    u_char  c1, c2;
    size_t  n;

    c2 = *(u_char *) s2++;

    n = ntg_strlen(s2);

    do {
        do {
            if (len-- == 0) {
                return NULL;
            }

            c1 = *s1++;

            if (c1 == 0) {
                return NULL;
            }

        } while (c1 != c2);

        if (n > len) {
            return NULL;
        }

    } while (ntg_strncmp(s1, (u_char *) s2, n) != 0);

    return --s1;
}


/*
 * ntg_strstrn() and ntg_strcasestrn() are intended to search for static
 * substring with known length in null-terminated string. The argument n
 * must be length of the second substring - 1.
 */
//n 表示以s2开头,长为n的子串
u_char *ntg_strstrn(u_char *s1, char *s2, size_t n){
    u_char  c1, c2;

    c2 = *(u_char *) s2++;

    do {
        do {
            c1 = *s1++;

            if (c1 == 0) {
            	//c1<c2
                return NULL;
            }
        } while (c1 != c2);
        //c1 == c2

    } while (ntg_strncmp(s1, (u_char *) s2, n) != 0);

    return --s1;
}


u_char *ntg_strcasestrn(u_char *s1, char *s2, size_t n){
    ntg_uint_t  c1, c2;

    c2 = (ntg_uint_t) *s2++;
    c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

    do {
        do {
            c1 = (ntg_uint_t) *s1++;

            if (c1 == 0) {
                return NULL;
            }

            c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;

        } while (c1 != c2);

    } while (ntg_strncasecmp(s1, (u_char *) s2, n) != 0);

    return --s1;
}


/*
 * ntg_strlcasestrn() is intended to search for static substring
 * with known length in string until the argument last. The argument n
 * must be length of the second substring - 1.
 */

u_char *ntg_strlcasestrn(u_char *s1, u_char *last, u_char *s2, size_t n){
    ntg_uint_t  c1, c2;

    c2 = (ntg_uint_t) *s2++;
    c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
    last -= n;

    do {
        do {
            if (s1 >= last) {
                return NULL;
            }

            c1 = (ntg_uint_t) *s1++;
            c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;

        } while (c1 != c2);

    } while (ntg_strncasecmp(s1, s2, n) != 0);

    return --s1;
}


ntg_int_t ntg_rstrncmp(u_char *s1, u_char *s2, size_t n){
    if (n == 0) {
        return 0;
    }

    n--;

    for ( ;; ) {
        if (s1[n] != s2[n]) {
            return s1[n] - s2[n];
        }

        if (n == 0) {
            return 0;
        }

        n--;
    }
    //不可达
    return 0;
}


ntg_int_t ntg_rstrncasecmp(u_char *s1, u_char *s2, size_t n){
    u_char  c1, c2;

    if (n == 0) {
        return 0;
    }

    n--;

    for ( ;; ) {
        c1 = s1[n];
        if (c1 >= 'a' && c1 <= 'z') {
            c1 -= 'a' - 'A';
        }

        c2 = s2[n];
        if (c2 >= 'a' && c2 <= 'z') {
            c2 -= 'a' - 'A';
        }

        if (c1 != c2) {
            return c1 - c2;
        }

        if (n == 0) {
            return 0;
        }

        n--;
    }
    return 0;
}


ntg_int_t ntg_memn2cmp(u_char *s1, u_char *s2, size_t n1, size_t n2){
    size_t     n;
    ntg_int_t  m, z;

    if (n1 <= n2) {
        n = n1;
        z = -1;

    } else {
        n = n2;
        z = 1;
    }

    m = ntg_memcmp(s1, s2, n);

    if (m || n1 == n2) {
        return m;
    }

    return z;
}


ntg_int_t ntg_dns_strcmp(u_char *s1, u_char *s2){
    ntg_uint_t  c1, c2;

    for ( ;; ) {
        c1 = (ntg_uint_t) *s1++;
        c2 = (ntg_uint_t) *s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {

            if (c1) {
                continue;
            }

            return 0;
        }

        /* in ASCII '.' > '-', but we need '.' to be the lowest character */

        c1 = (c1 == '.') ? ' ' : c1;
        c2 = (c2 == '.') ? ' ' : c2;

        return c1 - c2;
    }
}

ntg_int_t
ntg_filename_cmp(u_char *s1, u_char *s2, size_t n)
{
    ntg_uint_t  c1, c2;

    while (n) {
        c1 = (ntg_uint_t) *s1++;
        c2 = (ntg_uint_t) *s2++;

#if (NGX_HAVE_CASELESS_FILESYSTEM)
        c1 = tolower(c1);
        c2 = tolower(c2);
#endif

        if (c1 == c2) {

            if (c1) {
                n--;
                continue;
            }

            return 0;
        }

        /* we need '/' to be the lowest character */

        if (c1 == 0 || c2 == 0) {
            return c1 - c2;
        }

        c1 = (c1 == '/') ? 0 : c1;
        c2 = (c2 == '/') ? 0 : c2;

        return c1 - c2;
    }

    return 0;
}


/**
 * 将字符数串转为整型
 * @param[in] line 输入的数字串
 * @param[in] n	长度
 * @return 成功返回转后的数,否则返回NTG_ERROR
 * @note 只能转化非负整数, 如发生溢出直接返回NTG_ERROR
 */
ntg_int_t ntg_atoi(u_char *line, size_t n){
    ntg_int_t  value;

    if (n == 0) {
        return NTG_ERROR;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NTG_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {//发生溢出
        return NTG_ERROR;

    } else {
        return value;
    }
}


/* parse a fixed point number, e.g., ntg_atofp("10.5", 4, 2) returns 1050 */

ntg_int_t ntg_atofp(u_char *line, size_t n, size_t point){
    ntg_int_t   value;
    ntg_uint_t  dot;

    if (n == 0) {
        return NTG_ERROR;
    }

    dot = 0;

    for (value = 0; n--; line++) {

        if (point == 0) {
            return NTG_ERROR;
        }

        if (*line == '.') {
            if (dot) {
                return NTG_ERROR;
            }

            dot = 1;
            continue;
        }

        if (*line < '0' || *line > '9') {
            return NTG_ERROR;
        }

        value = value * 10 + (*line - '0');
        point -= dot;
    }

    while (point--) {
        value = value * 10;
    }

    if (value < 0) {
        return NTG_ERROR;

    } else {
        return value;
    }
}


ssize_t ntg_atosz(u_char *line, size_t n) {
    ssize_t  value;

    if (n == 0) {
        return NTG_ERROR;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NTG_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {
        return NTG_ERROR;

    } else {
        return value;
    }
}


off_t ntg_atoof(u_char *line, size_t n) {
    off_t  value;

    if (n == 0) {
        return NTG_ERROR;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NTG_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {
        return NTG_ERROR;

    } else {
        return value;
    }
}


time_t ntg_atotm(u_char *line, size_t n) {
    time_t  value;

    if (n == 0) {
        return NTG_ERROR;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return NTG_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {
        return NTG_ERROR;

    } else {
        return value;
    }
}


ntg_int_t ntg_hextoi(u_char *line, size_t n){
    u_char     c, ch;
    ntg_int_t  value;

    if (n == 0) {
        return NTG_ERROR;
    }

    for (value = 0; n--; line++) {
        ch = *line;

        if (ch >= '0' && ch <= '9') {
            value = value * 16 + (ch - '0');
            continue;
        }

        c = (u_char) (ch | 0x20);//转为小写

        if (c >= 'a' && c <= 'f') {
            value = value * 16 + (c - 'a' + 10);
            continue;
        }

        return NTG_ERROR;
    }

    if (value < 0) {
        return NTG_ERROR;

    } else {
        return value;
    }
}


u_char * ntg_hex_dump(u_char *dst, u_char *src, size_t len){
    static u_char  hex[] = "0123456789abcdef";

    while (len--) {
        *dst++ = hex[*src >> 4];
        *dst++ = hex[*src++ & 0xf];
    }

    return dst;
}


void ntg_encode_base64(ntg_str_t *dst, ntg_str_t *src){
    u_char         *d, *s;
    size_t          len;
    static u_char   basis64[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    len = src->len;
    s = src->data;
    d = dst->data;

    while (len > 2) {
        *d++ = basis64[(s[0] >> 2) & 0x3f];
        *d++ = basis64[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis64[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis64[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len) {
        *d++ = basis64[(s[0] >> 2) & 0x3f];

        if (len == 1) {
            *d++ = basis64[(s[0] & 3) << 4];
            *d++ = '=';

        } else {
            *d++ = basis64[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis64[(s[1] & 0x0f) << 2];
        }

        *d++ = '=';
    }

    dst->len = d - dst->data;
}


ntg_int_t ntg_decode_base64(ntg_str_t *dst, ntg_str_t *src){
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return ntg_decode_base64_internal(dst, src, basis64);
}

ntg_int_t ntg_decode_base64url(ntg_str_t *dst, ntg_str_t *src){
    static u_char   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 63,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return ntg_decode_base64_internal(dst, src, basis64);
}


static ntg_int_t ntg_decode_base64_internal(ntg_str_t *dst, ntg_str_t *src, const u_char *basis){
    size_t          len;
    u_char         *d, *s;

    for (len = 0; len < src->len; len++) {
        if (src->data[len] == '=') {
            break;
        }

        if (basis[src->data[len]] == 77) {
            return NTG_ERROR;
        }
    }

    if (len % 4 == 1) {
        return NTG_ERROR;
    }

    s = src->data;
    d = dst->data;

    while (len > 3) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
        *d++ = (u_char) (basis[s[2]] << 6 | basis[s[3]]);

        s += 4;
        len -= 4;
    }

    if (len > 1) {
        *d++ = (u_char) (basis[s[0]] << 2 | basis[s[1]] >> 4);
    }

    if (len > 2) {
        *d++ = (u_char) (basis[s[1]] << 4 | basis[s[2]] >> 2);
    }

    dst->len = d - dst->data;

    return NTG_OK;
}


/*
 * ntg_utf8_decode() decodes two and more bytes UTF sequences only
 * the return values:
 *    0x80 - 0x10ffff         valid character
 *    0x110000 - 0xfffffffd   invalid sequence
 *    0xfffffffe              incomplete sequence
 *    0xffffffff              error
 */

uint32_t ntg_utf8_decode(u_char **p, size_t n){
    size_t    len;
    uint32_t  u, i, valid;

    u = **p;

    if (u >= 0xf0) {

        u &= 0x07;
        valid = 0xffff;
        len = 3;

    } else if (u >= 0xe0) {

        u &= 0x0f;
        valid = 0x7ff;
        len = 2;

    } else if (u >= 0xc2) {

        u &= 0x1f;
        valid = 0x7f;
        len = 1;

    } else {
        (*p)++;
        return 0xffffffff;
    }

    if (n - 1 < len) {
        return 0xfffffffe;
    }

    (*p)++;

    while (len) {
        i = *(*p)++;

        if (i < 0x80) {
            return 0xffffffff;
        }

        u = (u << 6) | (i & 0x3f);

        len--;
    }

    if (u > valid) {
        return u;
    }

    return 0xffffffff;
}


size_t ntg_utf8_length(u_char *p, size_t n){
    u_char  c, *last;
    size_t  len;

    last = p + n;

    for (len = 0; p < last; len++) {

        c = *p;

        if (c < 0x80) {
            p++;
            continue;
        }

        if (ntg_utf8_decode(&p, n) > 0x10ffff) {
            /* invalid UTF-8 */
            return n;
        }
    }

    return len;
}


u_char * ntg_utf8_cpystrn(u_char *dst, u_char *src, size_t n, size_t len){
    u_char  c, *next;

    if (n == 0) {
        return dst;
    }

    while (--n) {

        c = *src;
        *dst = c;

        if (c < 0x80) {

            if (c != '\0') {
                dst++;
                src++;
                len--;

                continue;
            }

            return dst;
        }

        next = src;

        if (ntg_utf8_decode(&next, len) > 0x10ffff) {
            /* invalid UTF-8 */
            break;
        }

        while (src < next) {
            *dst++ = *src++;
            len--;
        }
    }

    *dst = '\0';

    return dst;
}


uintptr_t ntg_escape_uri(u_char *dst, u_char *src, size_t size, ntg_uint_t type){
    ntg_uint_t      n;
    uint32_t       *escape;
    static u_char   hex[] = "0123456789abcdef";

                    /* " ", "#", "%", "?", %00-%1F, %7F-%FF */

    static uint32_t   uri[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x80000029, /* 1000 0000 0000 0000  0000 0000 0010 1001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", "#", "%", "&", "+", "?", %00-%1F, %7F-%FF */
    static uint32_t   args[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x88000869, /* 1000 1000 0000 0000  0000 1000 0110 1001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* not ALPHA, DIGIT, "-", ".", "_", "~" */

    static uint32_t   uri_component[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0xfc009fff, /* 1111 1100 0000 0000  1001 1111 1111 1111 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x78000001, /* 0111 1000 0000 0000  0000 0000 0000 0001 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0xb8000001, /* 1011 1000 0000 0000  0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", "#", """, "%", "'", %00-%1F, %7F-%FF */

    static uint32_t   html[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x000000ad, /* 0000 0000 0000 0000  0000 0000 1010 1101 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", """, "%", "'", %00-%1F, %7F-%FF */

    static uint32_t   refresh[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000085, /* 0000 0000 0000 0000  0000 0000 1000 0101 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x80000000, /* 1000 0000 0000 0000  0000 0000 0000 0000 */

        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
        0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    };

                    /* " ", "%", %00-%1F *///2^8个
    static uint32_t   memcached[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000021, /* 0000 0000 0000 0000  0000 0000 0010 0001 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };

                    /* mail_auth is the same as memcached */

    static uint32_t  *map[] =
        { uri, args, uri_component, html, refresh, memcached, memcached };

    escape = map[type];

    if (dst == NULL) {

        /* find the number of the characters to be escaped */

        n = 0;

        while (size) {
            if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
                n++;
            }
            src++;
            size--;
        }

        return (uintptr_t) n;//放回长度
    }

    while (size) {
        if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
        	//对需要转换的字符进行转换
            *dst++ = '%';
            *dst++ = hex[*src >> 4];
            *dst++ = hex[*src & 0xf];
            src++;

        } else {
            *dst++ = *src++;
        }
        size--;
    }

    return (uintptr_t) dst;
}


void ntg_unescape_uri(u_char **dst, u_char **src, size_t size, ntg_uint_t type){
    u_char  *d, *s, ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    d = *dst;
    s = *src;

    state = 0;
    decoded = 0;

    while (size--) {

        ch = *s++;

        switch (state) {
        case sw_usual:
            if (ch == '?'
                && (type & (NTG_UNESCAPE_URI|NTG_UNESCAPE_REDIRECT)))
            {
                *d++ = ch;
                goto done;
            }

            if (ch == '%') {
                state = sw_quoted;
                break;
            }

            *d++ = ch;
            break;

        case sw_quoted:

            if (ch >= '0' && ch <= '9') {
                decoded = (u_char) (ch - '0');
                state = sw_quoted_second;
                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                decoded = (u_char) (c - 'a' + 10);
                state = sw_quoted_second;
                break;
            }

            /* the invalid quoted character */

            state = sw_usual;

            *d++ = ch;

            break;

        case sw_quoted_second:

            state = sw_usual;

            if (ch >= '0' && ch <= '9') {
                ch = (u_char) ((decoded << 4) + ch - '0');

                if (type & NTG_UNESCAPE_REDIRECT) {
                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);

                    break;
                }

                *d++ = ch;

                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (u_char) ((decoded << 4) + c - 'a' + 10);

                if (type & NTG_UNESCAPE_URI) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    *d++ = ch;
                    break;
                }

                if (type & NTG_UNESCAPE_REDIRECT) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
                    break;
                }

                *d++ = ch;

                break;
            }

            /* the invalid quoted character */

            break;
        }
    }

done:

    *dst = d;
    *src = s;
}


uintptr_t
ntg_escape_html(u_char *dst, u_char *src, size_t size)
{
    u_char      ch;
    ntg_uint_t  len;

    if (dst == NULL) {

        len = 0;

        while (size) {
            switch (*src++) {

            case '<':
                len += sizeof("&lt;") - 2;
                break;

            case '>':
                len += sizeof("&gt;") - 2;
                break;

            case '&':
                len += sizeof("&amp;") - 2;
                break;

            case '"':
                len += sizeof("&quot;") - 2;
                break;

            default:
                break;
            }
            size--;
        }

        return (uintptr_t) len;
    }

    while (size) {
        ch = *src++;

        switch (ch) {

        case '<':
            *dst++ = '&'; *dst++ = 'l'; *dst++ = 't'; *dst++ = ';';
            break;

        case '>':
            *dst++ = '&'; *dst++ = 'g'; *dst++ = 't'; *dst++ = ';';
            break;

        case '&':
            *dst++ = '&'; *dst++ = 'a'; *dst++ = 'm'; *dst++ = 'p';
            *dst++ = ';';
            break;

        case '"':
            *dst++ = '&'; *dst++ = 'q'; *dst++ = 'u'; *dst++ = 'o';
            *dst++ = 't'; *dst++ = ';';
            break;

        default:
            *dst++ = ch;
            break;
        }
        size--;
    }

    return (uintptr_t) dst;
}


//void
//ntg_str_rbtree_insert_value(ntg_rbtree_node_t *temp,
//    ntg_rbtree_node_t *node, ntg_rbtree_node_t *sentinel)
//{
//    ntg_str_node_t      *n, *t;
//    ntg_rbtree_node_t  **p;
//
//    for ( ;; ) {
//
//        n = (ntg_str_node_t *) node;
//        t = (ntg_str_node_t *) temp;
//
//        // 首先比较key关键字，红黑树中以key作为第一索引关键字
//        if (node->key != temp->key) {
//            // 左子树节点的关键节小于右子树
//            p = (node->key < temp->key) ? &temp->left : &temp->right;
//            // 当key关键字相同时，以字符串长度为第二索引关键字
//        } else if (n->str.len != t->str.len) {
//            // 左子树节点字符串的长度小于右子树
//            p = (n->str.len < t->str.len) ? &temp->left : &temp->right;
//        } else {
//            // key关键字相同且字符串长度相同时，再继续比较字符串内容
//            p = (ntg_memcmp(n->str.data, t->str.data, n->str.len) < 0)
//                 ? &temp->left : &temp->right;
//        }
//
//        // 如果当前节点p是哨兵节点，那么iaochu循环准备插入节点
//        if (*p == sentinel) {
//            break;
//        }
//
//        // p节点与要插入的节点具有相同的标识符时，必须覆盖内容
//        temp = *p;
//    }
//
//    *p = node;
//    // 置插入节点的父节点
//    node->parent = temp;
//    // 左右子节点都是哨兵节点
//    node->left = sentinel;
//    node->right = sentinel;
//
//    // 将节点颜色设置为红色。
//    ntg_rbt_red(node);
//}


//ntg_str_node_t *
//ntg_str_rbtree_lookup(ntg_rbtree_t *rbtree, ntg_str_t *val, uint32_t hash)
//{
//    ntg_int_t           rc;
//    ntg_str_node_t     *n;
//    ntg_rbtree_node_t  *node, *sentinel;
//
//    node = rbtree->root;
//    sentinel = rbtree->nil;
//
//    while (node != sentinel) {
//
//        n = (ntg_str_node_t *) node;
//
//        if (hash != node->key) {
//            node = (hash < node->key) ? node->left : node->right;
//            continue;
//        }
//
//        if (val->len != n->str.len) {
//            node = (val->len < n->str.len) ? node->left : node->right;
//            continue;
//        }
//
//        rc = ntg_memcmp(val->data, n->str.data, val->len);
//
//        if (rc < 0) {
//            node = node->left;
//            continue;
//        }
//
//        if (rc > 0) {
//            node = node->right;
//            continue;
//        }
//
//        return n;
//    }
//
//    return NULL;
//}


/* ntg_sort() is implemented as insertion sort because we need stable sort */


void ntg_sort(void *base, size_t n, size_t size,
    ntg_int_t (*cmp)(const void *, const void *)){
    u_char  *p1, *p2, *p;

    p = ntg_alloc(size, ntg_cycle->log);
    if (p == NULL) {
        return;
    }

    for (p1 = (u_char *) base + size;
         p1 < (u_char *) base + n * size;
         p1 += size)
    {
        ntg_memcpy(p, p1, size);

        for (p2 = p1;
             p2 > (u_char *) base && cmp(p2 - size, p) > 0;
             p2 -= size)
        {
            ntg_memcpy(p2, p2 - size, size);
        }

        ntg_memcpy(p2, p, size);
    }

    ntg_free(p);
}


#if (NTG_MEMCPY_LIMIT)

void *
ntg_memcpy(void *dst, void *src, size_t n)
{
    if (n > NTG_MEMCPY_LIMIT) {
        ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "memcpy %uz bytes", n);
        ntg_debug_point();
    }

    return memcpy(dst, src, n);
}

#endif

