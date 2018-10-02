
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_string.h"
#include "ntg_errno.h"
#include "ntg_array.h"
#include "ntg_alloc.h"
#include "ntg_cycle.h"
#include "ntg_files.h"
#include "ntg_conf_file.h"
#include "ntg_file.h"
//#include "ntg_atomic.h"




static ntg_int_t ntg_test_full_name(ntg_str_t *name);


static ntg_atomic_t  	temp_number = 0;
ntg_atomic_t			*ntg_temp_number = &temp_number;
ntg_atomic_int_t      	ntg_random_number = 123456;


/**
 * 获取完整路经
 * @param[in] pool 内存池指针
 * @param[in] prefix 路经前缀
 * @param[in][out] name 文件名
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
ntg_int_t
ntg_get_full_name(ntg_pool_t *pool, ntg_str_t *prefix, ntg_str_t *name)
{
    size_t      len;
    u_char     *p, *n;
    ntg_int_t   rc;
    //测试name是否为完整路经
    rc = ntg_test_full_name(name);

    if (rc == NTG_OK) {
        return rc;
    }

    len = prefix->len;

    n = ntg_pnalloc(pool, len + name->len + 1);
    if (n == NULL) {
        return NTG_ERROR;
    }

    p = ntg_cpymem(n, prefix->data, len);
    ntg_cpystrn(p, name->data, name->len + 1);

    name->len += len;
    name->data = n;

    return NTG_OK;
}


/**
 * 测试name是否为完整路经
 * @param[in] name 文件名称
 * @return 是完整路经返回NTG_OK,否则返回NTG_DECLINED
 * @ref ntg_types.h
 */
static ntg_int_t
ntg_test_full_name(ntg_str_t *name)
{

    if (name->data[0] == '/') {
        return NTG_OK;
    }

    return NTG_DECLINED;
}


/*
 * 将数据写入到临时文件中
 */
ssize_t ntg_write_chain_to_temp_file(ntg_temp_file_t *tf, ntg_chain_t *chain){
    ntg_int_t  rc;

    if (tf->file.fd == NTG_INVALID_FILE) {
        rc = ntg_create_temp_file(&tf->file, tf->path, tf->pool,
                                  tf->persistent, tf->clean, tf->access);

        if (rc != NTG_OK) {
            return rc;
        }

        if (tf->log_level) {
            ntg_log_error(tf->log_level, tf->file.log, 0, "%s %V",
                          tf->warn, &tf->file.name);
        }
    }

    return ntg_write_chain_to_file(&tf->file, chain, tf->offset, tf->pool);
}

/*
 * 创建一个临时文件
 *
 */
ntg_int_t ntg_create_temp_file(ntg_file_t *file, ntg_path_t *path, ntg_pool_t *pool,
    ntg_uint_t persistent, ntg_uint_t clean, ntg_uint_t access){
    uint32_t                  n;
    ntg_err_t                 err;
    ntg_pool_cleanup_t       *cln;
    ntg_pool_cleanup_file_t  *clnf;

    file->name.len = path->name.len + 1 + path->len + 10;

    file->name.data = ntg_pnalloc(pool, file->name.len + 1);
    if (file->name.data == NULL) {
        return NTG_ERROR;
    }

    ntg_memcpy(file->name.data, path->name.data, path->name.len);

    n = (uint32_t) ntg_next_temp_number(0);//获取临时文件名

    cln = ntg_pool_cleanup_add(pool, sizeof(ntg_pool_cleanup_file_t));
    if (cln == NULL) {
        return NTG_ERROR;
    }

    for ( ;; ) {
        (void) ntg_sprintf(file->name.data + path->name.len + 1 + path->len,
                           "%010uD%Z", n);
        //1)对filename进行hash处理
        ntg_create_hashed_filename(path, file->name.data, file->name.len);

        ntg_log_debug1(NTG_LOG_DEBUG_CORE, file->log, 0,
                       "hashed path: %s", file->name.data);
        //2)打开临时文件
        file->fd = ntg_open_tempfile(file->name.data, persistent, access);

        ntg_log_debug1(NTG_LOG_DEBUG_CORE, file->log, 0,
                       "temp fd:%d", file->fd);

        if (file->fd != NTG_INVALID_FILE) {//成功创建

            cln->handler = clean ? ntg_pool_delete_file : ntg_pool_cleanup_file;
            clnf = cln->data;

            clnf->fd = file->fd;
            clnf->name = file->name.data;
            clnf->log = pool->log;

            return NTG_OK;
        }

        err = ntg_errno;
        //3)临时文件以存在
        if (err == NTG_EEXIST) {
            n = (uint32_t) ntg_next_temp_number(1);//获取下一个零时名,然后继续
            continue;
        }
        //4)其他错误处理
        if ((path->level[0] == 0) || (err != NTG_ENOPATH)) {//4.1没有中间路经或有这个路经 (致命错误)
            ntg_log_error(NTG_LOG_CRIT, file->log, err,
                          ntg_open_tempfile_n " \"%s\" failed",
                          file->name.data);
            return NTG_ERROR;
        }

        if (ntg_create_path(file, path) == NTG_ERROR) {//4.2 没有这个路经
            return NTG_ERROR;
        }
    }
    return NTG_ERROR;//不可能执行到这
}

