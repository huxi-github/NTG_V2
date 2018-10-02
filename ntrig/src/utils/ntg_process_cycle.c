/*
 * ntg_process_cycle.c
 *
 *  Created on: Oct 13, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_cycle.h"
#include "ntg_process_cycle.h"

#include "ntg_channel.h"
#include "ntg_times.h"
#include "ntg_setproctitle.h"
#include "ntg_setaffinity.h"
#include "ntg_array.h"
#include "ntg_remote.h"
#include "ntg_database.h"
#include "../user/ntg_user.h"
#include "../record/ntg_record.h"

static void ntg_connect_handler(evutil_socket_t fd, short what, void *arg);
static void ntg_request_handler(evutil_socket_t fd, short what, void *arg);

static void ntg_message_handler(evutil_socket_t fd, short what, void *arg);

static void ntg_start_virtual_process(ntg_cycle_t *cycle, ntg_int_t n,
		ntg_int_t type);
static void ntg_start_record_process(ntg_cycle_t *cycle, ntg_int_t n,
		ntg_int_t type);
static void ntg_start_traffic_processes(ntg_cycle_t *cycle, ntg_int_t n,
		ntg_int_t type);

static void ntg_pass_open_channel(ntg_cycle_t *cycle, ntg_message_t *ch,
		ntg_int_t type);
static void ntg_signal_worker_processes(ntg_cycle_t *cycle, int signo);
static ntg_uint_t ntg_reap_children(ntg_cycle_t *cycle);
static void ntg_master_process_exit(ntg_cycle_t *cycle);

static void ntg_virtual_process_cycle(ntg_cycle_t *cycle, void *data);
static void ntg_traffic_process_cycle(ntg_cycle_t *cycle, void *data);
static void ntg_worker_process_init(ntg_cycle_t *cycle, ntg_int_t worker);
static void ntg_worker_process_exit(ntg_cycle_t *cycle);

static void ntg_record_process_cycle(ntg_cycle_t *cycle, void *data);
//static void ntg_record_process_init(ntg_cycle_t *cycle, ntg_int_t worker);
static void ntg_result_cb(evutil_socket_t fd, short whart, void *arg);
static void ntg_channel_handler(evutil_socket_t fd, short whart, void *arg);

ntg_uint_t ntg_process;///< 进程所属的类型
ntg_pid_t ntg_pid;///< 进程id
/**
 * @name 信号处理程序中的事件标志
 * @{
 */
sig_atomic_t ntg_reap;///< 有子进程退出,收割标志
sig_atomic_t ntg_sigio;///< io标志
sig_atomic_t ntg_sigalrm;///< 定时信号
sig_atomic_t ntg_terminate;///< 终止
sig_atomic_t ntg_quit;///< 退出
sig_atomic_t ntg_debug_quit;///< 调试退出
ntg_uint_t ntg_exiting;///< 正在退出
sig_atomic_t ntg_reconfigure;///< 重新配置
sig_atomic_t ntg_add;///< 添加流量产生进程


sig_atomic_t ntg_connections;///< 连接数统计
sig_atomic_t ntg_requests;

sig_atomic_t ntg_add_request;

sig_atomic_t ntg_add_user;
ntg_int_t ntg_user_num;

sig_atomic_t ntg_change_binary;///< 执行文件改变
ntg_pid_t ntg_new_binary;
ntg_uint_t ntg_inherited;//已继承socket标志
ntg_uint_t ntg_daemonized;//已为守护进程标志

sig_atomic_t ntg_noaccept;///< 不接收标志
ntg_uint_t ntg_noaccepting;
ntg_uint_t ntg_restart;
/**@}*/

static u_char master_process[] = "master process";

//static ntg_cache_manager_ctx_t ntg_cache_manager_ctx = {
//		ntg_cache_manager_process_handler, "cache manager process", 0 };
//
//static ntg_cache_manager_ctx_t ntg_cache_loader_ctx = {
//		ntg_cache_loader_process_handler, "cache loader process", 60000 };

/**
 * 退出时保留的数据
 */
static ntg_cycle_t ntg_exit_cycle;///< 退出时的循环体
static ntg_log_t ntg_exit_log;///< 退出时的日志
static ntg_open_file_t ntg_exit_log_file;///< 退出时的日志文件

/**
 * master进程循环
 * @param[in] cycle 循环体
 * @note 主要执行过程
 * 	1) 设置信号屏蔽字
 * 	2) 处理进程参数
 * 	3) 设置进程主题
 * 	4) 产生子进程
 * 	5) 进程循环
 * 		5.1 延时标志处理
 * 		5.2 开放信号,并挂起
 * 		5.3 更新时间
 * 		5.4 收割标志,表示有子进程退出
 * 		5.5 退出检查
 * 		5.6 终止标志
 * 		5.7 退出标志
 * 		5.8 重配置标志
 * 		5.9 重启标志
 * 		5.10 重打开日志
 * 		5.11 执行文件改变标志
 * 		5.12 不接受accept标志
 */
void ntg_master_process_cycle(ntg_cycle_t *cycle) {
	char *title;
	u_char *p;
	size_t size;
	ntg_int_t i;
	ntg_uint_t sigio;
	sigset_t set;
	struct itimerval itv;
	ntg_uint_t live;
	ntg_msec_t delay;
	ntg_core_conf_t *ccf;
	struct timeval time;

	//    cycle->test = malloc(20);
	//1) 设置信号屏蔽字
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigaddset(&set, SIGALRM);
	sigaddset(&set, SIGIO);
	sigaddset(&set, SIGINT);
	sigaddset(&set, ntg_signal_value(NTG_RECONFIGURE_SIGNAL));
	sigaddset(&set, ntg_signal_value(NTG_ADD_SIGNAL));
	sigaddset(&set, ntg_signal_value(NTG_TERMINATE_SIGNAL));
	sigaddset(&set, ntg_signal_value(NTG_SHUTDOWN_SIGNAL));
	sigaddset(&set, ntg_signal_value(NTG_CHANGEBIN_SIGNAL));

	if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
		ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
				"sigprocmask() failed");
	}

	//2) 处理进程参数
	size = sizeof(master_process);

	for (i = 0; i < ntg_argc; i++) {//获取所有进程参数长度
		size += ntg_strlen(ntg_argv[i]) + 1;
	}

	title = ntg_pnalloc(cycle->pool, size);
	if (title == NULL) {
		/* fatal */
		exit(2);
	}

	p = ntg_cpymem(title, master_process, sizeof(master_process) - 1);
	for (i = 0; i < ntg_argc; i++) {
		*p++ = ' ';//以一个空格分割
		p = ntg_cpystrn(p, (u_char *) ntg_argv[i], size);
	}

	//3) 设置进程主题
	ntg_setproctitle(title);

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	/* 产生虚拟用户进程 */
	ntg_start_virtual_process(cycle, 0, NTG_PROCESS_RESPAWN);

	/* 产生日志记录进程 */
