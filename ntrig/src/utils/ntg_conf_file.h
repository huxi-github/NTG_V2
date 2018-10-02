/*
 * ntg_config_file.h
 *
 *  Created on: Aug 18, 2015
 *      Author: tzh
 */

#ifndef CORE_NTG_CONF_FILE_H_
#define CORE_NTG_CONF_FILE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_cycle.h"
#include "ntg_buf.h"
#include "ntg_array.h"

/*
 *        AAAA  number of agruments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */
/**参数个数配置宏(个数0-7)*/
#define NTG_CONF_NOARGS      0x00000001 ///< 没有参数
#define NTG_CONF_TAKE1       0x00000002
#define NTG_CONF_TAKE2       0x00000004
#define NTG_CONF_TAKE3       0x00000008
#define NTG_CONF_TAKE4       0x00000010
#define NTG_CONF_TAKE5       0x00000020
#define NTG_CONF_TAKE6       0x00000040
#define NTG_CONF_TAKE7       0x00000080

#define NTG_CONF_MAX_ARGS    8	///< 最大配置参数数

/**组合个数参数宏*/
#define NTG_CONF_TAKE12      (NTG_CONF_TAKE1|NTG_CONF_TAKE2)
#define NTG_CONF_TAKE13      (NTG_CONF_TAKE1|NTG_CONF_TAKE3)

#define NTG_CONF_TAKE23      (NTG_CONF_TAKE2|NTG_CONF_TAKE3)

#define NTG_CONF_TAKE123     (NTG_CONF_TAKE1|NTG_CONF_TAKE2|NTG_CONF_TAKE3)
#define NTG_CONF_TAKE1234    (NTG_CONF_TAKE1|NTG_CONF_TAKE2|NTG_CONF_TAKE3   \
                              |NTG_CONF_TAKE4)

#define NTG_CONF_ARGS_NUMBER 0x000000ff		///参数标志区间
#define NTG_CONF_BLOCK       0x00000100		///配置指令可以接受一个配置块，如server
#define NTG_CONF_FLAG        0x00000200		///配置标志
#define NTG_CONF_ANY         0x00000400		///任意配置
#define NTG_CONF_1MORE       0x00000800		///一个或多个参数
#define NTG_CONF_2MORE       0x00001000		///2个或多个参数
#define NTG_CONF_MULTI       0x00002000		///3个及以上

#define NTG_DIRECT_CONF      0x00010000		///可以出现在配置文件的最外层，例如master_process、daemon

#define NTG_MAIN_CONF        0x01000000		///http、events等
#define NTG_ANY_CONF         0x0F000000		///该配置指令可以出现在任意配置级别上


/**配置项初始化的默认值*/
#define NTG_CONF_UNSET       -1
#define NTG_CONF_UNSET_UINT  (ntg_uint_t) -1
#define NTG_CONF_UNSET_PTR   (void *) -1
#define NTG_CONF_UNSET_SIZE  (size_t) -1
#define NTG_CONF_UNSET_MSEC  (ntg_msec_t) -1

/**配置模块相关函数的返回值*/
#define NTG_CONF_OK          NULL
#define NTG_CONF_ERROR       (void *) -1

/**配置状态标志*/
#define NTG_CONF_BLOCK_START 1
#define NTG_CONF_BLOCK_DONE  2
#define NTG_CONF_FILE_DONE   3

/**核心模块与配置模块的魔数*/
#define NTG_CORE_MODULE      0x45524F43  /* "CORE" */
#define NTG_CONF_MODULE      0x464E4F43  /* "CONF" */


#define NTG_MAX_CONF_ERRSTR  1024///最大的配置错误字符串长度


/**
 * commands数组用于定义模块的配置文件参数
 */
struct ntg_command_s {
    ntg_str_t             name;///< 配置项名称


    ntg_uint_t            type;///< 配置项类型(有几个参数或者可以在什么地方出现等)

