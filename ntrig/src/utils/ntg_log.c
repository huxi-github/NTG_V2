/*
 * ntg_log.c
 *
 *  Created on: Aug 26, 2015
 *      Author: tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_string.h"
#include "ntg_log.h"
#include "ntg_cycle.h"
#include "ntg_conf_file.h"
#include "ntg_files.h"
#include "ntg_times.h"
#include "ntg_parse.h"


static char *ntg_error_log(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_log_set_levels(ntg_conf_t *cf, ntg_log_t *log);
static void ntg_log_insert(ntg_log_t *log, ntg_log_t *new_log);


#if (NTG_DEBUG)

static void ntg_log_memory_writer(ntg_log_t *log, ntg_uint_t level,
    u_char *buf, size_t len);
static void ntg_log_memory_cleanup(void *data);


typedef struct {
    u_char        *start;
    u_char        *end;
    u_char        *pos;
    ntg_atomic_t   written;
} ntg_log_memory_buf_t;

#endif


static ntg_command_t  ntg_errlog_commands[] = {

    {ntg_string("error_log"),
     NTG_MAIN_CONF|NTG_CONF_1MORE,
     ntg_error_log,
     0,
     0,
     NULL},

    ntg_null_command
};


static ntg_core_module_t  ntg_errlog_module_ctx = {
    ntg_string("errlog"),
    NULL,
    NULL
};


ntg_module_t  ntg_errlog_module = {
    NTG_MODULE_V1,
    &ntg_errlog_module_ctx,                /* module context */
    ntg_errlog_commands,                   /* module directives */
    NTG_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NTG_MODULE_V1_PADDING
};


static ntg_log_t        ntg_log;///<日志对象
static ntg_open_file_t  ntg_log_file;///< 日志文件对象
ntg_uint_t              ntg_use_stderr = 1;///< 标准错误使用标志


static ntg_str_t err_levels[] = {
    ntg_null_string,
    ntg_string("emerg"),
    ntg_string("alert"),
    ntg_string("crit"),
    ntg_string("error"),
    ntg_string("warn"),
    ntg_string("notice"),
    ntg_string("info"),
    ntg_string("debug")
};

static const char *debug_levels[] = {
    "debug_core",
    "debug_alloc",
    "debug_mutex",
    "debug_event",
    "debug_http",
    "debug_mail",
    "debug_mysql",
    "debug_discard"
};


#if (NTG_HAVE_VARIADIC_MACROS)
/**
 *
 * @param level
 * @param log
 * @param err
 * @param fmt
 * @param args
 */
void
ntg_log_error_core(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, ...)

#else

void
ntg_log_error_core(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, va_list args)

