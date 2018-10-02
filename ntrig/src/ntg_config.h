/*
 * ntg_config.h
 *
 *  Created on: Aug 25, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_CONFIG_H_
#define UTILS_NTG_CONFIG_H_

/********公共头部***************/
#include "ntg_types.h"
//#include "ntg_auto_config.h"
#include "ntg_auto_header.h"
#include "utils/ntg_linux_config.h"


/********第三方库头文件****************/
//#include <ghttp.h>
//#include <zdb.h>
//#include <mysql.h>

#include <event2/event.h>
#include <event2/dns.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>

#include "../lib/cJSON.h"
#include "../lib/cJSON_Utils.h"

#include <mysql/mysql.h>

/**************tilera**********************/
#if (NTG_TILERA_PLATFORM)
#include <tmc/alloc.h>

#include <tmc/cpus.h>
#include <tmc/sync.h>
#include <tmc/task.h>
#include <tmc/queue.h>

#endif


#define ntg_signal_helper(n)     (SIG##n)
#define ntg_signal_value(n)      n
//#define ntg_signal_value(n)      (SIG##n)

#define ntg_random               random

#define NTG_SHUTDOWN_SIGNAL		SIGQUIT
#define NTG_TERMINATE_SIGNAL	SIGTERM
#define NTG_RECONFIGURE_SIGNAL	SIGHUP

#define NTG_ADD_SIGNAL		SIGUSR1///添加流量产生进程

#define NTG_CHANGEBIN_SIGNAL	SIGUSR2

#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"

#define ntg_abs(value)	((value>=0) ? (value) : -(value))
#define	ntg_min(a,b)	((a) < (b) ? (a) : (b))
#define	ntg_max(a,b)	((a) > (b) ? (a) : (b))

/**映射基本的数据类型*/
typedef intptr_t        ntg_int_t;
typedef uintptr_t       ntg_uint_t;
typedef intptr_t        ntg_flag_t;

#define NTG_INT32_LEN   (sizeof("-2147483648") - 1)
#define NTG_INT64_LEN   (sizeof("-9223372036854775808") - 1)

#if (NTG_PTR_SIZE == 4)
#define NTG_INT_T_LEN   NTG_INT32_LEN
#define NTG_MAX_INT_T_VALUE  2147483647

#else
#define NTG_INT_T_LEN   NTG_INT64_LEN
#define NTG_MAX_INT_T_VALUE  9223372036854775807
#endif


#ifndef NTG_ALIGNMENT
#define NTG_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define ntg_align(d, a)     (((d) + (a - 1)) & ~(a - 1))  ///基于a的大小对齐
#define ntg_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define ntg_abort       abort


/* TODO: platform specific: array[NTG_INVALID_ARRAY_INDEX] must cause SIGSEGV */
#define NTG_INVALID_ARRAY_INDEX 0x80000000


/* TODO: auto_conf: ntg_inline   inline __inline __inline__ */
#ifndef ntg_inline
#define ntg_inline      inline
#endif

#ifndef INADDR_NONE  /* Solaris */
#define INADDR_NONE  ((unsigned int) -1)
#endif

#ifdef MAXHOSTNAMELEN
#define NTG_MAXHOSTNAMELEN  MAXHOSTNAMELEN
#else
#define NTG_MAXHOSTNAMELEN  256
#endif


//有32位符号和无符号最大值
#define NTG_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#define NTG_MAX_INT32_VALUE   (uint32_t) 0x7fffffff

/***/
#define NTG_DEBUG 1

#endif /* UTILS_NTG_CONFIG_H_ */
