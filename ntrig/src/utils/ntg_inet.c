/*
 * ntg_inet.c
 *
 *  Created on: Oct 30, 2015
 *      Author: tzh
 */


#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_inet.h"


static ntg_int_t ntg_parse_unix_domain_url(ntg_pool_t *pool, ntg_url_t *u);
static ntg_int_t ntg_parse_inet_url(ntg_pool_t *pool, ntg_url_t *u);
static ntg_int_t ntg_parse_inet6_url(ntg_pool_t *pool, ntg_url_t *u);

/**
 * 将文本解析为ip地址
 * @param[in] text 文本内容
 * @param[in] len 文本长度
 * @return 成功返回ip地址,否则返回INADDR_NONE(0xffffffff)
 */
in_addr_t
ntg_inet_addr(u_char *text, size_t len)
{
    u_char      *p, c;
    in_addr_t    addr;
    ntg_uint_t   octet, n;

    addr = 0;
    octet = 0;
    n = 0;

    for (p = text; p < text + len; p++) {

        if (octet > 255) {

            return INADDR_NONE;
        }

        c = *p;

        if (c >= '0' && c <= '9') {
            octet = octet * 10 + (c - '0');
            continue;
        }

        if (c == '.') {
            addr = (addr << 8) + octet;
            octet = 0;
            n++;
            continue;
        }

        return INADDR_NONE;
    }

    if (n == 3) {
        addr = (addr << 8) + octet;
        return htonl(addr);
    }
    return INADDR_NONE;
}


#if (NTG_HAVE_INET6)

ntg_int_t
ntg_inet6_addr(u_char *p, size_t len, u_char *addr)
{
    u_char      c, *zero, *digit, *s, *d;
    size_t      len4;
    ntg_uint_t  n, nibbles, word;

    if (len == 0) {
        return NTG_ERROR;
    }

    zero = NULL;
    digit = NULL;
    len4 = 0;
    nibbles = 0;
    word = 0;
    n = 8;

    if (p[0] == ':') {
        p++;
        len--;
    }

    for (/* void */; len; len--) {
        c = *p++;

        if (c == ':') {
            if (nibbles) {
                digit = p;
                len4 = len;
                *addr++ = (u_char) (word >> 8);
                *addr++ = (u_char) (word & 0xff);

                if (--n) {
                    nibbles = 0;
                    word = 0;
                    continue;
                }

            } else {
                if (zero == NULL) {
                    digit = p;
                    len4 = len;
                    zero = addr;
                    continue;
                }
            }

            return NTG_ERROR;
        }

        if (c == '.' && nibbles) {
            if (n < 2 || digit == NULL) {
                return NTG_ERROR;
            }

            word = ntg_inet_addr(digit, len4 - 1);
            if (word == INADDR_NONE) {
                return NTG_ERROR;
            }

            word = ntohl(word);
            *addr++ = (u_char) ((word >> 24) & 0xff);
            *addr++ = (u_char) ((word >> 16) & 0xff);
            n--;
            break;
        }

        if (++nibbles > 4) {
            return NTG_ERROR;
        }

        if (c >= '0' && c <= '9') {
            word = word * 16 + (c - '0');
            continue;
        }

        c |= 0x20;

        if (c >= 'a' && c <= 'f') {
            word = word * 16 + (c - 'a') + 10;
            continue;
        }

        return NTG_ERROR;
    }

    if (nibbles == 0 && zero == NULL) {
        return NTG_ERROR;
    }

    *addr++ = (u_char) (word >> 8);
    *addr++ = (u_char) (word & 0xff);

    if (--n) {
        if (zero) {
            n *= 2;
            s = addr - 1;
            d = s + n;
            while (s >= zero) {
                *d-- = *s--;
            }
            ntg_memzero(zero, n);
            return NTG_OK;
        }

    } else {
        if (zero == NULL) {
            return NTG_OK;
        }
    }

    return NTG_ERROR;
}

#endif