//	ntg_start_record_process(cycle, 1, NTG_PROCESS_RESPAWN);

	/* 产生流量产生进程 */
	ntg_start_traffic_processes(cycle, ccf->worker_processes,
			NTG_PROCESS_RESPAWN);

	if (ntg_remote_init(cycle) != NTG_OK) {//初始化远程监听
		/* fatal exit*/
		ntg_terminate = 1;
	}
	/* 监听虚拟用户层返回的处理结果消息 */
	if (ntg_add_channel_event(cycle, ntg_processes[0].channel[0], EV_READ, ntg_channel_handler)
			== NTG_ERROR) {
		/* fatal exit*/
		ntg_terminate = 1;
	}

	/* 所有模块的主进程初始化 TODO 没有模块实现主进程的初始化 */
	printf("---ntg_modules-init master -end--\n");

	/* 设置程序亲和集 */
	if (ntg_setaffinity(&ccf->cpus, 0, cycle->log) != NTG_OK) {
		ntg_terminate = 1;
	}

//	/* 添加用户事件 */
//	cycle->et_user = event_new(cycle->base, -1, EV_TIMEOUT,
//			ntg_message_handler, cycle);
//	if (cycle->et_user == NULL) {
//		ntg_terminate = 1;
//	}
//
//	time.tv_sec = 2;
//	time.tv_usec = 0;
//
//	/* 注册定时事件 */
//	event_add(cycle->et_user, &time);

	/* 前端监听初始化 */
	ntg_new_binary = 0;
	delay = 0;
	sigio = 0;
	live = 1;

	sigemptyset(&set);
	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
		ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
				"sigprocmask() failed");
	}

	//5) 进程循环
	for (;;) {
		//5.1 延时标志处理
		if (delay) {
			if (ntg_sigalrm) {
				sigio = 0;
				delay *= 2;
				ntg_sigalrm = 0;
			}

			ntg_log_debug1(NTG_LOG_DEBUG_EVENT, cycle->log, 0,
					"termination cycle: %d", delay);

			itv.it_interval.tv_sec = 0;
			itv.it_interval.tv_usec = 0;
			itv.it_value.tv_sec = delay / 1000;
			itv.it_value.tv_usec = (delay % 1000) * 1000;

			if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
				ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
						"setitimer() failed");
			}
		}

		ntg_log_debug0(NTG_LOG_DEBUG_EVENT, cycle->log, 0, "sigsuspend");
		//5.3 更新时间
		ntg_time_update();
		//printf(".....master start loop.....................start\n");
		int ret = event_base_loop(cycle->base, EVLOOP_ONCE);
		//printf(".....master start loop...............%d.............end\n", ret);
		ntg_log_debug1(NTG_LOG_DEBUG_EVENT, cycle->log, 0,
				"wake up, sigio %i", sigio);
		//5.4 收割标志,表示有子进程退出
		if (ntg_reap) {
			ntg_reap = 0;
			ntg_log_debug0(NTG_LOG_DEBUG_EVENT, cycle->log, 0, "reap children");

			live = ntg_reap_children(cycle);
		}

		//5.5 退出检查
		if (!live && (ntg_terminate || ntg_quit)) {//没有活跃子进程或收到退出信号
			ntg_master_process_exit(cycle);
		}
		//5.6 终止标志
		if (ntg_terminate) {
			if (delay == 0) {
				delay = 50;
			}

			if (sigio) {
				sigio--;
				continue;
			}

			sigio = ccf->worker_processes + 2 /* cache processes */;

			if (delay > 1000) {
				ntg_signal_worker_processes(cycle, SIGKILL);
			} else {
				ntg_signal_worker_processes(cycle,
						ntg_signal_value(NTG_TERMINATE_SIGNAL));
			}

			continue;
		}
		//5.7 退出标志
		if (ntg_quit) {

			ntg_signal_worker_processes(cycle,
					ntg_signal_value(NTG_SHUTDOWN_SIGNAL));
			continue;
		}
		//5.8 重配置标志
		if (ntg_reconfigure) {
			ntg_reconfigure = 0;

			//			if (ntg_new_binary) {
			//				ntg_start_worker_processes(cycle, ccf->worker_processes,
			//						NTG_PROCESS_RESPAWN);
			//				ntg_noaccepting = 0;
			//
			//				continue;
			//			}
			//
			//			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "reconfiguring");
			//
			//			cycle = ntg_init_cycle(cycle);
			//			if (cycle == NULL) {
			//				cycle = (ntg_cycle_t *) ntg_cycle;
			//				continue;
			//			}
			//
			//			ntg_cycle = cycle;
			//			ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx,
			//					ntg_core_module);
			//			ntg_start_traffic_processes(cycle, ccf->worker_processes,
			//					NTG_PROCESS_JUST_RESPAWN);
			//			//            ntg_start_cache_manager_processes(cycle, 1);
			//
			//			/* allow new processes to start */ntg_msleep(100);
			//
			//			live = 1;
			//			ntg_signal_worker_processes(cycle,
			//					ntg_signal_value(NTG_SHUTDOWN_SIGNAL));
		}
		//5.9 重启标志
		if (ntg_restart) {
			ntg_restart = 0;
			//			ntg_start_worker_processes(cycle, ccf->worker_processes,
			//					NTG_PROCESS_RESPAWN);
			//            ntg_start_cache_manager_processes(cycle, 0);
			live = 1;
		}
		//5.10 添加流量产生进程
		if (ntg_add) {
			ntg_add = 0;
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "adding logs");
			/* 添加流量产生进程 */
			ntg_start_traffic_processes(cycle, 1, NTG_PROCESS_RESPAWN);
		}
		//5.11 执行文件改变标志
		if (ntg_change_binary) {
			ntg_change_binary = 0;
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "changing binary");
			ntg_new_binary = ntg_exec_new_binary(cycle, ntg_argv);
		}
		//5.12 不接受accept标志
		if (ntg_noaccept) {
			ntg_noaccept = 0;
			ntg_noaccepting = 1;
			ntg_signal_worker_processes(cycle,
					ntg_signal_value(NTG_SHUTDOWN_SIGNAL));
		}
	}
}

/**
 * 单进程循环
 * @param[in] cycle 循环体
 */
void ntg_single_process_cycle(ntg_cycle_t *cycle) {
	ntg_uint_t i;

	if (ntg_set_environment(cycle, NULL) == NULL) {
		/* fatal */
		exit(2);
	}

	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->init_process) {
			if (ntg_modules[i]->init_process(cycle) == NTG_ERROR) {
				/* fatal */
				exit(2);
			}
		}
	}

	for (;;) {
		ntg_log_debug0(NTG_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");

		if (ntg_terminate || ntg_quit) {

			for (i = 0; ntg_modules[i]; i++) {
				if (ntg_modules[i]->exit_process) {
					ntg_modules[i]->exit_process(cycle);
				}
			}

			ntg_master_process_exit(cycle);
		}

		if (ntg_reconfigure) {
			ntg_reconfigure = 0;
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "reconfiguring");

			cycle = ntg_init_cycle(cycle);
			if (cycle == NULL) {
				cycle = (ntg_cycle_t *) ntg_cycle;
				continue;
			}

			ntg_cycle = cycle;
		}

		//		if (ntg_reopen) {
		//			ntg_reopen = 0;
		//			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "reopening logs");
		//			ntg_reopen_files(cycle, (ntg_uid_t) -1);
		//		}
	}
}

/**
 * 开始流量产生进程
 * @param[in] cycle 循环体
 * @param[in] n 进程数
 * @param[in] respawn 重生进程或重生标志, 当respawn>=0时表示重启进程的所在的槽位,其他表示重启标志
 */