    ///出现了name中制定的配置项后，将会调用set方法处理配置项参数。
    ///这个可以使用预设的14个解析配置方法，也可以使用自定义的。
	///cf参数保存了从配置文件中读取到的原始字符串以及相关的一些信息，其中有一个args字段，它表示配置指令以及该配置指令的参数，ntg_str_t类型。
    ///conf就是定义的存储这个配置值的结构体，在使用的时候需要转换成自己使用的类型。
	char               *(*set)(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);

    ///在配置文件中的偏移量，它的取值范围是：
    /*
    NTG_HTTP_MAIN_CONF_OFFSET
    NTG_HTTP_SRV_CONF_OFFSET
    NTG_HTTP_LOC_CONF_OFFSET
    */
    ///因为有可能模块同时会有main，srv，loc三种配置结构体，但是这个配置项解析完后要放在哪个结构体内呢？
	///当设置为0时，就是NTG_HTTP_MAIN_CONF_OFFSET
    ntg_uint_t            conf;

    ///表示当前配置项在整个存储配置项的结构体中的偏移位置，
    ///可以使用offsetof(test_stru, b)来获取
	///对于有些配置项，它的值不需要保存，就可以设置为0。
    ntg_uint_t            offset;

    ///命令处理完后的回调指针，对于set的14种预设的解析配置方法， 可能的结构有：
    /*
    ntg_conf_post_t
    ntg_conf_enum_t
    ntg_conf_bitmask_t
    null
    */
    void                 *post;
};

#define ntg_null_command  { ntg_null_string, 0, NULL, 0, 0, NULL }  ///空命令

/**
 * 打开的文件对象
 */
struct ntg_open_file_s {
    ntg_fd_t              fd;///文件描述符
    ntg_str_t             name;///文件名称

    void                (*flush)(ntg_open_file_t *file, ntg_log_t *log);///TODO
    void                 *data;///TODO
};

#define NTG_MODULE_V1          0, 0, 0, 0, 0, 0, 1      ///该宏用来初始化前7个字段
#define NTG_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0   ///该宏用来初始化最后8个字段



///ntg_module_s是模块的定义
struct ntg_module_s {
    ///对于一类模块（由下面的type成员决定类别）而言，ctx_index标示当前模块在这类模块中的序号。
    ///这个成员常常是由管理这类模块的一个nginx核心模块设置的，对于所有的HTTP模块而言，ctx_index
    ///是由核心模块ntg_http_module设置的。
    ntg_uint_t            ctx_index;

    ///index表示当前模块在ntg_modules数组中的序号。Nginx启动的时候会根据ntg_modules数组设置各个模块的index值
    ntg_uint_t            index;

    ntg_uint_t            spare0;
    ntg_uint_t            spare1;
    ntg_uint_t            spare2;
    ntg_uint_t            spare3;
    ///nginx模块版本，目前只有一种，暂定为1
    ntg_uint_t            version;

    ///模块上下文，每个模块有不同模块上下文,每个模块都有自己的特性，而ctx会指向特定类型模块的公共接口。
    ///比如，在HTTP模块中，ctx需要指向ntg_http_module_t结构体。
    void                 *ctx;

    ///模块命令集，将处理nginx.conf中的配置项
    ntg_command_t        *commands;

    ///标示该模块的类型，和ctx是紧密相关的。它的取值范围是以下几种:
    ///NTG_HTTP_MODULE,NTG_CORE_MODULE,NTG_CONF_MODULE,
    ///NTG_EVENT_MODULE,NTG_MAIL_MODULE
    ntg_uint_t            type;

    ///下面7个函数是nginx在启动，停止过程中的7个执行点
    ntg_int_t           (*init_master)(ntg_cycle_t *cycle);     ///初始化master，在master_cycle中被调用
    ntg_int_t           (*init_module)(ntg_cycle_t *cycle);     ///初始化模块，在init_cycle被调用
    ntg_int_t           (*init_process)(ntg_cycle_t *cycle);    ///初始化进程，在worker_cycle中被调用
    ntg_int_t           (*init_thread)(ntg_cycle_t *cycle);     ///初始化线程
    void                (*exit_thread)(ntg_cycle_t *cycle);     ///退出线程
    void                (*exit_process)(ntg_cycle_t *cycle);    ///退出进程
    void                (*exit_master)(ntg_cycle_t *cycle);     ///退出master
    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};