/**
 * 将sock地址转为字符串
 * @param[int] sa sockaddr指针
 * @param[int] socklen 地址长度
 * @param[out] text 文本指针
 * @param[int] len 文本最大长度
 * @param[int] port 端口标志
 * @return 返回text数据的实际长度
 */
size_t
ntg_sock_ntop(struct sockaddr *sa, socklen_t socklen, u_char *text, size_t len,
    ntg_uint_t port)
{
    u_char               *p;
    struct sockaddr_in   *sin;
#if (NTG_HAVE_INET6)
    size_t                n;
    struct sockaddr_in6  *sin6;
#endif
#if (NTG_HAVE_UNIX_DOMAIN)
    struct sockaddr_un   *saun;
#endif

    switch (sa->sa_family) {

    case AF_INET:

        sin = (struct sockaddr_in *) sa;
        p = (u_char *) &sin->sin_addr;

        if (port) {
            p = ntg_snprintf(text, len, "%ud.%ud.%ud.%ud:%d",
                             p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
        } else {
            p = ntg_snprintf(text, len, "%ud.%ud.%ud.%ud",
                             p[0], p[1], p[2], p[3]);
        }

        return (p - text);

#if (NTG_HAVE_INET6)

    case AF_INET6:

        sin6 = (struct sockaddr_in6 *) sa;

        n = 0;

        if (port) {
            text[n++] = '[';
        }

        n = ntg_inet6_ntop(sin6->sin6_addr.s6_addr, &text[n], len);

        if (port) {
            n = ntg_sprintf(&text[1 + n], "]:%d",
                            ntohs(sin6->sin6_port)) - text;
        }

        return n;
#endif

#if (NTG_HAVE_UNIX_DOMAIN)

    case AF_UNIX:
        saun = (struct sockaddr_un *) sa;

        /* on Linux sockaddr might not include sun_path at all */

        if (socklen <= (socklen_t) offsetof(struct sockaddr_un, sun_path)) {
            p = ntg_snprintf(text, len, "unix:%Z");

        } else {
            p = ntg_snprintf(text, len, "unix:%s%Z", saun->sun_path);
        }

        /* we do not include trailing zero in address length */

        return (p - text - 1);

#endif

    default:
        return 0;
    }
}

/**
 * 将网络地址转为字符串
 * @param[in] family
 * @param[in] addr
 * @param[in] text
 * @param[in] len
 * @return 返回text数据的实际长度
 */
size_t
ntg_inet_ntop(int family, void *addr, u_char *text, size_t len)
{
    u_char  *p;

    switch (family) {

    case AF_INET:

        p = addr;

        return ntg_snprintf(text, len, "%ud.%ud.%ud.%ud",
                            p[0], p[1], p[2], p[3])
               - text;

#if (NTG_HAVE_INET6)

    case AF_INET6:
        return ntg_inet6_ntop(addr, text, len);

#endif

    default:
        return 0;
    }
}


#if (NTG_HAVE_INET6)

size_t
ntg_inet6_ntop(u_char *p, u_char *text, size_t len)
{
    u_char      *dst;
    size_t       max, n;
    ntg_uint_t   i, zero, last;

    if (len < NTG_INET6_ADDRSTRLEN) {
        return 0;
    }

    zero = (ntg_uint_t) -1;
    last = (ntg_uint_t) -1;
    max = 1;
    n = 0;

    for (i = 0; i < 16; i += 2) {

        if (p[i] || p[i + 1]) {

            if (max < n) {
                zero = last;
                max = n;
            }

            n = 0;
            continue;
        }

        if (n++ == 0) {
            last = i;
        }
    }

    if (max < n) {
        zero = last;
        max = n;
    }

    dst = text;
    n = 16;

    if (zero == 0) {

        if ((max == 5 && p[10] == 0xff && p[11] == 0xff)
            || (max == 6)
            || (max == 7 && p[14] != 0 && p[15] != 1))
        {
            n = 12;
        }

        *dst++ = ':';
    }

    for (i = 0; i < n; i += 2) {

        if (i == zero) {
            *dst++ = ':';
            i += (max - 1) * 2;
            continue;
        }

        dst = ntg_sprintf(dst, "%uxi", p[i] * 256 + p[i + 1]);

        if (i < 14) {
            *dst++ = ':';
        }
    }

    if (n == 12) {
        dst = ntg_sprintf(dst, "%ud.%ud.%ud.%ud", p[12], p[13], p[14], p[15]);
    }

    return dst - text;
}

#endif



//ntg_int_t
//ntg_inet_pton(int af, const char *src, void *dst){
//
//}

ntg_int_t
ntg_ptocidr(ntg_str_t *text, ntg_cidr_t *cidr)
{
    u_char      *addr, *mask, *last;
    size_t       len;
    ntg_int_t    shift;
#if (NTG_HAVE_INET6)
    ntg_int_t    rc;
    ntg_uint_t   s, i;
#endif

    addr = text->data;
    last = addr + text->len;

    mask = ntg_strlchr(addr, last, '/');
    len = (mask ? mask : last) - addr;

    cidr->u.in.addr = ntg_inet_addr(addr, len);

    if (cidr->u.in.addr != INADDR_NONE) {
        cidr->family = AF_INET;

        if (mask == NULL) {
            cidr->u.in.mask = 0xffffffff;
            return NTG_OK;
        }

#if (NTG_HAVE_INET6)
    } else if (ntg_inet6_addr(addr, len, cidr->u.in6.addr.s6_addr) == NTG_OK) {
        cidr->family = AF_INET6;

        if (mask == NULL) {
            ntg_memset(cidr->u.in6.mask.s6_addr, 0xff, 16);
            return NTG_OK;
        }

#endif
    } else {
        return NTG_ERROR;
    }

    mask++;

    shift = ntg_atoi(mask, last - mask);
    if (shift == NTG_ERROR) {
        return NTG_ERROR;
    }

    switch (cidr->family) {

#if (NTG_HAVE_INET6)
    case AF_INET6:
        if (shift > 128) {
            return NTG_ERROR;
        }

        addr = cidr->u.in6.addr.s6_addr;
        mask = cidr->u.in6.mask.s6_addr;
        rc = NTG_OK;

        for (i = 0; i < 16; i++) {

            s = (shift > 8) ? 8 : shift;
            shift -= s;

            mask[i] = (u_char) (0xffu << (8 - s));

            if (addr[i] != (addr[i] & mask[i])) {
                rc = NTG_DONE;
                addr[i] &= mask[i];
            }
        }

        return rc;
#endif

    default: /* AF_INET */
        if (shift > 32) {
            return NTG_ERROR;
        }

        if (shift) {
            cidr->u.in.mask = htonl((uint32_t) (0xffffffffu << (32 - shift)));

        } else {
            /* x86 compilers use a shl instruction that shifts by modulo 32 */
            cidr->u.in.mask = 0;
        }

        if (cidr->u.in.addr == (cidr->u.in.addr & cidr->u.in.mask)) {
            return NTG_OK;
        }

        cidr->u.in.addr &= cidr->u.in.mask;

        return NTG_DONE;
    }
}


ntg_int_t
ntg_parse_addr(ntg_pool_t *pool, ntg_addr_t *addr, u_char *text, size_t len)
{
    in_addr_t             inaddr;
    ntg_uint_t            family;
    struct sockaddr_in   *sin;
#if (NTG_HAVE_INET6)
    struct in6_addr       inaddr6;
    struct sockaddr_in6  *sin6;

    /*
     * prevent MSVC8 warning:
     *    potentially uninitialized local variable 'inaddr6' used
     */
    ntg_memzero(&inaddr6, sizeof(struct in6_addr));
#endif

    inaddr = ntg_inet_addr(text, len);

    if (inaddr != INADDR_NONE) {
        family = AF_INET;
        len = sizeof(struct sockaddr_in);

#if (NTG_HAVE_INET6)
    } else if (ntg_inet6_addr(text, len, inaddr6.s6_addr) == NTG_OK) {
        family = AF_INET6;
        len = sizeof(struct sockaddr_in6);

#endif
    } else {
        return NTG_DECLINED;
    }

    addr->sockaddr = ntg_pcalloc(pool, len);
    if (addr->sockaddr == NULL) {
        return NTG_ERROR;
    }

    addr->sockaddr->sa_family = (u_char) family;
    addr->socklen = len;

    switch (family) {

#if (NTG_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) addr->sockaddr;
        ntg_memcpy(sin6->sin6_addr.s6_addr, inaddr6.s6_addr, 16);
        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) addr->sockaddr;
        sin->sin_addr.s_addr = inaddr;
        break;
    }

    return NTG_OK;
}

