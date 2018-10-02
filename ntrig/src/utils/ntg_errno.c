/*
 * ntg_errno.c
 *
 *  Created on: Jul 26, 2015
 *      Author: tzh
 */
#include "../ntg_types.h"
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_errno.h"
//#include "ntg_string.h"








/*
 * The strerror() messages are copied because:
 *
 * 1) strerror() and strerror_r() functions are not Async-Signal-Safe,
 *    therefore, they cannot be used in signal handlers;
 *
 * 2) a direct sys_errlist[] array may be used instead of these functions,
 *    but Linux linker warns about its usage:
 *
 * warning: `sys_errlist' is deprecated; use `strerror' or `strerror_r' instead
 * warning: `sys_nerr' is deprecated; use `strerror' or `strerror_r' instead
 *
 *    causing false bug reports.
 */


static ntg_str_t  *ntg_sys_errlist;
static ntg_str_t   ntg_unknown_error = ntg_string("Unknown error");

/*
 * ntg_strerror 用于获取err对应的字符串描述
 * err 			错误号
 * errstr		错误描述串
 * size			描述串大小
 * 返回下一个errstr可用指针
 */
u_char * ntg_strerror(ntg_err_t err, u_char *errstr, size_t size)
{
    ntg_str_t  *msg;

    msg = ((ntg_uint_t) err < NTG_SYS_NERR) ? &ntg_sys_errlist[err]:
                                              &ntg_unknown_error;
    size = ntg_min(size, msg->len);

    return ntg_cpymem(errstr, msg->data, size);
}

/**
 * @brief 初始化sys_errlist
 * @details
 * 构建一个err描述表,以后出现某个错误时,就可以直接获取
 * NTG_SYS_NERROR 是系统的最大错误号
 * @return 成功返回NTG_OK, 否则返回 NTG_ERROR
 * @ref ntg_errno.h
 */
ntg_uint_t ntg_strerror_init(void){
    char       *msg;
    u_char     *p;
    size_t      len;
    ntg_err_t   err;

    /*
     * ntg_strerror() is not ready to work at this stage, therefore,
     * malloc() is used and possible errors are logged using strerror().
     */
    //1)为ntg_sys_errlist列表分配内存
    len = NTG_SYS_NERR * sizeof(ntg_str_t);

    ntg_sys_errlist = malloc(len);
    if (ntg_sys_errlist == NULL) {
        goto failed;
    }
    //2)初始化每个列表元素
    for (err = 0; err < NTG_SYS_NERR; err++) {
        msg = strerror(err);
        len = ntg_strlen(msg);

        p = malloc(len);
        if (p == NULL) {
            goto failed;
        }

        ntg_memcpy(p, msg, len);
        ntg_sys_errlist[err].len = len;
        ntg_sys_errlist[err].data = p;
    }

    return NTG_OK;

failed:
	//失败进行日志记录
    err = errno;
    ntg_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));

    return NTG_ERROR;
}