/*
 * 对文件名进行hash处理
 * len = strlen(file)
 */

void ntg_create_hashed_filename(ntg_path_t *path, u_char *file, size_t len) {
    size_t      i, level;
    ntg_uint_t  n;

    i = path->name.len + 1;

    file[path->name.len + path->len]  = '/';

    for (n = 0; n < 3; n++) {
        level = path->level[n];

        if (level == 0) {//没有下一个路经层次
            break;
        }

        len -= level;
        file[i - 1] = '/';
        ntg_memcpy(&file[i], &file[len], level);
        i += level + 1;
    }
}

/*
 * 创建一个路经
 */
ntg_int_t
ntg_create_path(ntg_file_t *file, ntg_path_t *path)
{
    size_t      pos;
    ntg_err_t   err;
    ntg_uint_t  i;

    pos = path->name.len;

    for (i = 0; i < 3; i++) {
        if (path->level[i] == 0) {
            break;
        }

        pos += path->level[i] + 1;

        file->name.data[pos] = '\0';//1)

        ntg_log_debug1(NTG_LOG_DEBUG_CORE, file->log, 0,
                       "temp file: \"%s\"", file->name.data);

        if (ntg_create_dir(file->name.data, 0700) == NTG_FILE_ERROR) {//创建中间路经
            err = ntg_errno;
            if (err != NTG_EEXIST) {//致命错误
                ntg_log_error(NTG_LOG_CRIT, file->log, err,
                              ntg_create_dir_n " \"%s\" failed",
                              file->name.data);
                return NTG_ERROR;
            }
        }

        file->name.data[pos] = '/';//2)
    }

    return NTG_OK;
}

/*
 * 以dir字符串创建完整路经
 */
ntg_err_t
ntg_create_full_path(u_char *dir, ntg_uint_t access)
{
    u_char     *p, ch;
    ntg_err_t   err;

    err = 0;

    p = dir + 1;
    for ( /* void */ ; *p; p++) {
        ch = *p;

        if (ch != '/') {
            continue;
        }

        *p = '\0';

        if (ntg_create_dir(dir, access) == NTG_FILE_ERROR) {
            err = ntg_errno;

            switch (err) {
            case NTG_EEXIST:
                err = 0;
            case NTG_EACCES:
                break;

            default:
                return err;
            }
        }

        *p = '/';
    }

    return err;
}

/*
 * 获取下一个临时数
 */
ntg_atomic_uint_t
ntg_next_temp_number(ntg_uint_t collision)//碰撞避免因子大小
{
	ntg_atomic_uint_t  n, add;

    add = collision ? ntg_random_number : 1;

    n = ntg_atomic_fetch_add(ntg_temp_number, add);

    return n + add;
}