/**
 * 解析url对象
 * @param[in] pool 内存池对象
 * @param[in] u url对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 支持对unix, ipv4 和ipv6的解析
 */
ntg_int_t
ntg_parse_url(ntg_pool_t *pool, ntg_url_t *u)
{
    u_char  *p;

    p = u->url.data;

    if (ntg_strncasecmp(p, (u_char *) "unix:", 5) == 0) {
        return ntg_parse_unix_domain_url(pool, u);
    }

    if (p[0] == '[') {
        return ntg_parse_inet6_url(pool, u);
    }

    return ntg_parse_inet_url(pool, u);
}

/**
 * 解析unix域的url
 * @param[in] pool 内存池
 * @param[in] u url对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t
ntg_parse_unix_domain_url(ntg_pool_t *pool, ntg_url_t *u)
{
#if (NTG_HAVE_UNIX_DOMAIN)
    u_char              *path, *uri, *last;
    size_t               len;
    struct sockaddr_un  *saun;

    len = u->url.len;
    path = u->url.data;

    path += 5;
    len -= 5;

    if (u->uri_part) {

        last = path + len;
        uri = ntg_strlchr(path, last, ':');

        if (uri) {
            len = uri - path;
            uri++;
            u->uri.len = last - uri;
            u->uri.data = uri;
        }
    }

    if (len == 0) {
        u->err = "no path in the unix domain socket";
        return NTG_ERROR;
    }

    u->host.len = len++;
    u->host.data = path;

    if (len > sizeof(saun->sun_path)) {
        u->err = "too long path in the unix domain socket";
        return NTG_ERROR;
    }

    u->socklen = sizeof(struct sockaddr_un);
    saun = (struct sockaddr_un *) &u->sockaddr;
    saun->sun_family = AF_UNIX;
    (void) ntg_cpystrn((u_char *) saun->sun_path, path, len);

    u->addrs = ntg_pcalloc(pool, sizeof(ntg_addr_t));
    if (u->addrs == NULL) {
        return NTG_ERROR;
    }

    saun = ntg_pcalloc(pool, sizeof(struct sockaddr_un));
    if (saun == NULL) {
        return NTG_ERROR;
    }

    u->family = AF_UNIX;
    u->naddrs = 1;

    saun->sun_family = AF_UNIX;
    (void) ntg_cpystrn((u_char *) saun->sun_path, path, len);

    u->addrs[0].sockaddr = (struct sockaddr *) saun;
    u->addrs[0].socklen = sizeof(struct sockaddr_un);
    u->addrs[0].name.len = len + 4;
    u->addrs[0].name.data = u->url.data;

    return NTG_OK;

#else

    u->err = "the unix domain sockets are not supported on this platform";

    return NTG_ERROR;

#endif
}

/**
 * 解析inet(ipv4)的url
 * @param[in] pool 内存池
 * @param[in] u url对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t
ntg_parse_inet_url(ntg_pool_t *pool, ntg_url_t *u)
{
    u_char               *p, *host, *port, *last, *uri, *args;
    size_t                len;
    ntg_int_t             n;
    struct sockaddr_in   *sin;
#if (NTG_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    u->socklen = sizeof(struct sockaddr_in);
    sin = (struct sockaddr_in *) &u->sockaddr;
    sin->sin_family = AF_INET;

    u->family = AF_INET;

    host = u->url.data;

    last = host + u->url.len;

    port = ntg_strlchr(host, last, ':');

    uri = ntg_strlchr(host, last, '/');

    args = ntg_strlchr(host, last, '?');

    if (args) {
        if (uri == NULL || args < uri) {
            uri = args;
        }
    }

    if (uri) {
        if (u->listen || !u->uri_part) {
            u->err = "invalid host";
            return NTG_ERROR;
        }

        u->uri.len = last - uri;
        u->uri.data = uri;

        last = uri;

        if (uri < port) {
            port = NULL;
        }
    }

    if (port) {
        port++;

        len = last - port;

        n = ntg_atoi(port, len);

        if (n < 1 || n > 65535) {
            u->err = "invalid port";
            return NTG_ERROR;
        }

        u->port = (in_port_t) n;
        sin->sin_port = htons((in_port_t) n);

        u->port_text.len = len;
        u->port_text.data = port;

        last = port - 1;

    } else {
        if (uri == NULL) {

            if (u->listen) {

                /* test value as port only */

                n = ntg_atoi(host, last - host);

                if (n != NTG_ERROR) {

                    if (n < 1 || n > 65535) {
                        u->err = "invalid port";
                        return NTG_ERROR;
                    }

                    u->port = (in_port_t) n;
                    sin->sin_port = htons((in_port_t) n);

                    u->port_text.len = last - host;
                    u->port_text.data = host;

                    u->wildcard = 1;

                    return NTG_OK;
                }
            }
        }

        u->no_port = 1;
        u->port = u->default_port;
        sin->sin_port = htons(u->default_port);
    }

    len = last - host;

    if (len == 0) {
        u->err = "no host";
        return NTG_ERROR;
    }

    u->host.len = len;
    u->host.data = host;

    if (u->listen && len == 1 && *host == '*') {
        sin->sin_addr.s_addr = INADDR_ANY;
        u->wildcard = 1;
        return NTG_OK;
    }

    sin->sin_addr.s_addr = ntg_inet_addr(host, len);//ip地址

    if (sin->sin_addr.s_addr != INADDR_NONE) {
    	//合法
        if (sin->sin_addr.s_addr == INADDR_ANY) {
            u->wildcard = 1;
        }

        u->naddrs = 1;

        u->addrs = ntg_pcalloc(pool, sizeof(ntg_addr_t));
        if (u->addrs == NULL) {
            return NTG_ERROR;
        }

        sin = ntg_pcalloc(pool, sizeof(struct sockaddr_in));
        if (sin == NULL) {
            return NTG_ERROR;
        }

        ntg_memcpy(sin, u->sockaddr, sizeof(struct sockaddr_in));

        u->addrs[0].sockaddr = (struct sockaddr *) sin;
        u->addrs[0].socklen = sizeof(struct sockaddr_in);

        p = ntg_pnalloc(pool, u->host.len + sizeof(":65535") - 1);
        if (p == NULL) {
            return NTG_ERROR;
        }

        u->addrs[0].name.len = ntg_sprintf(p, "%V:%d",
                                           &u->host, u->port) - p;
        u->addrs[0].name.data = p;

        return NTG_OK;
    }
    /*不是ip地址形式*/
    if (u->no_resolve) {
        return NTG_OK;
    }

    if (ntg_inet_resolve_host(pool, u) != NTG_OK) {
        return NTG_ERROR;
    }

    u->family = u->addrs[0].sockaddr->sa_family;
    u->socklen = u->addrs[0].socklen;
    ntg_memcpy(u->sockaddr, u->addrs[0].sockaddr, u->addrs[0].socklen);

    switch (u->family) {

#if (NTG_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) &u->sockaddr;

        if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
            u->wildcard = 1;
        }

        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) &u->sockaddr;

        if (sin->sin_addr.s_addr == INADDR_ANY) {
            u->wildcard = 1;
        }

        break;
    }

    return NTG_OK;
}

