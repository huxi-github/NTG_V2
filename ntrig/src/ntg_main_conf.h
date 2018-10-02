/*
 * ntg_main_con.h
 *
 *  Created on: Aug 28, 2015
 *      Author: tzh
 */

#ifndef NTG_MAIN_CONF_H_
#define NTG_MAIN_CONF_H_

#include "ntg_config.h"
#include "ntg_types.h"
#include "utils/ntg_time.h"

typedef struct {
	ntg_flag_t daemon;
	ntg_flag_t master;

	ntg_msec_t timer_resolution;

	ntg_int_t worker_processes;
	ntg_int_t debug_points;

	ntg_int_t rlimit_nofile; //最大文件描述符数
	ntg_int_t rlimit_sigpending;
	off_t rlimit_core; //core文件大小

	int priority;

	ntg_uint_t cpu_affinity_n; //cpu数
	uint64_t *cpu_affinity; //

	char *username;
	uid_t user;//用户id
	gid_t group;

	ntg_str_t working_directory;
	ntg_str_t lock_file;

	ntg_str_t pid;
	ntg_str_t oldpid;

	ntg_array_t env;
	char **environment;
} ntg_main_conf_t;

#endif /* UTILS_NTG_MAIN_CON_H_ */