char *
ntg_conf_set_path_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ssize_t      level;
    ntg_str_t   *value;
    ntg_uint_t   i, n;
    ntg_path_t  *path, **slot;

    slot = (ntg_path_t **) (p + cmd->offset);

    if (*slot) {
        return "is duplicate";
    }

    path = ntg_pcalloc(cf->pool, sizeof(ntg_path_t));
    if (path == NULL) {
        return (void *) -1;
    }

    value = cf->args->elts;

    path->name = value[1];

    if (path->name.data[path->name.len - 1] == '/') {
        path->name.len--;
    }

    if (ntg_conf_full_name(cf->cycle, &path->name, 0) != NTG_OK) {
        return (void *) -1;
    }

    path->conf_file = cf->conf_file->file.name.data;
    path->line = cf->conf_file->line;

    for (i = 0, n = 2; n < cf->args->nelts; i++, n++) {
        level = ntg_atoi(value[n].data, value[n].len);
        if (level == NTG_ERROR || level == 0) {
            return "invalid value";
        }

        path->level[i] = level;
        path->len += level + 1;
    }

    if (path->len > 10 + i) {
        return "invalid value";
    }

    *slot = path;

    if (ntg_add_path(cf, slot) == NTG_ERROR) {
        return NTG_CONF_ERROR;
    }

    return NTG_CONF_OK;
}


char *
ntg_conf_merge_path_value(ntg_conf_t *cf, ntg_path_t **path, ntg_path_t *prev,
    ntg_path_init_t *init)
{
    if (*path) {
        return NTG_CONF_OK;
    }

    if (prev) {
        *path = prev;
        return NTG_CONF_OK;
    }

    *path = ntg_pcalloc(cf->pool, sizeof(ntg_path_t));
    if (*path == NULL) {
        return NTG_CONF_ERROR;
    }

    (*path)->name = init->name;

    if (ntg_conf_full_name(cf->cycle, &(*path)->name, 0) != NTG_OK) {
        return NTG_CONF_ERROR;
    }

    (*path)->level[0] = init->level[0];
    (*path)->level[1] = init->level[1];
    (*path)->level[2] = init->level[2];

    (*path)->len = init->level[0] + (init->level[0] ? 1 : 0)
                   + init->level[1] + (init->level[1] ? 1 : 0)
                   + init->level[2] + (init->level[2] ? 1 : 0);

    if (ntg_add_path(cf, path) != NTG_OK) {
        return NTG_CONF_ERROR;
    }

    return NTG_CONF_OK;
}


char *
ntg_conf_set_access_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *confp = conf;

    u_char      *p;
    ntg_str_t   *value;
    ntg_uint_t   i, right, shift, *access;

    access = (ntg_uint_t *) (confp + cmd->offset);

    if (*access != NTG_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *access = 0600;

    for (i = 1; i < cf->args->nelts; i++) {

        p = value[i].data;

        if (ntg_strncmp(p, "user:", sizeof("user:") - 1) == 0) {
            shift = 6;
            p += sizeof("user:") - 1;

        } else if (ntg_strncmp(p, "group:", sizeof("group:") - 1) == 0) {
            shift = 3;
            p += sizeof("group:") - 1;

        } else if (ntg_strncmp(p, "all:", sizeof("all:") - 1) == 0) {
            shift = 0;
            p += sizeof("all:") - 1;

        } else {
            goto invalid;
        }

        if (ntg_strcmp(p, "rw") == 0) {
            right = 6;

        } else if (ntg_strcmp(p, "r") == 0) {
            right = 4;

        } else {
            goto invalid;
        }

        *access |= right << shift;
    }

    return NTG_CONF_OK;

invalid:

    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, "invalid value \"%V\"", &value[i]);

    return NTG_CONF_ERROR;
}


