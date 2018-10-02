/*
 * ntg_core.h
 *
 *  Created on: Aug 25, 2015
 *      Author: tzh
 */

#ifndef NTG_CORE_H_
#define NTG_CORE_H_

#include "ntg_config.h"

typedef uid_t ntg_uid_t;
typedef gid_t ntg_gid_t;

typedef struct ntg_module_s      ntg_module_t;
typedef struct ntg_conf_s        ntg_conf_t;
typedef struct ntg_cycle_s       ntg_cycle_t;
typedef struct ntg_pool_s        ntg_pool_t;
typedef struct ntg_chain_s       ntg_chain_t;
typedef struct ntg_log_s         ntg_log_t;
typedef struct ntg_open_file_s   ntg_open_file_t;
typedef struct ntg_command_s     ntg_command_t;
typedef struct ntg_file_s        ntg_file_t;//用于解决file与files的相互包含
typedef struct ntg_event_s       ntg_event_t;
//typedef struct ntg_event_aio_s   ntg_event_aio_t;

typedef struct ntg_remote_s ntg_remote_t;
typedef struct ntg_db_mysql_s ntg_db_mysql_t;

typedef struct ntg_connection_s  ntg_connection_t;

typedef struct ntg_user_s ntg_user_t;
typedef struct ntg_user_manage_s ntg_user_manage_t;
typedef struct ntg_user_consumer_s ntg_user_consumer_t;
typedef struct ntg_user_group_s ntg_user_group_t;
typedef struct ntg_user_balance_s ntg_user_balance_t;

typedef struct ntg_http_request_s ntg_http_request_t;

typedef struct ntg_record_s       ntg_record_t;
typedef struct ntg_record_manage_s ntg_record_manage_t;

typedef struct ntg_buf_s  ntg_buf_t;//buf与connection相互包含
//typedef pid_t	ntg_pid_t;//cycle与process相互包含

typedef void (*ntg_event_handler_pt)(ntg_event_t *ev);///< 事件处理函数指针定义
typedef void (*ntg_connection_handler_pt)(ntg_connection_t *c);///< 链接处理函数指针定义


//#include "ntg_config.h"
//#include "ntg_types.h"
//#include "utils/ntg_queue.h"
//#include "utils/ntg_errno.h"
#include "utils/ntg_time.h"
#include "utils/ntg_string.h"
#include "utils/ntg_files.h"

#include "utils/ntg_log.h"

#include "utils/ntg_palloc.h"
#include "utils/ntg_array.h"
//#include "utils/ntg_file.h"
//

#include "utils/ntg_conf_file.h"


//#include "utils/ntg_cycle.h"

//#include "dbi/ntgs_mysql.h"

void ntg_cpuinfo(void);

#if (NTG_HAVE_OPENAT)
#define NTG_DISABLE_SYMLINKS_OFF        0
#define NTG_DISABLE_SYMLINKS_ON         1
#define NTG_DISABLE_SYMLINKS_NOTOWNER   2
#endif

#endif /* NTG_CORE_H_ */