/**
 * 解析inet6类型的url
 * @param[in] pool 内存池
 * @param[in] u url对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t
ntg_parse_inet6_url(ntg_pool_t *pool, ntg_url_t *u)
{
#if (NTG_HAVE_INET6)
    u_char               *p, *host, *port, *last, *uri;
    size_t                len;
    ntg_int_t             n;
    struct sockaddr_in6  *sin6;

    u->socklen = sizeof(struct sockaddr_in6);
    sin6 = (struct sockaddr_in6 *) &u->sockaddr;
    sin6->sin6_family = AF_INET6;

    host = u->url.data + 1;

    last = u->url.data + u->url.len;

    p = ntg_strlchr(host, last, ']');

    if (p == NULL) {
        u->err = "invalid host";
        return NTG_ERROR;
    }

    if (last - p) {

        port = p + 1;

        uri = ntg_strlchr(port, last, '/');

        if (uri) {
            if (u->listen || !u->uri_part) {
                u->err = "invalid host";
                return NTG_ERROR;
            }

            u->uri.len = last - uri;
            u->uri.data = uri;

            last = uri;
        }

        if (*port == ':') {
            port++;

            len = last - port;

            n = ntg_atoi(port, len);

            if (n < 1 || n > 65535) {
                u->err = "invalid port";
                return NTG_ERROR;
            }

            u->port = (in_port_t) n;
            sin6->sin6_port = htons((in_port_t) n);

            u->port_text.len = len;
            u->port_text.data = port;

        } else {
            u->no_port = 1;
            u->port = u->default_port;
            sin6->sin6_port = htons(u->default_port);
        }
    }

    len = p - host;

    if (len == 0) {
        u->err = "no host";
        return NTG_ERROR;
    }

    u->host.len = len + 2;
    u->host.data = host - 1;

    if (ntg_inet6_addr(host, len, sin6->sin6_addr.s6_addr) != NTG_OK) {
        u->err = "invalid IPv6 address";
        return NTG_ERROR;
    }

    if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
        u->wildcard = 1;
    }

    u->family = AF_INET6;
    u->naddrs = 1;

    u->addrs = ntg_pcalloc(pool, sizeof(ntg_addr_t));
    if (u->addrs == NULL) {
        return NTG_ERROR;
    }

    sin6 = ntg_pcalloc(pool, sizeof(struct sockaddr_in6));
    if (sin6 == NULL) {
        return NTG_ERROR;
    }

    ntg_memcpy(sin6, u->sockaddr, sizeof(struct sockaddr_in6));

    u->addrs[0].sockaddr = (struct sockaddr *) sin6;
    u->addrs[0].socklen = sizeof(struct sockaddr_in6);

    p = ntg_pnalloc(pool, u->host.len + sizeof(":65535") - 1);
    if (p == NULL) {
        return NTG_ERROR;
    }

    u->addrs[0].name.len = ntg_sprintf(p, "%V:%d",
                                       &u->host, u->port) - p;
    u->addrs[0].name.data = p;

    return NTG_OK;

#else

    u->err = "the INET6 sockets are not supported on this platform";

    return NTG_ERROR;

#endif
}


#if (NTG_HAVE_GETADDRINFO && NTG_HAVE_INET6)

ntg_int_t
ntg_inet_resolve_host(ntg_pool_t *pool, ntg_url_t *u)
{
    u_char               *p, *host;
    size_t                len;
    in_port_t             port;
    ntg_uint_t            i;
    struct addrinfo       hints, *res, *rp;
    struct sockaddr_in   *sin;
    struct sockaddr_in6  *sin6;

    port = htons(u->port);

    host = ntg_alloc(u->host.len + 1, pool->log);
    if (host == NULL) {
        return NTG_ERROR;
    }

    (void) ntg_cpystrn(host, u->host.data, u->host.len + 1);

    ntg_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
    hints.ai_flags = AI_ADDRCONFIG;
#endif

    if (getaddrinfo((char *) host, NULL, &hints, &res) != 0) {
        u->err = "host not found";
        ntg_free(host);
        return NTG_ERROR;
    }

    ntg_free(host);

    for (i = 0, rp = res; rp != NULL; rp = rp->ai_next) {

        switch (rp->ai_family) {

        case AF_INET:
        case AF_INET6:
            break;

        default:
            continue;
        }

        i++;
    }

    if (i == 0) {
        u->err = "host not found";
        goto failed;
    }

    /* MP: ntg_shared_palloc() */

    u->addrs = ntg_pcalloc(pool, i * sizeof(ntg_addr_t));
    if (u->addrs == NULL) {
        goto failed;
    }

    u->naddrs = i;

    i = 0;

    /* AF_INET addresses first */

    for (rp = res; rp != NULL; rp = rp->ai_next) {

        if (rp->ai_family != AF_INET) {
            continue;
        }

        sin = ntg_pcalloc(pool, rp->ai_addrlen);
        if (sin == NULL) {
            goto failed;
        }

        ntg_memcpy(sin, rp->ai_addr, rp->ai_addrlen);

        sin->sin_port = port;

        u->addrs[i].sockaddr = (struct sockaddr *) sin;
        u->addrs[i].socklen = rp->ai_addrlen;

        len = NTG_INET_ADDRSTRLEN + sizeof(":65535") - 1;

        p = ntg_pnalloc(pool, len);
        if (p == NULL) {
            goto failed;
        }

        len = ntg_sock_ntop((struct sockaddr *) sin, rp->ai_addrlen, p, len, 1);

        u->addrs[i].name.len = len;
        u->addrs[i].name.data = p;

        i++;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {

        if (rp->ai_family != AF_INET6) {
            continue;
        }

        sin6 = ntg_pcalloc(pool, rp->ai_addrlen);
        if (sin6 == NULL) {
            goto failed;
        }

        ntg_memcpy(sin6, rp->ai_addr, rp->ai_addrlen);

        sin6->sin6_port = port;

        u->addrs[i].sockaddr = (struct sockaddr *) sin6;
        u->addrs[i].socklen = rp->ai_addrlen;

        len = NTG_INET6_ADDRSTRLEN + sizeof("[]:65535") - 1;

        p = ntg_pnalloc(pool, len);
        if (p == NULL) {
            goto failed;
        }

        len = ntg_sock_ntop((struct sockaddr *) sin6, rp->ai_addrlen, p,
                            len, 1);

        u->addrs[i].name.len = len;
        u->addrs[i].name.data = p;

        i++;
    }

    freeaddrinfo(res);
    return NTG_OK;

failed:

    freeaddrinfo(res);
    return NTG_ERROR;
}