ntg_int_t
ntg_add_path(ntg_conf_t *cf, ntg_path_t **slot)
{
    ntg_uint_t   i, n;
    ntg_path_t  *path, **p;

    path = *slot;

    p = cf->cycle->paths.elts;
    for (i = 0; i < cf->cycle->paths.nelts; i++) {
        if (p[i]->name.len == path->name.len
            && ntg_strcmp(p[i]->name.data, path->name.data) == 0)
        {
            if (p[i]->data != path->data) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                   "the same path name \"%V\" "
                                   "used in %s:%ui and",
                                   &p[i]->name, p[i]->conf_file, p[i]->line);
                return NTG_ERROR;
            }

            for (n = 0; n < 3; n++) {
                if (p[i]->level[n] != path->level[n]) {
                    if (path->conf_file == NULL) {
                        if (p[i]->conf_file == NULL) {
                            ntg_log_error(NTG_LOG_EMERG, cf->log, 0,
                                      "the default path name \"%V\" has "
                                      "the same name as another default path, "
                                      "but the different levels, you need to "
                                      "redefine one of them in http section",
                                      &p[i]->name);
                            return NTG_ERROR;
                        }

                        ntg_log_error(NTG_LOG_EMERG, cf->log, 0,
                                      "the path name \"%V\" in %s:%ui has "
                                      "the same name as default path, but "
                                      "the different levels, you need to "
                                      "define default path in http section",
                                      &p[i]->name, p[i]->conf_file, p[i]->line);
                        return NTG_ERROR;
                    }

                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                      "the same path name \"%V\" in %s:%ui "
                                      "has the different levels than",
                                      &p[i]->name, p[i]->conf_file, p[i]->line);
                    return NTG_ERROR;
                }

                if (p[i]->level[n] == 0) {
                    break;
                }
            }

            *slot = p[i];

            return NTG_OK;
        }
    }

    p = ntg_array_push(&cf->cycle->paths);

    if (p == NULL) {
        return NTG_ERROR;
    }

    *p = path;

    return NTG_OK;
}

/**
 * 创建所有路经
 * @param[in] cycle 全局循环体
 * @param[in] user 所属用户
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 如果设置了文件的所有者,则设置目录的读写执行模式
 */
ntg_int_t ntg_create_paths(ntg_cycle_t *cycle, ntg_uid_t user){
    ntg_err_t         err;
    ntg_uint_t        i;
    ntg_path_t      **path;

    path = cycle->paths.elts;
    for (i = 0; i < cycle->paths.nelts; i++) {

        if (ntg_create_dir(path[i]->name.data, 0700) == NTG_FILE_ERROR) {
            err = ntg_errno;
            if (err != NTG_EEXIST) {
                ntg_log_error(NTG_LOG_EMERG, cycle->log, err,
                              ntg_create_dir_n " \"%s\" failed",
                              path[i]->name.data);
                return NTG_ERROR;
            }
        }

        if (user == (ntg_uid_t) NTG_CONF_UNSET_UINT) {//没有设置用户
            continue;
        }

#if !(NTG_WIN32)
        {
        ntg_file_info_t   fi;

        //获取路经stat
        if (ntg_file_info((const char *) path[i]->name.data, &fi)
            == NTG_FILE_ERROR)
        {
            ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                          ntg_file_info_n " \"%s\" failed", path[i]->name.data);

            return NTG_ERROR;
        }

        if (fi.st_uid != user) {//设置路经的所属用户
            if (chown((const char *) path[i]->name.data, user, -1) == -1) {
                ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                              "chown(\"%s\", %d) failed",
                              path[i]->name.data, user);

                return NTG_ERROR;
            }
        }

        if ((fi.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR))
                                                  != (S_IRUSR|S_IWUSR|S_IXUSR))
        {//修改路经模式可读可写可执行
            fi.st_mode |= (S_IRUSR|S_IWUSR|S_IXUSR);

            if (chmod((const char *) path[i]->name.data, fi.st_mode) == -1) {
                ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
                              "chmod() \"%s\" failed", path[i]->name.data);

                return NTG_ERROR;
            }
        }
        }
#endif
    }

    return NTG_OK;
}

