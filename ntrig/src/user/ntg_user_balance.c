/**
 * @file ntg_user_balance.c
 * @brief
 * @details
 * @author tzh
 * @date Mar 13, 2016
 * @version V0.1
 * @copyright tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_user.h"
#include "ntg_user_balance.h"

static ntg_int_t ntg_user_balance_dynamic(ntg_user_manage_t *mg, ntg_message_t *msg);
static ntg_int_t ntg_user_balance_process_request(ntg_user_balance_t *blc,
		ntg_message_t *msg, ntg_log_t *log);
static ntg_int_t ntg_user_balance_process_feedback(ntg_user_balance_t *blc,
		ntg_message_t *msg, ntg_log_t *log);
static void ntg_user_balance_update(ntg_user_balance_t *blc);

static ntg_int_t ntg_user_balance_polling(ntg_user_manage_t *mg, ntg_message_t *msg);

/**
 * 初始化负载均衡模块
 * @param[in] cycle 循环对象
 * @param[in] blc 负载均衡对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 在user manage初始化中被调用
 * @see ntg_user_manage_init
 */
ntg_int_t ntg_user_balance_init(ntg_user_manage_t *mg){
	ntg_user_balance_t *blc;

	blc = ntg_pcalloc(mg->pool, sizeof(*blc));//pcalloc内部已做了错误日志记录
	if(blc == NULL)
		return NTG_ERROR;

	blc->cur_slot = ntg_start_process;
	blc->min_reqs = blc->count = 0;
	blc->n = ntg_last_process-2;//TODO 需要处理进程添加或异常退出的情况
	blc->pros = ntg_processes;
	blc->mg = mg;
	if(mg->cycle->blc_type){
		blc->blance = ntg_user_balance_dynamic;
	}else{
		blc->blance = ntg_user_balance_polling;
	}


	mg->blc = blc;

	return NTG_OK;
}

/**
 * 动态反馈的负载均衡处理
 * @param[in] mg 用户管理对象
 * @param[in] msg 消息对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 已进程错误日志处理
 * @see ntg_user_balance_process_request
 * @see ntg_user_balance_process_feedback
 */
static ntg_int_t ntg_user_balance_dynamic(ntg_user_manage_t *mg, ntg_message_t *msg) {
	ntg_int_t ret;
	ntg_user_balance_t *blc;

	blc = mg->blc;

	switch (msg->type) {
	case NTG_REQUEST_TYPE:
		ret = ntg_user_balance_process_request(blc,msg, mg->log);
		break;

	case NTG_FEEDBACK_TYPE:
		ret = ntg_user_balance_process_feedback(blc,msg, mg->log);
		break;

	default:
		ret = NTG_ERROR;
		ntg_log_error(NTG_LOG_CRIT, mg->log, 0, "message type(%d) is inconsistent in balance", msg->type);
		break;
	}

	return ret;
}


/**
 * 请求消息分发处理
 * @param[in] blc 负载均衡对象
 * @param[in] msg 消息对象
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 当向当前进程发送请求消息时，如果失败，将调度到其他进程
 * @note 当没有存活的流量产生进程，将放回NTG_ERROR
 */
static ntg_int_t ntg_user_balance_process_request(ntg_user_balance_t *blc,
		ntg_message_t *msg, ntg_log_t *log){

	ntg_process_t *pro;

	/* 获取负载接收进程 */
	pro = &blc->pros[blc->cur_slot];//保证cur_slot是有效的即使活跃的进程为0

    //发送请求消息
    while( ntg_write_channel(pro->channel[0],msg,
    		sizeof(msg->type) + sizeof(msg->body.rqt), log) != NTG_OK ){
    	//管道已关闭,感知到该流量产生进程正在退出
    	pro->exiting = 1;
    	pro->reqs = 0;

    	ntg_log_error(NTG_LOG_CRIT, log, 0,
                   "failed to send a message to the (%P) process.", pro->pid);

    	ntg_user_balance_update(blc);

    	if(blc->n !=0){
    		pro = &blc->pros[blc->cur_slot];
    	} else {
        	ntg_log_error(NTG_LOG_CRIT, log, 0, "there is no active process");
        	return NTG_ERROR;
    	}
    }

    pro->reqs++;
    blc->count++;
//    printf("==========send request message to traffic process====%d===\n", pro->pid);
//    printf("===count==%d== =====n==%d=\n", blc->count, blc->n>>2);

//    if( blc->count >= (blc->n>>2) ) {
//    	printf("---------------123----------------\n");
    	ntg_user_balance_update(blc);
//    }

	return NTG_OK;
}

