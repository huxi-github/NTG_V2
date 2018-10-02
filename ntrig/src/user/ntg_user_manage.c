/**
 * @file ntg_user_manage.c
 * @brief
 * @details
 * @author tzh
 * @date Jan 24, 2016
 * @version V0.1
 * @copyright tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_user.h"
#include "ntg_user_manage.h"
#include "ntg_user_consumer.h"

#include "../message/ntg_message.h"

static ntg_user_consumer_t *ntg_user_get_consumer(ntg_user_manage_t *mg);
static void ntg_user_manage_process_task(evutil_socket_t fd, short what, void *arg);
static ntg_int_t ntg_user_control_process(ntg_user_manage_t *mg, ntg_control_message_t *sm);
static ntg_int_t ntg_user_simulation_process(ntg_user_manage_t *mg, ntg_simulation_message_t *sm);
/**
 * 初始化用户管理对象
 * @param[in] mg 用户管理对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 在master进程初始化中被调用
 */
ntg_int_t ntg_user_manage_init(ntg_user_manage_t *mg){
	ntg_pool_t *pool;
	ntg_int_t i;
	ntg_cycle_t *cycle;
	ntg_core_conf_t *ccf;
	ntg_user_t *users;
	ntg_user_t *u, *u_next;
	ntg_user_consumer_t *csms;
	ntg_user_consumer_t *c, *c_next;

	ntg_hash_table_t *table;

	cycle = mg->cycle;
	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	pool = mg->pool;

	/* 将用户链表化为用户池 */
	users = ntg_palloc(pool, sizeof(ntg_user_t) * mg->user_n);
	if (users == NULL) {

		return NTG_ERROR;
	}
	mg->users = users;

	u = mg->users;
	i = mg->user_n;
	u_next = NULL;

	do {
		i--;
		u[i].data = u_next;
		u[i].user_id = i;
		u[i].live = 0;
		u_next = &u[i];
	} while (i);

	mg->free_users = u_next;
	mg->free_user_n = mg->user_n;

	/* 消费者对象池初始化 */
	if(ccf->consumes != NTG_CONF_UNSET){
		mg->csm_n = ccf->consumes;
	}else{
		mg->csm_n = NTG_USER_DEFAULT_CONSUMER_NUM;
	}

	csms = ntg_palloc(pool, sizeof(ntg_user_consumer_t) * mg->csm_n);
	if (csms == NULL) {

		return NTG_ERROR;
	}
	mg->consumers = csms;

	c = csms;
	i = mg->csm_n;
	c_next = NULL;

	do {
		i--;
		c[i].data = c_next;
		c_next = &c[i];
	} while (i);

	mg->free_csms = c_next;
	mg->free_csm_n = mg->csm_n;


	/* 消费者hash表初始化*/
	table = ntg_palloc(pool, sizeof(ntg_hash_table_t));
	if(table == NULL){
		return NTG_ERROR;
	}

	mg->csm_table = table;
	//size、key、cmp、node_n、pool

	table->size = mg->csm_n/1.5;//TODO 在配置文件中设置， 最好设置为2^x
	table->key = ntg_user_consumer_get_key;
	table->cmp = ntg_user_consumer_compare;
	table->node_n = mg->csm_n;
	table->pool = pool;
	ntg_hash_table_init(table);

	/* 任务链表 */
	mg->factor = NTG_USER_DEFAULT_INCREASE_FACTOR;
	mg->tasks = NULL;
	mg->task_n = 0;
	mg->task_pt = ntg_user_manage_process_task;
	mg->task_flag = 0;


	/* 负载均衡 */
	if (ntg_user_balance_init(mg) != NTG_OK){

		return NTG_ERROR;
	}

	return NTG_OK;
}
/**
 * 处理控制发送的仿真消息和控制消息，
 * 并向控制层发送结果消息
 * @param mg 用户管理对象
 * @param msg 消息对象
 * @return
 */
