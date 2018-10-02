/**
 * @file ntg_posix_init.c
 *
 * @brief	posix属性的一些初始化
 * @author	tzh
 * @date	Oct 31, 2015
 * @version		V0.1
 * @copyright	tzh
 */


#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../ntrig.h"
#include "ntg_os.h"
#include "ntg_times.h"
#include "ntg_setproctitle.h"

/**
 * @name posix的一些属性
 * @{
 */
ntg_int_t   ntg_ncpu;///< cpu的数目
ntg_int_t   ntg_max_sockets;///< 最大sockets数
ntg_uint_t  ntg_inherited_nonblocking;///< 继承非阻塞
ntg_uint_t  ntg_tcp_nodelay_and_tcp_nopush;///< TCP的非延时非push
/**@}*/

struct rlimit  rlmt;///< 资源限制



/**
 * 操作系统的初始化
 * @param log 日志对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 * @note
 * 		初始化的属性有:
 * 		 ntg_pagesize
 * 		 ntg_cacheline_size
 * 		 ntg_pagesize_shift
 * 		 ntg_ncpu
 * 		 ntg_max_sockets
 * 		并初始化了随机种子
 */
ntg_int_t
ntg_os_init(ntg_log_t *log)
{
    ntg_uint_t  n;

#if (NTG_HAVE_OS_SPECIFIC_INIT)
    if (ntg_os_specific_init(log) != NTG_OK) {
        return NTG_ERROR;
    }
#endif

    if (ntg_init_setproctitle(log) != NTG_OK) {
        return NTG_ERROR;
    }

    //设置相关属性
    ntg_pagesize = getpagesize();
    ntg_cacheline_size = NTG_CPU_CACHE_LINE;

    for (n = ntg_pagesize; n >>= 1; ntg_pagesize_shift++) { /* void */ }

    //获取cpu核的个数
#if (NTG_HAVE_SC_NPROCESSORS_ONLN)
    if (ntg_ncpu == 0) {
        ntg_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    }
#endif

    if (ntg_ncpu < 1) {
        ntg_ncpu = 1;
    }

    ntg_cpuinfo();

    //获取进程能打开的最大文件数
    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        ntg_log_error(NTG_LOG_ALERT, log, errno,
                      "getrlimit(RLIMIT_NOFILE) failed)");
        return NTG_ERROR;
    }

    ntg_max_sockets = (ntg_int_t) rlmt.rlim_cur;

#if (NTG_HAVE_INHERITED_NONBLOCK || NTG_HAVE_ACCEPT4)
    ntg_inherited_nonblocking = 1;
#else
    ntg_inherited_nonblocking = 0;
#endif

    srandom(ntg_time());//设置随机种子

    return NTG_OK;
}

/**
 * 在日志文件中记录操作系统的一些状态
 * @param[in] log 日志对象
 */
void
ntg_os_status(ntg_log_t *log)
{
    ntg_log_error(NTG_LOG_NOTICE, log, 0, NTRIG_VER_BUILD);

#ifdef NTG_COMPILER
    ntg_log_error(NTG_LOG_NOTICE, log, 0, "built by " NTG_COMPILER);
#endif

#if (NTG_HAVE_OS_SPECIFIC_INIT)
    ntg_os_specific_status(log);
#endif

    ntg_log_error(NTG_LOG_NOTICE, log, 0,
                  "getrlimit(RLIMIT_NOFILE): %r:%r",
                  rlmt.rlim_cur, rlmt.rlim_max);
}


#if 0

ntg_int_t
ntg_posix_post_conf_init(ntg_log_t *log)
{
    ntg_fd_t  pp[2];

    if (pipe(pp) == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "pipe() failed");
        return NTG_ERROR;
    }

    if (dup2(pp[1], STDERR_FILENO) == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, errno, "dup2(STDERR) failed");
        return NTG_ERROR;
    }

    if (pp[1] > STDERR_FILENO) {
        if (close(pp[1]) == -1) {
            ntg_log_error(NTG_LOG_EMERG, log, errno, "close() failed");
            return NTG_ERROR;
        }
    }

    return NTG_OK;
}

#endif