#endif
{
#if (NTG_HAVE_VARIADIC_MACROS)
    va_list      args;
#endif
    u_char      *p, *last, *msg;
    ssize_t      n;
    ntg_uint_t   wrote_stderr, debug_connection;
    u_char       errstr[NTG_MAX_ERROR_STR];

    last = errstr + NTG_MAX_ERROR_STR;

    p = ntg_cpymem(errstr, ntg_cached_err_log_time.data,
                   ntg_cached_err_log_time.len);

    p = ntg_slprintf(p, last, " [%V] ", &err_levels[level]);

    /* pid#tid */
    p = ntg_slprintf(p, last, "%P: ",ntg_log_pid);

//    if (log->connection) {
//        p = ntg_slprintf(p, last, "*%uA ", log->connection);
//    }

    msg = p;

#if (NTG_HAVE_VARIADIC_MACROS)

    va_start(args, fmt);
    p = ntg_vslprintf(p, last, fmt, args);
    va_end(args);

#else

    p = ntg_vslprintf(p, last, fmt, args);

#endif

    if (err) {
        p = ntg_log_errno(p, last, err);
    }

    if (level != NTG_LOG_DEBUG && log->handler) {
        p = log->handler(log, p, last - p);
    }

    if (p > last - NTG_LINEFEED_SIZE) {
        p = last - NTG_LINEFEED_SIZE;
    }

    ntg_linefeed(p);

    wrote_stderr = 0;
    debug_connection = (log->log_level & NTG_LOG_DEBUG_CONNECTION) != 0;

    while (log) {

        if (log->log_level < level && !debug_connection) {
            break;
        }

        if (log->writer) {
            log->writer(log, level, errstr, p - errstr);
            goto next;
        }

        if (ntg_time() == log->disk_full_time) {

            /*
             * on FreeBSD writing to a full filesystem with enabled softupdates
             * may block process for much longer time than writing to non-full
             * filesystem, so we skip writing to a log for one second
             */

            goto next;
        }

        n = ntg_write_fd(log->file->fd, errstr, p - errstr);

        if (n == -1 && ntg_errno == NTG_ENOSPC) {
            log->disk_full_time = ntg_time();
        }

        if (log->file->fd == ntg_stderr) {
            wrote_stderr = 1;
        }

    next:

        log = log->next;
    }

    if (!ntg_use_stderr
        || level > NTG_LOG_WARN
        || wrote_stderr)
    {
        return;
    }

    msg -= (7 + err_levels[level].len + 3);

    (void) ntg_sprintf(msg, "ntrig: [%V] ", &err_levels[level]);

    (void) ntg_write_console(ntg_stderr, msg, p - msg);
}


#if !(NTG_HAVE_VARIADIC_MACROS)

void ntg_cdecl
ntg_log_error(ntg_uint_t level, ntg_log_t *log, ntg_err_t err,
    const char *fmt, ...)
{
    va_list  args;

    if (log->log_level >= level) {
        va_start(args, fmt);
        ntg_log_error_core(level, log, err, fmt, args);
        va_end(args);
    }
}


void ntg_cdecl
ntg_log_debug_core(ntg_log_t *log, ntg_err_t err, const char *fmt, ...)
{
    va_list  args;

    va_start(args, fmt);
    ntg_log_error_core(NTG_LOG_DEBUG, log, err, fmt, args);
    va_end(args);
}

#endif


void
ntg_log_abort(ntg_err_t err, const char *fmt, ...)
{
    u_char   *p;
    va_list   args;
    u_char    errstr[NTG_MAX_CONF_ERRSTR];

    va_start(args, fmt);
    p = ntg_vsnprintf(errstr, sizeof(errstr) - 1, fmt, args);
    va_end(args);

    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, err,
                  "%*s", p - errstr, errstr);
}


/**
 * 标准错误输出
 * @details 用于log模块还没初始化情况下错误的输出,其实是将错误输出到终端
 * @param err 错误类型
 * @param fmt 格式化字符串
 * @return 无
 * @note NTG_MAX_ERROR_STR为最大错误描述输出长度
 */
void
ntg_log_stderr(ntg_err_t err, const char *fmt, ...)
{
    u_char   *p, *last;
    va_list   args;
    u_char    errstr[NTG_MAX_ERROR_STR];

    last = errstr + NTG_MAX_ERROR_STR;

    p = ntg_cpymem(errstr, "ntrig: ", 7);

    va_start(args, fmt);
    p = ntg_vslprintf(p, last, fmt, args);//格式化处理
    va_end(args);

    if (err) {
        p = ntg_log_errno(p, last, err);//输出编号
    }

    if (p > last - NTG_LINEFEED_SIZE) {//溢出处理
        p = last - NTG_LINEFEED_SIZE;
    }

    ntg_linefeed(p);//添加换行符

    (void) ntg_write_console(ntg_stderr, errstr, p - errstr);//标准错误输出
}

/**
 * 输出错误码输出到buf中
 * @param[in] buf 输出缓冲区位置
 * @param[in] last 缓冲区结束位置
 * @param[in] err 错误编号
 * @return 下一个可用位置
 */
