/*
 * ntg_cycle.c
 *
 *  Created on: Aug 28, 2015
 *      Author: tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_cycle.h"
#include "ntg_process_cycle.h"
#include "ntg_inet.h"
#include "ntg_file.h"
#include "ntg_times.h"

static void ntg_destroy_cycle_pools(ntg_conf_t *conf);
static ntg_int_t ntg_test_lockfile(u_char *file, ntg_log_t *log);


volatile ntg_cycle_t  *ntg_cycle;///< 保存当前的循环体
ntg_array_t            ntg_old_cycles;

static ntg_pool_t     *ntg_temp_pool;
//static ntg_event_t     ntg_cleaner_event;

ntg_uint_t             ntg_test_config;///< 测试配置
ntg_uint_t             ntg_quiet_mode;///< 安静模式


/**
 * 初始化循环体
 * @param[in] old_cycle 老的循环体
 * @return 成功返回新的循环体,否则返回NULL
 * @note 完成了所有监听套接字的事件监听
 */
ntg_cycle_t *
ntg_init_cycle(ntg_cycle_t *old_cycle)
{
    void                *rv;
    char               **senv, **env;
    ntg_uint_t           i, n;
    ntg_log_t           *log;
    ntg_time_t          *tp;
    ntg_conf_t           conf;
    ntg_pool_t          *pool;
    ntg_cycle_t         *cycle, **old;
    ntg_list_part_t     *part;
    ntg_open_file_t     *file;

    ntg_core_conf_t     *ccf, *old_ccf;
    ntg_core_module_t   *module;
    char                 hostname[NTG_MAXHOSTNAMELEN];

    ntg_timezone_update();

    /* force localtime update with a new timezone */
    //获取之前的缓存时间
    tp = ntg_timeofday();
    tp->sec = 0;
    //更新缓存时间
    ntg_time_update();

    //初始化所有容器
    log = old_cycle->log;

    pool = ntg_create_pool(NTG_CYCLE_POOL_SIZE, log);
    if (pool == NULL) {
        return NULL;
    }
    pool->log = log;

    cycle = ntg_pcalloc(pool, sizeof(ntg_cycle_t));
    if (cycle == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    cycle->pool = pool;
    cycle->log = log;
    cycle->old_cycle = old_cycle;

    cycle->conf_prefix.len = old_cycle->conf_prefix.len;
    cycle->conf_prefix.data = ntg_pstrdup(pool, &old_cycle->conf_prefix);
    if (cycle->conf_prefix.data == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    cycle->prefix.len = old_cycle->prefix.len;
    cycle->prefix.data = ntg_pstrdup(pool, &old_cycle->prefix);
    if (cycle->prefix.data == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    cycle->conf_file.len = old_cycle->conf_file.len;
    cycle->conf_file.data = ntg_pnalloc(pool, old_cycle->conf_file.len + 1);
    if (cycle->conf_file.data == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }
    ntg_cpystrn(cycle->conf_file.data, old_cycle->conf_file.data,
                old_cycle->conf_file.len + 1);

    cycle->conf_param.len = old_cycle->conf_param.len;
    cycle->conf_param.data = ntg_pstrdup(pool, &old_cycle->conf_param);
    if (cycle->conf_param.data == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    //路经数组初始化
    n = old_cycle->paths.nelts ? old_cycle->paths.nelts : 10;

    cycle->paths.elts = ntg_pcalloc(pool, n * sizeof(ntg_path_t *));
    if (cycle->paths.elts == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    cycle->paths.nelts = 0;
    cycle->paths.size = sizeof(ntg_path_t *);
    cycle->paths.nalloc = n;
    cycle->paths.pool = pool;

    //打开的文件链表初始化
    if (old_cycle->open_files.part.nelts) {
        n = old_cycle->open_files.part.nelts;
        for (part = old_cycle->open_files.part.next; part; part = part->next) {
            n += part->nelts;
        }

    } else {
        n = 20;
    }

    if (ntg_list_init(&cycle->open_files, pool, n, sizeof(ntg_open_file_t))
        != NTG_OK)
    {
        ntg_destroy_pool(pool);
        return NULL;
    }

    //共享内存链表初始化

    n = 1;



    //可重复使用链接队列初始化
    //ntg_queue_init(&cycle->reusable_connections_queue);


    cycle->conf_ctx = ntg_pcalloc(pool, ntg_max_module * sizeof(void *));
    if (cycle->conf_ctx == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }


    if (gethostname(hostname, NTG_MAXHOSTNAMELEN) == -1) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno, "gethostname() failed");
        ntg_destroy_pool(pool);
        return NULL;
    }

    /* on Linux gethostname() silently truncates name that does not fit */

    hostname[NTG_MAXHOSTNAMELEN - 1] = '\0';//在截短情况下
    cycle->hostname.len = ntg_strlen(hostname);

    cycle->hostname.data = ntg_pnalloc(pool, cycle->hostname.len);
    if (cycle->hostname.data == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    ntg_strlow(cycle->hostname.data, (u_char *) hostname, cycle->hostname.len);

//    printf("调用所有核心模块的创建配置函数 \n");

    //调用所有核心模块的创建配置函数
    for (i = 0; ntg_modules[i]; i++) {
        if (ntg_modules[i]->type != NTG_CORE_MODULE) {
            continue;
        }

        module = ntg_modules[i]->ctx;

        if (module->create_conf) {
            rv = module->create_conf(cycle);//调用模块的配置函数,其实只有core_module,ngx_regex_module有这个函数
            if (rv == NULL) {
                ntg_destroy_pool(pool);

                return NULL;
            }
            cycle->conf_ctx[ntg_modules[i]->index] = rv;
        }
    }


    senv = environ;

    //配置对象ntg_conf_t初始化
    ntg_memzero(&conf, sizeof(ntg_conf_t));
    /* STUB: init array ? */
    conf.args = ntg_array_create(pool, 10, sizeof(ntg_str_t));
    if (conf.args == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }

    conf.temp_pool = ntg_create_pool(NTG_CYCLE_POOL_SIZE, log);
    if (conf.temp_pool == NULL) {
        ntg_destroy_pool(pool);
        return NULL;
    }


    conf.ctx = cycle->conf_ctx;
    conf.cycle = cycle;
    conf.pool = pool;
    conf.log = log;
    conf.module_type = NTG_CORE_MODULE;//核心模块
    conf.cmd_type = NTG_MAIN_CONF;//仅对顶配置处理


    //解析参数-g的配置
    if (ntg_conf_param(&conf) != NTG_CONF_OK) {
        environ = senv;
        ntg_destroy_cycle_pools(&conf);
        return NULL;
    }

    if (ntg_conf_parse(&conf, &cycle->conf_file) != NTG_CONF_OK) {
        environ = senv;
        ntg_destroy_cycle_pools(&conf);
        return NULL;
    }

    if (ntg_test_config && !ntg_quiet_mode) {
        ntg_log_stderr(0, "the configuration file %s syntax is ok",
                       cycle->conf_file.data);
    }

    //调用所有核心模块的初始化配置函数
    for (i = 0; ntg_modules[i]; i++) {
        if (ntg_modules[i]->type != NTG_CORE_MODULE) {
            continue;
        }

        module = ntg_modules[i]->ctx;

        if (module->init_conf) {
            if (module->init_conf(cycle, cycle->conf_ctx[ntg_modules[i]->index])
                == NTG_CONF_ERROR)
            {
                environ = senv;
                ntg_destroy_cycle_pools(&conf);
                return NULL;
            }
        }
    }

    if (ntg_process == NTG_PROCESS_SIGNALLER) {//TODO 信号处理进程
        return cycle;
    }

    ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);


    if (ntg_test_config) {

        if (ntg_create_pidfile(&ccf->pid, log) != NTG_OK) {
            goto failed;
        }

    } else if (!ntg_is_init_cycle(old_cycle)) {

        /*
         * we do not create the pid file in the first ntg_init_cycle() call
         * because we need to write the demonized process pid
         */

        old_ccf = (ntg_core_conf_t *) ntg_get_conf(old_cycle->conf_ctx,
                                                   ntg_core_module);
        if (ccf->pid.len != old_ccf->pid.len
            || ntg_strcmp(ccf->pid.data, old_ccf->pid.data) != 0)
        {
            /* new pid file name */

            if (ntg_create_pidfile(&ccf->pid, log) != NTG_OK) {
                goto failed;
            }

            ntg_delete_pidfile(old_cycle);
        }
    }

    //测试锁文件
    if (ntg_test_lockfile(cycle->lock_file.data, log) != NTG_OK) {
        goto failed;
    }

    //创建路经
    if (ntg_create_paths(cycle, ccf->user) != NTG_OK) {
        goto failed;
    }

    //打开一个日志对象
    if (ntg_log_open_default(cycle) != NTG_OK) {
        goto failed;
    }

    /* open the new files */
    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].name.len == 0) {//无效的
            continue;
        }

        file[i].fd = ntg_open_file(file[i].name.data,
                                   NTG_FILE_APPEND,
                                   NTG_FILE_CREATE_OR_OPEN,
                                   NTG_FILE_DEFAULT_ACCESS);

        ntg_log_debug3(NTG_LOG_DEBUG_CORE, log, 0,
                       "log: %p %d \"%s\"",
                       &file[i], file[i].fd, file[i].name.data);

        if (file[i].fd == NTG_INVALID_FILE) {
            ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                          ntg_open_file_n " \"%s\" failed",
                          file[i].name.data);
            goto failed;
        }

        //设置CLOEXEC属性
        if (fcntl(file[i].fd, F_SETFD, FD_CLOEXEC) == -1) {
            ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                          "fcntl(FD_CLOEXEC) \"%s\" failed",
                          file[i].name.data);
            goto failed;
        }
    }

    //代替原日志链表
    cycle->log = &cycle->new_log;
    pool->log = &cycle->new_log;


    /* commit the new cycle configuration */

    if (!ntg_use_stderr) {
        (void) ntg_log_redirect_stderr(cycle);
    }

    pool->log = cycle->log;

    //调用所有模块的初始化函数
    for (i = 0; ntg_modules[i]; i++) {
        if (ntg_modules[i]->init_module) {
            if (ntg_modules[i]->init_module(cycle) != NTG_OK) {
                /* fatal */
                exit(1);
            }
        }
    }


    /* close the unnecessary open files */

    part = &old_cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].fd == NTG_INVALID_FILE || file[i].fd == ntg_stderr) {
            continue;
        }

        if (ntg_close_file(file[i].fd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                          ntg_close_file_n " \"%s\" failed",
                          file[i].name.data);
        }
    }

    ntg_destroy_pool(conf.temp_pool);

    //主进程
    if (ntg_process == NTG_PROCESS_MASTER || ntg_is_init_cycle(old_cycle)) {

        /*
         * perl_destruct() frees environ, if it is not the same as it was at
         * perl_construct() time, therefore we save the previous cycle
         * environment before ntg_conf_parse() where it will be changed.
         */

        env = environ;
        environ = senv;

        ntg_destroy_pool(old_cycle->pool);
        cycle->old_cycle = NULL;

        environ = env;

        return cycle;
    }


    if (ntg_temp_pool == NULL) {
        ntg_temp_pool = ntg_create_pool(128, cycle->log);
        if (ntg_temp_pool == NULL) {
            ntg_log_error(NTG_LOG_EMERG, cycle->log, 0,
                          "could not create ntg_temp_pool");
            exit(1);
        }

        n = 10;
        ntg_old_cycles.elts = ntg_pcalloc(ntg_temp_pool,
                                          n * sizeof(ntg_cycle_t *));
        if (ntg_old_cycles.elts == NULL) {
            exit(1);
        }
        ntg_old_cycles.nelts = 0;
        ntg_old_cycles.size = sizeof(ntg_cycle_t *);
        ntg_old_cycles.nalloc = n;
        ntg_old_cycles.pool = ntg_temp_pool;

    }

    ntg_temp_pool->log = cycle->log;

    old = ntg_array_push(&ntg_old_cycles);
    if (old == NULL) {
        exit(1);
    }
    *old = old_cycle;



    return cycle;


