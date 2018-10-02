/*
 * ntg_process.c
 *
 *  Created on: Aug 28, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_process.h"
#include "ntg_times.h"

#include "ntg_process_cycle.h"
#include "ntg_channel.h"

/**
 * @name 带处理函数的信号对象
 */
typedef struct {
    int     signo;///< 信号编号
    char   *signame;///< 信号名称
    char   *name;///< 应用名称
    struct event *ev_sig;
    void (*handler)(int signo , short flags, void *args) ;///< 信号处理函数
} ntg_signal_t;

/**
 * @name 忽略信号对象
 */
typedef struct {
    int     signo;///< 信号编号
    char   *signame;///< 信号名称
    char   *name;///< 应用名称
} ntg_ignore_signal_t;


static void ntg_execute_proc(ntg_cycle_t *cycle, void *data);
static void ntg_signal_handler (int signo , short flags, void *args);
static void ntg_process_get_status(void);


int              ntg_argc;///< 输入参数个数
char           **ntg_argv;///< 输入参数数组
char           **ntg_os_argv;///< 系统的命令行参数指针

ntg_int_t        ntg_process_slot;///< 进程槽位记录
ntg_socket_t     ntg_channel;///< 通道fd

ntg_int_t        ntg_start_process=1;///< 流量产生进程开始位置
ntg_int_t        ntg_last_process=1;///< 下一个流量产生进程位置

ntg_int_t        ntg_current_process=1;
ntg_process_t    ntg_processes[NTG_MAX_PROCESSES];///< 进程组, 0为用户进程， 1为记录进程, 其他为流量产生进程电脑该


/**
 * @name 系统信号集合
 */
ntg_signal_t  signals[] = {
    { ntg_signal_value(NTG_RECONFIGURE_SIGNAL),
      "SIG" ntg_value(NTG_RECONFIGURE_SIGNAL),
      "reload",
      NULL,
      ntg_signal_handler },

    { ntg_signal_value(NTG_ADD_SIGNAL),
      "SIG" ntg_value(NTG_ADD_SIGNAL),
      "add",
      NULL,
      ntg_signal_handler },

    { ntg_signal_value(NTG_TERMINATE_SIGNAL),
      "SIG" ntg_value(NTG_TERMINATE_SIGNAL),
      "stop",
      NULL,
      ntg_signal_handler },

    { ntg_signal_value(NTG_SHUTDOWN_SIGNAL),
      "SIG" ntg_value(NTG_SHUTDOWN_SIGNAL),
      "quit",
      NULL,
      ntg_signal_handler },

    { ntg_signal_value(NTG_CHANGEBIN_SIGNAL),
      "SIG" ntg_value(NTG_CHANGEBIN_SIGNAL),
      "",
      NULL,
      ntg_signal_handler },

    { SIGALRM, "SIGALRM", "", NULL, ntg_signal_handler },

    { SIGINT, "SIGINT", "", NULL, ntg_signal_handler },

    { SIGIO, "SIGIO", "", NULL, ntg_signal_handler },

    { SIGCHLD, "SIGCHLD", "", NULL, ntg_signal_handler },

    { 0, NULL, "", NULL, NULL }
};

/**
 * @name 忽略的信号集合
 */
ntg_ignore_signal_t  ignore_signals[] = {
	{ SIGSYS, "SIGSYS, SIG_IGN", ""},
	{ SIGPIPE, "SIGPIPE, SIG_IGN", ""},
	{ 0, NULL, ""}
};



/**
 * 产生子进程(spawn 大量生产)
 * @param[in] cycle 循环体
 * @param[in] proc 进程对应的执行例程
 * @param[in] data 传递子进程的数据, 工作进程槽位号worker
 * @param[in] name 对应的进程名称
 * @param[in] respawn 重生进程或重生标志, 当respawn>=0时表示重启进程的所在的槽位,其他表示重启标志
 * @param[in] type 进程类型，分为NTG_TRAFFIC_PROCESS, NTG_VIRTUAL_PROCESS, NTG_RECORD_PROCESS
 * @param[in] point 指定进程位置，小于0表示不指定
 * @return 成功返回进程id,否则返回NTG_INVALID_PID
 * @note 该函数中设置了ntg_channel的值为[1]端
 */