ntg_int_t ntg_ext_rename_file(ntg_str_t *src, ntg_str_t *to, ntg_ext_rename_file_t *ext){
    u_char           *name;
    ntg_err_t         err;
    ntg_copy_file_t   cf;

#if !(NTG_WIN32)

    if (ext->access) {
        if (ntg_change_file_access(src->data, ext->access) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_CRIT, ext->log, ntg_errno,
                          ntg_change_file_access_n " \"%s\" failed", src->data);
            err = 0;
            goto failed;
        }
    }

#endif

    if (ext->time != -1) {
        if (ntg_set_file_time(src->data, ext->fd, ext->time) != NTG_OK) {
            ntg_log_error(NTG_LOG_CRIT, ext->log, ntg_errno,
                          ntg_set_file_time_n " \"%s\" failed", src->data);
            err = 0;
            goto failed;
        }
    }

    if (ntg_rename_file(src->data, to->data) != NTG_FILE_ERROR) {
        return NTG_OK;
    }

    err = ntg_errno;

    if (err == NTG_ENOPATH) {

        if (!ext->create_path) {
            goto failed;
        }

        err = ntg_create_full_path(to->data, ntg_dir_access(ext->path_access));

        if (err) {
            ntg_log_error(NTG_LOG_CRIT, ext->log, err,
                          ntg_create_dir_n " \"%s\" failed", to->data);
            err = 0;
            goto failed;
        }

        if (ntg_rename_file(src->data, to->data) != NTG_FILE_ERROR) {
            return NTG_OK;
        }

        err = ntg_errno;
    }

#if (NTG_WIN32)

    if (err == NTG_EEXIST) {
        err = ntg_win32_rename_file(src, to, ext->log);

        if (err == 0) {
            return NTG_OK;
        }
    }

#endif

    if (err == NTG_EXDEV) {

        cf.size = -1;
        cf.buf_size = 0;
        cf.access = ext->access;
        cf.time = ext->time;
        cf.log = ext->log;

        name = ntg_alloc(to->len + 1 + 10 + 1, ext->log);
        if (name == NULL) {
            return NTG_ERROR;
        }

        (void) ntg_sprintf(name, "%*s.%010uD%Z", to->len, to->data,
                           (uint32_t) ntg_next_temp_number(0));

        if (ntg_copy_file(src->data, name, &cf) == NTG_OK) {

            if (ntg_rename_file(name, to->data) != NTG_FILE_ERROR) {
                ntg_free(name);

                if (ntg_delete_file(src->data) == NTG_FILE_ERROR) {
                    ntg_log_error(NTG_LOG_CRIT, ext->log, ntg_errno,
                                  ntg_delete_file_n " \"%s\" failed",
                                  src->data);
                    return NTG_ERROR;
                }

                return NTG_OK;
            }

            ntg_log_error(NTG_LOG_CRIT, ext->log, ntg_errno,
                          ntg_rename_file_n " \"%s\" to \"%s\" failed",
                          name, to->data);

            if (ntg_delete_file(name) == NTG_FILE_ERROR) {
                ntg_log_error(NTG_LOG_CRIT, ext->log, ntg_errno,
                              ntg_delete_file_n " \"%s\" failed", name);

            }
        }

        ntg_free(name);

        err = 0;
    }

failed:

    if (ext->delete_file) {
        if (ntg_delete_file(src->data) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_CRIT, ext->log, ntg_errno,
                          ntg_delete_file_n " \"%s\" failed", src->data);
        }
    }

    if (err) {
        ntg_log_error(NTG_LOG_CRIT, ext->log, err,
                      ntg_rename_file_n " \"%s\" to \"%s\" failed",
                      src->data, to->data);
    }

    return NTG_ERROR;
}