ntg_int_t ntg_user_message_handler(ntg_user_manage_t *mg, ntg_message_t *msg){
	ntg_simulation_message_t *sm;
	ntg_control_message_t *ctl;
	ntg_message_t tmp_msg;
	ntg_result_message_t *rst;
	ntg_int_t ret;

	tmp_msg.type = NTG_RESULT_TYPE;
	rst = &tmp_msg.body.rst;
	printf("........ntg_user_message_handler...0.......(%x).....\n", (unsigned int)mg);
	/* 消息处理 */
	switch(msg->type){
	case NTG_SIMULATION_TYPE:
		printf("........ntg_user_message_handler........1....%d...\n");
		sm = &msg->body.sm;
		rst->csm_id = sm->csm_id;
		ret = ntg_user_simulation_process(mg, sm);
		break;
	case NTG_CONTROL_TYPE:
		ctl = &msg->body.ctl;
		rst->csm_id = ctl->csm_id;
		ret = ntg_user_control_process(mg,ctl);
		printf("........ntg_user_message_handler......2.....%d....\n", ctl->csm_id);
		break;
	default:
		return NTG_ERROR;
		break;
	}

	if(ret == NTG_OK){
		rst->status = NTG_RST_SUCCESS;
	}else{
		printf("ntg_user_message_handler===message type=(%d)=is faild\n", msg->type);
		rst->status = NTG_RST_FAILED;
	}

	/* 向控制层发送结果消息 */
	do {
		printf("..%d...%d...%d...%d....\n",sizeof(tmp_msg), sizeof(tmp_msg.type), sizeof(tmp_msg.body.rst), sizeof(tmp_msg.body));
//		char *data = "hello...12345678";
//		ret = ntg_write_channel(ntg_processes[ntg_process_slot].channel[1],
//				(ntg_message_t*) data,strlen(data), mg->log);
    	ret = ntg_write_channel(ntg_processes[ntg_process_slot].channel[1],(ntg_message_t*)&tmp_msg,
    		sizeof(tmp_msg.type) + sizeof(tmp_msg.body.rst), mg->log);

    }while(ret == NTG_AGAIN);

    if(ret != NTG_OK){
    	printf("-------ntg_user_message_handler-----failed-------\n");
    }
    printf("........ntg_user_message_handler.........4......\n");
    fflush(NULL);
	return ret;
}
/**
 * 仿真消息处理
 * @param[in] mg 用户管理对象
 * @param[in] sm 仿真消息对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 前台处理函数对接收的json消息处理后，转化为一定的消息对象，然后调用相应的消息处理函数
 * @note 前台需要保证仿真消息中的gp_id组没有活跃
 * @note 组对象以及组内用户对象都采用malloc的固定分配方法
 */