#else /* !NTG_HAVE_GETADDRINFO || !NTG_HAVE_INET6 */
/**
 *
 * @param pool
 * @param u
 * @return
 */
ntg_int_t
ntg_inet_resolve_host(ntg_pool_t *pool, ntg_url_t *u)
{
    u_char              *p, *host;
    size_t               len;
    in_port_t            port;
    in_addr_t            in_addr;
    ntg_uint_t           i;
    struct hostent      *h;
    struct sockaddr_in  *sin;

    /* AF_INET only */

    port = htons(u->port);

    in_addr = ntg_inet_addr(u->host.data, u->host.len);

    if (in_addr == INADDR_NONE) {//需要域名解析
        host = ntg_alloc(u->host.len + 1, pool->log);
        if (host == NULL) {
            return NTG_ERROR;
        }

        (void) ntg_cpystrn(host, u->host.data, u->host.len + 1);

        h = gethostbyname((char *) host);

        ntg_free(host);

        if (h == NULL || h->h_addr_list[0] == NULL) {
            u->err = "host not found";
            return NTG_ERROR;
        }

        for (i = 0; h->h_addr_list[i] != NULL; i++) { /* void */ }

        /* MP: ntg_shared_palloc() */

        u->addrs = ntg_pcalloc(pool, i * sizeof(ntg_addr_t));
        if (u->addrs == NULL) {
            return NTG_ERROR;
        }

        u->naddrs = i;

        for (i = 0; i < u->naddrs; i++) {

            sin = ntg_pcalloc(pool, sizeof(struct sockaddr_in));
            if (sin == NULL) {
                return NTG_ERROR;
            }

            sin->sin_family = AF_INET;
            sin->sin_port = port;
            sin->sin_addr.s_addr = *(in_addr_t *) (h->h_addr_list[i]);

            u->addrs[i].sockaddr = (struct sockaddr *) sin;
            u->addrs[i].socklen = sizeof(struct sockaddr_in);

            len = NTG_INET_ADDRSTRLEN + sizeof(":65535") - 1;

            p = ntg_pnalloc(pool, len);
            if (p == NULL) {
                return NTG_ERROR;
            }

            len = ntg_sock_ntop((struct sockaddr *) sin,
                                sizeof(struct sockaddr_in), p, len, 1);

            u->addrs[i].name.len = len;
            u->addrs[i].name.data = p;
        }

    } else {

        /* MP: ntg_shared_palloc() */

        u->addrs = ntg_pcalloc(pool, sizeof(ntg_addr_t));
        if (u->addrs == NULL) {
            return NTG_ERROR;
        }

        sin = ntg_pcalloc(pool, sizeof(struct sockaddr_in));
        if (sin == NULL) {
            return NTG_ERROR;
        }

        u->naddrs = 1;

        sin->sin_family = AF_INET;
        sin->sin_port = port;
        sin->sin_addr.s_addr = in_addr;

        u->addrs[0].sockaddr = (struct sockaddr *) sin;
        u->addrs[0].socklen = sizeof(struct sockaddr_in);

        p = ntg_pnalloc(pool, u->host.len + sizeof(":65535") - 1);
        if (p == NULL) {
            return NTG_ERROR;
        }

        u->addrs[0].name.len = ntg_sprintf(p, "%V:%d",
                                           &u->host, ntohs(port)) - p;
        u->addrs[0].name.data = p;
    }

    return NTG_OK;
}

