/*
 * ntg_process.h
 *
 *  Created on: Aug 28, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_PROCESS_H_
#define UTILS_NTG_PROCESS_H_

#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include "ntg_cycle.h"
#include "ntg_socket.h"

typedef pid_t	ntg_pid_t;

#define NTG_INVALID_PID	-1

typedef  void (*ntg_spawn_proc_pt) (ntg_cycle_t *cycle, void *data);

/**
 * @name 进程对象
 */
typedef struct {
    pid_t				pid;///< 进程ID
    unsigned            type;///< 进程类型
    int					status;///< 由waitpid系统调用获取到的进程状态
    int     			channel[2];///< sockpair
    struct event        *event;///通道监听事件
    ntg_log_t			*log;///日志

    ntg_spawn_proc_pt   proc;///< 子进程的循环执行方法
    void               *data;///< 传递给子进程的数据
    char               *name; ///< 进程名称。操作系统中显示的进程名称与name相同

    ntg_uint_t			reqs;///< 进程当前的请求对象数reqs
    ntg_uint_t          failes;///< 失败次数
    /** 重生标志 */
    unsigned            respawn:1; ///< 为1表示在重新生成子进程
    unsigned            just_spawn:1;///< 为1表示正在生成子进程
    unsigned            detached:1;///< 为1表示分离父，子进程
    unsigned            exiting:1;///< 为1表示进程正在退出
    unsigned            exited:1;///< 为1表示进程已经退出
} ntg_process_t;

/**
 * @name 执行上下文
 */
typedef struct {
    char         *path;///< 路经
    char         *name;///< 名称
    char *const  *argv;///< 传入参数指针
    char *const  *envp;///< 环境指针
} ntg_exec_ctx_t;

#define NTG_MAX_PROCESSES         1024 ///< 最大的进程数

/**
 * @name 子进程类型
 */
#define NTG_VIRTUAL_PROCESS	0x01    ///< 用户管理进程
#define NTG_TRAFFIC_PROCESS	0x02	///< 流量产生进程
#define NTG_RECORD_PROCESS  0x04	///< 记录进程
#define NTG_CACHE_PROCESS	0x08	///< cache管理进程

/**
 * @name 产生进程的重生标志
 * @{
 */
#define NTG_PROCESS_NORESPAWN     -1	///< 不重生
#define NTG_PROCESS_JUST_SPAWN    -2	///< 仅仅产生
#define NTG_PROCESS_RESPAWN       -3	///< 重生
#define NTG_PROCESS_JUST_RESPAWN  -4	///< 正在
#define NTG_PROCESS_DETACHED      -5	///< 分离标志
/**@}*/

#define ntg_getpid   getpid

#ifndef ntg_log_pid
#define ntg_log_pid  ntg_pid
#endif


ntg_pid_t ntg_spawn_process(ntg_cycle_t *cycle,
    ntg_spawn_proc_pt proc, void *data, char *name, ntg_int_t respawn, ntg_uint_t type, ntg_int_t point);
ntg_pid_t ntg_execute(ntg_cycle_t *cycle, ntg_exec_ctx_t *ctx);
ntg_int_t ntg_init_base_and_signals(ntg_cycle_t *log);
ntg_int_t
ntg_os_signal_process(ntg_cycle_t *cycle, char *name, ntg_int_t pid);
void ntg_debug_point(void);


#if (NTG_HAVE_SCHED_YIELD)
#define ntg_sched_yield()  sched_yield()
#else
#define ntg_sched_yield()  usleep(1)
#endif


#if (NTG_HAVE_SCHED_YIELD)
#define ntg_sched_yield()  sched_yield()
#else
#define ntg_sched_yield()  usleep(1)
#endif




extern int            ntg_argc;
extern char         **ntg_argv;
extern char         **ntg_os_argv;

extern ntg_pid_t      ntg_pid;
extern ntg_socket_t   ntg_channel;
extern ntg_int_t      ntg_process_slot;
extern ntg_int_t 	  ntg_start_process;
extern ntg_int_t      ntg_last_process;
extern ntg_process_t  ntg_processes[NTG_MAX_PROCESSES];

extern ntg_int_t      ntg_current_process;

#endif /* UTILS_NTG_PROCESS_H_ */
