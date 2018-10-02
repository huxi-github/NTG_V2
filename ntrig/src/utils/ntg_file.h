/*
 * ntg_file.h
 *
 *  Created on: Jul 26, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_FILE_H_
#define CORE_NTG_FILE_H_

#include "../ntg_config.h"

#include "ntg_files.h"
#include "ntg_palloc.h"
#include "ntg_atomic.h"
#include "ntg_conf_file.h"



/**
 * 文件结构
 */
struct ntg_file_s {
    ntg_fd_t                   fd;  //文件描述符
    ntg_str_t                  name; //文件名称
    ntg_file_info_t            info;  //文件大小等信息，实际上就是linux系统定义的stat结构

    //该偏移量告诉nginx现在处理到文件何处了，一般不用手动设置
    off_t                      offset;
    //当前文件系统偏移量，一般不用设置
    off_t                      sys_offset;

    //日志对象，相关的日志会输出到log指定的日志文件中
    ntg_log_t                 *log;

#if (NGX_HAVE_FILE_AIO)
    ntg_event_aio_t           *aio;
#endif

    //目前未使用
    unsigned                   valid_info:1;
    //与配置文件中的directio配置项对应，在发送大文件的时候可以设置为1
    unsigned                   directio:1;
};

//typedef struct ntg_open_file_s ntg_open_file_t;
//struct ntg_open_file_s {
//    ntg_fd_t              fd;
//    ntg_str_t             name;
//
//    u_char               *buffer;
//    u_char               *pos;
//    u_char               *last;
//
//#if 0
//    /* e.g. append mode, error_log */
//    ntg_uint_t            flags;
//    /* e.g. reopen db file */
//    ntg_uint_t          (*handler)(void *data, ntg_open_file_t *file);
//    void                 *data;
//#endif
//};

#define NGX_MAX_PATH_LEVEL  3


typedef time_t (*ntg_path_manager_pt) (void *data);
typedef void (*ntg_path_loader_pt) (void *data);


/**
 * @name 路经对象
 * @note 该对象用于缓存管理，表示缓存的路径信息
 */
typedef struct {
    ntg_str_t                  name;///路经名称
    size_t                     len;///中间hash路经总长度
    size_t                     level[3];///中间hash路经中各层名字长度(共3层)

    ntg_path_manager_pt        manager;///路径的管理处理函数
    ntg_path_loader_pt         loader;///路径的加载处理函数
    void                      *data;///关联的数据

    u_char                    *conf_file;
    ntg_uint_t                 line;
} ntg_path_t;


typedef struct {
    ntg_str_t                  name;
    size_t                     level[3];
} ntg_path_init_t;


typedef struct {
    ntg_file_t                 file;
    off_t                      offset;
    ntg_path_t                *path;
    ntg_pool_t                *pool;
    char                      *warn;

    ntg_uint_t                 access;

    unsigned                   log_level:8;
    unsigned                   persistent:1;
    unsigned                   clean:1;
} ntg_temp_file_t;


typedef struct {
    ntg_uint_t                 access;
    ntg_uint_t                 path_access;
    time_t                     time;
    ntg_fd_t                   fd;

    unsigned                   create_path:1;
    unsigned                   delete_file:1;

    ntg_log_t                 *log;
} ntg_ext_rename_file_t;


typedef struct {
    off_t                      size;
    size_t                     buf_size;

    ntg_uint_t                 access;
    time_t                     time;

    ntg_log_t                 *log;
} ntg_copy_file_t;


typedef struct ntg_tree_ctx_s  ntg_tree_ctx_t;

typedef ntg_int_t (*ntg_tree_init_handler_pt) (void *ctx, void *prev);
typedef ntg_int_t (*ntg_tree_handler_pt) (ntg_tree_ctx_t *ctx, ntg_str_t *name);

struct ntg_tree_ctx_s {
    off_t                      size;
    off_t                      fs_size;
    ntg_uint_t                 access;
    time_t                     mtime;

    ntg_tree_init_handler_pt   init_handler;
    ntg_tree_handler_pt        file_handler;
    ntg_tree_handler_pt        pre_tree_handler;
    ntg_tree_handler_pt        post_tree_handler;
    ntg_tree_handler_pt        spec_handler;

    void                      *data;
    size_t                     alloc;

    ntg_log_t                 *log;
};


ntg_int_t ntg_get_full_name(ntg_pool_t *pool, ntg_str_t *prefix,
    ntg_str_t *name);

ssize_t ntg_write_chain_to_temp_file(ntg_temp_file_t *tf, ntg_chain_t *chain);
ntg_int_t ntg_create_temp_file(ntg_file_t *file, ntg_path_t *path,
    ntg_pool_t *pool, ntg_uint_t persistent, ntg_uint_t clean,
    ntg_uint_t access);
void ntg_create_hashed_filename(ntg_path_t *path, u_char *file, size_t len);
ntg_int_t ntg_create_path(ntg_file_t *file, ntg_path_t *path);
ntg_err_t ntg_create_full_path(u_char *dir, ntg_uint_t access);
ntg_int_t ntg_add_path(ntg_conf_t *cf, ntg_path_t **slot);
ntg_int_t ntg_create_paths(ntg_cycle_t *cycle, ntg_uid_t user);
ntg_int_t ntg_ext_rename_file(ntg_str_t *src, ntg_str_t *to,
    ntg_ext_rename_file_t *ext);
ntg_int_t ntg_copy_file(u_char *from, u_char *to, ntg_copy_file_t *cf);
ntg_int_t ntg_walk_tree(ntg_tree_ctx_t *ctx, ntg_str_t *tree);

ntg_atomic_uint_t ntg_next_temp_number(ntg_uint_t collision);

char *ntg_conf_set_path_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_merge_path_value(ntg_conf_t *cf, ntg_path_t **path,
    ntg_path_t *prev, ntg_path_init_t *init);
char *ntg_conf_set_access_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);


extern ntg_atomic_t      *ntg_temp_number;
extern ntg_atomic_int_t   ntg_random_number;

#endif /* CORE_NTG_FILE_H_ */
