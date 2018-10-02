/*
 * ntg_channel.h
 *
 *  Created on: Oct 15, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_CHANNEL_H_
#define UTILS_NTG_CHANNEL_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "../message/ntg_message.h"
#include "ntg_process.h"
#include "ntg_files.h"
#include "ntg_socket.h"
#include "ntg_log.h"


/**
 * 进程间通信通道
 */
//typedef struct{
//	ntg_uint_t 	command;///< 命令
//	ntg_pid_t	pid;///< 发送方进程id
//	ntg_int_t	slot;///< 发送方进程在process进程数组中的序号
//	ntg_fd_t	fd;///< 通信的套接字句柄, 为master进程的监听套接字
//} ntg_channel_t;


ntg_int_t ntg_write_channel(ntg_socket_t s, ntg_message_t *msg, size_t size,
		ntg_log_t *log);
ntg_int_t ntg_read_channel(ntg_socket_t s, ntg_message_t *msg, size_t size,
		ntg_log_t *log);

ntg_int_t ntg_add_channel_event(ntg_cycle_t *cycle, evutil_socket_t  fd,
		short  event, event_callback_fn handler);

void ntg_close_channel(ntg_fd_t *fd, ntg_log_t *log);


#endif /* UTILS_NTG_CHANNEL_H_ */
