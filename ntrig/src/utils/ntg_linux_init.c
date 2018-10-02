/*
 * ntg_linux_init.c
 *
 *  Created on: Oct 30, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_linux.h"


u_char  ntg_linux_kern_ostype[50];///< linux内核类型
u_char  ntg_linux_kern_osrelease[50];///< linux发布版本

int     ntg_linux_rtsig_max;///< 实时信号对大值


/**
 * 操作系统特性初始化
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 * @note
 *  初始化的属性:
 *  	ntg_linux_kern_ostype
 *  	ntg_linux_kern_osrelease
 *  	ntg_os_io
 */
ntg_int_t
ntg_os_specific_init(ntg_log_t *log)
{
    struct utsname  u;

    if (uname(&u) == -1) {
        ntg_log_error(NTG_LOG_ALERT, log, ntg_errno, "uname() failed");
        return NTG_ERROR;
    }

    (void) ntg_cpystrn(ntg_linux_kern_ostype, (u_char *) u.sysname,
                       sizeof(ntg_linux_kern_ostype));

    (void) ntg_cpystrn(ntg_linux_kern_osrelease, (u_char *) u.release,
                       sizeof(ntg_linux_kern_osrelease));


    return NTG_OK;
}

/**
 *
 * @param log
 */
void
ntg_os_specific_status(ntg_log_t *log)
{
    ntg_log_error(NTG_LOG_NOTICE, log, 0, "OS: %s %s",
                  ntg_linux_kern_ostype, ntg_linux_kern_osrelease);

#if (NTG_HAVE_RTSIG)
    ntg_log_error(NTG_LOG_NOTICE, log, 0, "sysctl(KERN_RTSIGMAX): %d",
                  ntg_linux_rtsig_max);
#endif
}