failed:

    if (!ntg_is_init_cycle(old_cycle)) {
        old_ccf = (ntg_core_conf_t *) ntg_get_conf(old_cycle->conf_ctx,
                                                   ntg_core_module);
        if (old_ccf->environment) {
            environ = old_ccf->environment;
        }
    }

    /* rollback the new cycle configuration */

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].fd == NTG_INVALID_FILE || file[i].fd == ntg_stderr) {
            continue;
        }

        if (ntg_close_file(file[i].fd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                          ntg_close_file_n " \"%s\" failed",
                          file[i].name.data);
        }
    }

    if (ntg_test_config) {
        ntg_destroy_cycle_pools(&conf);
        return NULL;
    }

    ntg_destroy_cycle_pools(&conf);

    return NULL;
}

/**
 * 释放内存
 * @param[in] conf 配置对象
 */
static void
ntg_destroy_cycle_pools(ntg_conf_t *conf)
{
    ntg_destroy_pool(conf->temp_pool);
    ntg_destroy_pool(conf->pool);
}


/**
 * 创建一个进程id文件
 * @param[in] name pid文件名称
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note
 * 	1) ntg_test_config = 0 创建pid文件,并将进程id写入文件
 * 	2) ntg_test_config = 1 仅仅测试是否能够创建pid文件
 */