static void ntg_start_traffic_processes(ntg_cycle_t *cycle, ntg_int_t n,
		ntg_int_t respawn) {
	ntg_int_t i, j;
	ntg_message_t msg;
	ntg_channel_message_t *ch;

	ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "start traffic processes");

	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_CHANNEL_TYPE;
	ch = &msg.body.ch;

	ch->command = NTG_CMD_OPEN_CHANNEL;

	for (i = 0; i < n; i++) {
		/* TODO 获取进程的亲和集 */
		j = ntg_last_process;
		ntg_spawn_process(cycle, ntg_traffic_process_cycle,
				(void *) (intptr_t) j, "traffic process", respawn,
				NTG_TRAFFIC_PROCESS, -1);
		//printf("%d\n", ntg_processes[ntg_process_slot].pid);

		ch->pid = ntg_processes[ntg_process_slot].pid;
		ch->slot = ntg_process_slot;
		ch->fd = ntg_processes[ntg_process_slot].channel[0];
		ntg_pass_open_channel(cycle, &msg, NTG_TRAFFIC_PROCESS);
	}
}

/**
 * 开始虚拟用户进程
 * @param[in] cycle 循环体
 * @param[in] n 虚拟用户进程槽号
 * @param[in] respawn 重生进程或重生标志, 当respawn>=0时表示重启进程的所在的槽位,其他表示重启标志
 * @note 工作进程的序号与进程的cpu亲和性关联
 */

static void ntg_start_virtual_process(ntg_cycle_t *cycle, ntg_int_t n,
		ntg_int_t respawn) {

	ntg_message_t msg;
	ntg_channel_message_t *ch;

	ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "start virtual processes");

	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_CHANNEL_TYPE;
	ch = &msg.body.ch;

	ch->command = NTG_CMD_OPEN_CHANNEL;

	/* TODO 获取进程的亲和集 */
	ntg_spawn_process(cycle, ntg_virtual_process_cycle, (void *) (intptr_t) n,
			"virtual processes", respawn, NTG_VIRTUAL_PROCESS, 0);//指定了进程槽位为0
	//printf("%d\n", ntg_processes[ntg_process_slot].pid);

	ch->pid = ntg_processes[ntg_process_slot].pid;
	ch->slot = ntg_process_slot;
	ch->fd = ntg_processes[ntg_process_slot].channel[0];

	ntg_pass_open_channel(cycle, &msg, NTG_VIRTUAL_PROCESS);

}

/**
 * 开始记录进程
 * @param[in] cycle
 * @param[in] n 工作进程的序号
 * @param[in] respawn 重生进程或重生标志, 当respawn>=0时表示重启进程的所在的槽位,其他表示重启标志
 * @note 工作进程的序号与进程的cpu亲和性关联
 */
static void ntg_start_record_process(ntg_cycle_t *cycle, ntg_int_t n,
		ntg_int_t respawn) {
	ntg_message_t msg;
	ntg_channel_message_t *ch;

	ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "start record processes");

	ntg_spawn_process(cycle, ntg_record_process_cycle, (void *) (intptr_t) n,
			"record process", respawn, NTG_RECORD_PROCESS, 1);//指定了进程槽位为1

	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_CHANNEL_TYPE;
	ch = &msg.body.ch;

	ch->command = NTG_CMD_OPEN_CHANNEL;
	ch->pid = ntg_processes[ntg_process_slot].pid;
	ch->slot = ntg_process_slot;
	ch->fd = ntg_processes[ntg_process_slot].channel[0];

	ntg_pass_open_channel(cycle, &msg, NTG_RECORD_PROCESS);
}

/**
 * 打开与子进程的通道
 * @param[in] cycle 循环体
 * @param[in] ch 通道对象
 * @param[in] type 进程类型
 */
static void ntg_pass_open_channel(ntg_cycle_t *cycle, ntg_message_t *msg,
		ntg_int_t type) {
	ntg_int_t i;
	ntg_channel_message_t *ch;

	if (msg->type != NTG_CHANNEL_TYPE) {
		printf("msg->type != NTG_CHANNEL_TYPE in ntg_pass_open_channel\n");
	}
	ch = &msg->body.ch;

	/* 向所有的其他子进程发送打开通道信息 */
	for (i = 0; i < ntg_last_process; i++) {//&& type == NTG_TRAFFIC_PROCESS
		if ((i == ntg_process_slot) || ntg_processes[i].type == 0
				|| ntg_processes[i].pid == -1 || ntg_processes[i].channel[0]
				== -1)//排除自身,没占用及分离进程槽位
		{
			continue;
		}

		ntg_log_debug6(NTG_LOG_DEBUG_CORE, cycle->log, 0,
				"pass channel s:%d pid:%P fd:%d to s:%i pid:%P fd:%d",
				ch->slot, ch->pid, ch->fd,
				i, ntg_processes[i].pid,
				ntg_processes[i].channel[0]);

		/* TODO: NTG_AGAIN */

		ntg_write_channel(ntg_processes[i].channel[0], msg,
				sizeof(msg->type) + sizeof(*ch), cycle->log);
		//		printf("\n---ntg_pass_open_channel----ok------%d--%d---%d------\n",
		//				(int) ch->command, (int) ch->fd, ch->pid);
	}
}

/**
 * 向工作进程发送信号
 * @param[in] cycle 循环体
 * @param[in] signo 信号
 * @note
 *  发送的命令有:
 * 	NTG_CMD_QUIT
 * 	NTG_CMD_TERMINATE
 * 	NTG_CMD_REOPEN
 * 	执行过程:
 * 	1) 获取发下命令
 * 	2) 对所有有效且非分离子进程发送命令
 * 		2.1 跳过无效子进程
 * 		2.2 以通道方式发送命令
 * 		2.3 通道发送失败,采用kill方式
 */
static void ntg_signal_worker_processes(ntg_cycle_t *cycle, int signo) {
	ntg_int_t i;
	ntg_err_t err;
	ntg_message_t msg;
	ntg_channel_message_t *ch;

	//1) 获取发下命令
	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_CHANNEL_TYPE;
	ch = &msg.body.ch;

	switch (signo) {

	case ntg_signal_value(NTG_SHUTDOWN_SIGNAL):
		ch->command = NTG_CMD_QUIT;
		break;

	case ntg_signal_value(NTG_TERMINATE_SIGNAL):
		ch->command = NTG_CMD_TERMINATE;
		break;

		//	case ntg_signal_value(NTG_ADD_SIGNAL):
		//		ch->command = NTG_CMD_REOPEN;
		//		break;

	default:
		ch->command = 0;
	}

	ch->fd = -1;

	//2) 对所有有效且非分离子进程发送命令
	for (i = 0; i < ntg_last_process; i++) {

		ntg_log_debug7(NTG_LOG_DEBUG_EVENT, cycle->log, 0,
				"child: %d %P e:%d t:%d d:%d r:%d j:%d",
				i,
				ntg_processes[i].pid,
				ntg_processes[i].exiting,
				ntg_processes[i].exited,
				ntg_processes[i].detached,
				ntg_processes[i].respawn,
				ntg_processes[i].just_spawn);
		//2.1 跳过无效子进程
		if (ntg_processes[i].detached || ntg_processes[i].pid == -1) {
			continue;
		}

		if (ntg_processes[i].just_spawn) {
			ntg_processes[i].just_spawn = 0;
			continue;
		}

		if (ntg_processes[i].exiting && signo
				== ntg_signal_value(NTG_SHUTDOWN_SIGNAL)) {
			continue;
		}
		//2.2 以通道方式发送命令
		if (ch->command) {
			if (ntg_write_channel(ntg_processes[i].channel[0], &msg,
					sizeof(msg.type) + sizeof(*ch), cycle->log) == NTG_OK) {
				ntg_processes[i].exiting = 1;

				continue;
			}
		}
		//2.3 通道发送失败,采用kill方式
		ntg_log_debug2(NTG_LOG_DEBUG_CORE, cycle->log, 0,
				"kill (%P, %d)", ntg_processes[i].pid, signo);

		if (kill(ntg_processes[i].pid, signo) == -1) {
			err = ntg_errno;
			ntg_log_error(NTG_LOG_ALERT, cycle->log, err,
					"kill(%P, %d) failed", ntg_processes[i].pid, signo);

			if (err == NTG_ESRCH) {
				ntg_processes[i].exited = 1;
				ntg_processes[i].exiting = 0;
				ntg_reap = 1;
			}

			continue;
		}

		ntg_processes[i].exiting = 1;//设置进程将要退出标志
	}
}
///**
// * 前端回调函数
// * @param fd
// * @param what
// * @param arg
// */
//static void ntg_front_end_cb(evutil_socket_t fd, short what, void *arg) {
//	ntg_cycle_t *cycle;
//	ntg_user_manage_t *mg;
//
//	cycle = (ntg_cycle_t*) arg;
//	mg = cycle->manage;
//
//}

