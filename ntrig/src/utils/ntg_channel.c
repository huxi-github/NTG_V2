/*
 * ntg_channel.c
 *
 *  Created on: Oct 15, 2015
 *      Author: tzh
 */


#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_channel.h"
#include "ntg_process_cycle.h"
#include "ntg_process.h"

/**
 * 写通道
 * @param[in] s 套接字
 * @param[in] body 待发消息体指针
 * @param[in] size 大小
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 信号中断返回NTG_AGAIN, 失败返回NTG_ERROR
 */
ntg_int_t
ntg_write_channel(ntg_socket_t s, ntg_message_t *msg, size_t size,
    ntg_log_t *log)
{
    ssize_t             n;
    ntg_err_t           err;
    struct iovec        iov[1];
    struct msghdr       msgh;

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    if (msg->type == NTG_CHANNEL_TYPE) {//通道消息
		ntg_channel_message_t *ch = (ntg_channel_message_t*) &msg->body.ch;

		if (ch->fd == -1) {
			msgh.msg_control = NULL;
			msgh.msg_controllen = 0;

		} else {//传递文件描述符
			msgh.msg_control = (caddr_t) &cmsg;
			msgh.msg_controllen = sizeof(cmsg);

			ntg_memzero(&cmsg, sizeof(cmsg));

			cmsg.cm.cmsg_len = CMSG_LEN(sizeof(int));
			cmsg.cm.cmsg_level = SOL_SOCKET;
			cmsg.cm.cmsg_type = SCM_RIGHTS;

			/*
			 * We have to use ntg_memcpy() instead of simple
			 *   *(int *) CMSG_DATA(&cmsg.cm) = ch->fd;
			 * because some gcc 4.4 with -O2/3/s optimization issues the warning:
			 *   dereferencing type-punned pointer will break strict-aliasing rules
			 *
			 * Fortunately, gcc with -O1 compiles this ntg_memcpy()
			 * in the same simple assignment as in the code above
			 */

			ntg_memcpy(CMSG_DATA(&cmsg.cm), &ch->fd, sizeof(int));
		}
	} else {//其他消息情况
		msgh.msg_control = NULL;
		msgh.msg_controllen = 0;
	}

    msgh.msg_flags = 0;

    printf("------------size----%d-------\n", size);
    //设置iovec
    iov[0].iov_base = (char *) msg;
    iov[0].iov_len = size;

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;

    n = sendmsg(s, &msgh, 0);//发送信息

    if (n == -1) {//错误处理
    	printf("()(()错误处理\n");
        err = ntg_errno;
        if (err == NTG_EAGAIN) {
            return NTG_AGAIN;
        }

        ntg_log_error(NTG_LOG_ALERT, log, err, "sendmsg() failed");
        return NTG_ERROR;
    }

    return NTG_OK;
}


/**
 * 从频道中读取数据
 * @param[in] s 套接字
 * @param[out] ch 通道对象
 * @param[out] size ch的大小
 * @param[in] log 日志对象
 * @return 成功返回读取到的数据大小,信号中断返回NTG_AGAIN, 失败返回NTG_ERROR
 * @note 出现中断情况交由调用着自行处理
 */
ntg_int_t
ntg_read_channel(ntg_socket_t s, ntg_message_t *msg, size_t size, ntg_log_t *log)
{
    ssize_t             n;
    ntg_err_t           err;
    struct iovec        iov[1];
    struct msghdr       msgh;

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    iov[0].iov_base = (char *) msg;
    iov[0].iov_len = size;

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;

    /* 控制消息 */
    msgh.msg_control = (caddr_t) &cmsg;
    msgh.msg_controllen = sizeof(cmsg);

    //printf("--------------------------------------\n");
    n = recvmsg(s, &msgh, 0);


    if (n == -1) {
        err = ntg_errno;
        if (err == NTG_EAGAIN) {
            return NTG_AGAIN;
        }
        printf("-------------------1-------------------\n");
        ntg_log_error(NTG_LOG_ALERT, log, err, "recvmsg() failed");
        return NTG_ERROR;
    }

    if (n == 0) {
        printf("---------------------2-----------------\n");
        ntg_log_debug0(NTG_LOG_DEBUG_CORE, log, 0, "recvmsg() returned zero");
        return NTG_ERROR;
    }

//    if ((size_t) n < (sizeof(msg->type) + msg_size[msg->type]) ) {
//        printf("--------------------3------------------\n");
//        ntg_log_error(NTG_LOG_ALERT, log, 0, "recvmsg() returned not enough data: %z", n);
//        return NTG_ERROR;
//    }

    if (msg->type == NTG_CHANNEL_TYPE) {
		ntg_channel_message_t *ch = (ntg_channel_message_t*) &msg->body.ch;

		//处理OPEN_CHANNEL命令
		if (ch->command == NTG_CMD_OPEN_CHANNEL) {
			//数据有效性检查
			if (cmsg.cm.cmsg_len < (socklen_t) CMSG_LEN(sizeof(int))) {
				ntg_log_error(NTG_LOG_ALERT, log, 0,
						"recvmsg() returned too small ancillary data");
				printf("-------------------5-------------------\n");
				return NTG_ERROR;
			}

			if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type
					!= SCM_RIGHTS) {
				printf("-------------------6-------------------\n");
				ntg_log_error(NTG_LOG_ALERT, log, 0,
						"recvmsg() returned invalid ancillary data "
						"level %d or type %d",
						cmsg.cm.cmsg_level, cmsg.cm.cmsg_type);
				return NTG_ERROR;
			}

			/* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */

			ntg_memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
		}
	}


    if (msgh.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) {//消息被截断标志
        printf("-------------------7-------------------\n");
        ntg_log_error(NTG_LOG_ALERT, log, 0, "recvmsg() truncated data");
    }

    return n;
}

/**
 * 将channel添加到event中
 * @param[in] cycle 循环对象
 * @param[in] fd 套接字
 * @param[in] event 事件类型
 * @param[in] handler 回调处理函数
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note Unix套接字的监听为PERSIST类型
 */
ntg_int_t
ntg_add_channel_event(ntg_cycle_t *cycle, evutil_socket_t  fd,
		short  event, event_callback_fn handler)
{
	struct event_base *base;
	struct event	  **ev_ptr;
	ntg_process_t     *pro;

	pro = &ntg_processes[ntg_process_slot];
	ev_ptr = &pro->event;
	base = cycle->base;

	*ev_ptr = event_new(base, fd, event | EV_PERSIST, handler, cycle);
	if(*ev_ptr == NULL){
		return NTG_ERROR;
	}

	pro->log = cycle->log;

	event_add(*ev_ptr, NULL);

	printf("ntg_add_channel_event---->ok\n");
    return NTG_OK;
}

/**
 * 关闭通道
 * @param[in] fd fd[2] Unix套接子
 * @param[in] log 日志对象
 */
void
ntg_close_channel(ntg_fd_t *fd, ntg_log_t *log)
{
    if (close(fd[0]) == -1) {
        ntg_log_error(NTG_LOG_ALERT, log, ntg_errno, "close() channel failed");
    }

    if (close(fd[1]) == -1) {
        ntg_log_error(NTG_LOG_ALERT, log, ntg_errno, "close() channel failed");
    }
}
