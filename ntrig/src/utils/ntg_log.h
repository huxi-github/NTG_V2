/*
 * ntg_log.h
 *
 *  Created on: Aug 26, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_LOG_H_
#define UTILS_NTG_LOG_H_


#include "ntg_errno.h"
#include "ntg_string.h"
#include "ntg_files.h"
//#include "ntg_atomic.h"

#include "../ntg_config.h"
#include "../ntg_core.h"

/**
 * @name 日志等级
 * @details 等级越小表示的日志级别越高
 * @{
 */
#define NTG_LOG_STDERR            0///< 标准错误
#define NTG_LOG_EMERG             1///< 紧急错误
#define NTG_LOG_ALERT             2///< 严重错误
#define NTG_LOG_CRIT              3///< 关键错误
#define NTG_LOG_ERR               4///< 一般错误
#define NTG_LOG_WARN              5///< 警告错误
#define NTG_LOG_NOTICE            6///< 提醒错误
#define NTG_LOG_INFO              7///< 一般信息
#define NTG_LOG_DEBUG             8///< 调试信息
/**@}*/
/**
 * @name 调试日志等级
 */
#define NTG_LOG_DEBUG_CORE        0x010///< 核心模块
#define NTG_LOG_DEBUG_ALLOC       0x020///< 内存分配
#define NTG_LOG_DEBUG_MUTEX       0x040///< 锁
#define NTG_LOG_DEBUG_EVENT       0x080///< 事件模块
#define NTG_LOG_DEBUG_HTTP        0x100///< http模块
#define NTG_LOG_DEBUG_MAIL        0x200///< 邮件模块
#define NTG_LOG_DEBUG_MYSQL       0x400///< 数据库

#define NTG_LOG_DEBUG_DISCARD     0x800
/**@}*/

/*
 * do not forget to update debug_levels[] in src/core/NTG_log.c
 * after the adding a new debug level
 */

#define NTG_LOG_DEBUG_FIRST       NTG_LOG_DEBUG_CORE
#define NTG_LOG_DEBUG_LAST        NTG_LOG_DEBUG_DISCARD
#define NTG_LOG_DEBUG_CONNECTION  0x80000000
#define NTG_LOG_DEBUG_ALL         0x7ffffff0


typedef u_char *(*ntg_log_handler_pt) (ntg_log_t *log, u_char *buf, size_t len);
typedef void (*ntg_log_writer_pt) (ntg_log_t *log, ntg_uint_t level,
    u_char *buf, size_t len);
/**
 * 日志对象
 */
struct ntg_log_s {
    ntg_uint_t           log_level;///< 日志水平
    ntg_open_file_t     *file;///< 打开的文件对象

//    ntg_atomic_uint_t    connection;///< TODO

    time_t               disk_full_time;///< TODO

    ntg_log_handler_pt   handler;///< 处理函数
    void                *data;///< 传递给处理函数的数据

    ntg_log_writer_pt    writer;///< TODO
    void                *wdata;///< TODO

    /*
     * we declare "action" as "char *" because the actions are usually
     * the static strings and in the "u_char *" case we have to override
     * their types all the time
     */

    char                *action;

    ntg_log_t           *next;///< 下一个日志对象
};

//#define NTG_ERROR_LOG_PATH 	"logs/error.log"
#define NTG_MAX_ERROR_STR   2048  //最大的error串长度


/*********************************/

#if (NTG_HAVE_C99_VARIADIC_MACROS)

#define NTG_HAVE_VARIADIC_MACROS  1

#define ntg_log_error(level, log, ...)                                        \
    if ((log)->log_level >= level) ntg_log_error_core(level, log, __VA_ARGS__)

void ntg_log_error_core(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, ...);

#define ntg_log_debug(level, log, ...)                                        \
    if ((log)->log_level & level)                                             \
        ntg_log_error_core(ntg_LOG_DEBUG, log, __VA_ARGS__)

/*********************************/

#elif (NTG_HAVE_GCC_VARIADIC_MACROS)

#define NTG_HAVE_VARIADIC_MACROS  1

#define ntg_log_error(level, log, args...)                                    \
    if ((log)->log_level >= level) ntg_log_error_core(level, log, args)

void ntg_log_error_core(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, ...);
//TODO & 改为 >
#define ntg_log_debug(level, log, args...)                                    \
    if ((log)->log_level & level)                                             \
        ntg_log_error_core(NTG_LOG_DEBUG, log, args)