///参考：
///http:///blog.csdn.net/livelylittlefish/article/details/7247080
typedef struct {
    ntg_str_t             name;                                         /////模块名，即ntg_core_module_ctx结构体对象的
    void               *(*create_conf)(ntg_cycle_t *cycle);             ///解析配置项茜，nginx框架会调用create_conf方法
    char               *(*init_conf)(ntg_cycle_t *cycle, void *conf);   ///解析配置项完成后，nginx框架会调用init_conf方法
} ntg_core_module_t;

/**
 * 配置文件对象
 */
typedef struct {
    ntg_file_t            file;///< 文件对象
    ntg_buf_t            *buffer;///< buffer对象
    ntg_uint_t            line;///< 记录当前的行
} ntg_conf_file_t;



typedef char *(*ntg_conf_handler_pt)(ntg_conf_t *cf,
    ntg_command_t *dummy, void *conf);

/**
 * 配置对象
 */
struct ntg_conf_s {
    char                 *name;  ///<没有使用
    ntg_array_t          *args;  ///<指令的参数

    ntg_cycle_t          *cycle; ///< 全局的循环对象cycle
    ntg_pool_t           *pool;  ///< 内存池
    ntg_pool_t           *temp_pool; ///<分配临时数据空间的内存池
    ntg_conf_file_t      *conf_file; ///<配置文件的信息
    ntg_log_t            *log; ///<日志

    void                 *ctx;  ///<模块的配置信息
    ntg_uint_t            module_type; ///<模块的类型
    ntg_uint_t            cmd_type; ///<命令的类型

    ntg_conf_handler_pt   handler; ///<指令处理函数，有自己行为的在这里实现
    char                 *handler_conf; ///<指令处理函数的配置信息
};

///配置过时处理函数指针
typedef char *(*ntg_conf_post_handler_pt) (ntg_conf_t *cf,
    void *data, void *conf);
///过时处理对象
typedef struct {
    ntg_conf_post_handler_pt  post_handler;
} ntg_conf_post_t;

///配置过时对象
typedef struct {
    ntg_conf_post_handler_pt  post_handler;
    char                     *old_name;
    char                     *new_name;
} ntg_conf_deprecated_t;


typedef struct {
    ntg_conf_post_handler_pt  post_handler;
    ntg_int_t                 low;
    ntg_int_t                 high;
} ntg_conf_num_bounds_t;


typedef struct {
    ntg_str_t                 name;
    ntg_uint_t                value;
} ntg_conf_enum_t;


#define NTG_CONF_BITMASK_SET  1

typedef struct {
    ntg_str_t                 name;
    ntg_uint_t                mask;
} ntg_conf_bitmask_t;



char * ntg_conf_deprecated(ntg_conf_t *cf, void *post, void *data);
char *ntg_conf_check_num_bounds(ntg_conf_t *cf, void *post, void *data);


#define ntg_get_conf(conf_ctx, module)  conf_ctx[module.index]


#define ntg_conf_init_value(conf, default)                                   \
    if (conf == NTG_CONF_UNSET) {                                            \
        conf = default;                                                      \
    }

#define ntg_conf_init_ptr_value(conf, default)                               \
    if (conf == NTG_CONF_UNSET_PTR) {                                        \
        conf = default;                                                      \
    }

#define ntg_conf_init_uint_value(conf, default)                              \
    if (conf == NTG_CONF_UNSET_UINT) {                                       \
        conf = default;                                                      \
    }

#define ntg_conf_init_size_value(conf, default)                              \
    if (conf == NTG_CONF_UNSET_SIZE) {                                       \
        conf = default;                                                      \
    }

