/*
 * ntg_process_cycle.h
 *
 *  Created on: Oct 15, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_PROCESS_CYCLE_H_
#define UTILS_NTG_PROCESS_CYCLE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

/**通过channel通信的命令 */
#define NTG_CMD_OPEN_CHANNEL   1///< 打开channel,向其他进程传递描述符
#define NTG_CMD_CLOSE_CHANNEL  2///< 关闭channel
#define NTG_CMD_QUIT           3///< 退出
#define NTG_CMD_TERMINATE      4///< 终止
#define NTG_CMD_REOPEN         5///< 重启

/**
 * @name 进程类别
 * @{
 */
#define NTG_PROCESS_SINGLE     0///< 单进程
#define NTG_PROCESS_MASTER     1///< 主进程
#define NTG_PROCESS_SIGNALLER  2///< 信号进程

#define NTG_PROCESS_VIRTUAL    3///< 虚拟用户进程
#define NTG_PROCESS_TRAFFIC    4///< 流量产生进程
#define NTG_PROCESS_RECORD     5///< 日志记录进程

#define NTG_PROCESS_HELPER     6///< 帮助进程
/**@}*/

typedef struct {
    ntg_event_handler_pt       handler;
    char                      *name;
    ntg_msec_t                 delay;
} ntg_cache_manager_ctx_t;

void event_failed_cd(int err);
void ntg_master_process_cycle(ntg_cycle_t *cycle);
void ntg_single_process_cycle(ntg_cycle_t *cycle);


extern ntg_uint_t      ntg_process;
extern ntg_pid_t       ntg_pid;
extern ntg_pid_t       ntg_new_binary;
extern ntg_uint_t      ntg_inherited;
extern ntg_uint_t      ntg_daemonized;
extern ntg_uint_t      ntg_exiting;

extern sig_atomic_t    ntg_reap;
extern sig_atomic_t    ntg_sigio;
extern sig_atomic_t    ntg_sigalrm;
extern sig_atomic_t    ntg_quit;
extern sig_atomic_t    ntg_debug_quit;
extern sig_atomic_t    ntg_terminate;
extern sig_atomic_t    ntg_noaccept;
extern sig_atomic_t    ntg_reconfigure;
extern sig_atomic_t    ntg_add;
extern sig_atomic_t    ntg_change_binary;


extern sig_atomic_t  ntg_connections;
extern sig_atomic_t  ntg_requests;

#endif /* UTILS_NTG_PROCESS_CYCLE_H_ */