ntg_int_t
ntg_create_pidfile(ntg_str_t *name, ntg_log_t *log)
{
    size_t      len;
    ntg_uint_t  create;
    ntg_file_t  file;
    u_char      pid[NTG_INT64_LEN + 2];

    if (ntg_process > NTG_PROCESS_MASTER) {
        return NTG_OK;
    }

    ntg_memzero(&file, sizeof(ntg_file_t));

    file.name = *name;
    file.log = log;

    create = ntg_test_config ? NTG_FILE_CREATE_OR_OPEN : NTG_FILE_TRUNCATE;

    file.fd = ntg_open_file(file.name.data, NTG_FILE_RDWR,
                            create, NTG_FILE_DEFAULT_ACCESS);

    if (file.fd == NTG_INVALID_FILE) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                      ntg_open_file_n " \"%s\" failed", file.name.data);
        return NTG_ERROR;
    }

    if (!ntg_test_config) {
        len = ntg_snprintf(pid, NTG_INT64_LEN + 2, "%P%N", ntg_pid) - pid;

        if (ntg_write_file(&file, pid, len, 0) == NTG_ERROR) {
            return NTG_ERROR;
        }
    }

    if (ntg_close_file(file.fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", file.name.data);
    }

    return NTG_OK;
}

/**
 * 删除进程id文件
 * @param[in] cycle 全局循环体
 */
