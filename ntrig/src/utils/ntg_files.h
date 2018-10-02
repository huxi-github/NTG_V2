/*
 * ntg_files.h
 *
 *  Created on: Jul 25, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_FILES_H_
#define CORE_NTG_FILES_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_errno.h"

//#ifndef _GNU_SOURCE
//#define _GNU_SOURCE 1
//#endif

typedef int                      ntg_fd_t;
typedef struct stat              ntg_file_info_t;
typedef ino_t                    ntg_file_uniq_t;

/*内存映射文件对象*/
typedef struct {
	u_char 						*name;
	size_t						size;
	void						*addr;
	ntg_fd_t					fd;
	ntg_log_t					*log;
} ntg_file_mapping_t;

/*目录对象*/
typedef	struct	{
	DIR							*dir;
	struct dirent				*de;
	struct stat					info;

	unsigned					type:8;
	unsigned					valid_info:1;
} ntg_dir_t;

typedef struct {
    size_t                       n;
    glob_t                       pglob;
    u_char                      *pattern;
    ntg_log_t                   *log;
    ntg_uint_t                   test;
} ntg_glob_t;

#define NTG_INVALID_FILE         -1///< 非法文件描述符
#define NTG_FILE_ERROR           -1///< 文件错误

/**
 * 打开文件
 * @param name 绝对文件名
 * @param mode 文件模式
 * @param create 创建方式
 * @param access 访问权限
 * @return 文件描述符
 */
#define ntg_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)
#define ntg_open_file_n          "open()"

#define NTG_FILE_RDONLY          O_RDONLY
#define NTG_FILE_WRONLY          O_WRONLY
#define NTG_FILE_RDWR            O_RDWR
#define NTG_FILE_CREATE_OR_OPEN  O_CREAT
#define NTG_FILE_OPEN            0
#define NTG_FILE_TRUNCATE        (O_CREAT|O_TRUNC)
#define NTG_FILE_APPEND          (O_WRONLY|O_APPEND)
#define NTG_FILE_NONBLOCK        O_NONBLOCK

#if (NTG_HAVE_OPENAT)
#define NTG_FILE_NOFOLLOW        O_NOFOLLOW

#if defined(O_DIRECTORY)
#define NTG_FILE_DIRECTORY       O_DIRECTORY
#else
#define NTG_FILE_DIRECTORY       0
#endif

#if defined(O_SEARCH)
#define NTG_FILE_SEARCH          (O_SEARCH|NTG_FILE_DIRECTORY)

#elif defined(O_EXEC)
#define NTG_FILE_SEARCH          (O_EXEC|NTG_FILE_DIRECTORY)

#elif (NTG_HAVE_O_PATH)
#define NTG_FILE_SEARCH          (O_PATH|O_RDONLY|NTG_FILE_DIRECTORY)

#else
#define NTG_FILE_SEARCH          (O_RDONLY|NTG_FILE_DIRECTORY)
#endif

#endif /* NTG_HAVE_OPENAT */

#define NTG_FILE_DEFAULT_ACCESS  0644
#define NTG_FILE_OWNER_ACCESS    0600


#define ntg_close_file           close
#define ntg_close_file_n         "close()"


#define ntg_delete_file(name)    unlink((const char *) name)
#define ntg_delete_file_n        "unlink()"


ntg_fd_t ntg_open_tempfile(u_char *name, ntg_uint_t persistent,
    ntg_uint_t access);
#define ntg_open_tempfile_n      "open()"


ssize_t ntg_read_file(ntg_file_t *file, u_char *buf, size_t size, off_t offset);
#if (NTG_HAVE_PREAD)
#define ntg_read_file_n          "pread()"
#else
#define ntg_read_file_n          "read()"
#endif

ssize_t ntg_write_file(ntg_file_t *file, u_char *buf, size_t size,
    off_t offset);

ssize_t ntg_write_chain_to_file(ntg_file_t *file, ntg_chain_t *ce,
    off_t offset, ntg_pool_t *pool);


#define ntg_read_fd              read
#define ntg_read_fd_n            "read()"

/*
 * we use inlined function instead of simple #define
 * because glibc 2.3 sets warn_unused_result attribute for write()
 * and in this case gcc 4.3 ignores (void) cast
 */
