/*
 * ntg_inet.h
 *
 *  Created on: Oct 14, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_INET_H_
#define UTILS_NTG_INET_H_


#include "../ntg_config.h"
#include "../ntg_core.h"




/*
 * TODO: autoconfigure NTG_SOCKADDRLEN and NTG_SOCKADDR_STRLEN as
 *       sizeof(struct sockaddr_storage)
 *       sizeof(struct sockaddr_un)
 *       sizeof(struct sockaddr_in6)
 *       sizeof(struct sockaddr_in)
 */

#define NTG_INET_ADDRSTRLEN   (sizeof("255.255.255.255") - 1)
#define NTG_INET6_ADDRSTRLEN                                                 \
    (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") - 1)
#define NTG_UNIX_ADDRSTRLEN                                                  \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#if (NTG_HAVE_UNIX_DOMAIN)
#define NTG_SOCKADDR_STRLEN   (sizeof("unix:") - 1 + NTG_UNIX_ADDRSTRLEN)
#else
#define NTG_SOCKADDR_STRLEN   (NTG_INET6_ADDRSTRLEN + sizeof("[]:65535") - 1)
#endif

#if (NTG_HAVE_UNIX_DOMAIN)
#define NTG_SOCKADDRLEN       sizeof(struct sockaddr_un)
#else
#define NTG_SOCKADDRLEN       512
#endif


typedef struct {
    in_addr_t                 addr;
    in_addr_t                 mask;
} ntg_in_cidr_t;


#if (NTG_HAVE_INET6)

typedef struct {
    struct in6_addr           addr;
    struct in6_addr           mask;
} ntg_in6_cidr_t;

#endif


typedef struct {
    ntg_uint_t                family;
    union {
        ntg_in_cidr_t         in;
#if (NTG_HAVE_INET6)
        ntg_in6_cidr_t        in6;
#endif
    } u;
} ntg_cidr_t;

/**
 * @name 地址对象
 */
typedef struct {
    struct sockaddr          *sockaddr;///地址
    socklen_t                 socklen;///地址长度
    ntg_str_t                 name;///名称
} ntg_addr_t;

/**
 * @name url对象
 * @note 这个url对象是不包含协议包头不的.比如"http://123.com",其代url仅仅是"123.com".
 */
typedef struct {
    ntg_str_t                 url;///url字符串
    ntg_str_t                 host;///host部分
    ntg_str_t                 port_text;///端口文本
    ntg_str_t                 uri;///uri

    in_port_t                 port;///端口
    in_port_t                 default_port;///默认端口
    int                       family;///地址族

    unsigned                  listen:1;
    unsigned                  uri_part:1;
    unsigned                  no_resolve:1;///不需要解析标志
    unsigned                  one_addr:1;  /* compatibility */

    unsigned                  no_port:1;///没有端口标志
    unsigned                  wildcard:1;///通配符标志

    socklen_t                 socklen;///地址长度
    u_char                    sockaddr[NTG_SOCKADDRLEN];///对应的地址

    ntg_addr_t               *addrs;///地址数组
    ntg_uint_t                naddrs;///对应地址的个数

    char                     *err;///错误记录
} ntg_url_t;


in_addr_t ntg_inet_addr(u_char *text, size_t len);
#if (NTG_HAVE_INET6)
ntg_int_t ntg_inet6_addr(u_char *p, size_t len, u_char *addr);
size_t ntg_inet6_ntop(u_char *p, u_char *text, size_t len);
#endif
size_t ntg_sock_ntop(struct sockaddr *sa, socklen_t socklen, u_char *text,
    size_t len, ntg_uint_t port);
size_t ntg_inet_ntop(int family, void *addr, u_char *text, size_t len);
ntg_int_t ntg_ptocidr(ntg_str_t *text, ntg_cidr_t *cidr);
ntg_int_t ntg_parse_addr(ntg_pool_t *pool, ntg_addr_t *addr, u_char *text,
    size_t len);
ntg_int_t ntg_parse_url(ntg_pool_t *pool, ntg_url_t *u);
ntg_int_t ntg_inet_resolve_host(ntg_pool_t *pool, ntg_url_t *u);
ntg_int_t ntg_cmp_sockaddr(struct sockaddr *sa1, socklen_t slen1,
    struct sockaddr *sa2, socklen_t slen2, ntg_uint_t cmp_port);


#endif /* UTILS_NTG_INET_H_ */
