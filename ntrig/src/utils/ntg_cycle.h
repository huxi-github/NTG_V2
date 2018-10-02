/*
 * ntg_cycle.h
 *
 *  Created on: Aug 28, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_CYCLE_H_
#define UTILS_NTG_CYCLE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_queue.h"

#include "ntg_list.h"
#include "ntg_process.h"
#include "ntg_time.h"
#include "ntg_conf_file.h"

#ifndef NTG_CYCLE_POOL_SIZE
#define NTG_CYCLE_POOL_SIZE     NTG_DEFAULT_POOL_SIZE
#endif


#define NTG_DEBUG_POINTS_STOP   1
#define NTG_DEBUG_POINTS_ABORT  2

typedef struct ntg_shm_zone_s  ntg_shm_zone_t;

typedef ntg_int_t (*ntg_shm_zone_init_pt) (ntg_shm_zone_t *zone, void *data);

//共享内存区
//struct ntg_shm_zone_s {
//    void                     *data;
//    ntg_shm_t                 shm;
//    ntg_shm_zone_init_pt      init;
//    void                     *tag;
//};

//typedef struct ntg_cycle_s       ntg_cycle_t;
#define NTG_HTT_URL_MAX      200
/**
 * 全局循环对象
 * @brief 保存所有系统的相关信息
 */
struct ntg_cycle_s {
	/*
	 保存着所有模块存储配置项的结构体指针，
	 它首先是一个数组，数组大小为ntg_max_module，正好与Nginx的module个数一样；
	 每个数组成员又是一个指针，指向另一个存储着指针的数组，因此会看到void ****
	 */
	void ****conf_ctx;//系统配置上下文
	ntg_uint_t   blc_type;

	/*******************/
	ntg_remote_t *rmt;//远程管理对象
	ntg_db_mysql_t *db;//数据库对象

	/******************************/
	//主进程工作数据
	struct event *et_user;//TODO 用户定时事件
	ntg_user_manage_t *manage;

	/*******************************/
	//工作进程

	//请求对象池
	ntg_http_request_t *requests;///请求对象资源
	ntg_int_t			request_n;///请求数
	ntg_http_request_t *free_reqs;///可用请求对象
	ntg_int_t           free_req_n;///可用请求数

	/*******************************/
	//记录进程
	ntg_record_manage_t *record_mg;
	/*******************************/
	struct event_base *base;
	struct evdns_base *dns;
	/********************************/
	// 内存池
	ntg_pool_t *pool;

	/*
	 日志模块中提供了生成基本ntg_log_t日志对象的功能，这里的log实际上是在还没有执行ntg_init_cycle方法前，
	 也就是还没有解析配置前，如果有信息需要输出到日志，就会暂时使用log对象，它会输出到屏幕。
	 在ntg_init_cycle方法执行后，将会根据nginx.conf配置文件中的配置项，构造出正确的日志文件，此时会对log重新赋值。
	 */
	ntg_log_t *log;
	/*
	 由nginx.conf配置文件读取到日志文件路径后，将开始初始化error_log日志文件，由于log对象还在用于输出日志到屏幕，
	 这时会用new_log对象暂时性地替代log日志，待初始化成功后，会用new_log的地址覆盖上面的log指针
	 */
	ntg_log_t new_log;
	//将标准输入定向到错误日志fd
    ntg_uint_t log_use_stderr;  /* unsigned  log_use_stderr:1; */

	/*
	 动态数组容器，它保存着nginx所有要操作的目录。如果有目录不存在，就会试图创建，而创建目录失败就会导致nginx启动失败。
	 */
	ntg_array_t paths;
	/*
	 单链表容器，元素类型是ntg_open_file_t 结构体，它表示nginx已经打开的所有文件。事实上，nginx框架不会向open_files链表中添加文件。
	 而是由对此感兴趣的模块向其中添加文件路径名，nginx框架会在ntg_init_cycle 方法中打开这些文件
	 */
	ntg_list_t open_files;



	// 当前进程中所有链接对象的总数，与connections成员配合使用

	uint files_n;

	// 指向当前进程中的所有连接对象，与connection_n配合使用
//	ntg_connection_t *connections;

