/**
 * @file 	ntg_daemon.c
 * @brief	将进程设置为守护进程
 * @details
 * @author	tzh
 * @date	Nov 9, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

/**
 * 设置守护进程
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t
ntg_daemon(ntg_log_t *log)
{
    int  fd;

    //1) 父进程直接退出,子进程成为孤儿进程
    switch (fork()) {
    case -1:
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "fork() failed");
        return NTG_ERROR;

    case 0:
        break;

    default:
        exit(0);
    }
    //2) 获取进程id
    ntg_pid = ntg_getpid();
    //3) 设置会话leader,并创建以新的组
    if (setsid() == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "setsid() failed");
        return NTG_ERROR;
    }
    //4) 设置进程屏蔽字
    umask(0);

    //5) 将标准输入输出定向到null文件
    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                      "open(\"/dev/null\") failed");
        return NTG_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "dup2(STDIN) failed");
        return NTG_ERROR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "dup2(STDOUT) failed");
        return NTG_ERROR;
    }

#if 0
    if (dup2(fd, STDERR_FILENO) == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "dup2(STDERR) failed");
        return NTG_ERROR;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "close() failed");
            return NTG_ERROR;
        }
    }

    return NTG_OK;
}
