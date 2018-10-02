/*
 * ntg_errno.h
 *
 *  Created on: Jul 26, 2015
 *      Author: tzh
 */

#ifndef OS_NTG_ERRNO_H_
#define OS_NTG_ERRNO_H_

#include "../ntg_config.h"

typedef int               ntg_err_t;

#define NTG_EPERM         EPERM
#define NTG_ENOENT        ENOENT
#define NTG_ENOPATH       ENOENT//没有这个文件或目录
#define NTG_ESRCH         ESRCH ///< No such process
#define NTG_EINTR         EINTR
#define NTG_ECHILD        ECHILD//没有子进程
#define NTG_ENOMEM        ENOMEM
#define NTG_EACCES        EACCES
#define NTG_EBUSY         EBUSY
#define NTG_EEXIST        EEXIST
#define NTG_EXDEV         EXDEV
#define NTG_ENOTDIR       ENOTDIR
#define NTG_EISDIR        EISDIR
#define NTG_EINVAL        EINVAL
#define NTG_ENFILE        ENFILE
#define NTG_EMFILE        EMFILE
#define NTG_ENOSPC        ENOSPC
#define NTG_EPIPE         EPIPE
#define NTG_EINPROGRESS   EINPROGRESS
#define NTG_ENOPROTOOPT   ENOPROTOOPT
#define NTG_EOPNOTSUPP    EOPNOTSUPP
#define NTG_EADDRINUSE    EADDRINUSE
#define NTG_ECONNABORTED  ECONNABORTED
#define NTG_ECONNRESET    ECONNRESET
#define NTG_ENOTCONN      ENOTCONN
#define NTG_ETIMEDOUT     ETIMEDOUT
#define NTG_ECONNREFUSED  ECONNREFUSED
#define NTG_ENAMETOOLONG  ENAMETOOLONG
#define NTG_ENETDOWN      ENETDOWN
#define NTG_ENETUNREACH   ENETUNREACH
#define NTG_EHOSTDOWN     EHOSTDOWN
#define NTG_EHOSTUNREACH  EHOSTUNREACH
#define NTG_ENOSYS        ENOSYS
#define NTG_ECANCELED     ECANCELED
#define NTG_EILSEQ        EILSEQ
#define NTG_ENOMOREFILES  0
#define NTG_ELOOP         ELOOP
#define NTG_EBADF         EBADF

#if (NTG_HAVE_OPENAT)
#define NTG_EMLINK        EMLINK
#endif

#if (__hpux__)
#define NTG_EAGAIN        EWOULDBLOCK
#else
#define NTG_EAGAIN        EAGAIN
#endif

//TODO 自动获取
//#define NTG_SYS_NERR	200

#define ntg_errno                  errno
#define ntg_socket_errno           errno
#define ntg_set_errno(err)         errno = err
#define ntg_set_socket_errno(err)  errno = err


u_char *ntg_strerror(ntg_err_t err, u_char *errstr, size_t size);
ntg_uint_t ntg_strerror_init(void);
#endif /* OS_NTG_ERRNO_H_ */