#endif /* NTG_HAVE_GETADDRINFO && NTG_HAVE_INET6 */


ntg_int_t
ntg_cmp_sockaddr(struct sockaddr *sa1, socklen_t slen1,
    struct sockaddr *sa2, socklen_t slen2, ntg_uint_t cmp_port)
{
    struct sockaddr_in   *sin1, *sin2;
#if (NTG_HAVE_INET6)
    struct sockaddr_in6  *sin61, *sin62;
#endif
#if (NTG_HAVE_UNIX_DOMAIN)
    struct sockaddr_un   *saun1, *saun2;
#endif

    if (sa1->sa_family != sa2->sa_family) {
        return NTG_DECLINED;
    }

    switch (sa1->sa_family) {

#if (NTG_HAVE_INET6)
    case AF_INET6:

        sin61 = (struct sockaddr_in6 *) sa1;
        sin62 = (struct sockaddr_in6 *) sa2;

        if (cmp_port && sin61->sin6_port != sin62->sin6_port) {
            return NTG_DECLINED;
        }

        if (ntg_memcmp(&sin61->sin6_addr, &sin62->sin6_addr, 16) != 0) {
            return NTG_DECLINED;
        }

        break;
#endif

#if (NTG_HAVE_UNIX_DOMAIN)
    case AF_UNIX:

       /* TODO length */

       saun1 = (struct sockaddr_un *) sa1;
       saun2 = (struct sockaddr_un *) sa2;

       if (ntg_memcmp(&saun1->sun_path, &saun2->sun_path,
                      sizeof(saun1->sun_path))
           != 0)
       {
           return NTG_DECLINED;
       }

       break;
#endif

    default: /* AF_INET */

        sin1 = (struct sockaddr_in *) sa1;
        sin2 = (struct sockaddr_in *) sa2;

        if (cmp_port && sin1->sin_port != sin2->sin_port) {
            return NTG_DECLINED;
        }

        if (sin1->sin_addr.s_addr != sin2->sin_addr.s_addr) {
            return NTG_DECLINED;
        }

        break;
    }

    return NTG_OK;
}
