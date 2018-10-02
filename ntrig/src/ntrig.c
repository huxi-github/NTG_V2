/**
 * @file 	ntrig.c
 * @brief
 * @author	tzh
 * @date	Oct 31, 2015
 * @version		V0.1
 * @copyright	tzh
 */

#include "ntg_config.h"
#include "ntg_core.h"

#include "ntrig.h"
#include "utils/ntg_errno.h"
#include "utils/ntg_process_cycle.h"
#include "utils/ntg_conf_file.h"
#include "utils/ntg_times.h"
#include "utils/ntg_os.h"
#include "utils/ntg_inet.h"
#include "utils/ntg_remote.h"

static ntg_int_t ntg_get_options(int argc, char * const *argv);
static ntg_int_t ntg_process_options(ntg_cycle_t *cycle);
static void ntg_read_url_file( ntg_cycle_t *cycle, char* url_file);
static ntg_int_t
		ntg_save_argv(ntg_cycle_t *cycle, int argc, char * const *argv);
static void *ntg_core_module_create_conf(ntg_cycle_t *cycle);
static char *ntg_core_module_init_conf(ntg_cycle_t *cycle, void *conf);
static char *ntg_set_user(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_set_env(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_set_priority(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_set_cpu_affinity(ntg_conf_t *cf, ntg_command_t *cmd,
		void *conf);
static char *ntg_set_worker_processes(ntg_conf_t *cf, ntg_command_t *cmd,
		void *conf);
static char *ntg_set_front_end(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);
static char *ntg_set_database(ntg_conf_t *cf, ntg_command_t *cmd, void *conf);

/**调试选项*/
static ntg_conf_enum_t ntg_debug_points[] = { { ntg_string("stop"),
		NTG_DEBUG_POINTS_STOP },/**< stop*/
{ ntg_string("abort"), NTG_DEBUG_POINTS_ABORT },/**< abort*/
{ ntg_null_string, 0 } };

/**核心模块配置命令集合*/
static ntg_command_t ntg_core_commands[] = {

		{ ntg_string("daemon"), NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_FLAG,
		ntg_conf_set_flag_slot, 0, offsetof(ntg_core_conf_t, daemon), NULL },

		{ ntg_string("master_process"),
		NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_FLAG,
		ntg_conf_set_flag_slot, 0, offsetof(ntg_core_conf_t, master), NULL },

		{ ntg_string("timer_resolution"), NTG_MAIN_CONF | NTG_DIRECT_CONF
		| NTG_CONF_TAKE1, ntg_conf_set_msec_slot, 0,
		offsetof(ntg_core_conf_t, timer_resolution), NULL },

		{ ntg_string("pid"), NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_TAKE1,
		ntg_conf_set_str_slot, 0, offsetof(ntg_core_conf_t, pid), NULL },

		{ ntg_string("lock_file"), NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_TAKE1,
		ntg_conf_set_str_slot, 0, offsetof(ntg_core_conf_t, lock_file), NULL },

		{ ntg_string("consume_num"), NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_TAKE1,
		ntg_conf_set_num_slot, 0, offsetof(ntg_core_conf_t, consumes), NULL },

		{ ntg_string("listen_address"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE2, ntg_set_front_end, 0,
				offsetof(ntg_core_conf_t, front), NULL },

		{ntg_string("database"), NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_TAKE6,
				ntg_set_database, 0,
				offsetof(ntg_core_conf_t, database), NULL },

		{ntg_string("worker_processes"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE1, ntg_set_worker_processes, 0, 0, NULL },

		{ ntg_string("debug_points"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE1, ntg_conf_set_enum_slot, 0,
				offsetof(ntg_core_conf_t, debug_points), &ntg_debug_points },

		{ ntg_string("user"),
				NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_TAKE12,
				ntg_set_user, 0, 0, NULL },

		{ ntg_string("worker_priority"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE1, ntg_set_priority, 0, 0, NULL },

		{ ntg_string("worker_cpu_affinity"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_1MORE, ntg_set_cpu_affinity, 0, 0, NULL },

		{ ntg_string("worker_rlimit_nofile"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE1, ntg_conf_set_num_slot, 0,
				offsetof(ntg_core_conf_t, rlimit_nofile), NULL },

		{ ntg_string("worker_rlimit_core"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE1, ntg_conf_set_off_slot, 0,
				offsetof(ntg_core_conf_t, rlimit_core), NULL },

		{ ntg_string("worker_rlimit_sigpending"), NTG_MAIN_CONF
				| NTG_DIRECT_CONF | NTG_CONF_TAKE1, ntg_conf_set_num_slot, 0,
				offsetof(ntg_core_conf_t, rlimit_sigpending), NULL },

		{ ntg_string("working_directory"), NTG_MAIN_CONF | NTG_DIRECT_CONF
				| NTG_CONF_TAKE1, ntg_conf_set_str_slot, 0,
				offsetof(ntg_core_conf_t, working_directory), NULL },

		{ ntg_string("env"), NTG_MAIN_CONF | NTG_DIRECT_CONF | NTG_CONF_TAKE1,
				ntg_set_env, 0, 0, NULL }, ntg_null_command };

/** 核心模块上下文,用于配置项的解析 */
static ntg_core_module_t ntg_core_module_ctx = {
		ntg_string("core"),
		ntg_core_module_create_conf,
		ntg_core_module_init_conf
};

/** 核心模块 */
ntg_module_t ntg_core_module = { NTG_MODULE_V1, &ntg_core_module_ctx, /* module context */
ntg_core_commands, /* module directives */
NTG_CORE_MODULE, /* module type */
NULL, /* init master */
NULL, /* init module */
NULL, /* init process */
NULL, /* init thread */
NULL, /* exit thread */
NULL, /* exit process */
NULL, /* exit master */
NTG_MODULE_V1_PADDING
};

ntg_uint_t ntg_max_module;///< 最大模块数
/**
 * @name 输入选项的标志属性
 * @{
 */
static ntg_uint_t ntg_show_help;///< 显示帮助
static ntg_uint_t ntg_show_version;///< 显示版本
static ntg_uint_t ntg_show_configure;///< 显示编译配置

static u_char *ntg_prefix;///< 路经前缀
static u_char *ntg_conf_file;///< 配置文件
static u_char *ntg_conf_params;///< 配置参数

static char *ntg_url_file;///< url集文件
//static char *ntg_addr;///< 地址

static char *ntg_signal;///< 发送的信号

static char **ntg_os_environ;///< 系统环境变量
/**@}*/

/**
 * 程序入口
 * @param argc 输入参数个数
 * @param argv 参数数组
 * @return 成功返回0, 否则返回1
 * @note 主要执行过程:
 * 	1)调试初始化,暂时没有实现
 * 	2)初始化错误列表
 * 	3)获取选项
 * 	4)检测显示版本标志
 * 	5)初始化时间缓存
 * 	6)初始化日志系统
 * 	7)初始化init_cycle对象
 * 	8)保存argv的值
 * 	9)处理输入选项
 * 	10)初始化操作系统属性
 * 	11)添加继承的监听套接字
 * 	12)初始化各模块的序号
 * 	13)初始化循环体
 * 	14)测试test_config选项,若设置返回
 * 	15)测试ntg_signal选项
 * 	16)日志文件中记录操作系统的一些状态
 * 	17)设置当前的ntg_cycle
 * 	18)进程类型设置
 * 	19)初始化信号的行为函数
 * 	20)守护进程标志的测试与设置
 * 	21)测试已继承标志
 * 	22)创建pid文件
 * 	23)定向标准错误到日志
 * 	24)测试日志文件是否为标准错误,不是关闭它
 * 	25)设置ntg_use_stderr = 0
 * 	26)进入不同的进程模式
 */
int main(int argc, char ** argv) {

	ntg_int_t i;
	ntg_log_t *log;
	ntg_cycle_t *cycle, init_cycle;
	ntg_core_conf_t *ccf;

	//1)调试初始化,TODO 暂时没有实现
	ntg_debug_init();

	//2)初始化错误列表
	if (ntg_strerror_init() != NTG_OK) {
		return 1;
	}

	//3)获取选项
	if (ntg_get_options(argc, argv) != NTG_OK) {
		return 1;
	}

	//4)检测显示版本标志
	if (ntg_show_version) {

		ntg_write_stderr("ntrig version: " NTRIG_VER_BUILD NTG_LINEFEED);

		if (ntg_show_help) {

			ntg_write_stderr("Usage: ntrig [-?hvt] [-s signal] " NTG_LINEFEED
			NTG_LINEFEED
			"Options:" NTG_LINEFEED
			"  -?,-h         : this help" NTG_LINEFEED
			"  -v            : show version and exit" NTG_LINEFEED
			"  -t            : test configuration and exit" NTG_LINEFEED
			"  -f            : local URL file name" NTG_LINEFEED
			"  -s signal     : send signal to a master process: "
			"stop, quit, add, reload" NTG_LINEFEED);
		}

		if (ntg_show_configure) {
#ifdef NTG_COMPILER//编译器选项
			ntg_write_stderr("built by " NTG_COMPILER NTG_LINEFEED);
#endif
			//			ntg_write_stderr("configure arguments:" NTG_CONFIGURE NTG_LINEFEED);
		}

		if (!ntg_test_config) {
			return 0;
		}
	}

	//	ntg_max_sockets = -1;

	//5)初始化时间缓存
	ntg_time_init();

	ntg_pid = ntg_getpid();

	//6)初始化日志系统
	log = ntg_log_init(ntg_prefix);
	if (log == NULL) {
		return 1;
	}

	//7)初始化init_cycle对象
	ntg_memzero(&init_cycle, sizeof(ntg_cycle_t));
	init_cycle.log = log;
	ntg_cycle = &init_cycle;
	init_cycle.pool = ntg_create_pool(1024, log);
	if (init_cycle.pool == NULL) {
		return 1;
	}
	//8)保存argv的值
	if (ntg_save_argv(&init_cycle, argc, argv) != NTG_OK) {
		return 1;
	}
	//9)处理输入选项
	if (ntg_process_options(&init_cycle) != NTG_OK) {
		return 1;
	}
	//10)初始化操作系统属性
	if (ntg_os_init(log) != NTG_OK) {
		return 1;
	}

	//12)初始化各模块的序号
	ntg_max_module = 0;
	for (i = 0; ntg_modules[i]; i++) {
		ntg_modules[i]->index = ntg_max_module++;
	}

	//13)初始化循环体
	cycle = ntg_init_cycle(&init_cycle);
	if (cycle == NULL) {
		if (ntg_test_config) {
			ntg_log_stderr(0, "configuration file %s test failed",
					init_cycle.conf_file.data);
		}

		return 1;
	}


	//14)测试test_config选项,若设置返回
	if (ntg_test_config) {
		if (!ntg_quiet_mode) {
			ntg_log_stderr(0, "configuration file %s test is successful",
					cycle->conf_file.data);
		}

		return 0;
	}
	//15)测试ntg_signal选项,若设置返送信号并返回
	if (ntg_signal) {
		return ntg_signal_process(cycle, ntg_signal);
	}

    /* 处理url配置文件 */
    if(ntg_url_file){
    	ntg_read_url_file(cycle, ntg_url_file);
    } else {
#ifdef NTG_URL_FILE
    	ntg_read_url_file(cycle, NTG_PREFIX NTG_URL_FILE);
#else
    	return 1;
 #endif
    }

    if(cycle->url_n < 1){
    	return 1;
    }

	//16)日志文件中记录操作系统的一些状态
	ntg_os_status(cycle->log);

	//17)设置当前的ntg_cycle
	ntg_cycle = cycle;

	//18)进程类型设置
	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	if (ccf->master && ntg_process == NTG_PROCESS_SINGLE) {//切换到master进程
		ntg_process = NTG_PROCESS_MASTER;
	}

	/* 初始化事件驱动并注册信号事件 */
	if (ntg_init_base_and_signals(cycle) != NTG_OK) {
		exit(2);
	}

	/* 守护进程标志的测试与设置 */
	if (!ntg_inherited && ccf->daemon) {
		if (ntg_daemon(cycle->log) != NTG_OK) {
			return 1;
		}

		ntg_daemonized = 1;
	}

	//21)测试已继承标志
	if (ntg_inherited) {
		ntg_daemonized = 1;
	}
	//22)创建pid文件
	if (ntg_create_pidfile(&ccf->pid, cycle->log) != NTG_OK) {
		return 1;
	}
	//23)定向标准错误到日志
	if (ntg_log_redirect_stderr(cycle) != NTG_OK) {
		return 1;
	}
	//24)测试日志文件是否为标准错误,不是关闭它
	if (log->file->fd != ntg_stderr) {
		if (ntg_close_file(log->file->fd) == NTG_FILE_ERROR) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					ntg_close_file_n " built-in log failed");
		}
	}

	//25)设置ntg_use_stderr = 0
	ntg_use_stderr = 0;

	//26)进入不同的进程模式
	if (ntg_process == NTG_PROCESS_SINGLE) {
		ntg_single_process_cycle(cycle);//单进程模式

	} else {//master模式
		printf("--ntg_master_process_cycle-=====-=\n");
		ntg_master_process_cycle(cycle);
	}

	return 0;
}

/**
 * 设置环境变量
 * @param[in] cycle 全局循环体
 * @param[in] last
 * @return 环境变量指针
 */
char **
ntg_set_environment(ntg_cycle_t *cycle, ntg_uint_t *last) {
	char **p, **env;
	ntg_str_t *var;
	ntg_uint_t i, n;
	ntg_core_conf_t *ccf;

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	if (last == NULL && ccf->environment) {//存在
		return ccf->environment;
	}

	var = ccf->env.elts;
	//查找TZ
	for (i = 0; i < ccf->env.nelts; i++) {
		if (ntg_strcmp(var[i].data, "TZ") == 0
				|| ntg_strncmp(var[i].data, "TZ=", 3) == 0) {
			goto tz_found;
		}
	}
	//没有找到,则分配一个空间
	var = ntg_array_push(&ccf->env);
	if (var == NULL) {
		return NULL;
	}

	var->len = 2;
	var->data = (u_char *) "TZ";//时区

	var = ccf->env.elts;

	tz_found:

	n = 0;//表示可用的环境标量数

	for (i = 0; i < ccf->env.nelts; i++) {

		if (var[i].data[var[i].len] == '=') {//有=表示该环境变量是有值的
			n++;
			continue;
		}
		//通过查找ntg_os_environ得到值
		for (p = ntg_os_environ; *p; p++) {

			if (ntg_strncmp(*p, var[i].data, var[i].len) == 0
					&& (*p)[var[i].len] == '=') {
				n++;
				break;
			}
		}
	}
	//分配内存
	if (last) {//last不为空
		env = ntg_alloc((*last + n + 1) * sizeof(char *), cycle->log);
		*last = n;

	} else {
		env = ntg_palloc(cycle->pool, (n + 1) * sizeof(char *));
	}

	if (env == NULL) {
		return NULL;
	}

	n = 0;

	for (i = 0; i < ccf->env.nelts; i++) {

		if (var[i].data[var[i].len] == '=') {
			env[n++] = (char *) var[i].data;
			continue;
		}

		for (p = ntg_os_environ; *p; p++) {

			if (ntg_strncmp(*p, var[i].data, var[i].len) == 0
					&& (*p)[var[i].len] == '=') {
				env[n++] = *p;
				break;
			}
		}
	}

	env[n] = NULL;
	//设置环境变量
	if (last == NULL) {
		ccf->environment = env;
		environ = env;
	}

	return env;
}

/**
 * 执行一个新的二进文件
 * @param cycle 全局循环体
 * @param argv	传递给可执行文件的参数
 * @return 成功返回进程id,否则返回-1(NTG_INVALID_PID)
 */
ntg_pid_t ntg_exec_new_binary(ntg_cycle_t *cycle, char * const *argv) {
	char **env, *var;
	u_char *p;
	ntg_uint_t n;
	ntg_pid_t pid;
	ntg_exec_ctx_t ctx;
	ntg_core_conf_t *ccf;

	ntg_memzero(&ctx, sizeof(ntg_exec_ctx_t));

	ctx.path = argv[0];
	ctx.name = "new binary process";
	ctx.argv = argv;

	n = 2;
	env = ntg_set_environment(cycle, &n);
	if (env == NULL) {
		return NTG_INVALID_PID;
	}

	var = ntg_alloc(sizeof(NTRIG_VAR) + 2, cycle->log);
	if (var == NULL) {
		ntg_free(env);
		return NTG_INVALID_PID;
	}
	//TODO
	p = ntg_cpymem(var, NTRIG_VAR "=", sizeof(NTRIG_VAR));

	//    ls = cycle->listening.elts;
	//    for (i = 0; i < cycle->listening.nelts; i++) {
	//        p = ntg_sprintf(p, "%ud;", ls[i].fd);
	//    }

	*p = '\0';

	env[n++] = var;

#if (NTG_SETPROCTITLE_USES_ENV)

	/* allocate the spare 300 bytes for the new binary process title */

	env[n++] = "SPARE=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
	"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

#endif

	env[n] = NULL;

#if (NTG_DEBUG)
	{
		char **e;
		for (e = env; *e; e++) {
			ntg_log_debug1(NTG_LOG_DEBUG_CORE, cycle->log, 0, "env: %s", *e);
		}
	}
#endif

	ctx.envp = (char * const *) env;

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	if (ntg_rename_file(ccf->pid.data, ccf->oldpid.data) == NTG_FILE_ERROR) {
		ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
				ntg_rename_file_n " %s to %s failed "
				"before executing new binary process \"%s\"",
				ccf->pid.data, ccf->oldpid.data, argv[0]);

		ntg_free(env);
		ntg_free(var);

		return NTG_INVALID_PID;
	}

	pid = ntg_execute(cycle, &ctx);

	if (pid == NTG_INVALID_PID) {
		if (ntg_rename_file(ccf->oldpid.data, ccf->pid.data) == NTG_FILE_ERROR) {
			ntg_log_error(NTG_LOG_ALERT, cycle->log, ntg_errno,
					ntg_rename_file_n " %s back to %s failed after "
					"an attempt to execute new binary process \"%s\"",
					ccf->oldpid.data, ccf->pid.data, argv[0]);
		}
	}

	ntg_free(env);
	ntg_free(var);

	return pid;
}

/**
 * 获取选项
 * @details 主要对各个输入参数的格式进行分析并设置对应的标志或值
 * @param[in] argc 输入参数个数
 * @param[in] argv	参数数组
 * @return 成功返回 NTG_OK, 否则返回 NTG_ERROR
 * @note
 * 	设置的标志有:
 * 		-?, -h 	ntg_show_version, ntg_show_help
 * 		-v		ntg_show_version
 * 		-V 		ntg_show_version, ntg_show_configure
 * 		-t		ntg_test_config
 * 		-q		ntg_quiet_mode
 * 		-p		ntg_prefix
 * 		-c		ntg_conf_file
 * 		-g		ntg_conf_params
 * 		-s		ntg_signal
 * 		-f      ntg_url_file
 */
static ntg_int_t ntg_get_options(int argc, char * const *argv) {
	u_char *p;
	ntg_int_t i;

	//处理每一个输入选项
	for (i = 1; i < argc; i++) {

		p = (u_char *) argv[i];

		if (*p++ != '-') {//如果不以'-'开头表示出错
			ntg_log_stderr(0, "invalid option: \"%s\"", argv[i]);
			return NTG_ERROR;
		}

		while (*p) {

			switch (*p++) {
			//帮助选项
			case '?':
			case 'h':
				ntg_show_version = 1;
				ntg_show_help = 1;
				break;
				//显示版本
			case 'v':
				ntg_show_version = 1;
				break;
				//显示版本和配置
			case 'V':
				ntg_show_version = 1;
				ntg_show_configure = 1;
				break;
				//配置测试
			case 't':
				ntg_test_config = 1;
				break;
				//退出
			case 'q':
				ntg_quiet_mode = 1;
				break;
				//设置路经前缀
			case 'p':
				if (*p) {//处理选项与值之间没有空格的情况
					ntg_prefix = p;
					goto next;
				}

				if (argv[++i]) {
					ntg_prefix = (u_char *) argv[i];
					goto next;
				}
				//没有选项值
				ntg_log_stderr(0, "option \"-p\" requires directory name");
				return NTG_ERROR;
				//设置配置文件
			case 'c':
				if (*p) {
					ntg_conf_file = p;
					goto next;
				}

				if (argv[++i]) {
					ntg_conf_file = (u_char *) argv[i];
					goto next;
				}

				ntg_log_stderr(0, "option \"-c\" requires file name");
				return NTG_ERROR;
				//全局配置文件目录
			case 'g':
				if (*p) {
					ntg_conf_params = p;
					goto next;
				}

				if (argv[++i]) {
					ntg_conf_params = (u_char *) argv[i];
					goto next;
				}

				ntg_log_stderr(0, "option \"-g\" requires parameter");
				return NTG_ERROR;
				//发送信号给master进程
			case 's':
				if (*p) {
					ntg_signal = (char *) p;

				} else if (argv[++i]) {
					ntg_signal = argv[i];

				} else {
					ntg_log_stderr(0, "option \"-s\" requires parameter");
					return NTG_ERROR;
				}

				//分析信号类型
				if (ntg_strcmp(ntg_signal, "stop") == 0
						|| ntg_strcmp(ntg_signal, "quit") == 0
						|| ntg_strcmp(ntg_signal, "add") == 0
						|| ntg_strcmp(ntg_signal, "reload") == 0) {
					ntg_process = NTG_PROCESS_SIGNALLER;
					goto next;
				}

				ntg_log_stderr(0, "invalid option: \"-s %s\"", ntg_signal);
				return NTG_ERROR;
			/* url文件路径选项 */
			case 'f':
				if (*p) {
					ntg_url_file = (char *) p;
					goto next;
				}

				if (argv[++i]) {
					ntg_url_file = (char *) argv[i];
					goto next;
				}

				ntg_log_stderr(0, "option \"-f\" requires file name");
				return NTG_ERROR;
			default:
				ntg_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
				return NTG_ERROR;
			}
		}

		next:

		continue;
	}

	return NTG_OK;
}

/**
 * 保存输入参数的值
 * @param[in] cycle cycle对象指针
 * @param[in] argc argc输入参数的个数
 * @param[in] argv argv输入参数数组
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 不仅保存了输入的参数,还将系统的环境变量保存到ntg_os_environ中.
 * @note 设置的相关属性有:
 * 		ntg_os_argv
 * 		ntg_argc
 * 		ntg_argv
 * 		ntg_os_environ
 * @note 带os的参数表示是系统布局连续存放的.
 */
static ntg_int_t ntg_save_argv(ntg_cycle_t *cycle, int argc, char * const *argv) {

	size_t len;
	ntg_int_t i;

	ntg_os_argv = (char **) argv;
	ntg_argc = argc;

	ntg_argv = ntg_alloc((argc + 1) * sizeof(char *), cycle->log);
	if (ntg_argv == NULL) {
		return NTG_ERROR;
	}

	for (i = 0; i < argc; i++) {
		len = ntg_strlen(argv[i]) + 1;

		ntg_argv[i] = ntg_alloc(len, cycle->log);
		if (ntg_argv[i] == NULL) {
			return NTG_ERROR;
		}

		(void) ntg_cpystrn((u_char *) ntg_argv[i], (u_char *) argv[i], len);
	}

	ntg_argv[i] = NULL;

	ntg_os_environ = environ;

	return NTG_OK;
}

/**
 * 处理选项参数
 * @details 将输入的选项参数保存到循环体中
 * @param[in] cycle 循环对象指针
 * @return 成功返回NTG_OK, 否则,返回NTG_ERROR
 * @note 处理的选项有:
 * 	ntg_prefix
 * 	ntg_conf_file
 * 	ntg_conf_params
 * 	ntg_test_config
 *
 * @note 执行过程:
 * 	1)路经前缀的设置
 * 	2)配置文件的设置
 * 	3)获取配置文件完整路经
 * 	4)重新设置配置前缀
 * 	5)设置配置参数
 * 	6)测试配置设置时,设置日志等级为NTG_LOG_INFO
 *
 */
static ntg_int_t ntg_process_options(ntg_cycle_t *cycle) {
	u_char *p;
	size_t len;
	//1)路经前缀的设置
	if (ntg_prefix) {//指定了路经前缀
		len = ntg_strlen(ntg_prefix);
		p = ntg_prefix;
		//确保以'/'结尾
		if (len && !ntg_path_separator(p[len - 1])) {
			p = ntg_pnalloc(cycle->pool, len + 1);
			if (p == NULL) {
				return NTG_ERROR;
			}

			ntg_memcpy(p, ntg_prefix, len);
			p[len++] = '/';
		}

		cycle->conf_prefix.len = len;
		cycle->conf_prefix.data = p;
		cycle->prefix.len = len;
		cycle->prefix.data = p;

	} else {

#ifndef NTG_PREFIX

		p = ntg_pnalloc(cycle->pool, NTG_MAX_PATH);
		if (p == NULL) {
			return NTG_ERROR;
		}

		if (ntg_getcwd(p, NTG_MAX_PATH) == 0) {
			ntg_log_stderr(ntg_errno, "[emerg]: " ntg_getcwd_n " failed");
			return NTG_ERROR;
		}

		len = ntg_strlen(p);

		p[len++] = '/';

		cycle->conf_prefix.len = len;
		cycle->conf_prefix.data = p;
		cycle->prefix.len = len;
		cycle->prefix.data = p;

#else

#ifdef NTG_CONF_PREFIX
		ntg_str_set(&cycle->conf_prefix, NTG_CONF_PREFIX);
#else
		ntg_str_set(&cycle->conf_prefix, NTG_PREFIX);
#endif
		ntg_str_set(&cycle->prefix, NTG_PREFIX);
#endif

	}
	//2)配置文件的设置
	if (ntg_conf_file) {
		cycle->conf_file.len = ntg_strlen(ntg_conf_file);
		cycle->conf_file.data = ntg_conf_file;

	} else {
		ntg_str_set(&cycle->conf_file, NTG_CONF_PATH);
	}
	//3)获取配置文件完整路经
	if (ntg_conf_full_name(cycle, &cycle->conf_file, 0) != NTG_OK) {
		return NTG_ERROR;
	}

	//4)重新设置配置前缀
	for (p = cycle->conf_file.data + cycle->conf_file.len - 1; p
			> cycle->conf_file.data; p--) {
		if (ntg_path_separator(*p)) {//带'/'结束
			cycle->conf_prefix.len = p - ntg_cycle->conf_file.data + 1;
			cycle->conf_prefix.data = ntg_cycle->conf_file.data;
			break;
		}
	}

	//5)设置配置参数
	if (ntg_conf_params) {
		cycle->conf_param.len = ntg_strlen(ntg_conf_params);
		cycle->conf_param.data = ntg_conf_params;
	}
	//6)测试配置设置时,设置日志等级为NTG_LOG_INFO
	if (ntg_test_config) {
		cycle->log->log_level = NTG_LOG_INFO;
	}

	return NTG_OK;
}

static void ntg_read_url_file( ntg_cycle_t *cycle, char* url_file) {
	FILE* fp;
	char line[5000];
	int num_urls;

	fp = fopen(url_file, "r");
	if (fp == (FILE*) 0) {
		perror(url_file);
		exit(1);
	}

	num_urls = 0;
	while (fgets(line, sizeof(line), fp) != (char*) 0) {
		/* Nuke trailing newline. */
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';
		}

		/* Add to table. */
		cycle->ntg_urls[num_urls] = strdup(line);

		++num_urls;

		if(num_urls >= NTG_HTT_URL_MAX){
			break;
		}
	}

	cycle->url_n = num_urls;
	cycle->isurl = 1;

	fclose(fp);
}

/**
 * core_module的创建配置
 * @details 分配一个ntg_core_conf_t,并对其各个属性设置默认值
 * @param[in] cycle 全局循环对象
 * @note 这些默认值大多是不可用的
 */
static void *
ntg_core_module_create_conf(ntg_cycle_t *cycle) {
	ntg_core_conf_t *ccf;

	ccf = ntg_pcalloc(cycle->pool, sizeof(ntg_core_conf_t));
	if (ccf == NULL) {
		return NULL;
	}

	/*
	 * set by ntg_pcalloc()
	 *
	 *     ccf->pid = NULL;
	 *     ccf->oldpid = NULL;
	 *     ccf->priority = 0;
	 *     ccf->cpu_affinity_n = 0;
	 *     ccf->cpu_affinity = NULL;
	 */

	ccf->daemon = NTG_CONF_UNSET;
	ccf->master = NTG_CONF_UNSET;
	ccf->timer_resolution = NTG_CONF_UNSET_MSEC;

	ccf->consumes = NTG_CONF_UNSET;
	ccf->worker_processes = NTG_CONF_UNSET;
	ccf->debug_points = NTG_CONF_UNSET;

	ccf->rlimit_nofile = NTG_CONF_UNSET;
	ccf->rlimit_core = NTG_CONF_UNSET;
	ccf->rlimit_sigpending = NTG_CONF_UNSET;

	ccf->user = (ntg_uid_t) NTG_CONF_UNSET_UINT;
	ccf->group = (ntg_gid_t) NTG_CONF_UNSET_UINT;

#if (NTG_OLD_THREADS)
	ccf->worker_threads = NTG_CONF_UNSET;
	ccf->thread_stack_size = NTG_CONF_UNSET_SIZE;
#endif

	if (ntg_array_init(&ccf->env, cycle->pool, 1, sizeof(ntg_str_t)) != NTG_OK) {
		return NULL;
	}

	return ccf;
}

/**
 * core_module的配置初始化
 * @param[in] cycle 全局循环对象
 * @param[in] conf 模块对象的配置结构体
 * @return 成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR
 */
static char *
ntg_core_module_init_conf(ntg_cycle_t *cycle, void *conf) {
	ntg_core_conf_t *ccf = conf;

	ntg_conf_init_value(ccf->daemon, 1);
	ntg_conf_init_value(ccf->master, 1);
	ntg_conf_init_msec_value(ccf->timer_resolution, 0);

	ntg_conf_init_value(ccf->worker_processes, 1);
	ntg_conf_init_value(ccf->debug_points, 0);

#if (NTG_HAVE_CPU_AFFINITY)

	if (ccf->cpu_affinity_n
			&& ccf->cpu_affinity_n != 1
			&& ccf->cpu_affinity_n != (ntg_uint_t) ccf->worker_processes)
	{
		ntg_log_error(NTG_LOG_WARN, cycle->log, 0,
				"the number of \"worker_processes\" is not equal to "
				"the number of \"worker_cpu_affinity\" masks, "
				"using last mask for remaining worker processes");
	}

#endif

#if (NTG_OLD_THREADS)

	ntg_conf_init_value(ccf->worker_threads, 0);
	ntg_threads_n = ccf->worker_threads;
	ntg_conf_init_size_value(ccf->thread_stack_size, 2 * 1024 * 1024);

#endif

	if (ccf->pid.len == 0) {
		ntg_str_set(&ccf->pid, NTG_PID_PATH)
		;
	}

	if (ntg_conf_full_name(cycle, &ccf->pid, 0) != NTG_OK) {
		return NTG_CONF_ERROR;
	}
	//设置老的进程pid文件
	ccf->oldpid.len = ccf->pid.len + sizeof(NTG_OLDPID_EXT);

	ccf->oldpid.data = ntg_pnalloc(cycle->pool, ccf->oldpid.len);
	if (ccf->oldpid.data == NULL) {
		return NTG_CONF_ERROR;
	}

	ntg_memcpy(ntg_cpymem(ccf->oldpid.data, ccf->pid.data, ccf->pid.len),
			NTG_OLDPID_EXT, sizeof(NTG_OLDPID_EXT));

	if (ccf->user == (uid_t) NTG_CONF_UNSET_UINT && geteuid() == 0) {
		struct group *grp;
		struct passwd *pwd;

		ntg_set_errno(0);
		pwd = getpwnam(NTG_USER);
		if (pwd == NULL) {
			ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
					"getpwnam(\"" NTG_USER "\") failed");
			return NTG_CONF_ERROR;
		}

		ccf->username = NTG_USER;
		ccf->user = pwd->pw_uid;

		ntg_set_errno(0);
		grp = getgrnam(NTG_GROUP);
		if (grp == NULL) {
			ntg_log_error(NTG_LOG_EMERG, cycle->log, ntg_errno,
					"getgrnam(\"" NTG_GROUP "\") failed");
			return NTG_CONF_ERROR;
		}

		ccf->group = grp->gr_gid;
	}

	if (ccf->lock_file.len == 0) {
		ntg_str_set(&ccf->lock_file, NTG_LOCK_PATH)
		;
	}

	if (ntg_conf_full_name(cycle, &ccf->lock_file, 0) != NTG_OK) {
		return NTG_CONF_ERROR;
	}

	{
		ntg_str_t lock_file;

		lock_file = cycle->old_cycle->lock_file;

		if (lock_file.len) {
			lock_file.len--;

			if (ccf->lock_file.len != lock_file.len
					|| ntg_strncmp(ccf->lock_file.data, lock_file.data, lock_file.len)
							!= 0) {
				ntg_log_error(NTG_LOG_EMERG, cycle->log, 0,
						"\"lock_file\" could not be changed, ignored");
			}

			cycle->lock_file.len = lock_file.len + 1;
			lock_file.len += sizeof(".accept");

			cycle->lock_file.data = ntg_pstrdup(cycle->pool, &lock_file);
			if (cycle->lock_file.data == NULL) {
				return NTG_CONF_ERROR;
			}

		} else {
			cycle->lock_file.len = ccf->lock_file.len + 1;
			cycle->lock_file.data = ntg_pnalloc(cycle->pool,
					ccf->lock_file.len + sizeof(".accept"));
			if (cycle->lock_file.data == NULL) {
				return NTG_CONF_ERROR;
			}

			ntg_memcpy(ntg_cpymem(cycle->lock_file.data, ccf->lock_file.data,
							ccf->lock_file.len),
					".accept", sizeof(".accept"));
		}
	}

	return NTG_CONF_OK;
}

static char *
ntg_set_user(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
#if (NTG_WIN32)

	ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
			"\"user\" is not supported, ignored");

	return NTG_CONF_OK;

#else

	ntg_core_conf_t *ccf = conf;

	char *group;
	struct passwd *pwd;
	struct group *grp;
	ntg_str_t *value;

	if (ccf->user != (uid_t) NTG_CONF_UNSET_UINT) {
		return "is duplicate";
	}

	if (geteuid() != 0) {
		ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
				"the \"user\" directive makes sense only "
					"if the master process runs "
					"with super-user privileges, ignored");
		return NTG_CONF_OK;
	}

	value = (ntg_str_t *) cf->args->elts;

	ccf->username = (char *) value[1].data;

	ntg_set_errno(0);
	pwd = getpwnam((const char *) value[1].data);
	if (pwd == NULL) {
		ntg_conf_log_error(NTG_LOG_EMERG, cf, ntg_errno,
				"getpwnam(\"%s\") failed", value[1].data);
		return NTG_CONF_ERROR;
	}

	ccf->user = pwd->pw_uid;

	group = (char *) ((cf->args->nelts == 2) ? value[1].data : value[2].data);

	ntg_set_errno(0);
	grp = getgrnam(group);
	if (grp == NULL) {
		ntg_conf_log_error(NTG_LOG_EMERG, cf, ntg_errno,
				"getgrnam(\"%s\") failed", group);
		return NTG_CONF_ERROR;
	}

	ccf->group = grp->gr_gid;

	return NTG_CONF_OK;

#endif
}

static char *
ntg_set_env(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_core_conf_t *ccf = conf;

	ntg_str_t *value, *var;
	ntg_uint_t i;

	var = ntg_array_push(&ccf->env);
	if (var == NULL) {
		return NTG_CONF_ERROR;
	}

	value = cf->args->elts;
	*var = value[1];

	for (i = 0; i < value[1].len; i++) {

		if (value[1].data[i] == '=') {

			var->len = i;

			return NTG_CONF_OK;
		}
	}

	return NTG_CONF_OK;
}

static char *
ntg_set_priority(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_core_conf_t *ccf = conf;

	ntg_str_t *value;
	ntg_uint_t n, minus;

	if (ccf->priority != 0) {
		return "is duplicate";
	}

	value = cf->args->elts;

	if (value[1].data[0] == '-') {
		n = 1;
		minus = 1;

	} else if (value[1].data[0] == '+') {
		n = 1;
		minus = 0;

	} else {
		n = 0;
		minus = 0;
	}

	ccf->priority = ntg_atoi(&value[1].data[n], value[1].len - n);
	if (ccf->priority == NTG_ERROR) {
		return "invalid number";
	}

	if (minus) {
		ccf->priority = -ccf->priority;
	}

	return NTG_CONF_OK;
}

/**
 * 设置系统的cpu亲和集
 * @param cf
 * @param cmd
 * @param conf
 * @return
 */
static char *
ntg_set_cpu_affinity(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	//#if (NTG_HAVE_CPU_AFFINITY)
	ntg_core_conf_t *ccf = conf;
	unsigned int *mask;
	ntg_str_t *value;
	ntg_int_t i, n;

	if (ccf->cpu_affinity) {
		return "is duplicate";
	}

	mask = ntg_palloc(cf->pool, (cf->args->nelts - 1) * sizeof(unsigned int));
	if (mask == NULL) {
		return NTG_CONF_ERROR;
	}

	ccf->cpu_affinity_n = cf->args->nelts - 1;
	ccf->cpu_affinity = mask;

	value = cf->args->elts;

	for (n = 1; n < cf->args->nelts; n++) {
		i = ntg_atoi(value[n].data, value[n].len);

		if (i == NTG_ERROR) {
			ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
					"invalid cpu affinity num \"%V\"", &value[n]);
			return NTG_CONF_ERROR;
		}
		if (i > 64) {
			ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
					"\"cpu_affinity\" supports up to 64 CPUs only");
			return NTG_CONF_ERROR;
		}

		mask[n - 1] = i;
	}

#if (NTG_CPU_AFFINITY)
	//将cpu亲和集数组转为cpu亲和集
	if (tmc_cpus_from_array(&ccf->cpus, ccf->cpu_affinity, ccf->cpu_affinity_n)) {
		return NTG_CONF_ERROR;
	}
	//设置cpu亲和集
	if(tmc_cpus_set_my_affinity(&ccf->cpus) < 0) {
		return NTG_CONF_ERROR;
	}
#endif

	//#else
	//
	//    ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
	//                       "\"worker_cpu_affinity\" is not supported "
	//                       "on this platform, ignored");
	//#endif

	return NTG_CONF_OK;
}

/**
 * 获取cpu亲和力
 * @param[in] n 亲和力数组索引
 * @return 返回cpu号
 * @note 主进程cpu号为0,其余为进程槽号+1
 */
ntg_uint_t ntg_get_cpu_affinity(ntg_uint_t n) {
	ntg_core_conf_t *ccf;

	ccf = (ntg_core_conf_t *) ntg_get_conf(ntg_cycle->conf_ctx,
			ntg_core_module);

	if (ccf->cpu_affinity == NULL) {
		return 0;
	}

	if (ccf->cpu_affinity_n > n) {
		return ccf->cpu_affinity[n];
	}

	return ccf->cpu_affinity[ccf->cpu_affinity_n - 1];
}

/**
 * 设置工作进程数
 * @param cf 配置对象
 * @param cmd 命令对象
 * @param conf 所在配置位置
 * @return 成功返回NTG_CONF_OK，否则返回
 */
static char *
ntg_set_worker_processes(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_str_t *value;
	ntg_core_conf_t *ccf;

	ccf = (ntg_core_conf_t *) conf;

	if (ccf->worker_processes != NTG_CONF_UNSET) {
		return "is duplicate";
	}

	value = (ntg_str_t *) cf->args->elts;

	//    if (ntg_strcmp(value[1].data, "auto") == 0) {
	//        ccf->worker_processes = ntg_ncpu;
	//        return NTG_CONF_OK;
	//    }

	ccf->worker_processes = ntg_atoi(value[1].data, value[1].len);

	if (ccf->worker_processes == NTG_ERROR) {
		return "invalid value";
	}

	return NTG_CONF_OK;
}

/**
 * 设置前端监听地址
 * @param cf 配置对象
 * @param cmd 命令对象
 * @param conf 所在配置位置
 * @return 成功返回NTG_CONF_OK，否则返回相应的字符串
 */
static char *
ntg_set_front_end(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_str_t *value;
	//    ntg_core_conf_t  *ccf;
	ntg_conf_post_t *post;
	ntg_int_t n;
	ntg_front_listen_t *front;
	struct sockaddr_in *sin;

	char *p = conf;

	front = (ntg_front_listen_t *) (p + cmd->offset);
	if (front->sin.sin_addr.s_addr) {
		return "is duplicate";
	}

	sin = &front->sin;

	value = cf->args->elts;

	sin->sin_family = AF_INET;

	value = (ntg_str_t *) cf->args->elts;

	/* ip 地址设置 */
	if (value[1].len) {
		sin->sin_addr.s_addr = ntg_inet_addr(value[1].data, value[1].len);
	}

	if (sin->sin_addr.s_addr == INADDR_NONE) {
		return "invalid value";
	}

	/* 端口设置 */
	n = ntg_atoi(value[2].data, value[2].len);
	if (n < 1025 || n > 65535) {
		return "invalid port";
	}
	sin->sin_port = htons((in_port_t) n);

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, front);
	}

	return NTG_CONF_OK;
}

/**
 * 设置前端监听地址
 * @param cf 配置对象
 * @param cmd 命令对象
 * @param conf 所在配置位置
 * @return 成功返回NTG_CONF_OK，否则返回相应的字符串
 */
static char *
ntg_set_database(ntg_conf_t *cf, ntg_command_t *cmd, void *conf) {
	ntg_str_t *value;
	//    ntg_core_conf_t  *ccf;
	ntg_conf_post_t *post;
	ntg_database_conf_t *database;

	char *p = conf;

	database = (ntg_database_conf_t *) (p + cmd->offset);
	if (database->type.data) {
		return "is duplicate";
	}

	value = cf->args->elts;

	database->type = value[1];
	database->host = value[2];
	database->port = ntg_atoi(value[3].data, value[3].len);
	database->user = value[4];
	database->password = value[5];
	database->name = value[6];

	if (cmd->post) {
		post = cmd->post;
		return post->post_handler(cf, post, database);
	}

	return NTG_CONF_OK;
}