ntg_int_t
ntg_copy_file(u_char *from, u_char *to, ntg_copy_file_t *cf)
{
    char             *buf;
    off_t             size;
    size_t            len;
    ssize_t           n;
    ntg_fd_t          fd, nfd;
    ntg_int_t         rc;
    ntg_file_info_t   fi;

    rc = NTG_ERROR;
    buf = NULL;
    nfd = NTG_INVALID_FILE;

    fd = ntg_open_file(from, NTG_FILE_RDONLY, NTG_FILE_OPEN, 0);

    if (fd == NTG_INVALID_FILE) {
        ntg_log_error(NTG_LOG_CRIT, cf->log, ntg_errno,
                      ntg_open_file_n " \"%s\" failed", from);
        goto failed;
    }

    if (cf->size != -1) {
        size = cf->size;

    } else {
        if (ntg_fd_info(fd, &fi) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_fd_info_n " \"%s\" failed", from);

            goto failed;
        }

        size = ntg_file_size(&fi);
    }

    len = cf->buf_size ? cf->buf_size : 65536;

    if ((off_t) len > size) {
        len = (size_t) size;
    }

    buf = ntg_alloc(len, cf->log);
    if (buf == NULL) {
        goto failed;
    }

    nfd = ntg_open_file(to, NTG_FILE_WRONLY, NTG_FILE_CREATE_OR_OPEN,
                        cf->access);

    if (nfd == NTG_INVALID_FILE) {
        ntg_log_error(NTG_LOG_CRIT, cf->log, ntg_errno,
                      ntg_open_file_n " \"%s\" failed", to);
        goto failed;
    }

    while (size > 0) {

        if ((off_t) len > size) {
            len = (size_t) size;
        }

        n = ntg_read_fd(fd, buf, len);

        if (n == -1) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_read_fd_n " \"%s\" failed", from);
            goto failed;
        }

        if ((size_t) n != len) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, 0,
                          ntg_read_fd_n " has read only %z of %uz from %s",
                          n, size, from);
            goto failed;
        }

        n = ntg_write_fd(nfd, buf, len);

        if (n == -1) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_write_fd_n " \"%s\" failed", to);
            goto failed;
        }

        if ((size_t) n != len) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, 0,
                          ntg_write_fd_n " has written only %z of %uz to %s",
                          n, size, to);
            goto failed;
        }

        size -= n;
    }

    if (cf->time != -1) {
        if (ntg_set_file_time(to, nfd, cf->time) != NTG_OK) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_set_file_time_n " \"%s\" failed", to);
            goto failed;
        }
    }

    rc = NTG_OK;

failed:

    if (nfd != NTG_INVALID_FILE) {
        if (ntg_close_file(nfd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_close_file_n " \"%s\" failed", to);
        }
    }

    if (fd != NTG_INVALID_FILE) {
        if (ntg_close_file(fd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_close_file_n " \"%s\" failed", from);
        }
    }

    if (buf) {
        ntg_free(buf);
    }

    return rc;
}


/*
 * ctx->init_handler() - see ctx->alloc
 * ctx->file_handler() - file handler
 * ctx->pre_tree_handler() - handler is called before entering directory
 * ctx->post_tree_handler() - handler is called after leaving directory
 * ctx->spec_handler() - special (socket, FIFO, etc.) file handler
 *
 * ctx->data - some data structure, it may be the same on all levels, or
 *     reallocated if ctx->alloc is nonzero
 *
 * ctx->alloc - a size of data structure that is allocated at every level
 *     and is initialized by ctx->init_handler()
 *
 * ctx->log - a log
 *
 * on fatal (memory) error handler must return NTG_ABORT to stop walking tree
 */