void
ntg_delete_pidfile(ntg_cycle_t *cycle)
{
    u_char           *name;
    ntg_core_conf_t  *ccf;

    ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

    name = ntg_new_binary ? ccf->oldpid.data : ccf->pid.data;

    if (ntg_delete_file(name) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                      ntg_delete_file_n " \"%s\" failed", name);
    }
}

/**
 * 向指定进程发送信号
 * @param[in] cycle 循环体
 * @param[in] sig 信号名称
 * @return 成功返回0, 失败返回1
 * @note 主要执行过程:
 * 	1)获取核心模块
 * 	2)设置pid文件对象
 * 	3)读取pid文件信息
 * 	4)关闭pid文件
 * 	5)获取进程id
 * 	6)向指定进程发送信号
 */
ntg_int_t
ntg_signal_process(ntg_cycle_t *cycle, char *sig)
{
    ssize_t           n;
    ntg_int_t         pid;
    ntg_file_t        file;
    ntg_core_conf_t  *ccf;
    u_char            buf[NTG_INT64_LEN + 2];

    ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "signal process started");
    //1)获取核心模块
    ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);
    //2)设置pid文件对象
    ntg_memzero(&file, sizeof(ntg_file_t));

    file.name = ccf->pid;
    file.log = cycle->log;

    file.fd = ntg_open_file(file.name.data, NTG_FILE_RDONLY,
                            NTG_FILE_OPEN, NTG_FILE_DEFAULT_ACCESS);

    if (file.fd == NTG_INVALID_FILE) {
        ntg_log_error(NTG_LOG_ERR, cycle->log, ntg_errno,
                      ntg_open_file_n " \"%s\" failed", file.name.data);
        return 1;
    }
    //3)读取pid文件信息
    n = ntg_read_file(&file, buf, NTG_INT64_LEN + 2, 0);

    //4)关闭pid文件
    if (ntg_close_file(file.fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", file.name.data);
    }

    if (n == NTG_ERROR) {
        return 1;
    }
    //5)获取进程id
    while (n-- && (buf[n] == CR || buf[n] == LF)) { /* void */ }

    pid = ntg_atoi(buf, ++n);

    if (pid == NTG_ERROR) {
        ntg_log_error(NTG_LOG_ERR, cycle->log, 0,
                      "invalid PID number \"%*s\" in \"%s\"",
                      n, buf, file.name.data);
        return 1;
    }
    //6)向指定进程发送信号
    return ntg_os_signal_process(cycle, sig, pid);

}