static void ntg_message_handler(evutil_socket_t fd, short what, void *arg) {
	ntg_cycle_t *cycle;
	ntg_message_t msg;
	ntg_simulation_message_t *sm;
	struct timeval time;

	static int i = 0;
	++i;

	//	static float y[24] = {
	//			3.11, 20.01, 25.41, 32.11, 40.97, 20.03, 10.45, 20.24, 10.63, 24.79,
	//			15.29, 15.29, 51.29, 55.43, 50.53, 15.65, 5.64, 10.38, 9.55, 10.19,
	//			16.48, 26.39, 12.71, 8.43};
	static float y[24] = { 83.3000, 82.2100, 81.5600, 81.2100, 81.0500,
			81.0700, 81.4200, 82.1800, 83.4700, 84.6400, 85.2400, 85.2900,
			85.1900, 85.4700, 85.6600, 85.7800, 85.8100, 85.4700, 85.3100,
			85.8400, 86.2300, 86.2600, 85.7500, 84.5700 };

	///2016-2预处理后数据
	static float a[24] = { 47.1429, 31.5714, 22.2857, 17.2857, 15.0000,
			15.2857, 20.2857, 31.1429, 49.5714, 66.2857, 74.8571, 75.5714,
			74.1429, 78.1429, 80.8571, 82.5714, 83.0000, 78.1429, 75.8571,
			83.4286, 89.0000, 89.4286, 82.1429, 65.2857 };
	//新闻场景
	static float x[24] = { 4.1000, 1.9000, 1.5000, 0.6000, 0.5000, 1.1000,
			1.8000, 10.2000, 71.2000, 98.4000, 98.5000, 88.7000, 71.2000,
			63.5000, 61.1000, 78.7000, 62.4000, 60.7000, 60.5000, 56.3000,
			50.4000, 40.1000, 31.4000, 18.8000 };
	//博客场景
	static float b[24] = { 9.2000, 4.1000, 3.9000, 2.7000, 1.9000, 1.6000,
			1.8000, 8.2000, 33.5000, 56.9000, 63.8000, 64.0000, 60.1000,
			76.2000, 80.6000, 75.1000, 71.1000, 59.5000, 61.3000, 68.7000,
			72.6000, 68.6000, 58.7000, 32.2000 };

	cycle = (ntg_cycle_t *) arg;

	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_SIMULATION_TYPE;
	sm = &msg.body.sm;

	sm->csm_id = i;
	sm->gp_id = 1;
	sm->user_n = 10000;

	/* 行为参数 */
	sm->ss_num_type = 2;//0.39
	sm->ss_num[0] = 0.39;

	sm->ss_itv_type = 5;//10, 0.93, 1.67
	sm->ss_itv[0] = 10;
	sm->ss_itv[1] = 0.93;
	sm->ss_itv[2] = 1.67;

	sm->clk_num_type = 1;//1.5, 0.47, 0.69
	sm->clk_num[0] = 1.5;
	sm->clk_num[1] = 0.47;
	sm->clk_num[2] = 0.69;

	sm->clk_itv_type = 1;//35, 0.49, 2.78
	sm->clk_itv[0] = 35;
	sm->clk_itv[1] = 0.49;
	sm->clk_itv[2] = 2.78;

	/* 场景参数 */
	sm->is_scene = 0;
	ntg_memcpy(sm->scene, x, sizeof(float) * 24);
	sm->st_point = 0;

	/* 预处理过程 */

	printf("ntg_user_message_process---start----usern=%d--\n", (int) sm->user_n);

	if (ntg_write_channel(ntg_processes[0].channel[0], &msg,
			sizeof(msg.type) + sizeof(msg.body.sm), cycle->log) != NTG_OK) {
		printf("-------message process is failed---\n");
	}

	printf("-------message process is success---\n");
	if (i >= 1) {
		//		event_del(cycle->et_user);
		event_free(cycle->et_user);
		cycle->et_user = NULL;
	} else {
		time.tv_sec = 1;
		time.tv_usec = 0;
		event_add(cycle->et_user, &time);
	}
}

/**
 * 收割子进程
 * @param[in] cycle 循环体
 * @return 如果存在活跃的非分离进程返回 1， 否则返回0
 */