	/*
	 旧的ntg_cycle_t 对象用于引用上一个ntg_cycle_t 对象中的成员。例如ntg_init_cycle 方法，在启动初期，
	 需要建立一个临时的ntg_cycle_t对象保存一些变量，再调用ntg_init_cycle 方法时就可以把旧的ntg_cycle_t 对象传进去，
	 而这时old_cycle对象就会保存这个前期的ntg_cycle_t对象。
	 */
	ntg_cycle_t *old_cycle;

	// 配置文件相对于安装目录的路径名称
	ntg_str_t conf_file;
	// nginx 处理配置文件时需要特殊处理的在命令行携带的参数，一般是-g 选项携带的参数
	ntg_str_t conf_param;
	// nginx配置文件所在目录的路径
	ntg_str_t conf_prefix;
	//nginx安装目录的路径
	ntg_str_t prefix;
	// 用于进程间同步的文件锁名称
	ntg_str_t lock_file;
	// 使用gethostname系统调用得到的主机名
	ntg_str_t hostname;

	ntg_int_t vt_users;///模拟的虚拟用户数

	uint url_n;
	char *ntg_urls[NTG_HTT_URL_MAX];

	struct in_addr addr;

	unsigned isurl:1;///url配置标志
	unsigned isaddr:1;///本地地址标志
};

typedef struct ntg_front_listen_s {
	struct sockaddr_in sin;
} ntg_front_listen_t;

typedef struct ntg_database_conf_s {
	ntg_str_t type;//数据库类型
//	struct sockaddr_in sin;//地址信息
	ntg_str_t host;//主机
	ntg_str_t name;//数据库名称
	ntg_uint_t port;//端口
	ntg_str_t user;//主机
	ntg_str_t password;//数据库名称
} ntg_database_conf_t;

/**
 * 核心配置对象
 */
typedef struct {
     ntg_flag_t               daemon;///< 守护进程标志
     ntg_flag_t               master;///< 系统进程模式(单进程还是多进程)

     ntg_msec_t               timer_resolution;///< 定时精度
     ntg_int_t				  consumes;///< 系统支持的消费者数
     ntg_front_listen_t		  front;
     ntg_database_conf_t	  database;
     ntg_int_t                worker_processes;///< 工作进程数
     ntg_int_t                debug_points;///< 调试点
     //相关限制
     ntg_int_t                rlimit_nofile;///< 文件打开数限制
     ntg_int_t                rlimit_sigpending;///< TODO
     off_t                    rlimit_core;///<

     int                      priority;///< 优先级
     //cpu的亲和度
     unsigned int             cpu_affinity_n;///< cpu亲和力
     unsigned int             *cpu_affinity;///< cpu亲和力数组
     cpu_set_t                cpus;///< cpu集
     //用户信息
     char                    *username;///< 用户名
     ntg_uid_t                user;///< 用户id
     ntg_gid_t                group;///< 组id

     ntg_str_t                working_directory;///< 工作目录
     ntg_str_t                lock_file;///< 锁文件

     ntg_str_t                pid;///< 进程id文件
     ntg_str_t                oldpid;///< 老的进程id文件

     ntg_array_t              env;///< 环境变量数组
     char                   **environment;///< 环境变量字符串

} ntg_core_conf_t;


#define ntg_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)


ntg_cycle_t *ntg_init_cycle(ntg_cycle_t *old_cycle);
ntg_int_t ntg_create_pidfile(ntg_str_t *name, ntg_log_t *log);
void ntg_delete_pidfile(ntg_cycle_t *cycle);
ntg_int_t ntg_signal_process(ntg_cycle_t *cycle, char *sig);
void ntg_reopen_files(ntg_cycle_t *cycle, ntg_uid_t user);
char **ntg_set_environment(ntg_cycle_t *cycle, ntg_uint_t *last);
ntg_pid_t ntg_exec_new_binary(ntg_cycle_t *cycle, char *const *argv);
ntg_uint_t ntg_get_cpu_affinity(ntg_uint_t n);
ntg_shm_zone_t *ntg_shared_memory_add(ntg_conf_t *cf, ntg_str_t *name,
    size_t size, void *tag);


extern volatile ntg_cycle_t  *ntg_cycle;
extern ntg_array_t            ntg_old_cycles;
extern ntg_module_t           ntg_core_module;
extern ntg_uint_t             ntg_test_config;
extern ntg_uint_t             ntg_quiet_mode;


#endif /* UTILS_NTG_CYCLE_H_ */