u_char *
ntg_log_errno(u_char *buf, u_char *last, ntg_err_t err)
{
    if (buf > last - 50) {//为error码的完整输出预留空间

        /* leave a space for an error code */

        buf = last - 50;
        *buf++ = '.';
        *buf++ = '.';
        *buf++ = '.';
    }

#if (NTG_WIN32)
    buf = ntg_slprintf(buf, last, ((unsigned) err < 0x80000000)
                                       ? " (%d: " : " (%Xd: ", err);
#else
    buf = ntg_slprintf(buf, last, " (%d: ", err);
#endif

    buf = ntg_strerror(err, buf, last - buf);

    if (buf < last) {
        *buf++ = ')';
    }

    return buf;
}

/**
 * 日志模块初始化
 * @param prefix 路经前缀
 * @return 日志对象指针
 * @note 这里打开了默认error日志文件,打开成功fd指向日志文件,否则指向标准错误输出
 *
 */
ntg_log_t *
ntg_log_init(u_char *prefix)
{
    u_char  *p, *name;
    size_t   nlen, plen;

    ntg_log.file = &ntg_log_file;
    ntg_log.log_level = NTG_LOG_NOTICE;
//    ntg_log.log_level = NTG_LOG_DEBUG;

    name = (u_char *) NTG_ERROR_LOG_PATH;

    /*
     * we use ntg_strlen() here since BCC warns about
     * condition is always false and unreachable code
     */

    nlen = ntg_strlen(name);

    if (nlen == 0) {//没有配置错误日志文件
        ntg_log_file.fd = ntg_stderr;
        return &ntg_log;
    }

    p = NULL;

    //获取绝对路经
    if (name[0] != '/') {
    	//使用的是相对路经
        if (prefix) {
            plen = ntg_strlen(prefix);

        } else {
#ifdef NTG_PREFIX
            prefix = (u_char *) NTG_PREFIX;
            plen = ntg_strlen(prefix);
#else
            plen = 0;
#endif
        }

        if (plen) {
            name = malloc(plen + nlen + 2);
            if (name == NULL) {
                return NULL;
            }

            p = ntg_cpymem(name, prefix, plen);

            if (!ntg_path_separator(*(p - 1))) {
                *p++ = '/';
            }

            ntg_cpystrn(p, (u_char *) NTG_ERROR_LOG_PATH, nlen + 1);

            p = name;
        }
    }
//    printf("%s\n", name);
    ntg_log_file.fd = ntg_open_file(name, NTG_FILE_APPEND,
                                    NTG_FILE_CREATE_OR_OPEN,
                                    NTG_FILE_DEFAULT_ACCESS);

    if (ntg_log_file.fd == NTG_INVALID_FILE) {
        ntg_log_stderr(ntg_errno,
                       "[alert] could not open error log file: "
                       ntg_open_file_n " \"%s\" failed", name);
        ntg_log_file.fd = ntg_stderr;//指定错误日志文件打开失败,则将其设置为标准错误输出
    }

    if (p) {
        ntg_free(p);
    }

    return &ntg_log;
}

/**
 * 打开一个新的日志对象
 * @param[in] cycle 全局循环对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t
ntg_log_open_default(ntg_cycle_t *cycle)
{
    ntg_log_t         *log;
    static ntg_str_t   error_log = ntg_string(NTG_ERROR_LOG_PATH);
    //1) 检查是否已存在
    if (ntg_log_get_file_log(&cycle->new_log) != NULL) {//已经存在一个带文件的日志
        return NTG_OK;
    }
    //2) 分配一个可用日志对象
    if (cycle->new_log.log_level != 0) {
        /* there are some error logs, but no files */

        log = ntg_pcalloc(cycle->pool, sizeof(ntg_log_t));
        if (log == NULL) {
            return NTG_ERROR;
        }

    } else {
        /* no error logs at all */
        log = &cycle->new_log;
    }

    log->log_level = NTG_LOG_ERR;

    log->file = ntg_conf_open_file(cycle, &error_log);
    if (log->file == NULL) {
        return NTG_ERROR;
    }

    if (log != &cycle->new_log) {
        ntg_log_insert(&cycle->new_log, log);//插入到日志链表中
    }

    return NTG_OK;
}

