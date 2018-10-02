/*
 * ntg_files.c
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

//#ifndef _GNU_SOURCE
//#define _GNU_SOURCE 1
//#endif

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_log.h"
#include "ntg_times.h"
#include "ntg_files.h"
#include "ntg_file.h"
#include "ntg_array.h"
#include "ntg_buf.h"
#include <stdio.h>

/*
 * 从指定的offset处读取数据
 */
ssize_t
ntg_read_file(ntg_file_t *file, u_char *buf, size_t size, off_t offset){
    ssize_t  n;

    ntg_log_debug4(NTG_LOG_DEBUG_CORE, file->log, 0,
                   "read: %d, %p, %uz, %O", file->fd, buf, size, offset);
    n = pread(file->fd, buf, size, offset);//实现偏移与读取的原子操作

    if (n == -1) {
        ntg_log_error(NTG_LOG_CRIT, file->log, ntg_errno,
                      "pread() \"%s\" failed", file->name.data);
        return NTG_ERROR;
    }

    file->offset += n;
    return n;
}
/*
 * 从指定的offset写入数据
 */
ssize_t
ntg_write_file(ntg_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t  n, written;

    ntg_log_debug4(NTG_LOG_DEBUG_CORE, file->log, 0,
                   "write: %d, %p, %uz, %O", file->fd, buf, size, offset);

    written = 0;

#if (NTG_HAVE_PWRITE)

    for ( ;; ) {
        n = pwrite(file->fd, buf + written, size, offset);

        if (n == -1) {
            ntg_log_error(NTG_LOG_CRIT, file->log, ntg_errno,
                          "pwrite() \"%s\" failed", file->name.data);
            return NTG_ERROR;
        }

        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        offset += n;
        size -= n;
    }

#else

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            ntg_log_error(NTG_LOG_CRIT, file->log, ntg_errno,
                          "lseek() \"%s\" failed", file->name.data);
            return NTG_ERROR;
        }

        file->sys_offset = offset;
    }

    for ( ;; ) {
        n = write(file->fd, buf + written, size);

        if (n == -1) {
            ntg_log_error(NTG_LOG_CRIT, file->log, ntg_errno,
                          "write() \"%s\" failed", file->name.data);
            return NTG_ERROR;
        }

        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        size -= n;
    }
#endif
}


/*
 * 打开一个临 时文件
 * name->文件名
 * persistent->是否unlink,(持续性),表示是否为临时文件
 * access->访问权限
 */
ntg_fd_t
ntg_open_tempfile(u_char *name, ntg_uint_t persistent, ntg_uint_t access)
{
    ntg_fd_t  fd;

    fd = open((const char *) name, O_CREAT|O_EXCL|O_RDWR,
              access ? access : 0600);

    if (fd != -1 && !persistent) {
        (void) unlink((const char *) name);
    }

    return fd;
}

#define NTG_IOVS  8

/*
 * 将chain数据写入文件
 */