#define ntg_conf_init_msec_value(conf, default)                              \
    if (conf == NTG_CONF_UNSET_MSEC) {                                       \
        conf = default;                                                      \
    }

#define ntg_conf_merge_value(conf, prev, default)                            \
    if (conf == NTG_CONF_UNSET) {                                            \
        conf = (prev == NTG_CONF_UNSET) ? default : prev;                    \
    }

#define ntg_conf_merge_ptr_value(conf, prev, default)                        \
    if (conf == NTG_CONF_UNSET_PTR) {                                        \
        conf = (prev == NTG_CONF_UNSET_PTR) ? default : prev;                \
    }

#define ntg_conf_merge_uint_value(conf, prev, default)                       \
    if (conf == NTG_CONF_UNSET_UINT) {                                       \
        conf = (prev == NTG_CONF_UNSET_UINT) ? default : prev;               \
    }

#define ntg_conf_merge_msec_value(conf, prev, default)                       \
    if (conf == NTG_CONF_UNSET_MSEC) {                                       \
        conf = (prev == NTG_CONF_UNSET_MSEC) ? default : prev;               \
    }

#define ntg_conf_merge_sec_value(conf, prev, default)                        \
    if (conf == NTG_CONF_UNSET) {                                            \
        conf = (prev == NTG_CONF_UNSET) ? default : prev;                    \
    }

#define ntg_conf_merge_size_value(conf, prev, default)                       \
    if (conf == NTG_CONF_UNSET_SIZE) {                                       \
        conf = (prev == NTG_CONF_UNSET_SIZE) ? default : prev;               \
    }

#define ntg_conf_merge_off_value(conf, prev, default)                        \
    if (conf == NTG_CONF_UNSET) {                                            \
        conf = (prev == NTG_CONF_UNSET) ? default : prev;                    \
    }

#define ntg_conf_merge_str_value(conf, prev, default)                        \
    if (conf.data == NULL) {                                                 \
        if (prev.data) {                                                     \
            conf.len = prev.len;                                             \
            conf.data = prev.data;                                           \
        } else {                                                             \
            conf.len = sizeof(default) - 1;                                  \
            conf.data = (u_char *) default;                                  \
        }                                                                    \
    }

#define ntg_conf_merge_bufs_value(conf, prev, default_num, default_size)     \
    if (conf.num == 0) {                                                     \
        if (prev.num) {                                                      \
            conf.num = prev.num;                                             \
            conf.size = prev.size;                                           \
        } else {                                                             \
            conf.num = default_num;                                          \
            conf.size = default_size;                                        \
        }                                                                    \
    }

#define ntg_conf_merge_bitmask_value(conf, prev, default)                    \
    if (conf == 0) {                                                         \
        conf = (prev == 0) ? default : prev;                                 \
    }


char *ntg_conf_param(ntg_conf_t *cf);
char *ntg_conf_parse(ntg_conf_t *cf, ntg_str_t *filename);
char *ntg_conf_include(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);


ntg_int_t ntg_conf_full_name(ntg_cycle_t *cycle, ntg_str_t *name,
    ntg_uint_t conf_prefix);
ntg_open_file_t *ntg_conf_open_file(ntg_cycle_t *cycle, ntg_str_t *name);
void  ntg_conf_log_error(ntg_uint_t level, ntg_conf_t *cf,
    ntg_err_t err, const char *fmt, ...);


char *ntg_conf_set_flag_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_str_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_str_array_slot(ntg_conf_t *cf, ntg_command_t *cmd,
    void *conf);
char *ntg_conf_set_keyval_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_num_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_size_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_off_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_msec_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_sec_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_bufs_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_enum_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
char *ntg_conf_set_bitmask_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);


extern ntg_uint_t     ntg_max_module;
extern ntg_module_t  *ntg_modules[];



#endif /* CORE_NTG_CONF_FILE_H_ */