/**
 * 重定向标准错误到日志fd
 * @param[in] cycle 全局循环体
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 将标准错误输出定向到指定的错误日志文件中
 */
ntg_int_t
ntg_log_redirect_stderr(ntg_cycle_t *cycle)
{
    ntg_fd_t  fd;

    if (cycle->log_use_stderr) {//已定位
        return NTG_OK;
    }

    /* file log always exists when we are called */
    fd = ntg_log_get_file_log(cycle->log)->file->fd;

    if (fd != ntg_stderr) {
        if (ntg_set_stderr(fd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          ntg_set_stderr_n " failed");

            return NTG_ERROR;
        }
    }

    return NTG_OK;
}

/**
 * 获取一个带文件的日志对象
 * @param[in] head 日志对象链表头
 * @return 成功返回ntg_log_t对象,否则返回NULL
 */
ntg_log_t *
ntg_log_get_file_log(ntg_log_t *head)
{
    ntg_log_t  *log;

    for (log = head; log; log = log->next) {
        if (log->file != NULL) {
            return log;
        }
    }

    return NULL;
}


static char *
ntg_log_set_levels(ntg_conf_t *cf, ntg_log_t *log)
{
    ntg_uint_t   i, n, d, found;
    ntg_str_t   *value;

    if (cf->args->nelts == 2) {
        log->log_level = NTG_LOG_ERR;
        return NTG_CONF_OK;
    }

    value = cf->args->elts;

    for (i = 2; i < cf->args->nelts; i++) {
        found = 0;

        for (n = 1; n <= NTG_LOG_DEBUG; n++) {
            if (ntg_strcmp(value[i].data, err_levels[n].data) == 0) {

                if (log->log_level != 0) {
                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                       "duplicate log level \"%V\"",
                                       &value[i]);
                    return NTG_CONF_ERROR;
                }

                log->log_level = n;
                found = 1;
                break;
            }
        }

        for (n = 0, d = NTG_LOG_DEBUG_FIRST; d <= NTG_LOG_DEBUG_LAST; d <<= 1) {
            if (ntg_strcmp(value[i].data, debug_levels[n++]) == 0) {
                if (log->log_level & ~NTG_LOG_DEBUG_ALL) {
                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                       "invalid log level \"%V\"",
                                       &value[i]);
                    return NTG_CONF_ERROR;
                }

                log->log_level |= d;
                found = 1;
                break;
            }
        }


        if (!found) {
            ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                               "invalid log level \"%V\"", &value[i]);
            return NTG_CONF_ERROR;
        }

        ntg_conf_log_error(NTG_LOG_NOTICE, cf, 0,
                           "level \"%V\"", &value[i]);
    }

    if (log->log_level == NTG_LOG_DEBUG) {
        log->log_level = NTG_LOG_DEBUG_ALL;
    }

    return NTG_CONF_OK;
}


static char *
ntg_error_log(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    ntg_log_t  *dummy;

    dummy = &cf->cycle->new_log;

    return ntg_log_set_log(cf, &dummy);
}