/**
 * 测试锁文件
 * @param[in] file 文件名
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t
ntg_test_lockfile(u_char *file, ntg_log_t *log)
{
#if !(NTG_HAVE_ATOMIC_OPS)
    ntg_fd_t  fd;

    fd = ntg_open_file(file, NTG_FILE_RDWR, NTG_FILE_CREATE_OR_OPEN,
                       NTG_FILE_DEFAULT_ACCESS);

    if (fd == NTG_INVALID_FILE) {
        ntg_log_error(NTG_LOG_EMERG, log, ntg_errno,
                      ntg_open_file_n " \"%s\" failed", file);
        return NTG_ERROR;
    }

    if (ntg_close_file(fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", file);
    }

    if (ntg_delete_file(file) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
                      ntg_delete_file_n " \"%s\" failed", file);
    }

#endif

    return NTG_OK;
}


void
ntg_reopen_files(ntg_cycle_t *cycle, ntg_uid_t user)
{
    ntg_fd_t          fd;
    ntg_uint_t        i;
    ntg_list_part_t  *part;
    ntg_open_file_t  *file;

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].name.len == 0) {
            continue;
        }

        if (file[i].flush) {
            file[i].flush(&file[i], cycle->log);
        }

        fd = ntg_open_file(file[i].name.data, NTG_FILE_APPEND,
                           NTG_FILE_CREATE_OR_OPEN, NTG_FILE_DEFAULT_ACCESS);

        ntg_log_debug3(NTG_LOG_DEBUG_EVENT, cycle->log, 0,
                       "reopen file \"%s\", old:%d new:%d",
                       file[i].name.data, file[i].fd, fd);

        if (fd == NTG_INVALID_FILE) {
            ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                          ntg_open_file_n " \"%s\" failed", file[i].name.data);
            continue;
        }

#if !(NTG_WIN32)
        if (user != (ntg_uid_t) NTG_CONF_UNSET_UINT) {
            ntg_file_info_t  fi;

            if (ntg_file_info((const char *) file[i].name.data, &fi)
                == NTG_FILE_ERROR)
            {
                ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                              ntg_file_info_n " \"%s\" failed",
                              file[i].name.data);

                if (ntg_close_file(fd) == NTG_FILE_ERROR) {
                    ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                                  ntg_close_file_n " \"%s\" failed",
                                  file[i].name.data);
                }

                continue;
            }

            if (fi.st_uid != user) {
                if (chown((const char *) file[i].name.data, user, -1) == -1) {
                    ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                                  "chown(\"%s\", %d) failed",
                                  file[i].name.data, user);

                    if (ntg_close_file(fd) == NTG_FILE_ERROR) {
                        ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                                      ntg_close_file_n " \"%s\" failed",
                                      file[i].name.data);
                    }

                    continue;
                }
            }

            if ((fi.st_mode & (S_IRUSR|S_IWUSR)) != (S_IRUSR|S_IWUSR)) {

                fi.st_mode |= (S_IRUSR|S_IWUSR);

                if (chmod((const char *) file[i].name.data, fi.st_mode) == -1) {
                    ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                                  "chmod() \"%s\" failed", file[i].name.data);

                    if (ntg_close_file(fd) == NTG_FILE_ERROR) {
                        ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                                      ntg_close_file_n " \"%s\" failed",
                                      file[i].name.data);
                    }

                    continue;
                }
            }
        }

        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                          "fcntl(FD_CLOEXEC) \"%s\" failed",
                          file[i].name.data);

            if (ntg_close_file(fd) == NTG_FILE_ERROR) {
                ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                              ntg_close_file_n " \"%s\" failed",
                              file[i].name.data);
            }

            continue;
        }
#endif

        if (ntg_close_file(file[i].fd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                          ntg_close_file_n " \"%s\" failed",
                          file[i].name.data);
        }

        file[i].fd = fd;
    }

    (void) ntg_log_redirect_stderr(cycle);
}