static ntg_inline ssize_t
ntg_write_fd(ntg_fd_t fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

#define ntg_write_fd_n           "write()"


#define ntg_write_console        ntg_write_fd


#define ntg_linefeed(p)          *p++ = LF;
#define NTG_LINEFEED_SIZE        1
#define NTG_LINEFEED             "\x0a"


#define ntg_rename_file(o, n)    rename((const char *) o, (const char *) n)
#define ntg_rename_file_n        "rename()"


#define ntg_change_file_access(n, a) chmod((const char *) n, a)
#define ntg_change_file_access_n "chmod()"


ntg_int_t ntg_set_file_time(u_char *name, ntg_fd_t fd, time_t s);
#define ntg_set_file_time_n      "utimes()"


#define ntg_file_info(file, sb)  stat((const char *) file, sb)
#define ntg_file_info_n          "stat()"

#define ntg_fd_info(fd, sb)      fstat(fd, sb)
#define ntg_fd_info_n            "fstat()"

#define ntg_link_info(file, sb)  lstat((const char *) file, sb)
#define ntg_link_info_n          "lstat()"

#define ntg_is_dir(sb)           (S_ISDIR((sb)->st_mode))
#define ntg_is_file(sb)          (S_ISREG((sb)->st_mode))
#define ntg_is_link(sb)          (S_ISLNK((sb)->st_mode))
#define ntg_is_exec(sb)          (((sb)->st_mode & S_IXUSR) == S_IXUSR)
#define ntg_file_access(sb)      ((sb)->st_mode & 0777)
#define ntg_file_size(sb)        (sb)->st_size
#define ntg_file_fs_size(sb)     ntg_max((sb)->st_size, (sb)->st_blocks * 512)
#define ntg_file_mtime(sb)       (sb)->st_mtime
#define ntg_file_uniq(sb)        (sb)->st_ino


ntg_int_t ntg_create_file_mapping(ntg_file_mapping_t *fm);
void ntg_close_file_mapping(ntg_file_mapping_t *fm);


#define ntg_realpath(p, r)       (u_char *) realpath((char *) p, (char *) r)
#define ntg_realpath_n           "realpath()"
#define ntg_getcwd(buf, size)    (getcwd((char *) buf, size) != NULL)
#define ntg_getcwd_n             "getcwd()"
#define ntg_path_separator(c)    ((c) == '/')


#if defined(PATH_MAX)

#define NTG_HAVE_MAX_PATH        1
#define NTG_MAX_PATH             PATH_MAX

#else

#define NTG_MAX_PATH             4096

#endif


#define NTG_DIR_MASK_LEN         0


ntg_int_t ntg_open_dir(ntg_str_t *name, ntg_dir_t *dir);
#define ntg_open_dir_n           "opendir()"


#define ntg_close_dir(d)         closedir((d)->dir)
#define ntg_close_dir_n          "closedir()"


ntg_int_t ntg_read_dir(ntg_dir_t *dir);
#define ntg_read_dir_n           "readdir()"


#define ntg_create_dir(name, access) mkdir((const char *) name, access)
#define ntg_create_dir_n         "mkdir()"


#define ntg_delete_dir(name)     rmdir((const char *) name)
#define ntg_delete_dir_n         "rmdir()"


#define ntg_dir_access(a)        (a | (a & 0444) >> 2)


#define ntg_de_name(dir)         ((u_char *) (dir)->de->d_name)
#if (NTG_HAVE_D_NAMLEN)
#define ntg_de_namelen(dir)      (dir)->de->d_namlen
#else
#define ntg_de_namelen(dir)      ntg_strlen((dir)->de->d_name)
#endif

static ntg_inline ntg_int_t
ntg_de_info(u_char *name, ntg_dir_t *dir)
{
    dir->type = 0;
    return stat((const char *) name, &dir->info);
}

#define ntg_de_info_n            "stat()"
#define ntg_de_link_info(name, dir)  lstat((const char *) name, &(dir)->info)
#define ntg_de_link_info_n       "lstat()"

#if (NTG_HAVE_D_TYPE)

/*
 * some file systems (e.g. XFS on Linux and CD9660 on FreeBSD)
 * do not set dirent.d_type
 */

#define ntg_de_is_dir(dir)                                                   \
    (((dir)->type) ? ((dir)->type == DT_DIR) : (S_ISDIR((dir)->info.st_mode)))
#define ntg_de_is_file(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_REG) : (S_ISREG((dir)->info.st_mode)))
#define ntg_de_is_link(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_LNK) : (S_ISLNK((dir)->info.st_mode)))

#else

#define ntg_de_is_dir(dir)       (S_ISDIR((dir)->info.st_mode))
#define ntg_de_is_file(dir)      (S_ISREG((dir)->info.st_mode))
#define ntg_de_is_link(dir)      (S_ISLNK((dir)->info.st_mode))

#endif

#define ntg_de_access(dir)       (((dir)->info.st_mode) & 0777)
#define ntg_de_size(dir)         (dir)->info.st_size
#define ntg_de_fs_size(dir)                                                  \
    ntg_max((dir)->info.st_size, (dir)->info.st_blocks * 512)
#define ntg_de_mtime(dir)        (dir)->info.st_mtime


ntg_int_t ntg_open_glob(ntg_glob_t *gl);
#define ntg_open_glob_n          "glob()"
ntg_int_t ntg_read_glob(ntg_glob_t *gl, ntg_str_t *name);
void ntg_close_glob(ntg_glob_t *gl);


ntg_err_t ntg_trylock_fd(ntg_fd_t fd);
ntg_err_t ntg_lock_fd(ntg_fd_t fd);
ntg_err_t ntg_unlock_fd(ntg_fd_t fd);

#define ntg_trylock_fd_n         "fcntl(F_SETLK, F_WRLCK)"
#define ntg_lock_fd_n            "fcntl(F_SETLKW, F_WRLCK)"
#define ntg_unlock_fd_n          "fcntl(F_SETLK, F_UNLCK)"


#if (NTG_HAVE_F_READAHEAD)

#define NTG_HAVE_READ_AHEAD      1

#define ntg_read_ahead(fd, n)    fcntl(fd, F_READAHEAD, (int) n)
#define ntg_read_ahead_n         "fcntl(fd, F_READAHEAD)"

#elif (NTG_HAVE_POSIX_FADVISE)

#define NTG_HAVE_READ_AHEAD      1

ntg_int_t ntg_read_ahead(ntg_fd_t fd, size_t n);
#define ntg_read_ahead_n         "posix_fadvise(POSIX_FADV_SEQUENTIAL)"

#else

#define ntg_read_ahead(fd, n)    0
#define ntg_read_ahead_n         "ntg_read_ahead_n"

#endif


#if (NTG_HAVE_O_DIRECT)

ntg_int_t ntg_directio_on(ntg_fd_t fd);
#define ntg_directio_on_n        "fcntl(O_DIRECT)"

ntg_int_t ntg_directio_off(ntg_fd_t fd);
#define ntg_directio_off_n       "fcntl(!O_DIRECT)"

#elif (NTG_HAVE_F_NOCACHE)

#define ntg_directio_on(fd)      fcntl(fd, F_NOCACHE, 1)
#define ntg_directio_on_n        "fcntl(F_NOCACHE, 1)"

#elif (NTG_HAVE_DIRECTIO)

#define ntg_directio_on(fd)      directio(fd, DIRECTIO_ON)
#define ntg_directio_on_n        "directio(DIRECTIO_ON)"

#else

#define ntg_directio_on(fd)      0
#define ntg_directio_on_n        "ntg_directio_on_n"

#endif

size_t ntg_fs_bsize(u_char *name);


#if (NTG_HAVE_OPENAT)

#define ntg_openat_file(fd, name, mode, create, access)                      \
    openat(fd, (const char *) name, mode|create, access)

#define ntg_openat_file_n        "openat()"

#define ntg_file_at_info(fd, name, sb, flag)                                 \
    fstatat(fd, (const char *) name, sb, flag)

#define ntg_file_at_info_n       "fstatat()"

#define NTG_AT_FDCWD             (ntg_fd_t) AT_FDCWD

#endif


#define ntg_stderr               STDERR_FILENO
#define ntg_set_stderr(fd)       dup2(fd, STDERR_FILENO)
#define ntg_set_stderr_n         "dup2(STDERR_FILENO)"


#if (NTG_HAVE_FILE_AIO)

ntg_int_t ntg_file_aio_init(ntg_file_t *file, ntg_pool_t *pool);
ssize_t ntg_file_aio_read(ntg_file_t *file, u_char *buf, size_t size,
    off_t offset, ntg_pool_t *pool);

extern ntg_uint_t  ntg_file_aio;

#endif

#if (NTG_THREADS)
ssize_t ntg_thread_read(ntg_thread_task_t **taskp, ntg_file_t *file,
    u_char *buf, size_t size, off_t offset, ntg_pool_t *pool);
#endif


#endif /* CORE_NTG_FILES_H_ */