static ntg_int_t ntg_user_simulation_process(ntg_user_manage_t *mg, ntg_simulation_message_t *sm) {

	ntg_user_consumer_t *csm;
	ntg_user_consumer_t tmp_csm;
	ntg_hash_node_t *node;

	ntg_user_group_t *gp;

	/* 获取一个可用的消费者对象 */
	tmp_csm.id = sm->csm_id;
	printf("-------ntg_user_simulation_process----------%x-------\n", mg);
	node = ntg_hash_find_node(mg->csm_table, &tmp_csm);	// 查找csm_table判断消费者是否存在
	printf("-------ntg_user_simulation_process-------1------\n");
	if(node == NULL){
		printf("-------ntg_user_simulation_process-------2-----\n");
		csm = ntg_user_get_consumer(mg);
		if(csm == NULL){
			//TODO 记录日志
			return NTG_ERROR;
		}

		/* 初始化消费者 */
		csm->data = mg;
		csm->id = sm->csm_id;//必须先初始化csm中用于计算key值的id

		/* 加入消费者hash表 */
		if( ntg_hash_insert_node(mg->csm_table, csm) != NTG_OK){
			//TODO 严重错误，记录日志
			return NTG_ERROR;
		}
	} else {
		csm = (ntg_user_consumer_t *)node->data;
	}
	printf("-------ntg_user_simulation_process-------3-----");
	/* 创建组对象 */
	gp = csm->gps[sm->gp_id];//前端必须保证gp_id的可用性
	if(gp == NULL){
		if( (gp = (ntg_user_group_t *)ntg_calloc(sizeof(ntg_user_group_t), mg->log)) == NULL){
			printf("-------------ntg_user_message_process----------------3--0----\n");
			return NTG_ERROR;
		}
		gp->gp_id = sm->gp_id;
		gp->consumer = csm;
		gp->manage = mg;
		csm->gps[sm->gp_id] = gp;
	}else{
		//先释放原有的信息,用户组重用

	}

	gp->users_n = sm->user_n;
	/* 用户行为参数 */
	gp->ss_num_type = sm->ss_num_type;
	ntg_memcpy(gp->ss_num, sm->ss_num, sizeof(float) * 3);
	gp->ss_itv_type = sm->ss_itv_type;
	ntg_memcpy(gp->ss_itv, sm->ss_itv, sizeof(float) * 3);
	gp->clk_num_type = sm->clk_num_type;
	ntg_memcpy(gp->clk_num, sm->clk_num, sizeof(float) * 3);
	gp->clk_itv_type = sm->clk_itv_type;
	ntg_memcpy(gp->clk_itv, sm->clk_itv, sizeof(float) * 3);

	/* 场景参数处理 */
	gp->is_scene = sm->is_scene;
	printf("gp->is_scene = %d====sm->is_scene=%d\n", gp->is_scene , sm->is_scene);
	if(gp->is_scene) {
		ntg_memcpy(gp->scene, sm->scene, sizeof(float) * 24);
		gp->point = sm->st_point;
	}
	printf("-------ntg_user_simulation_process------4-----\n");
	/* 初始化用户组 */
	if (ntg_user_group_init(gp) != NTG_OK){
		printf("-------------ntg_user_message_process----------------6---0---\n");
		ntg_user_group_free(gp);
		return NTG_ERROR;
	}
	printf("-------ntg_user_simulation_process-------5------\n");
	/* 加入任务链表 */
	gp->data = mg->tasks;
	mg->tasks = gp;
	mg->task_n++;
	gp->active = 1;

	if (!mg->task_flag) {//没有启动任务处理
		printf("-------ntg_user_simulation_process-------5----1--\n");
		struct timeval time;
		time.tv_sec = 1;
		time.tv_usec = 0;

		if (mg->ev_task == NULL) {
			printf("-------ntg_user_simulation_process-------5----9--\n");
			mg->ev_task = event_new(mg->cycle->base, -1, EV_TIMEOUT, mg->task_pt, mg);
			if (mg->ev_task == NULL) {
				printf("-------ntg_user_simulation_process-------5--2----\n");
				ntg_log_error(NTG_LOG_CRIT, mg->log, 0, "event_new is failed");
				return NTG_ERROR;
			}
		}
		if (event_add(mg->ev_task, &time) < 0 ){//激活任务处理事件
			printf("-------ntg_user_simulation_process-------5--12---\n");
			return NTG_ERROR;
		}
		mg->task_flag = 1;
		printf("-------ntg_user_simulation_process-------5--3----\n");
	}

	printf("-------ntg_user_simulation_process-------6------\n");
	/* 在master主循环中判断cycle->manage->task_n来决定是否需要处理任务 */
	return NTG_OK;
}
/**
 * 控制消息处理
 * @param mg 用户管理对象
 * @param ctl 控制消息对象
 * @return  成功返回NTG_OK, 否则返回NTG_ERROR
 */