/*********************************/

#else /* NO VARIADIC MACROS */

#define NTG_HAVE_VARIADIC_MACROS  0

void  ntg_log_error(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, ...);
void ntg_log_error_core(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, va_list args);
void  ntg_log_debug_core(ntg_log_t *log, ntg_err_t err,
    const char *fmt, ...);


#endif /* VARIADIC MACROS */


/*********************************/

#if (NTG_DEBUG)

#if (NTG_HAVE_VARIADIC_MACROS)

#define ntg_log_debug0(level, log, err, fmt)                                  \
        ntg_log_debug(level, log, err, fmt)

#define ntg_log_debug1(level, log, err, fmt, arg1)                            \
        ntg_log_debug(level, log, err, fmt, arg1)

#define ntg_log_debug2(level, log, err, fmt, arg1, arg2)                      \
        ntg_log_debug(level, log, err, fmt, arg1, arg2)

#define ntg_log_debug3(level, log, err, fmt, arg1, arg2, arg3)                \
        ntg_log_debug(level, log, err, fmt, arg1, arg2, arg3)

#define ntg_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)          \
        ntg_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4)

#define ntg_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)    \
        ntg_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)

#define ntg_log_debug6(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6)                    \
        ntg_log_debug(level, log, err, fmt,                                   \
                       arg1, arg2, arg3, arg4, arg5, arg6)

#define ntg_log_debug7(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)              \
        ntg_log_debug(level, log, err, fmt,                                   \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define ntg_log_debug8(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)        \
        ntg_log_debug(level, log, err, fmt,                                   \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)


#else /* NO VARIADIC MACROS */

#define ntg_log_debug0(level, log, err, fmt)                                  \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt)

#define ntg_log_debug1(level, log, err, fmt, arg1)                            \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt, arg1)

#define ntg_log_debug2(level, log, err, fmt, arg1, arg2)                      \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt, arg1, arg2)

#define ntg_log_debug3(level, log, err, fmt, arg1, arg2, arg3)                \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt, arg1, arg2, arg3)

#define ntg_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)          \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4)

#define ntg_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)    \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5)

#define ntg_log_debug6(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6)                    \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)

#define ntg_log_debug7(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)              \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt,                                     \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define ntg_log_debug8(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)        \
    if ((log)->log_level & level)                                             \
        ntg_log_debug_core(log, err, fmt,                                     \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)

#endif

#else /* NO NTG_DEBUG */

#define ntg_log_debug0(level, log, err, fmt)
#define ntg_log_debug1(level, log, err, fmt, arg1)
#define ntg_log_debug2(level, log, err, fmt, arg1, arg2)
#define ntg_log_debug3(level, log, err, fmt, arg1, arg2, arg3)
#define ntg_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)
#define ntg_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)
#define ntg_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
#define ntg_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5,    \
                       arg6, arg7)
#define ntg_log_debug8(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5,    \
                       arg6, arg7, arg8)

#endif

//TODO 与1.0.8有不同的地方
/*********************************/

ntg_log_t *ntg_log_init(u_char *prefix);
void  ntg_log_abort(ntg_err_t err, const char *fmt, ...);
void  ntg_log_stderr(ntg_err_t err, const char *fmt, ...);
u_char *ntg_log_errno(u_char *buf, u_char *last, ntg_err_t err);
ntg_int_t ntg_log_open_default(ntg_cycle_t *cycle);
ntg_int_t ntg_log_redirect_stderr(ntg_cycle_t *cycle);
ntg_log_t *ntg_log_get_file_log(ntg_log_t *head);
char *ntg_log_set_log(ntg_conf_t *cf, ntg_log_t **head);


/*
 * ntg_write_stderr() cannot be implemented as macro, since
 * MSVC does not allow to use #ifdef inside macro parameters.
 *
 * ntg_write_fd() is used instead of ntg_write_console(), since
 * CharToOemBuff() inside ntg_write_console() cannot be used with
 * read only buffer as destination and CharToOemBuff() is not needed
 * for ntg_write_stderr() anyway.
 */
static inline void ntg_write_stderr(char *text){
    ntg_write_fd(ntg_stderr, text, strlen(text));
}


//extern ntg_module_t  ntg_errlog_module;
extern ntg_uint_t    ntg_use_stderr;

#endif /* UTILS_NTG_LOG_H_ */