ntg_int_t
ntg_walk_tree(ntg_tree_ctx_t *ctx, ntg_str_t *tree)
{
    void       *data, *prev;
    u_char     *p, *name;
    size_t      len;
    ntg_int_t   rc;
    ntg_err_t   err;
    ntg_str_t   file, buf;
    ntg_dir_t   dir;

    ntg_str_null(&buf);

    ntg_log_debug1(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                   "walk tree \"%V\"", tree);

    if (ntg_open_dir(tree, &dir) == NTG_ERROR) {
        ntg_log_error(NTG_LOG_CRIT, ctx->log, ntg_errno,
                      ntg_open_dir_n " \"%s\" failed", tree->data);
        return NTG_ERROR;
    }

    prev = ctx->data;

    if (ctx->alloc) {
        data = ntg_alloc(ctx->alloc, ctx->log);
        if (data == NULL) {
            goto failed;
        }

        if (ctx->init_handler(data, prev) == NTG_ABORT) {
            goto failed;
        }

        ctx->data = data;

    } else {
        data = NULL;
    }

    for ( ;; ) {

        ntg_set_errno(0);

        if (ntg_read_dir(&dir) == NTG_ERROR) {
            err = ntg_errno;

            if (err == NTG_ENOMOREFILES) {
                rc = NTG_OK;

            } else {
                ntg_log_error(NTG_LOG_CRIT, ctx->log, err,
                              ntg_read_dir_n " \"%s\" failed", tree->data);
                rc = NTG_ERROR;
            }

            goto done;
        }

        len = ntg_de_namelen(&dir);
        name = ntg_de_name(&dir);

        ntg_log_debug2(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                      "tree name %uz:\"%s\"", len, name);

        if (len == 1 && name[0] == '.') {
            continue;
        }

        if (len == 2 && name[0] == '.' && name[1] == '.') {
            continue;
        }

        file.len = tree->len + 1 + len;

        if (file.len + NTG_DIR_MASK_LEN > buf.len) {

            if (buf.len) {
                ntg_free(buf.data);
            }

            buf.len = tree->len + 1 + len + NTG_DIR_MASK_LEN;

            buf.data = ntg_alloc(buf.len + 1, ctx->log);
            if (buf.data == NULL) {
                goto failed;
            }
        }

        p = ntg_cpymem(buf.data, tree->data, tree->len);
        *p++ = '/';
        ntg_memcpy(p, name, len + 1);

        file.data = buf.data;

        ntg_log_debug1(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                       "tree path \"%s\"", file.data);

        if (!dir.valid_info) {
            if (ntg_de_info(file.data, &dir) == NTG_FILE_ERROR) {
                ntg_log_error(NTG_LOG_CRIT, ctx->log, ntg_errno,
                              ntg_de_info_n " \"%s\" failed", file.data);
                continue;
            }
        }

        if (ntg_de_is_file(&dir)) {

            ntg_log_debug1(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                           "tree file \"%s\"", file.data);

            ctx->size = ntg_de_size(&dir);
            ctx->fs_size = ntg_de_fs_size(&dir);
            ctx->access = ntg_de_access(&dir);
            ctx->mtime = ntg_de_mtime(&dir);

            if (ctx->file_handler(ctx, &file) == NTG_ABORT) {
                goto failed;
            }

        } else if (ntg_de_is_dir(&dir)) {

            ntg_log_debug1(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                           "tree enter dir \"%s\"", file.data);

            ctx->access = ntg_de_access(&dir);
            ctx->mtime = ntg_de_mtime(&dir);

            rc = ctx->pre_tree_handler(ctx, &file);

            if (rc == NTG_ABORT) {
                goto failed;
            }

            if (rc == NTG_DECLINED) {
                ntg_log_debug1(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                               "tree skip dir \"%s\"", file.data);
                continue;
            }

            if (ntg_walk_tree(ctx, &file) == NTG_ABORT) {
                goto failed;
            }

            ctx->access = ntg_de_access(&dir);
            ctx->mtime = ntg_de_mtime(&dir);

            if (ctx->post_tree_handler(ctx, &file) == NTG_ABORT) {
                goto failed;
            }

        } else {

            ntg_log_debug1(NTG_LOG_DEBUG_CORE, ctx->log, 0,
                           "tree special \"%s\"", file.data);

            if (ctx->spec_handler(ctx, &file) == NTG_ABORT) {
                goto failed;
            }
        }
    }

failed:

    rc = NTG_ABORT;

done:

    if (buf.len) {
        ntg_free(buf.data);
    }

    if (data) {
        ntg_free(data);
        ctx->data = prev;
    }

    if (ntg_close_dir(&dir) == NTG_ERROR) {
        ntg_log_error(NTG_LOG_CRIT, ctx->log, ntg_errno,
                      ntg_close_dir_n " \"%s\" failed", tree->data);
    }

    return rc;
}