ssize_t
ntg_write_chain_to_file(ntg_file_t *file, ntg_chain_t *cl, off_t offset,
    ntg_pool_t *pool)
{
    u_char        *prev;
    size_t         size;
    ssize_t        total, n;
    ntg_array_t    vec;
    struct iovec  *iov, iovs[NTG_IOVS];

    /* use pwrite() if there is the only buf in a chain */

    if (cl->next == NULL) {
        return ntg_write_file(file, cl->buf->pos,
                              (size_t) (cl->buf->last - cl->buf->pos),
                              offset);
    }

    total = 0;

    vec.elts = iovs;
    vec.size = sizeof(struct iovec);
    vec.nalloc = NTG_IOVS;
    vec.pool = pool;

    do {
        prev = NULL;
        iov = NULL;
        size = 0;

        vec.nelts = 0;

        /* create the iovec and coalesce the neighbouring bufs */
        //将多个chain中数据映射到iovec中
        //TODO 不能链接到stdio_lim.h
        while (cl && vec.nelts < 1024 /*IOV_MAX*/) {
            if (prev == cl->buf->pos) {//将内存中连续的数据放到同一个iovec中
                iov->iov_len += cl->buf->last - cl->buf->pos;

            } else {//遇到不连续分配一个新的iovec
                iov = ntg_array_push(&vec);
                if (iov == NULL) {
                    return NTG_ERROR;
                }

                iov->iov_base = (void *) cl->buf->pos;
                iov->iov_len = cl->buf->last - cl->buf->pos;
            }

            size += cl->buf->last - cl->buf->pos;
            prev = cl->buf->last;
            cl = cl->next;
        }

        /* use pwrite() if there is the only iovec buffer */

        if (vec.nelts == 1) {
            iov = vec.elts;

            n = ntg_write_file(file, (u_char *) iov[0].iov_base,
                               iov[0].iov_len, offset);

            if (n == NTG_ERROR) {
                return n;
            }

            return total + n;
        }

        //移动偏移
        if (file->sys_offset != offset) {
            if (lseek(file->fd, offset, SEEK_SET) == -1) {
                ntg_log_error(NTG_LOG_CRIT, file->log, ntg_errno,
                              "lseek() \"%s\" failed", file->name.data);
                return NTG_ERROR;
            }

            file->sys_offset = offset;
        }

        n = writev(file->fd, vec.elts, vec.nelts);

        if (n == -1) {
            ntg_log_error(NTG_LOG_CRIT, file->log, ntg_errno,
                          "writev() \"%s\" failed", file->name.data);
            return NTG_ERROR;
        }

        if ((size_t) n != size) {
            ntg_log_error(NTG_LOG_CRIT, file->log, 0,
                          "writev() \"%s\" has written only %z of %uz",
                          file->name.data, n, size);
            return NTG_ERROR;
        }

        ntg_log_debug2(NTG_LOG_DEBUG_CORE, file->log, 0,
                       "writev: %d, %z", file->fd, n);

        file->sys_offset += n;
        file->offset += n;
        offset += n;
        total += n;

    } while (cl);

    return total;
}

//ssize_t ntg_write_fd(ntg_fd_t fd, void *buf, size_t n)
//{
//    return write(fd, buf, n);
//}

/*
 * 设置文件的访问时间和修改时间
 */
ntg_int_t ntg_set_file_time(u_char *name, ntg_fd_t fd, time_t s) {
    struct timeval  tv[2];

    tv[0].tv_sec = ntg_time();
    tv[0].tv_usec = 0;
    tv[1].tv_sec = s;
    tv[1].tv_usec = 0;

    if (utimes((char *) name, tv) != -1) {
        return NTG_OK;
    }

    return NTG_ERROR;
}
ntg_int_t
ntg_create_file_mapping(ntg_file_mapping_t *fm)
{
    fm->fd = ntg_open_file(fm->name, NTG_FILE_RDWR, NTG_FILE_TRUNCATE,
                           NTG_FILE_DEFAULT_ACCESS);
    if (fm->fd == NTG_INVALID_FILE) {
        ntg_log_error(NTG_LOG_CRIT, fm->log, ntg_errno,
                      ntg_open_file_n " \"%s\" failed", fm->name);
        return NTG_ERROR;
    }

    if (ftruncate(fm->fd, fm->size) == -1) {
        ntg_log_error(NTG_LOG_CRIT, fm->log, ntg_errno,
                      "ftruncate() \"%s\" failed", fm->name);
        goto failed;
    }

    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
        return NTG_OK;
    }

    ntg_log_error(NTG_LOG_CRIT, fm->log, ntg_errno,
                  "mmap(%uz) \"%s\" failed", fm->size, fm->name);

failed:

    if (ntg_close_file(fm->fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, fm->log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", fm->name);
    }

    return NTG_ERROR;
}