char *
ntg_log_set_log(ntg_conf_t *cf, ntg_log_t **head)
{
    ntg_log_t          *new_log;
    ntg_str_t          *value, name;

    if (*head != NULL && (*head)->log_level == 0) {
        new_log = *head;

    } else {

        new_log = ntg_pcalloc(cf->pool, sizeof(ntg_log_t));
        if (new_log == NULL) {
            return NTG_CONF_ERROR;
        }

        if (*head == NULL) {
            *head = new_log;
        }
    }

    value = cf->args->elts;

    if (ntg_strcmp(value[1].data, "stderr") == 0) {
        ntg_str_null(&name);
        cf->cycle->log_use_stderr = 1;

        new_log->file = ntg_conf_open_file(cf->cycle, &name);
        if (new_log->file == NULL) {
            return NTG_CONF_ERROR;
        }

     } else if (ntg_strncmp(value[1].data, "memory:", 7) == 0) {

#if (NTG_DEBUG)
        size_t                 size, needed;
        ntg_pool_cleanup_t    *cln;
        ntg_log_memory_buf_t  *buf;

        value[1].len -= 7;
        value[1].data += 7;

        needed = sizeof("MEMLOG  :" NTG_LINEFEED)
                 + cf->conf_file->file.name.len
                 + NTG_SIZE_T_LEN
                 + NTG_INT_T_LEN
                 + NTG_MAX_ERROR_STR;

        size = ntg_parse_size(&value[1]);

        if (size == (size_t) NTG_ERROR || size < needed) {
            ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                               "invalid buffer size \"%V\"", &value[1]);
            return NTG_CONF_ERROR;
        }

        buf = ntg_palloc(cf->pool, sizeof(ntg_log_memory_buf_t));
        if (buf == NULL) {
            return NTG_CONF_ERROR;
        }

        buf->start = ntg_pnalloc(cf->pool, size);
        if (buf->start == NULL) {
            return NTG_CONF_ERROR;
        }

        buf->end = buf->start + size;

        buf->pos = ntg_slprintf(buf->start, buf->end, "MEMLOG %uz %V:%ui%N",
                                size, &cf->conf_file->file.name,
                                cf->conf_file->line);

        ntg_memset(buf->pos, ' ', buf->end - buf->pos);

        cln = ntg_pool_cleanup_add(cf->pool, 0);
        if (cln == NULL) {
            return NTG_CONF_ERROR;
        }

        cln->data = new_log;
        cln->handler = ntg_log_memory_cleanup;

        new_log->writer = ntg_log_memory_writer;
        new_log->wdata = buf;

#else
        ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                           "nginx was built without debug support");
        return NTG_CONF_ERROR;
#endif

    } else {
        new_log->file = ntg_conf_open_file(cf->cycle, &value[1]);
        if (new_log->file == NULL) {
            return NTG_CONF_ERROR;
        }
    }

    if (ntg_log_set_levels(cf, new_log) != NTG_CONF_OK) {
        return NTG_CONF_ERROR;
    }

    if (*head != new_log) {
        ntg_log_insert(*head, new_log);
    }

    return NTG_CONF_OK;
}


static void
ntg_log_insert(ntg_log_t *log, ntg_log_t *new_log)
{
    ntg_log_t  tmp;

    if (new_log->log_level > log->log_level) {

        /*
         * list head address is permanent, insert new log after
         * head and swap its contents with head
         */

        tmp = *log;
        *log = *new_log;
        *new_log = tmp;

        log->next = new_log;
        return;
    }

    while (log->next) {
        if (new_log->log_level > log->next->log_level) {
            new_log->next = log->next;
            log->next = new_log;
            return;
        }

        log = log->next;
    }

    log->next = new_log;
}


#if (NTG_DEBUG)

static void
ntg_log_memory_writer(ntg_log_t *log, ntg_uint_t level, u_char *buf,
    size_t len)
{
    u_char                *p;
    size_t                 avail, written;
    ntg_log_memory_buf_t  *mem;

    mem = log->wdata;

    if (mem == NULL) {
        return;
    }

    written = ntg_atomic_fetch_add(&mem->written, len);

    p = mem->pos + written % (mem->end - mem->pos);

    avail = mem->end - p;

    if (avail >= len) {
        ntg_memcpy(p, buf, len);

    } else {
        ntg_memcpy(p, buf, avail);
        ntg_memcpy(mem->pos, buf + avail, len - avail);
    }
}


static void
ntg_log_memory_cleanup(void *data)
{
    ntg_log_t *log = data;

    ntg_log_debug0(NTG_LOG_DEBUG_CORE, log, 0, "destroy memory log buffer");

    log->wdata = NULL;
}

#endif