ntg_pid_t
ntg_spawn_process(ntg_cycle_t *cycle, ntg_spawn_proc_pt proc, void *data,
    char *name, ntg_int_t respawn, ntg_uint_t type, ntg_int_t point)
{
    u_long     on;
    ntg_pid_t  pid;
    ntg_int_t  s;
    ntg_core_conf_t *ccf;

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

    //1)找可用进程对象槽
    if (respawn >= 0) {
        s = respawn;

    } else if(point >= 0){
    	s = point;
    } else {
        for (s = ntg_start_process; s < ntg_last_process; s++) {
            if (ntg_processes[s].pid == -1) {
                break;
            }
        }

        if (s == NTG_MAX_PROCESSES) {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, 0,
                          "no more than %d processes can be spawned",
                          NTG_MAX_PROCESSES);
            return NTG_INVALID_PID;
        }
    }

    if(s > ccf->cpu_affinity_n - 2){///cpu资源不足
        ntg_log_error(NTG_LOG_ALERT, cycle->log, 0,
                      "%d CPU resources are insufficient",
                      ccf->cpu_affinity_n);
        return NTG_INVALID_PID;
    }

    ntg_processes[s].type = type;

    if (respawn != NTG_PROCESS_DETACHED) {//是否为分离进程

        /* Solaris 9 still has no AF_LOCAL */
    	//1) 创建Unix套接字
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, ntg_processes[s].channel) == -1)
        {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          "socketpair() failed while spawning \"%s\"", name);
            return NTG_INVALID_PID;
        }

        ntg_log_debug2(NTG_LOG_DEBUG_CORE, cycle->log, 0,
                       "channel %d:%d",
                       ntg_processes[s].channel[0],
                       ntg_processes[s].channel[1]);
        //设置非阻塞属性
        if (ntg_nonblocking(ntg_processes[s].channel[0]) == -1) {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          ntg_nonblocking_n " failed while spawning \"%s\"",
                          name);
            ntg_close_channel(ntg_processes[s].channel, cycle->log);
            return NTG_INVALID_PID;
        }

        if (ntg_nonblocking(ntg_processes[s].channel[1]) == -1) {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          ntg_nonblocking_n " failed while spawning \"%s\"",
                          name);
            ntg_close_channel(ntg_processes[s].channel, cycle->log);
            return NTG_INVALID_PID;
        }
        /* 开启信号驱动IO */
        //设置套接字的异步属性
		if ((int) data != 0) {//非虚拟用户进程
			on = 1;
			if (ioctl(ntg_processes[s].channel[0], FIOASYNC, &on) == -1) {
				ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
						"ioctl(FIOASYNC) failed while spawning \"%s\"", name);
				ntg_close_channel(ntg_processes[s].channel, cycle->log);
				return NTG_INVALID_PID;
			}
			//所有者属性
			if (fcntl(ntg_processes[s].channel[0], F_SETOWN, ntg_pid) == -1) {
				ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
						"fcntl(F_SETOWN) failed while spawning \"%s\"", name);
				ntg_close_channel(ntg_processes[s].channel, cycle->log);
				return NTG_INVALID_PID;
			}
		}


        //设置执行时,关闭属性
        if (fcntl(ntg_processes[s].channel[0], F_SETFD, FD_CLOEXEC) == -1) {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ntg_close_channel(ntg_processes[s].channel, cycle->log);
            return NTG_INVALID_PID;
        }

        if (fcntl(ntg_processes[s].channel[1], F_SETFD, FD_CLOEXEC) == -1) {
            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ntg_close_channel(ntg_processes[s].channel, cycle->log);
            return NTG_INVALID_PID;
        }

        ntg_channel = ntg_processes[s].channel[1];

    } else {//产生分离进程
        ntg_processes[s].channel[0] = -1;
        ntg_processes[s].channel[1] = -1;
    }

    ntg_process_slot = s;

#if (NTG_TILERA_PLATFORM)
	int watch_forked_children = tmc_task_watch_forked_children(1);//提醒检测子进程