void
ntg_close_file_mapping(ntg_file_mapping_t *fm)
{
    if (munmap(fm->addr, fm->size) == -1) {
        ntg_log_error(NTG_LOG_CRIT, fm->log, ntg_errno,
                      "munmap(%uz) \"%s\" failed", fm->size, fm->name);
    }

    if (ntg_close_file(fm->fd) == NTG_FILE_ERROR) {
        ntg_log_error(NTG_LOG_ALERT, fm->log, ntg_errno,
                      ntg_close_file_n " \"%s\" failed", fm->name);
    }
}
/*
 * 打开一个目录
 */
ntg_int_t
ntg_open_dir(ntg_str_t *name, ntg_dir_t *dir)
{
    dir->dir = opendir((const char *) name->data);

    if (dir->dir == NULL) {
        return NTG_ERROR;
    }

    dir->valid_info = 0;

    return NTG_OK;
}

/*
 * 读取一个目录
 */
ntg_int_t
ntg_read_dir(ntg_dir_t *dir)
{
    dir->de = readdir(dir->dir);

    if (dir->de) {
#if (NTG_HAVE_D_TYPE)
        dir->type = dir->de->d_type;
#else
        dir->type = 0;
#endif
        return NTG_OK;
    }

    return NTG_ERROR;
}

ntg_int_t
ntg_open_glob(ntg_glob_t *gl)
{
    int  n;

    n = glob((char *) gl->pattern, 0, NULL, &gl->pglob);

    if (n == 0) {
        return NTG_OK;
    }

#ifdef GLOB_NOMATCH

    if (n == GLOB_NOMATCH && gl->test) {
        return NTG_OK;
    }

#endif

    return NTG_ERROR;
}


ntg_int_t
ntg_read_glob(ntg_glob_t *gl, ntg_str_t *name)
{
    size_t  count;

#ifdef GLOB_NOMATCH
    count = (size_t) gl->pglob.gl_pathc;
#else
    count = (size_t) gl->pglob.gl_matchc;
#endif

    if (gl->n < count) {

        name->len = (size_t) ntg_strlen(gl->pglob.gl_pathv[gl->n]);
        name->data = (u_char *) gl->pglob.gl_pathv[gl->n];
        gl->n++;

        return NTG_OK;
    }

    return NTG_DONE;
}


void
ntg_close_glob(ntg_glob_t *gl)
{
    globfree(&gl->pglob);
}


ntg_err_t
ntg_trylock_fd(ntg_fd_t fd)
{
    struct flock  fl;

    ntg_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return ntg_errno;
    }

    return 0;
}


ntg_err_t
ntg_lock_fd(ntg_fd_t fd)
{
    struct flock  fl;

    ntg_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        return ntg_errno;
    }

    return 0;
}


ntg_err_t
ntg_unlock_fd(ntg_fd_t fd)
{
    struct flock  fl;

    ntg_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return  ntg_errno;
    }

    return 0;
}


#if (NTG_HAVE_POSIX_FADVISE) && !(NTG_HAVE_F_READAHEAD)

ntg_int_t
ntg_read_ahead(ntg_fd_t fd, size_t n)
{
    int  err;

    err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

    if (err == 0) {
        return 0;
    }

    ntg_set_errno(err);
    return NTG_FILE_ERROR;
}

#endif


#if (NTG_HAVE_O_DIRECT)

ntg_int_t
ntg_directio_on(ntg_fd_t fd)
{
    int  flags;

    flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        return NTG_FILE_ERROR;
    }
    return fcntl(fd, F_SETFL, flags | O_DIRECT);
}


ntg_int_t
ntg_directio_off(ntg_fd_t fd)
{
    int  flags;

    flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        return NTG_FILE_ERROR;
    }

    return fcntl(fd, F_SETFL, flags & ~O_DIRECT);
}

#endif


#if (NTG_HAVE_STATFS)

size_t
ntg_fs_bsize(u_char *name)
{
    struct statfs  fs;

    if (statfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_bsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_bsize;
}

#elif (NTG_HAVE_STATVFS)

size_t
ntg_fs_bsize(u_char *name)
{
    struct statvfs  fs;

    if (statvfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_frsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_frsize;
}

#else

size_t
ntg_fs_bsize(u_char *name)
{
    return 512;
}

#endif