static ntg_int_t ntg_user_control_process(ntg_user_manage_t *mg, ntg_control_message_t *ctl){
	ntg_user_consumer_t *csm;
	ntg_user_consumer_t tmp_csm;
	ntg_hash_node_t *node;
	ntg_user_group_t *gp;
	ntg_int_t i;

	printf(".........ntg_user_control_process......1...(%x)..\n", mg);
	/* 获取一个可用的消费者对象 */
	tmp_csm.id = ctl->csm_id;
	node = ntg_hash_find_node(mg->csm_table, &tmp_csm);	// 查找csm_table判断消费者是否存在

	if(node == NULL){
		//没有查找的，发送相应的结果消息
		printf("没有查找的，发送相应的结果消\n");
		return NTG_OK;
	}
	csm = (ntg_user_consumer_t *)node->data;

	/* 根据不同的消息类型进行不同的处理 */
	switch(ctl->type){
	case NTG_CTL_DELETE:
		printf(".........ntg_user_control_process......2...gp_id=%d..\n", ctl->gp_id);
		gp = csm->gps[ctl->gp_id];//获取对应的用户组
		if(gp != NULL){
			printf(".........ntg_user_control_process......2....2.\n");
			return ntg_user_group_free(gp);
		}
		break;
	case NTG_CTL_QUIT:
		printf(".........ntg_user_control_process......3..gp_id=%d...\n", ctl->gp_id);
		ntg_hash_delete_node(mg->csm_table, csm);

		for(i=0; i < NTG_USER_GROUP_SIZE; i++){
			gp = csm->gps[i];//获取对应的用户组
			if(gp == NULL){
				continue;
			}
			ntg_user_group_free(gp);
			csm->gps[i] = NULL;
		}
		/* 回收消费者 */
		csm->data = mg->consumers;
		mg->consumers = csm;
		break;
	}
	printf(".........ntg_user_control_process......4.....\n");
	return NTG_OK;
}

#define NTG_MAX_ACTIVATE_NUM   200
/**
 * 处理所有的仿真任务
 * @param[in] mg 管理对象
 */

static void ntg_user_manage_process_task(evutil_socket_t fd, short what, void *arg){
	ntg_uint_t i;
	struct timeval time;
	ntg_user_manage_t *mg;
	ntg_user_group_t *gp;
	ntg_user_group_t *tmp_gp = NULL;
	ntg_uint_t count=0;
	ntg_uint_t user_num = NTG_MAX_ACTIVATE_NUM;

//	printf("ntg_user_manage_process_task------start-----\n");

	mg = (ntg_user_manage_t *)arg;

	time.tv_sec = 1;
	time.tv_usec = 0;

	for(i=0; i< mg->task_n; i++) {
		gp = (ntg_user_group_t *)mg->tasks;
		mg->tasks = gp->data;
		gp->data = NULL;

		//激活组用户
		if (ntg_user_activate_group_users(gp, user_num) != NTG_OK){
			//TODO 记录日志,并通知前台
			//TODO 需要对NTG_AGAIN和NTG_ERROR进行不同的处理
			gp->data = tmp_gp;
			tmp_gp = gp;
			count++;
		}
		gp->active = 0;
		printf("live users======---------------->%d=\n", (int)gp->live_n);
	}

	//printf(" task num ------================--->%d\n", count);
	mg->task_n = count;
	mg->tasks = tmp_gp;

	if(mg->task_n != 0){
		evtimer_add(mg->ev_task, &time);
	}else{
		mg->task_flag = 0;
		evtimer_del(mg->ev_task);
	}

}

/**
 * 从用户管理对象中获取一个消费者对象
 * @param[in] mg 消费者对象
 * @return 成功返回一个消费者对象，否则返回NULL
 * @note 内部处理出错时的日志记录
 * @note 返回的消费者对象已被清零
 */
static ntg_user_consumer_t * ntg_user_get_consumer(ntg_user_manage_t *mg){

	ntg_user_consumer_t *c;

	c = mg->free_csms;

	if (c == NULL) {
		ntg_log_error(NTG_LOG_ALERT, mg->log, 0,
				"%ui consumers are not enough",
				mg->free_csm_n);

		return NULL;
	}

	mg->free_csms = c->data;
	mg->free_csm_n--;

	ntg_memzero(c, sizeof(ntg_user_consumer_t));

	return c;
}

//static void ntg_user_free_consumer(ntg_user_consumer_t *csm){
//	ntg_user_manage_t *mg;
//	mg = (ntg_user_manage_t*)csm->data;
//
//}

