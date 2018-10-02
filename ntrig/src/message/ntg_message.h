/**
 * @file ntg_message.h
 * @brief
 * @details
 * @author tzh
 * @date Jan 27, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_MESSAGE_H_
#define NTG_MESSAGE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

/**
 * @name 通道消息
 */
typedef struct{
	ntg_uint_t 	command;///< 命令
	ntg_pid_t	pid;///< 进程id
	ntg_int_t	slot;///< 进程槽位
	ntg_fd_t	fd;///< 通信的套接字句柄, 为master进程的监听套接字
} ntg_channel_message_t;

/**
 * @name 模拟消息类型对象
 */
typedef struct {
	ntg_uint_t csm_id;///消费者id
	float scene[24];///场景参数
	unsigned ss_num_type:8;///会话数分布函数类型
	unsigned ss_itv_type:8;///会话间隔分布函数类型
	unsigned clk_num_type:8;///点击数分布函数类型
	unsigned clk_itv_type:8;///点击间隔分布函数类型
	float ss_num[3];///会话数
	float ss_itv[3];///会话间隔
	float clk_num[3];///点击数
	float clk_itv[3];///点击间隔
	unsigned gp_id :4;///组id
	unsigned is_scene :1;///是否使用场景,1表示使用
	unsigned st_point :11;///模拟开始点
	ntg_uint_t user_n;///模拟用户数
} ntg_simulation_message_t;

/**
 * @name 控制消息
 */
typedef struct {
	ntg_int_t csm_id;///消费者id
	ntg_int_t gp_id;///组id
	ntg_int_t	type;///< 操作类型
} ntg_control_message_t;

/**
 * @name 结果消息
 */
typedef struct {
	ntg_int_t csm_id;///消费者id
	ntg_int_t status;///< 状态码
}ntg_result_message_t;

/**
 * @name 请求消息
 */
typedef struct {
	ntg_int_t csm_id;///消费者id
	ntg_int_t gp_id;///组id
	ntg_int_t user_id;///< 用户id
}ntg_request_message_t;

/**
 * @name 反馈消息
 */
typedef struct {
	ntg_uint_t proc_slot;///< 进程槽位
	ntg_uint_t reqs;///< 请求数目
} ntg_feedback_message_t;


/**
 * @name 记录消息
 */
typedef struct {
	unsigned csm_id :16;///消费者id
	unsigned gp_id :4;///组id
	unsigned state_code :12;///组id
	ntg_uint_t user_id;///< 用户id
	ntg_uint_t url_id;///< url id
	ntg_uint_t send;///发送大小
	ntg_uint_t receive;///接收大小
	time_t time;///时间戳
} ntg_record_message_t;


#define NTG_MESSAGE_TYPES  8
/**
 * @name 消息类型
 */
typedef enum {
	NTG_NULL_TPYE,
	NTG_CHANNEL_TYPE,   //!< 通道消息类型
	NTG_SIMULATION_TYPE,//!< 模拟消息类型
	NTG_CONTROL_TYPE,   //!< 控制消息类型
	NTG_RESULT_TYPE,    //!< 结果消息类型
	NTG_REQUEST_TYPE,   //!< 请求消息类型
	NTG_FEEDBACK_TYPE,  //!< 反馈消息类型
	NTG_RECORD_TPYE     //!< 记录消息类型
} ntg_message_type_e;
/**
 * @name 控制消息类型
 */
typedef enum {
	NTG_CTL_DELETE,		//!< 删除用户组
	NTG_CTL_QUIT		//!< 实验用户退出
} ntg_control_type_e;

/**
 * @name 结果消息类型
 */
typedef enum {
	NTG_RST_SUCCESS,	//!< 虚拟用户层处理成功
	NTG_RST_FAILED		//!< 虚拟用户层处理失败
} ntg_result_type_e;

/**
 * @name 消息公共部分
 */
typedef struct {
	ntg_message_type_e type;
	union {
		ntg_channel_message_t ch;
		ntg_simulation_message_t sm;
		ntg_control_message_t ctl;
		ntg_result_message_t rst;
		ntg_request_message_t rqt;
		ntg_feedback_message_t fb;
		ntg_record_message_t rcd;
	} body;
}ntg_message_t;

extern int msg_size[];

#endif /* NTG_MESSAGE_H_ */