/**
 * 处理反馈消息处理
 * @param[in] blc 负载对象
 * @param[in] msg 消息对象
 * @param[in] log 日志对象
 * @return NTG_OK
 */
static ntg_int_t ntg_user_balance_process_feedback(ntg_user_balance_t *blc,
		ntg_message_t *msg, ntg_log_t *log) {

	//printf("--------------ntg_user_balance_process_feedback--------23---------\n");
	blc->pros[msg->body.fb.proc_slot].reqs = msg->body.fb.reqs;

	if(blc->min_reqs > msg->body.fb.reqs ){
		ntg_user_balance_update(blc);
	}

	blc->count = 0;
	return NTG_OK;
}


/**
 * 更新负载均衡数据
 * @param[in] blc 负载均衡对象
 * @note 当活跃的流量产生进程为0时，默认选择的blc->cur_slot为ntg_start_process
 * 这样可以避免无效指针问题（blc->pros[blc->cur_slot]）
 */
static void ntg_user_balance_update(ntg_user_balance_t *blc){
	ntg_int_t i;
	ntg_uint_t tmp_slop;
	ntg_uint_t live;

	tmp_slop = ntg_start_process;
	live = 0;
	for(i = ntg_start_process  ; i < ntg_last_process; i++) {
		//跳过无效子进程
		if (blc->pros[i].detached || blc->pros[i].pid == -1) {
			continue;
		}
		if (blc->pros[i].just_spawn) {
			blc->pros[i].just_spawn = 0;
			continue;
		}
		if (blc->pros[i].exited || blc->pros[i].exiting) {
			continue;
		}

//		printf("===process= %d --->reqs======%d\n",i,  blc->pros[i].reqs);
		live++;
		if(live == 1) {
			tmp_slop = i;
		}else if( blc->pros[i].reqs < blc->pros[tmp_slop].reqs ){
			tmp_slop = i;
		}
	}
//	printf("tmp_slop======%d\n", tmp_slop);
	blc->n = live;
	blc->cur_slot = tmp_slop;
	blc->min_reqs = blc->pros[tmp_slop].reqs;
	blc->count = 0;
}

/**
 * 轮询机制的负载均衡处理
 * @param mg
 * @param msg
 * @return
 */
static ntg_int_t ntg_user_balance_polling(ntg_user_manage_t *mg, ntg_message_t *msg){
	ntg_int_t i, pre_last;

	/* 采用轮寻机制 */
	pre_last = ntg_current_process;

	static int count=0;
	count++;
	if(count<10){
		return NTG_OK;
	}
	/* 查找下一个通知进程 */
	/* 轮寻后面部分 */
	for(i = ntg_current_process ; i < ntg_last_process; i++){
		//2.1 跳过无效子进程
		if (ntg_processes[i].detached || ntg_processes[i].pid == -1) {
			continue;
		}

		if (ntg_processes[i].just_spawn) {
			ntg_processes[i].just_spawn = 0;
			continue;
		}

		if (ntg_processes[i].exited || ntg_processes[i].exiting ) {
			continue;
		}

		if (ntg_processes[i].type != NTG_TRAFFIC_PROCESS){
			continue;
		}

		break;
	}

	/* 轮寻前面部分 */
	if(i == ntg_last_process ){

		for(i = ntg_start_process; i < pre_last; i++){
			if (ntg_processes[i].detached || ntg_processes[i].pid == -1) {
				continue;
			}

			if (ntg_processes[i].just_spawn) {
				ntg_processes[i].just_spawn = 0;
				continue;
			}

			if (ntg_processes[i].exited || ntg_processes[i].exiting) {
				continue;
			}

			if (ntg_processes[i].type != NTG_TRAFFIC_PROCESS){
				continue;
			}

			break;
		}
	}

	printf(" current= %d; last=%d ; i=%d\n", ntg_current_process, ntg_last_process, i);
	ntg_current_process = ((i+1 == ntg_last_process) ? ntg_start_process : i+1);

//		printf("i=%, ntg_current_process= %d, ntg_last_process=%d\n", i, ntg_current_process,
//				ntg_last_process);
    //2.2 以通道方式发送命令

    if (ntg_write_channel(ntg_processes[i].channel[0],msg,
    		sizeof(msg->type) + sizeof(msg->body.rqt), mg->log) != NTG_OK)
    {
    	//管道已关闭
    	ntg_processes[i].exiting = 1;
    	ntg_log_error(NTG_LOG_CRIT, mg->log, 0,
                   "failed to send a message to the (%P) process.", ntg_processes[i].pid);
    	return NTG_ERROR;
    }

	return NTG_OK;
}