#endif

    pid = fork();

    switch (pid) {

    case -1:

        ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                      "fork() failed while spawning \"%s\"", name);
        ntg_close_channel(ntg_processes[s].channel, cycle->log);
#if (NTG_TILERA_PLATFORM)
        tmc_task_die("Failure in 'fork()'.");
#endif
        return NTG_INVALID_PID;

    case 0://子进程
//    	printf("  child kkkkkkkkkkkkkkkkkkkkk------------oooo\n");
        ntg_pid = ntg_getpid();//获取进程自身的pid
        proc(cycle, data);
        break;

    default:
        break;
    }
#if (NTG_TILERA_PLATFORM)
	(void) tmc_task_watch_forked_children(watch_forked_children);//已完成fork
#endif

    //父进程
    ntg_log_error(NTG_LOG_NOTICE, cycle->log, 0, "start %s %P", name, pid);

    ntg_processes[s].pid = pid;
    ntg_processes[s].exited = 0;

    if (respawn >= 0) {
        return pid;
    }

    ntg_processes[s].proc = proc;
    ntg_processes[s].data = data;
    ntg_processes[s].name = name;
    ntg_processes[s].exiting = 0;

    switch (respawn) {

    case NTG_PROCESS_NORESPAWN://不重生
        ntg_processes[s].respawn = 0;
        ntg_processes[s].just_spawn = 0;
        ntg_processes[s].detached = 0;
        break;

    case NTG_PROCESS_JUST_SPAWN://正在重生
        ntg_processes[s].respawn = 0;
        ntg_processes[s].just_spawn = 1;
        ntg_processes[s].detached = 0;
        break;

    case NTG_PROCESS_RESPAWN://重生
        ntg_processes[s].respawn = 1;
        ntg_processes[s].just_spawn = 0;
        ntg_processes[s].detached = 0;
        break;

    case NTG_PROCESS_JUST_RESPAWN:
        ntg_processes[s].respawn = 1;
        ntg_processes[s].just_spawn = 1;
        ntg_processes[s].detached = 0;
        break;

    case NTG_PROCESS_DETACHED://分离
        ntg_processes[s].respawn = 0;
        ntg_processes[s].just_spawn = 0;
        ntg_processes[s].detached = 1;
        break;
    }

    if (s == ntg_last_process && type == NTG_TRAFFIC_PROCESS ) {
        ntg_last_process++;
    }

    return pid;
}

/**
 * 产生一个无亲缘关系的执行进程
 * @param[in] cycle 循环体
 * @param[in] ctx 执行上下午
 * @return 成功返回进程id,否则返回NTG_INVALID_PID
 */
ntg_pid_t
ntg_execute(ntg_cycle_t *cycle, ntg_exec_ctx_t *ctx)
{
    return ntg_spawn_process(cycle, ntg_execute_proc, ctx, ctx->name,
                             NTG_PROCESS_DETACHED, 0, -1);
}

/**
 * 执行进程调用函数
 * @param[in] cycle 循环体
 * @param[in] data 传递给进程的数据
 */
static void
ntg_execute_proc(ntg_cycle_t *cycle, void *data)
{
    ntg_exec_ctx_t  *ctx = data;

    if (execve(ctx->path, ctx->argv, ctx->envp) == -1) {
        ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                      "execve() failed while executing %s \"%s\"",
                      ctx->name, ctx->path);
    }

    exit(1);
}

/**
 * 初始化事件驱动并注册信号事件
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 完成了对非忽略信号集和忽略信号集的信号行为处理
 */
