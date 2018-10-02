/*
 * ntg_socket.h
 *
 *  Created on: Oct 12, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_SOCKET_H_
#define UTILS_NTG_SOCKET_H_

#include "../ntg_config.h"

#define NTG_WRITE_SHUTDOWN SHUT_WR


typedef int  ntg_socket_t;

#define ntg_socket          socket
#define ntg_socket_n        "socket()"

//文件描述符的阻塞性设置(ioctl 或 fcntl)
#if (NTG_HAVE_FIONBIO)

int ntg_nonblocking(ntg_socket_t s);
int ntg_blocking(ntg_socket_t s);

#define ntg_nonblocking_n   "ioctl(FIONBIO)"
#define ntg_blocking_n      "ioctl(!FIONBIO)"

#else

#define ntg_nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define ntg_nonblocking_n   "fcntl(O_NONBLOCK)"

#define ntg_blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define ntg_blocking_n      "fcntl(!O_NONBLOCK)"

#endif

int ntg_tcp_nopush(ntg_socket_t s);
int ntg_tcp_push(ntg_socket_t s);

#if (NTG_LINUX)

#define ntg_tcp_nopush_n   "setsockopt(TCP_CORK)"
#define ntg_tcp_push_n     "setsockopt(!TCP_CORK)"

#else

#define ntg_tcp_nopush_n   "setsockopt(TCP_NOPUSH)"
#define ntg_tcp_push_n     "setsockopt(!TCP_NOPUSH)"

#endif


#define ntg_shutdown_socket    shutdown
#define ntg_shutdown_socket_n  "shutdown()"

#define ntg_close_socket    close
#define ntg_close_socket_n  "close() socket"



#endif /* UTILS_NTG_SOCKET_H_ */
