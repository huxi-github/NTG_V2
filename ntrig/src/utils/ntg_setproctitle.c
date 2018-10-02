/*
 * ntg_setproctitle.c
 *
 *  Created on: Oct 19, 2015
 *      Author: tzh
 */


#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_setproctitle.h"
#include "ntg_log.h"

#if (NTG_SETPROCTITLE_USES_ENV)

/*
 * To change the process title in Linux and Solaris we have to set argv[1]
 * to NULL and to copy the title to the same place where the argv[0] points to.
 * However, argv[0] may be too small to hold a new title.  Fortunately, Linux
 * and Solaris store argv[] and environ[] one after another.  So we should
 * ensure that is the continuous memory and then we allocate the new memory
 * for environ[] and copy it.  After this we could use the memory starting
 * from argv[0] for our process title.
 *
 * The Solaris's standard /bin/ps does not show the changed process title.
 * You have to use "/usr/ucb/ps -w" instead.  Besides, the UCB ps does not
 * show a new title if its length less than the origin command line length.
 * To avoid it we append to a new title the origin command line in the
 * parenthesis.
 */

extern char **environ;

static char *ntg_os_argv_last;//ntg_os_argv的结尾处

/**
 * 初始化设置进程标题
 * @details 实现将环境变量拷贝到一个自己分配的连续的空间中
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 * @note argv[]和environ[]都是一个接一个连续存储的
 * @note 设置的参数ntg_os_argv_last指向的是进程命令行参数与环境变量存放区的结尾处
 */
ntg_int_t ntg_init_setproctitle(ntg_log_t *log)
{
    u_char      *p;
    size_t       size;
    ntg_uint_t   i;

    size = 0;
    //获取所有环境变量的长度
    for (i = 0; environ[i]; i++) {
        size += ntg_strlen(environ[i]) + 1;
    }
    //分配连续的空间
    p = ntg_alloc(size, log);
    if (p == NULL) {
        return NTG_ERROR;
    }
    //将ntg_os_argv_last指向argv[]最后一个参数的结尾,即为environ[]数组的开始处
    ntg_os_argv_last = ntg_os_argv[0];

    for (i = 0; ntg_os_argv[i]; i++) {
        if (ntg_os_argv_last == ntg_os_argv[i]) {
            ntg_os_argv_last = ntg_os_argv[i] + ntg_strlen(ntg_os_argv[i]) + 1;
        }
    }
    //重新设置环境变量表的指向
    for (i = 0; environ[i]; i++) {
        if (ntg_os_argv_last == environ[i]) {

            size = ntg_strlen(environ[i]) + 1;
            ntg_os_argv_last = environ[i] + size;

            ntg_cpystrn(p, (u_char *) environ[i], size);
            environ[i] = (char *) p;
            p += size;
        }
    }

    ntg_os_argv_last--;

    return NTG_OK;
}

/**
 * 设置进程标题,主要是改变了进程高地址的命令行参数区
 * @param[in] title 标题指针
 * @note 进程名称和所有的输入参数
 */
void
ntg_setproctitle(char *title)
{
    u_char     *p;

#if (NTG_SOLARIS)

    ntg_int_t   i;
    size_t      size;

#endif

    ntg_os_argv[1] = NULL;

    p = ntg_cpystrn((u_char *) ntg_os_argv[0], (u_char *) "ntrig: ",
                    ntg_os_argv_last - ntg_os_argv[0]);

    p = ntg_cpystrn(p, (u_char *) title, ntg_os_argv_last - (char *) p);

#if (NTG_SOLARIS)

    size = 0;

    for (i = 0; i < ntg_argc; i++) {
        size += ntg_strlen(ntg_argv[i]) + 1;
    }

    if (size > (size_t) ((char *) p - ntg_os_argv[0])) {

        /*
         * ntg_setproctitle() is too rare operation so we use
         * the non-optimized copies
         */

        p = ntg_cpystrn(p, (u_char *) " (", ntg_os_argv_last - (char *) p);

        for (i = 0; i < ntg_argc; i++) {
            p = ntg_cpystrn(p, (u_char *) ntg_argv[i],
                            ntg_os_argv_last - (char *) p);
            p = ntg_cpystrn(p, (u_char *) " ", ntg_os_argv_last - (char *) p);
        }

        if (*(p - 1) == ' ') {
            *(p - 1) = ')';
        }
    }

#endif

    if (ntg_os_argv_last - (char *) p) {
        ntg_memset(p, NTG_SETPROCTITLE_PAD, ntg_os_argv_last - (char *) p);
    }

    ntg_log_debug1(NTG_LOG_DEBUG_CORE, ntg_cycle->log, 0,
                   "setproctitle: \"%s\"", ntg_os_argv[0]);
}

#endif /* NTG_SETPROCTITLE_USES_ENV */