ntg_int_t
ntg_init_base_and_signals(ntg_cycle_t *cycle)
{
    ntg_signal_t        	*sig;
    ntg_ignore_signal_t 	*ign_sig;
    ntg_log_t				*log;
    struct sigaction   		sa;

    log = cycle->log;

    /* 添加事件驱动 */
//	event_enable_debug_mode(); //不能添加，否则fork后出现异常，并且不能调用event_reinit.

	cycle->base = event_base_new();
	if (cycle->base == NULL) {
		/*fatal*/
		ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
				"event_base_new(%s) failed");
		return NTG_ERROR;
	}

	event_set_fatal_callback(event_failed_cd);

	/* 注册有信号处理函数的信号事件 */
    for (sig = signals; sig->signo != 0; sig++) {

    	sig->ev_sig = evsignal_new(cycle->base, sig->signo, sig->handler, NULL);
    	if(sig->ev_sig == NULL){
			ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
					"evsignal_new(%s) failed", sig->signame);
    		return NTG_ERROR;
    	}

    	if (evsignal_add(sig->ev_sig, NULL) < 0){
			ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
					"evsignal_add(%s) failed", sig->signame);
			return NTG_ERROR;
    	}
    }

    /* 设置该忽略的信号 */
    for (ign_sig = ignore_signals; ign_sig->signo != 0; ign_sig++) {

		ntg_memzero(&sa, sizeof(struct sigaction));
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);

		if (sigaction(ign_sig->signo, &sa, NULL) == -1) {
			ntg_log_error(NTG_LOG_ALERT, log, ntg_errno,
					"sigaction(%s) failed, ignored", ign_sig->signame);
			return NTG_ERROR;
		}
	}


    return NTG_OK;
}


/**
 * 信号处理函数
 * @param[in] signo 信号
 * @note 按照不同类别的进程进行分类处理
 */
void ntg_signal_handler (int signo , short flags, void *args) {
    char            *action;
    ntg_int_t        ignore;
    ntg_err_t        err;
    ntg_signal_t    *sig;

    printf("-------&&&&&&&&&&&&&&&&&&&&&&&&-----ntg_signal_handler- %d--&&&&&&-->\n", signo);
    ignore = 0;

    err = ntg_errno;
    //找到指定的信号
    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }
    //更新时间

    action = "";

    switch (ntg_process) {

    /* master或单进程情况 */
    case NTG_PROCESS_MASTER:
    case NTG_PROCESS_SINGLE:
        switch (signo) {

        case ntg_signal_value(NTG_SHUTDOWN_SIGNAL):
            ntg_quit = 1;
            action = ", shutting down";
            break;

        case ntg_signal_value(NTG_TERMINATE_SIGNAL):
        case SIGINT:
            ntg_terminate = 1;
            action = ", exiting";
            break;

        case ntg_signal_value(NTG_RECONFIGURE_SIGNAL):
            ntg_reconfigure = 1;
            action = ", reconfiguring";
            break;

        case ntg_signal_value(NTG_ADD_SIGNAL):
            ntg_add = 1;
            action = ", adding logs";
            break;

        case ntg_signal_value(NTG_CHANGEBIN_SIGNAL):
            if (getppid() > 1 || ntg_new_binary > 0) {

                /*
                 * Ignore the signal in the new binary if its parent is
                 * not the init process, i.e. the old binary's process
                 * is still running.  Or ignore the signal in the old binary's
                 * process if the new binary's process is already running.
                 */

                action = ", ignoring";
                ignore = 1;
                break;
            }

            ntg_change_binary = 1;
            action = ", changing binary";
            break;

        case SIGALRM:
            ntg_sigalrm = 1;
            break;

        case SIGIO:
            ntg_sigio = 1;
            break;

        case SIGCHLD:
            ntg_reap = 1;
            break;
        }

        break;

    /* 子进程情况 */
    case NTG_PROCESS_VIRTUAL:
    case NTG_PROCESS_TRAFFIC:
    case NTG_PROCESS_RECORD:
    case NTG_PROCESS_HELPER:
        switch (signo) {

        case ntg_signal_value(NTG_SHUTDOWN_SIGNAL):
            ntg_quit = 1;
            action = ", shutting down";
            break;

        case ntg_signal_value(NTG_TERMINATE_SIGNAL):
        case SIGINT:
            ntg_terminate = 1;
            action = ", exiting";
            break;

//        case ntg_signal_value(NTG_REOPEN_SIGNAL):
//            ntg_reopen = 1;
//            action = ", reopening logs";
//            break;

        case ntg_signal_value(NTG_RECONFIGURE_SIGNAL):
        case ntg_signal_value(NTG_CHANGEBIN_SIGNAL):
        case SIGIO:
            action = ", ignoring";
            break;
        }

        break;
    }

    ntg_log_error(NTG_LOG_NOTICE, ntg_cycle->log, 0,
                  "signal %d (%s) received%s", signo, sig->signame, action);

    if (ignore) {//忽略日志记录
        ntg_log_error(NTG_LOG_CRIT, ntg_cycle->log, 0,
                      "the changing binary signal is ignored: "
                      "you should shutdown or terminate "
                      "before either old or new binary's process");
    }

    /* 子进程退出信号处理 */
    if (signo == SIGCHLD) {
        ntg_process_get_status();
    }

    ntg_set_errno(err);//恢复错误记录
}