static ntg_uint_t ntg_reap_children(ntg_cycle_t *cycle) {
	ntg_int_t i, n;
	ntg_uint_t live;
	ntg_message_t msg;
	ntg_channel_message_t *ch;
	ntg_core_conf_t *ccf;

	ntg_memzero(&msg, sizeof(ntg_message_t));

	msg.type = NTG_CHANNEL_TYPE;
	ch = &msg.body.ch;

	ch->command = NTG_CMD_CLOSE_CHANNEL;
	ch->fd = -1;

	live = 0;
	/* 遍历所有的子进程 */
	for (i = 0; i < ntg_last_process; i++) {

		ntg_log_debug7(NTG_LOG_DEBUG_EVENT, cycle->log, 0,
				"child: %d %P e:%d t:%d d:%d r:%d j:%d",
				i,
				ntg_processes[i].pid,
				ntg_processes[i].exiting,
				ntg_processes[i].exited,
				ntg_processes[i].detached,
				ntg_processes[i].respawn,
				ntg_processes[i].just_spawn);

		if (ntg_processes[i].pid == -1) {
			continue;
		}

		if (ntg_processes[i].exited) {//子进程已经退出

			/* 不是分离进程，先向其他子进程发送关闭通道消息 */
			if (!ntg_processes[i].detached) {
				ntg_close_channel(ntg_processes[i].channel, cycle->log);//关闭主进程自身的通道

				ntg_processes[i].channel[0] = -1;
				ntg_processes[i].channel[1] = -1;

				ch->pid = ntg_processes[i].pid;
				ch->slot = i;

				/* 向其他没有结束的子进程发送关闭通道消息 */
				for (n = 0; n < ntg_last_process; n++) {
					if (ntg_processes[n].exited || ntg_processes[n].pid == -1
							|| ntg_processes[n].channel[0] == -1) {
						continue;
					}

					ntg_log_debug3(NTG_LOG_DEBUG_CORE, cycle->log, 0,
							"pass close channel s:%i pid:%P to:%P",
							ch->slot, ch->pid, ntg_processes[n].pid);

					/* TODO: NTG_AGAIN */

					ntg_write_channel(ntg_processes[n].channel[0], &msg,
							sizeof(msg.type) + sizeof(*ch), cycle->log);
				}
			}

			/* 子进程重生 */
			if (ntg_processes[i].respawn && !ntg_processes[i].exiting
					&& !ntg_terminate && !ntg_quit) {
				if (ntg_spawn_process(cycle, ntg_processes[i].proc,
						ntg_processes[i].data, ntg_processes[i].name, i,
						ntg_processes[i].type, -1) == NTG_INVALID_PID) {
					ntg_log_error(NTG_LOG_ALERT, cycle->log, 0,
							"could not respawn %s",
							ntg_processes[i].name);
					continue;
				}

				ch->command = NTG_CMD_OPEN_CHANNEL;
				ch->pid = ntg_processes[ntg_process_slot].pid;
				ch->slot = ntg_process_slot;
				ch->fd = ntg_processes[ntg_process_slot].channel[0];

				ntg_pass_open_channel(cycle, &msg, ntg_processes[i].type);

				live = 1;

				continue;
			}

			/* 执行新的二进制 */
			if (ntg_processes[i].pid == ntg_new_binary) {

				ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx,
						ntg_core_module);

				if (ntg_rename_file((char *) ccf->oldpid.data,
						(char *) ccf->pid.data) == NTG_FILE_ERROR) {
					ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
							ntg_rename_file_n " %s back to %s failed "
							"after the new binary process \"%s\" exited",
							ccf->oldpid.data, ccf->pid.data, ntg_argv[0]);
				}

				ntg_new_binary = 0;
				if (ntg_noaccepting) {
					ntg_restart = 1;
					ntg_noaccepting = 0;
				}
			}

			if (i == ntg_last_process - 1) {
				ntg_last_process--;

			} else {
				ntg_processes[i].pid = -1;
			}

		} else if (ntg_processes[i].exiting || !ntg_processes[i].detached) {//进程正在退出或父子进程还没分离
			live = 1;
		}
	}

	return live;
}

/**
 * 主进程退出
 * @param[in] cycle 循环体
 */
static void ntg_master_process_exit(ntg_cycle_t *cycle) {
	ntg_uint_t i;

	ntg_delete_pidfile(cycle);

	ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "exit");
	//1) 调用各个模块的退出函数
	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->exit_master) {
			ntg_modules[i]->exit_master(cycle);
		}
	}
	//2) 关闭所有监听套接字
	//    ntg_close_listening_sockets(cycle);

	/*
	 * Copy ntg_cycle->log related data to the special static exit cycle,
	 * log, and log file structures enough to allow a signal handler to log.
	 * The handler may be called when standard ntg_cycle->log allocated from
	 * ntg_cycle->pool is already destroyed.
	 */

	//3) 设置退出保存的数据
	ntg_exit_log = *ntg_log_get_file_log(ntg_cycle->log);

	ntg_exit_log_file.fd = ntg_exit_log.file->fd;
	ntg_exit_log.file = &ntg_exit_log_file;
	ntg_exit_log.next = NULL;
	ntg_exit_log.writer = NULL;

	ntg_exit_cycle.log = &ntg_exit_log;
	ntg_exit_cycle.files_n = ntg_cycle->files_n;
	ntg_cycle = &ntg_exit_cycle;
	//4) 销毁pool
	ntg_destroy_pool(cycle->pool);

	exit(0);
}

static inline void print_uri_parts_info(const struct evhttp_uri * http_uri) {
	printf("scheme:%s\n", evhttp_uri_get_scheme(http_uri));
	printf("host:%s\n", evhttp_uri_get_host(http_uri));
	printf("path:%s\n", evhttp_uri_get_path(http_uri));
	printf("port:%d\n", evhttp_uri_get_port(http_uri));
	printf("query:%s\n", evhttp_uri_get_query(http_uri));
	printf("userinfo:%s\n", evhttp_uri_get_userinfo(http_uri));
	printf("fragment:%s\n", evhttp_uri_get_fragment(http_uri));
}

static inline void print_request_head_info(struct evkeyvalq *header) {
	struct evkeyval *first_node = header->tqh_first;
	while (first_node) {
		printf("%s: %s\n", first_node->key, first_node->value);
		first_node = first_node->next.tqe_next;
	}
}

void http_requset_get_cb(struct evhttp_request *req, void *arg) {
	ntg_cycle_t *cycle = (ntg_cycle_t *) arg;
	switch (req->response_code) {
	case HTTP_OK: {
		struct evbuffer* buf = evhttp_request_get_input_buffer(req);
		size_t len = evbuffer_get_length(buf);
		printf("print the head info:");
		print_request_head_info(req->output_headers);

		printf("len:%zu  body size:%zu\n", len, req->body_size);
		char *tmp = malloc(len + 1);
		memcpy(tmp, evbuffer_pullup(buf, -1), len);
		tmp[len] = '\0';
		printf("print the body:");
		printf("HTML BODY:%s\n", tmp);
		free(tmp);

		//		event_base_loopexit(cycle->base, 0);
		break;
	}
	case HTTP_MOVEPERM:
		printf("%s\n", "the uri moved permanently");
		break;
	case HTTP_MOVETEMP: {
		printf("move ------------******************8\n");
		//		const char *new_location = evhttp_find_header(req->input_headers,
		//				"Location");
		//		struct evhttp_uri *new_uri = evhttp_uri_parse(new_location);
		//		evhttp_uri_free(uri);
		//		http_req_get->uri = new_uri;
		//		start_url_request(http_req_get, REQUEST_GET_FLAG);
		return;
	}

	default:
		printf("default ------------******************8\n");
		event_base_loopexit(cycle->base, 0);
		return;
	}
}

/**
 * 流量产生进程循环
 * @param[in] cycle 循环体
 * @param[in] data 输入的数据,为工作进程的序号
 * @note
 *  1. 工作进程初始化
 *  2. 设置进程标题
 *  3. 工作进程循环
 * 		3.1 正在退出标志
 * 		3.2 监听与分发事件
 * 		3.3 终止标志
 * 		3.4 退出标志
 * 		3.5 重新打开日志
 */

