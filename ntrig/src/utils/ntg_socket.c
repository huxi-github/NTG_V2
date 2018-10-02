/*
 * ntg_socket.c
 *
 *  Created on: Oct 12, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_socket.h"

#if (NTG_HAVE_FIONBIO)

int
ntg_nonblocking(ntg_socket_t s)
{
    int  nb;

    nb = 1;

    return ioctl(s, FIONBIO, &nb);
}


int
ntg_blocking(ntg_socket_t s)
{
    int  nb;

    nb = 0;

    return ioctl(s, FIONBIO, &nb);
}

#endif

//实现对不同平台的支持
#if (NTG_FREEBSD)

int
ntg_tcp_nopush(ntg_socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &tcp_nopush, sizeof(int));
}


int
ntg_tcp_push(ntg_socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &tcp_nopush, sizeof(int));
}

#elif (NTG_LINUX)


int
ntg_tcp_nopush(ntg_socket_t s)
{
    int  cork;

    cork = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &cork, sizeof(int));
}


int
ntg_tcp_push(ntg_socket_t s)
{
    int  cork;

    cork = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &cork, sizeof(int));
}

#else

int
ntg_tcp_nopush(ntg_socket_t s)
{
    return 0;
}


int
ntg_tcp_push(ntg_socket_t s)
{
    return 0;
}

#endif