/**
 * 获取子进程的退出状态
 * @note 因为收到的时子进程的SIGCHLD信号，表示子进程已退出，
 * 所以会设置该进程对象的exited标志
 */
static void
ntg_process_get_status(void)
{
    int              status;
    char            *process;
    ntg_pid_t        pid;
    ntg_err_t        err;
    ntg_int_t        i;
    ntg_uint_t       one;

    one = 0;//waitpid是否调用成功

    for ( ;; ) {
        pid = waitpid(-1, &status, WNOHANG);//非阻塞

        if (pid == 0) {//没有子进程结束
            return;
        }

        if (pid == -1) {//错误情况
            err = ntg_errno;

            if (err == NTG_EINTR) {
                continue;
            }

            if (err == NTG_ECHILD && one) {//没有子进程
                return;
            }

            /*
             * Solaris always calls the signal handler for each exited process
             * despite waitpid() may be already called for this process.
             *
             * When several processes exit at the same time FreeBSD may
             * erroneously call the signal handler for exited process
             * despite waitpid() may be already called for this process.
             */

            if (err == NTG_ECHILD) {
                ntg_log_error(NTG_LOG_INFO, ntg_cycle->log, err,
                              "waitpid() failed");
                return;
            }

            ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, err,
                          "waitpid() failed");
            return;
        }


        one = 1;
        process = "unknown process";

        for (i = 0; i < ntg_last_process; i++) {
            if (ntg_processes[i].pid == pid) {
                ntg_processes[i].status = status;
                ntg_processes[i].exited = 1;//设置已退出标志
                process = ntg_processes[i].name;
                break;
            }
        }

        if (WTERMSIG(status)) {///WTERMSIG宏测试成功用于返回进程退出的信号值
#ifdef WCOREDUMP
            ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0,
                          "%s %P exited on signal %d%s",
                          process, pid, WTERMSIG(status),
                          WCOREDUMP(status) ? " (core dumped)" : "");
#else
            ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0,
                          "%s %P exited on signal %d",
                          process, pid, WTERMSIG(status));
#endif

        } else {
            ntg_log_error(NTG_LOG_NOTICE, ntg_cycle->log, 0,
                          "%s %P exited with code %d",
                          process, pid, WEXITSTATUS(status));
        }

        /* 错误代码为2时，退出的进程不能重生 */
        if (WEXITSTATUS(status) == 2 && ntg_processes[i].respawn) {
            ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0,
                          "%s %P exited with fatal code %d "
                          "and cannot be respawned",
                          process, pid, WEXITSTATUS(status));
            ntg_processes[i].respawn = 0;
        }

    }
}


/**
 * 设置调试点
 */
void
ntg_debug_point(void)
{
    ntg_core_conf_t  *ccf;

    ccf = (ntg_core_conf_t *) ntg_get_conf(ntg_cycle->conf_ctx,
                                           ntg_core_module);

    switch (ccf->debug_points) {

    case NTG_DEBUG_POINTS_STOP:
        raise(SIGSTOP);
        break;

    case NTG_DEBUG_POINTS_ABORT:
        ntg_abort();
    }
}


/**
 * 向进程发送信号
 * @param[in] cycle 循环对象
 * @param[in] name 信号名称
 * @param[in] pid 进程id
 * @return 成功返回0,否则返回1
 */
ntg_int_t
ntg_os_signal_process(ntg_cycle_t *cycle, char *name, ntg_int_t pid)
{
    ntg_signal_t  *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        if (ntg_strcmp(name, sig->name) == 0) {
            if (kill(pid, sig->signo) != -1) {//发送信号
                return 0;
            }

            ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
                          "kill(%P, %d) failed", pid, sig->signo);
        }
    }

    return 1;
}