static void ntg_traffic_process_cycle(ntg_cycle_t *cycle, void *data) {
	ntg_int_t worker = (intptr_t) data;
	struct timeval time;
	ntg_process = NTG_PROCESS_TRAFFIC;

	//    free(cycle->test);
	//1. 工作进程初始化
	ntg_worker_process_init(cycle, worker);

	if (ntg_db_mydql_init(cycle) != NTG_OK){
		//记录日志
		printf("ntg_db_mydql_init failed %ld\n^^^^^^^^^^^^\n", (long int) ntg_pid);
		exit(2);
	}

	//2. 设置进程标题
	ntg_setproctitle("worker process");

	printf("ntg_traffic_process_cycle %ld\n^^^^^^^^^^^^\n", (long int) ntg_pid);
	/**************创建一个事件模块*********************/
	//17.初始化事件驱动
	//	event_enable_debug_mode();

	cycle->dns = evdns_base_new(cycle->base, 0);
	if (cycle->dns == NULL) {
		exit(2);
	}
	event_set_fatal_callback(event_failed_cd);
	printf("ntg_worker_process_init --------------------------------- ok\n");

	cycle->et_user = event_new(cycle->base, -1, EV_TIMEOUT | EV_PERSIST,
			ntg_connect_handler, cycle);
	if (cycle->et_user == NULL) {
		ntg_terminate = 1;
	}

	time.tv_sec = 1;
	time.tv_usec = 0;

	/* 注册定时事件 */
	event_add(cycle->et_user, &time);

	//3. 工作进程循环
	for (;;) {
		//3.1 正在退出标志
		if (ntg_exiting) {
			/* 释放一些资源 */
			ntg_worker_process_exit(cycle);
		}

		ntg_log_debug0(NTG_LOG_DEBUG_EVENT, cycle->log, 0, "traffic process cycle");
		//5.3 更新时间
		ntg_time_update();
		//3.2 监听与分发事件
		event_base_loop(cycle->base, EVLOOP_ONCE);

		//3.3 终止标志
		if (ntg_terminate) {
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "exiting");

			ntg_worker_process_exit(cycle);
		}
		//3.4 退出标志
		if (ntg_quit) {
			ntg_quit = 0;
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0,
					"gracefully shutting down");
			ntg_setproctitle("worker process is shutting down");

			if (!ntg_exiting) {
				//                ntg_close_listening_sockets(cycle);
				ntg_exiting = 1;
			}
		}

		//3.5 重新打开日志
	}
}
static void ntg_connect_handler(evutil_socket_t fd, short what, void *arg) {

	int num;

	static int i = 0;
	static int pre = 0;
	static int avg = 0;
	++i;

	num = ntg_connections - pre;
	avg = (num + avg * (i - 1)) / i;

	//printf("--pid=%d--count=%d--connections--%d--avg=%d->\n",(int)ntg_pid,  i, num, avg);
	pre = ntg_connections;

}

static void ntg_request_handler(evutil_socket_t fd, short what, void *arg) {
	int num;

	static int i = 0;
	static int pre = 0;
	static int avg = 0;
	++i;
	printf("---pre-count=%d---avg=%d->\n", i, avg);
	num = ntg_requests - pre;
	avg = (num + avg * (i - 1)) / i;

	printf("-count=%d--requests--%d--avg=%d->\n", i, num, avg);
	pre = ntg_requests;
}

void event_failed_cd(int err) {
	printf("^^^^^^^^^^^^^^^^^^^err=%d, %s^^^^^^^^^^^^^^\n", err, strerror(err));
}

/**
 * 工作进程初始化
 * @param[in] cycle 循环体
 * @param[in] worker 工作进程序号
 * @note worker与进程的cpu亲和性是关联的
 * @note 这是所有工作进程公共的初始化函数，其中包括模块init_process函数的调用。
 * 因此，在模块中应根据不同的工作进程类型进行不同的初始化。
 * @note
 * 	1. 设置环境变量
 * 	2. 设置优先级
 * 	3. 设置描述符数量限制
 * 	4. 设置core数目
 * 	5. 设置sigpending数
 * 	6. 设置用户的ID和组id
 * 	7. 设置cpu的亲和性
 * 	8. 允许coredump
 * 	9. 改变工作目录
 * 	10. 清空信号屏蔽字
 * 	11. 初始化随机种子
 * 	13. 模块的进程初始化
 * 	14. 关闭所有其他进程的通道[1]端
 * 	15. 关闭自身的[0]端通道
 * 	16. 注册通道[1]端的读事件监听
 */
static void ntg_worker_process_init(ntg_cycle_t *cycle, ntg_int_t worker) {
	sigset_t set;
	ntg_int_t n;
	ntg_uint_t i;
	struct rlimit rlmt;
	ntg_core_conf_t *ccf;

	//设置环境变量
	if (ntg_set_environment(cycle, NULL) == NULL) {
		/* fatal */
		exit(2);
	}
	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	//设置优先级
	if (worker >= 0 && ccf->priority != 0) {
		if (setpriority(PRIO_PROCESS, 0, ccf->priority) == -1) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					"setpriority(%d) failed", ccf->priority);
		}
	}
	//设置描述符数量限制
	if (ccf->rlimit_nofile != NTG_CONF_UNSET) {
		rlmt.rlim_cur = (rlim_t) ccf->rlimit_nofile;
		rlmt.rlim_max = (rlim_t) ccf->rlimit_nofile;

		if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					"setrlimit(RLIMIT_NOFILE, %i) failed",
					ccf->rlimit_nofile);
		}
	}
	//设置core 大小
	if (ccf->rlimit_core != NTG_CONF_UNSET) {
		rlmt.rlim_cur = (rlim_t) ccf->rlimit_core;
		rlmt.rlim_max = (rlim_t) ccf->rlimit_core;

		if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					"setrlimit(RLIMIT_CORE, %O) failed",
					ccf->rlimit_core);
		}
	}
	//设置sigpending数
#ifdef RLIMIT_SIGPENDING
	if (ccf->rlimit_sigpending != NTG_CONF_UNSET) {
		rlmt.rlim_cur = (rlim_t) ccf->rlimit_sigpending;
		rlmt.rlim_max = (rlim_t) ccf->rlimit_sigpending;

		if (setrlimit(RLIMIT_SIGPENDING, &rlmt) == -1) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					"setrlimit(RLIMIT_SIGPENDING, %i) failed",
					ccf->rlimit_sigpending);
		}
	}
#endif
	//设置用户id和组id
	if (geteuid() == 0) {//如果有效用户id为root
		if (setgid(ccf->group) == -1) {
			ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
					"setgid(%d) failed", ccf->group);
			/* fatal */
			exit(2);
		}

		if (initgroups(ccf->username, ccf->group) == -1) {
			ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
					"initgroups(%s, %d) failed",
					ccf->username, ccf->group);
		}

		if (setuid(ccf->user) == -1) {
			ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
					"setuid(%d) failed", ccf->user);
			/* fatal */
			exit(2);
		}
	}

	/* 设置自身进程对象的一些属性 */
	ntg_processes[ntg_process_slot].pid = ntg_pid;

#if (NTG_CPU_AFFINITY)
	//设置进程的cpu亲和性
	if (worker >= 0) {

		if (ccf->cpu_affinity) {
			if (ntg_setaffinity(&ccf->cpus, worker+1, cycle->log) != NTG_OK) {
				exit(2);
			}
		}
	}
#endif

#if (NTG_HAVE_PR_SET_DUMPABLE)

	/* allow coredump after setuid() in Linux 2.4.x */
	//允许coredump
	if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
		ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
				"prctl(PR_SET_DUMPABLE) failed");
	}

#endif
	//改变工作目录
	if (ccf->working_directory.len) {
		if (chdir((char *) ccf->working_directory.data) == -1) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					"chdir(\"%s\") failed", ccf->working_directory.data);
			/* fatal */
			exit(2);
		}
	}
	//清空信号屏蔽字
	sigemptyset(&set);

	if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
		ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
				"sigprocmask() failed");
	}

	//初始化随机种子
	srandom((ntg_pid << 16) ^ ntg_time());

	/* 重新初始化事件base */
	if (cycle->base != NULL) {
		if (event_reinit(cycle->base) < 0) {
			printf("if some events could not be re-added\n");
			/* fatal */
			exit(2);
		}
	} else {
		cycle->base = event_base_new();
		if (cycle->base == NULL) {
			/* fatal */
			exit(2);
		}
	}

	//模块的进程初始化
	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->init_process) {
			if (ntg_modules[i]->init_process(cycle) == NTG_ERROR) {
				/* fatal */
				exit(2);
			}
		}
	}
	//关闭所有其他进程的[1]端通道
	for (n = 0; n < ntg_last_process; n++) {

		if (ntg_processes[n].pid == -1) {//无效
			continue;
		}

		if (n == ntg_process_slot) {//自身
			continue;
		}

		if (ntg_processes[n].channel[1] == -1) {//分离
			continue;
		}

		if (close(ntg_processes[n].channel[1]) == -1) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					"close() channel failed");
		}
	}

	//关闭自身的[0]端通道
	if (close(ntg_processes[ntg_process_slot].channel[0]) == -1) {
		ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
				"close() channel failed");
	}

	//16. 注册通道的读事件监听
	if (ntg_add_channel_event(cycle, ntg_channel, EV_READ, ntg_channel_handler)
			== NTG_ERROR) {
		/* fatal */
		exit(2);
	}

}

