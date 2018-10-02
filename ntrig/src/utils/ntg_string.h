/*
 * ntg_string.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_STRING_H_
#define CORE_NTG_STRING_H_


#include "../ntg_config.h"
#include "../ntg_core.h"

typedef struct ntg_str_s{
	size_t len;
	u_char	*data;
}ntg_str_t;


typedef struct {
	ntg_str_t key;
	ntg_str_t value;
}ntg_keyval_t;

typedef struct {
    unsigned    len:28;             //变量值的长度
    unsigned    valid:1;            //变量是否有效
    unsigned    no_cacheable:1;     /* 变量是否是可缓存的，一般来说，某些变量在第一次得到变量值后，后面再次用到时，可以直接使用上
                                    * 次的值，而对于一些所谓的no_cacheable的变量，则需要在每次使用的时候，都要通过get_handler之
                                    * 类操作，再次获取
                                    */
    unsigned    not_found:1;        //变量没有找到，一般是指某个变量没用能够通过get获取到其变量值
    unsigned    escape:1;           //变量值是否需要作转义处理

    u_char     *data;               //变量值
} ntg_variable_value_t;


#define ntg_string(str) 	{sizeof(str) -1, (u_char*)str}
#define ntg_null_string		{0, NULL}
#define ntg_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#define ntg_str_null(str)   (str)->len = 0; (str)->data = NULL


#define ntg_tolower(c)		(u_char)((c >='A' && c<='Z') ? (c | 0x20) : c)
#define ntg_toupper(c)		(u_char)((c >='a' && c<='z') ? (c & ~0x20) : c)

//将src的前n个字符转换成小写存放在dst字符串当中
void ntg_strlow(u_char *dst, u_char *src, size_t n);

#define ntg_strncmp(s1, s2, n) 		strncmp((const char *)s1, (const char *)s2, n)
#define ntg_strcmp(s1, s2)			strcmp((const char*)s1, (const char *)s2)
#define ntg_strstr(s1, s2)			strstr((const char*)s1, (const char *)s2)
#define ntg_strlen(s)				strlen((const char *)s)
#define ntg_strchr(s,c)				strchr((const char *)s, (int)c)

//l表示带last结束标志
static inline u_char *
ntg_strlchr(u_char *p, u_char *last, u_char c)
{
    while (p < last) {

        if (*p == c) {
            return p;
        }

        p++;
    }

    return NULL;
}

#define ntg_memzero(buf, n)		(void) memset(buf, 0, n)
#define ntg_memset(buf, c, n)	(void) memset(buf, c, n)

#define ntg_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
//cpymem 拷贝数据后移动到下一个数据位置
#define ntg_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))
#define ntg_copy                  ntg_cpymem


#define ntg_memmove(dst, src, n)   (void) memmove(dst, src, n)
#define ntg_movemem(dst, src, n)   (((u_char *) memmove(dst, src, n)) + (n))

#define ntg_memcmp(s1, s2, n)		memcmp((const char *)s1, (const char*)s2, n)

u_char * ntg_cpystrn(u_char *dst, u_char *src, size_t n);
u_char * ntg_pstrdup(ntg_pool_t *pool, ntg_str_t *src);
u_char * ntg_sprintf(u_char *buf, const char *fmt, ...);
u_char * ntg_snprintf(u_char *buf, size_t max, const char *fmt, ...);
u_char * ntg_slprintf(u_char *buf, u_char *last, const char *fmt, ...);

u_char *ntg_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);
#define ntg_vsnprintf(buf, max, fmt, args)                                   \
    ntg_vslprintf(buf, buf + (max), fmt, args)

//不区分大小写的字符串比较
ntg_int_t ntg_strcasecmp(u_char *s1, u_char *s2);
ntg_int_t ntg_strncasecmp(u_char *s1, u_char *s2, size_t n);

u_char *ntg_strnstr(u_char *s1, char *s2, size_t n);

u_char *ntg_strstrn(u_char *s1, char *s2, size_t n);
u_char *ntg_strcasestrn(u_char *s1, char *s2, size_t n);
u_char *ntg_strlcasestrn(u_char *s1, u_char *last, u_char *s2, size_t n);

ntg_int_t ntg_rstrncmp(u_char *s1, u_char *s2, size_t n);
ntg_int_t ntg_rstrncasecmp(u_char *s1, u_char *s2, size_t n);
ntg_int_t ntg_memn2cmp(u_char *s1, u_char *s2, size_t n1, size_t n2);
ntg_int_t ntg_dns_strcmp(u_char *s1, u_char *s2);
ntg_int_t ntg_filename_cmp(u_char *s1, u_char *s2, size_t n);

ntg_int_t ntg_atoi(u_char *line, size_t n);
ntg_int_t ntg_atofp(u_char *line, size_t n, size_t point);
ssize_t ntg_atosz(u_char *line, size_t n);
off_t ntg_atoof(u_char *line, size_t n);
time_t ntg_atotm(u_char *line, size_t n);
ntg_int_t ntg_hextoi(u_char *line, size_t n);

u_char *ntg_hex_dump(u_char *dst, u_char *src, size_t len);

#define ntg_base64_encoded_length(len)  (((len + 2) / 3) * 4)
#define ntg_base64_decoded_length(len)  (((len + 3) / 4) * 3)

void ntg_encode_base64(ntg_str_t *dst, ntg_str_t *src);
void ntg_encode_base64url(ntg_str_t *dst, ntg_str_t *src);
ntg_int_t ntg_decode_base64(ntg_str_t *dst, ntg_str_t *src);
ntg_int_t ntg_decode_base64url(ntg_str_t *dst, ntg_str_t *src);

uint32_t ntg_utf8_decode(u_char **p, size_t n);
size_t ntg_utf8_length(u_char *p, size_t n);
u_char *ntg_utf8_cpystrn(u_char *dst, u_char *src, size_t n, size_t len);


#define NTG_ESCAPE_URI            0
#define NTG_ESCAPE_ARGS           1
#define NTG_ESCAPE_URI_COMPONENT  2
#define NTG_ESCAPE_HTML           3
#define NTG_ESCAPE_REFRESH        4
#define NTG_ESCAPE_MEMCACHED      5
#define NTG_ESCAPE_MAIL_AUTH      6

#define NTG_UNESCAPE_URI       1
#define NTG_UNESCAPE_REDIRECT  2


uintptr_t ntg_escape_uri(u_char *dst, u_char *src, size_t size,
    ntg_uint_t type);
void ntg_unescape_uri(u_char **dst, u_char **src, size_t size, ntg_uint_t type);
uintptr_t ntg_escape_html(u_char *dst, u_char *src, size_t size);
uintptr_t ntg_escape_json(u_char *dst, u_char *src, size_t size);


typedef struct {
    ntg_rbtree_node_t         node;
    ntg_str_t                 str;
} ntg_str_node_t;


void ntg_str_rbtree_insert_value(ntg_rbtree_node_t *temp,
    ntg_rbtree_node_t *node, ntg_rbtree_node_t *sentinel);
ntg_str_node_t *ntg_str_rbtree_lookup(ntg_rbtree_t *rbtree, ntg_str_t *name,
    uint32_t hash);


void ntg_sort(void *base, size_t n, size_t size,
    ntg_int_t (*cmp)(const void *, const void *));
#define ntg_qsort             qsort



#define ntg_value_helper(n)   #n
#define ntg_value(n)          ntg_value_helper(n)
#endif /* CORE_NTG_STRING_H_ */