/**
 * 子进程退出
 * @param[in] cycle 循环体
 */
static void ntg_worker_process_exit(ntg_cycle_t *cycle) {
	ntg_uint_t i;

	//1 调用所有模块退出函数
	for (i = 0; ntg_modules[i]; i++) {
		if (ntg_modules[i]->exit_process) {
			ntg_modules[i]->exit_process(cycle);
		}
	}

	if (ntg_exiting) {

		if (ntg_debug_quit) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, 0, "aborting");
			ntg_debug_point();
		}
	}

	/*
	 * Copy ntg_cycle->log related data to the special static exit cycle,
	 * log, and log file structures enough to allow a signal handler to log.
	 * The handler may be called when standard ntg_cycle->log allocated from
	 * ntg_cycle->pool is already destroyed.
	 */

	ntg_exit_log = *ntg_log_get_file_log(ntg_cycle->log);

	ntg_exit_log_file.fd = ntg_exit_log.file->fd;
	ntg_exit_log.file = &ntg_exit_log_file;
	ntg_exit_log.next = NULL;
	ntg_exit_log.writer = NULL;

	ntg_exit_cycle.log = &ntg_exit_log;
	ntg_exit_cycle.files_n = ntg_cycle->files_n;
	ntg_cycle = &ntg_exit_cycle;

	ntg_destroy_pool(cycle->pool);

	ntg_log_error(NTG_LOG_NOTICE, ntg_cycle->log, 0, "exit");

	exit(0);
}

//static void ntg_result_cb(evutil_socket_t fd, short whart, void *arg){
//	ntg_int_t n;
//	ntg_message_t msg;
//	ntg_cycle_t *cycle;
//
//	cycle = (ntg_cycle_t*) arg;
//
//	ntg_log_debug0(NTG_LOG_DEBUG_CORE, cycle->log, 0, "channel[0] handler");
//
//	ntg_memzero(&msg, sizeof(ntg_message_t));
//
////	printf("...type:%d..id..%d..state= %d.\n", msg.type, msg.body.rst.csm_id, msg.body.rst.status);
//
//	for (;;) {
//		n = ntg_read_channel(fd, &msg, sizeof(ntg_message_t), cycle->log);
////
////		printf("message:  %d\n", msg.body.rst.csm_id);
////		return;
//
//		ntg_log_debug1(NTG_LOG_DEBUG_CORE, cycle->log, 0, "channel: %i", n);
//
//		if (n == NTG_ERROR) {
//			printf("=============NTG_ERROR===============\n");
//			//        	event_free(pro->event);
//			break;
//		}
//
//		if (n == NTG_AGAIN) {
//			break;
//		}
//
//		ntg_log_debug1(NTG_LOG_DEBUG_CORE, cycle->log, 0,
//				"message type: %d", msg.type);
//
////		printf("...type:%d..id..%d..state= %d.\n", msg.type, msg.body.rst.csm_id, msg.body.rst.status);
//		/* 不同消息类型分类处理 */
//		switch (msg.type) {
//		case NTG_RESULT_TYPE: {
//			printf(".......result  len=%d.....pid==%d...\n", (int)n, ntg_pid);
//			ntg_result_message_t *rst = &msg.body.rst;
//			if (ntg_remote_result_handler(cycle->rmt, rst) != NTG_OK) {
//				printf("ntg_user_remote_result====is faild\n");
//			}
//		}break;
//		default:
//			printf("......ntg_result_cb....error............\n");
//			break;
//		}
//	}
//}

/**
 * 通道[1]端可读的处理函数
 * @param[in] fd 套接字
 * @param[in] whart 事件标志
 * @param[in] arg 回调参数
 * @details 接收并处理进程发送过来的NTG_CMD_QUIT, NTG_CMD_TERMINATE,
 * 	NTG_CMD_REOPEN, NTG_CMD_OPEN_CHANNEL, NTG_CMD_CLOSE_CHANNEL命令.
 * 	处理worker进程发送过来的日志信息
 * @note 可以设置的全局变量有:
 * 	ntg_quit
 * 	ntg_terminate
 * 	ntg_reopen
 */
static void ntg_channel_handler(evutil_socket_t fd, short whart, void *arg) {
	ntg_int_t n, ret;
	ntg_user_manage_t *mg;
	ntg_message_t msg;
	ntg_cycle_t *cycle;
	ntg_process_t *pro;
	ntg_record_t *rd;

	cycle = (ntg_cycle_t*) arg;
	pro = &ntg_processes[ntg_process_slot];///获取自身进程对象

	ntg_log_debug0(NTG_LOG_DEBUG_CORE, pro->log, 0, "channel handler");

	ntg_memzero(&msg, sizeof(ntg_message_t));

	struct timeval s_tv, e_tv;
	int count = 0;

	if (ntg_process == NTG_PROCESS_TRAFFIC) {
		if (evutil_gettimeofday(&s_tv, NULL) < 0) {
			printf("evutil_gettimeofday failed\n");
		}
	}

	for (;;) {
		count++;
		n = ntg_read_channel(fd, &msg, sizeof(ntg_message_t), pro->log);

		ntg_log_debug1(NTG_LOG_DEBUG_CORE, pro->log, 0, "channel: %i", n);

		if (n == NTG_ERROR) {

			printf("=============NTG_ERROR===============\n");
			//        	event_free(pro->event);
			break;
		}

		if (n == NTG_AGAIN) {
			break;
		}

		ntg_log_debug1(NTG_LOG_DEBUG_CORE, pro->log, 0,
				"message type: %d", msg.type);

		/* 不同消息类型分类处理 */
		switch (msg.type) {
		case NTG_CHANNEL_TYPE:{
			ntg_channel_message_t *ch = &msg.body.ch;
			switch (ch->command) {
			case NTG_CMD_QUIT:
				ntg_quit = 1;
				break;
			case NTG_CMD_TERMINATE:
				ntg_terminate = 1;
				break;
				//			case NTG_CMD_REOPEN:
				//				ntg_reopen = 1;
				//				break;
			case NTG_CMD_OPEN_CHANNEL:

				ntg_log_debug3(NTG_LOG_DEBUG_CORE, pro->log, 0,
						"get channel s:%i pid:%P fd:%d",
						ch->slot, ch->pid, ch->fd);
				//				printf("-------slot--%d--pid--%d--fd--%d-4--------\n",ch->slot, ch->pid, ch->fd);
				ntg_processes[ch->slot].pid = ch->pid;
				ntg_processes[ch->slot].channel[0] = ch->fd;

				if (ntg_processes[ntg_process_slot].type == NTG_VIRTUAL_PROCESS
						&& ch->slot >= ntg_start_process) {
					ntg_last_process++;
					cycle->manage->blc->n = ntg_last_process - ntg_start_process;
				}

				break;

			case NTG_CMD_CLOSE_CHANNEL:
				ntg_log_debug4(NTG_LOG_DEBUG_CORE, pro->log, 0,
						"close channel s:%i pid:%P our:%P fd:%d",
						ch->slot, ch->pid, ntg_processes[ch->slot].pid,
						ntg_processes[ch->slot].channel[0]);

				if (close(ntg_processes[ch->slot].channel[0]) == -1) {
					ntg_log_error(NTG_LOG_ALERT, pro->log, ntg_errno,
							"close() channel failed");
				}

				ntg_processes[ch->slot].channel[0] = -1;
				break;
			}
		}break;
		case NTG_SIMULATION_TYPE:
		case NTG_CONTROL_TYPE:
			printf(".......result  len=%d.....pid==%d...\n", (int)n, ntg_pid);
			printf(".........NTG_SIMULATION_TYPE...NTG_CONTROL_TYPE.........\n");
			ntg_user_message_handler(cycle->manage, &msg);
			break;
		case NTG_RESULT_TYPE: {
			printf(".......result  len=%d.....pid==%d...\n", (int)n, ntg_pid);
			printf(".........NTG_RESULT_TYPE....id..%d..state= %d.\n", msg.body.rst.csm_id, msg.body.rst.status);
			ntg_result_message_t *rst = &msg.body.rst;
			if (ntg_remote_result_handler(cycle->rmt, rst) != NTG_OK) {
				printf("ntg_user_remote_result====is faild\n");
			}
		}break;
		case NTG_REQUEST_TYPE:
			printf(".......result  len=%d.....pid==%d...\n", (int)n, ntg_pid);
			ntg_add_request = 1;
			ret = ntg_http_start(cycle, &msg);
			if (ret == NTG_ERROR) {
				printf("+++++++1+++++++ntg_http_start failed+++++\n");
			}
			break;
		case NTG_FEEDBACK_TYPE:
			mg = cycle->manage;
			ret = mg->blc->blance(mg, &msg);
			if (ret == NTG_ERROR) {
				printf("+++++3++++++++freedback type+++++++++++++++\n");
			}
			break;
		case NTG_RECORD_TPYE:
			//printf("=============NTG_CMD_RECORD===============\n");
			/**
			 * 从记录池中获取一个对象
			 * 记录池满，则刷新数据到数据库
			 */
			rd = ntg_get_record(cycle->record_mg);
			int count = 0;
			while (rd == NULL && count < 2) {
				count++;
				//进行刷新处理
			}
			if (count == 2) {
				//进行丢弃处理
				cycle->record_mg->dequeue_count += cycle->record_mg->record_n
						>> 8;
				rd = ntg_get_record(cycle->record_mg);
			}

			//			printf("record-->%d, %d, %d,%d, %d, %d, %d\n", msg.body.rcd.csm_id, msg.body.rcd.gp_id,
			//					msg.body.rcd.user_id, msg.body.rcd.url_id, msg.body.rcd.state_code, msg.body.rcd.send, msg.body.rcd.receive);
			break;
		default:
			break;
		}
	}

	if (ntg_process == NTG_PROCESS_TRAFFIC) {
		if (evutil_gettimeofday(&e_tv, NULL) < 0) {
			printf("evutil_gettimeofday failed\n");
		}

		//printf("------requests= %d-------coast time=%ld----\n", count, (long int)e_tv.tv_sec - s_tv.tv_sec);
	}
}
/**
 * 虚拟用户进程循环
 * @param[in] cycle 循环对象
 * @param[in] data 数据表示进程的cpu亲和力rank
 */
static void ntg_virtual_process_cycle(ntg_cycle_t *cycle, void *data) {
	ntg_int_t worker = (intptr_t) data;
	struct timeval time;

	ntg_process = NTG_PROCESS_VIRTUAL;

	//    free(cycle->test);
	//工作进程初始化
	ntg_worker_process_init(cycle, worker);///公共初始化

	//设置进程标题
	ntg_setproctitle("virtual process");

	printf("ntg_virtual_process_cycle %ld^^^^^rrrrrrr^^^^^\n",
			(long int) ntg_pid);
	/**************内部初始化********************/

	event_set_fatal_callback(event_failed_cd);
	printf("ntg_virtual_process_init -------------------- ok\n");

//	cycle->et_user = event_new(cycle->base, -1, EV_TIMEOUT | EV_PERSIST,
//			ntg_request_handler, cycle);
//	if (cycle->et_user == NULL) {
//		ntg_terminate = 1;
//	}
//	time.tv_sec = 1;
//	time.tv_usec = 0;
//	/* 注册定时事件 */
//	event_add(cycle->et_user, &time);

	/* 进程循环 */
	for (;;) {

		//正在退出标志
		if (ntg_exiting) {

			ntg_worker_process_exit(cycle);

		}
		ntg_log_debug0(NTG_LOG_DEBUG_EVENT, cycle->log, 0, "virtual cycle");

		//5.3 更新时间
		ntg_time_update();

		//事件监听与分发
		event_base_loop(cycle->base, EVLOOP_ONCE);

		//终止标志
		if (ntg_terminate) {
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "exiting");

			ntg_worker_process_exit(cycle);
		}

		//退出标志
		if (ntg_quit) {
			ntg_quit = 0;
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0,
					"gracefully shutting down");
			ntg_setproctitle("virtrual process is shutting down");

			if (!ntg_exiting) {
				//                ntg_close_listening_sockets(cycle);
				ntg_exiting = 1;
			}
		}

		//		//重新打开日志
		//		if (ntg_reopen) {
		//			ntg_reopen = 0;
		//			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "reopening logs");
		//			ntg_reopen_files(cycle, -1);
		//		}
	}
}

/**
 * 记录进程循环
 * @param cycle
 * @param data
 */
static void ntg_record_process_cycle(ntg_cycle_t *cycle, void *data) {
	ntg_int_t record = (intptr_t) data;

	ntg_process = NTG_PROCESS_RECORD;

	//1. 进程初始化
	ntg_worker_process_init(cycle, record);

	//2. 设置进程标题
	ntg_setproctitle("record process");

	printf("ntg_record_process_cycle %ld^^^^^^^^^^^^^^^^\n", (long int) ntg_pid);

	/**************创建一个事件模块*********************/
	if (ntg_record_init(cycle) != NTG_OK) {
		/* fatal */
		exit(2);
	}

	//3. 记录进程循环
	for (;;) {
		//3.1 正在退出标志
		if (ntg_exiting) {

			ntg_worker_process_exit(cycle);
		}

		ntg_log_debug0(NTG_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");

		//5.3 更新时间
		ntg_time_update();

		//3.2 监听与分发事件
		event_base_loop(cycle->base, EVLOOP_ONCE);
		//3.3 终止标志
		if (ntg_terminate) {
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "exiting");

			ntg_worker_process_exit(cycle);
		}
		//3.4 退出标志
		if (ntg_quit) {
			ntg_quit = 0;
			ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0,
					"gracefully shutting down");
			ntg_setproctitle("worker process is shutting down");

			if (!ntg_exiting) {
				//                ntg_close_listening_sockets(cycle);
				ntg_exiting = 1;
			}
		}
	}
}
