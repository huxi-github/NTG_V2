/**
 * @file ntg_remote.c
 * @brief
 * @details
 * @author tzh
 * @date May 31, 2016
 * @version V0.1
 * @copyright tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../message/ntg_message.h"
#include "ntg_channel.h"
#include "ntg_math.h"
#include "ntg_table.h"
#include "ntg_process_cycle.h"
#include "ntg_remote.h"

static void ntg_accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
		struct sockaddr *add, int socklen, void *ctx);
static void ntg_accept_error_cb(struct evconnlistener *listener, void *ctx);

static void ntg_remote_read_cb(struct bufferevent *bev, void* cxt);
static void ntg_remote_write_cb(struct bufferevent *bev, void* cxt);
static void ntg_remote_event_cb(struct bufferevent *bev, short events,
		void *cxt);

static ntg_uint_t ntg_remote_message_handler(ntg_connect_t *cnt, cJSON* root);
static void ntg_remote_response(struct bufferevent *bev, ntg_int_t state_code);

static ntg_int_t ntg_remote_get_type(cJSON *obj);
static void ntg_remote_get_args(cJSON *obj, float data[], int size);

static ntg_uint_t ntg_remote_get_key(void *data);
static ntg_uint_t ntg_remote_compare(void *d1, void *d2);

ntg_str_t result_msg[] = {

};

/**
 * 远程监听初始化
 * @param[in] rmt 远程监听对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t ntg_remote_init(ntg_cycle_t *cycle) {
	ntg_core_conf_t *ccf;
	ntg_remote_t *rmt;
	ntg_hash_table_t *table;
	printf(".............ntg_remote_init.....1...\n");
	rmt = (ntg_remote_t*) ntg_palloc(cycle->pool, sizeof(ntg_remote_t));
	if (rmt == NULL) {
		return NTG_ERROR;
	}

	rmt->cycle = cycle;
	rmt->pool = cycle->pool;

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);

	/* 初始化前端监听 */
	if (!ccf->front.sin.sin_addr.s_addr) {//地址不存在
		return NTG_ERROR;
	}

	rmt->listen = evconnlistener_new_bind(cycle->base, ntg_accept_cb, rmt,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE,
			-1, (const struct sockaddr *) &ccf->front.sin,
			sizeof(ccf->front.sin));
	if (!rmt->listen) {
		return NTG_ERROR;
	}
	evconnlistener_set_error_cb(rmt->listen, ntg_accept_error_cb);//设置错误处理

	/* 初始化链接hash表 */
	table = ntg_palloc(rmt->pool, sizeof(ntg_hash_table_t));
	if (table == NULL) {
		return NTG_ERROR;
	}

	rmt->cnt_table = table;
	//size、key、cmp、node_n、pool
	if (ccf->consumes != NTG_CONF_UNSET) {
		table->size = ccf->consumes / 1.5;//在配置文件中设置， 最好设置为2^x
		table->node_n = ccf->consumes;
	} else {
		table->size = NTG_USER_DEFAULT_CONSUMER_NUM / 1.5;//在配置文件中设置， 最好设置为2^x
		table->node_n = NTG_USER_DEFAULT_CONSUMER_NUM;
	}

	table->key = ntg_remote_get_key;
	table->cmp = ntg_remote_compare;
	table->pool = rmt->pool;

	if (ntg_hash_table_init(table) != NTG_OK) {
		return NTG_ERROR;
	}

	cycle->rmt = rmt;
	printf(".............ntg_remote_init......finish..pid=%d.\n", ntg_pid);
	return NTG_OK;
}

/**
 * 链接接受处理函数
 * @param listener 链接监听对象
 * @param fd 本地套接字
 * @param addr 源地址
 * @param socklen 地址长度
 * @param ctx 创建监听对象时传入的参数(远程对象)
 */
static void ntg_accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
		struct sockaddr *addr, int socklen, void *ctx) {
	ntg_remote_t *rmt;
	struct event_base *base;
	struct bufferevent *bev;
	ntg_connect_t *cnt;

	rmt = (ntg_remote_t*) ctx;

	base = evconnlistener_get_base(listener);

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev) {
		return;
	}

	/* 创建一个链接对象 */
	cnt = (ntg_connect_t*) ntg_calloc(sizeof(ntg_connect_t), rmt->cycle->log);//在处理结束后释放
	if (!cnt) {
		bufferevent_free(bev);
		return;
	}

	cnt->fd = fd;
	cnt->rmt = rmt;
	cnt->bev = bev;

	bufferevent_setcb(bev, ntg_remote_read_cb, ntg_remote_write_cb,
			ntg_remote_event_cb, cnt);
	bufferevent_enable(bev, EV_READ);

}
/**
 * 链接接受错误处理
 * @param listener 监听对象
 * @param ctx 传入的参数
 */
static void ntg_accept_error_cb(struct evconnlistener *listener, void *ctx) {
//	struct event_base *base;
	int err;

//	base = evconnlistener_get_base(listener);
	err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener."
		"Shutting down.", err, evutil_socket_error_to_string(err));
	//TODO 应该设置退出标志
	ntg_terminate = 1;
	//记录日志
	evconnlistener_free(listener);
}

/**
 * 远程链接可读处理函数
 * @param[in] bev 事件缓存对象
 * @param[in] cxt 传入的参数（链接对象）
 */
static void ntg_remote_read_cb(struct bufferevent *bev, void* ctx) {
	printf("-----------ntg_remote_read_cb-----------------\n");
	char data[1024];
	char * next, *last;
	cJSON* root = NULL;
	int size;
	ntg_connect_t *cnt;

	cnt = (ntg_connect_t*) ctx;
	next = data;
	last = data + 1024;

	while((size = bufferevent_read(bev, next, last - next)) > 0){
		next = next + size;
	}
	*next = '\0';

	if (size < 0 || (next - data) < 16) {//消息太短
		ntg_remote_response(bev, NTG_REMOTE_FROMAT_ERROR);
		goto err;
	}

	/* 构造json对象 */
	root = cJSON_Parse(data);
	if (!root) {
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
		ntg_remote_response(bev, NTG_REMOTE_FROMAT_ERROR);
		goto err;
	}
	/* 业务消息 */
	ntg_remote_message_handler(cnt, root);

	err:
	if (!root) {
		cJSON_Delete(root);
	}
}
/**
 * 远程链接可写处理
 * @param bev 事件缓存对象
 * @param cxt 传入的参数(链接对象)
 * @note 当数据写完后，回主动关闭链接
 */
static void ntg_remote_write_cb(struct bufferevent *bev, void* ctx) {
	printf("-----------ntg_remote_write_cb-----------------\n");
	ntg_connect_t *cnt;
	cnt = (ntg_connect_t*) ctx;

	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		printf("flushed answer\n");
		ntg_hash_delete_node(cnt->rmt->cnt_table, cnt);//从hash表中删除链接对象
		ntg_free(cnt);
		bufferevent_free(bev);
	}
}
/**
 * 远程链接的发生错误时的回调函数
 * @param[in] bev 事件缓存对象
 * @param[in] events 发送的事件标志
 * @param[in] cxt 传入的参数
 */
static void ntg_remote_event_cb(struct bufferevent *bev, short events,
		void *ctx) {
	ntg_connect_t *cnt;
	cnt = (ntg_connect_t*) ctx;

	if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");

	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n", strerror(errno));/*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	printf("-----------ntg_remote_event_cb-----------------\n");
	ntg_hash_delete_node(cnt->rmt->cnt_table, cnt);//从hash表中删除链接对象
	ntg_free(cnt);
	bufferevent_free(bev);
}

/**
 *
 * @param cnt 链接对象
 * @param json json对象
 * @return 前端消息处理成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 前端消息有两种：仿真消息和控制消息
 * @note 如果消息处理过程出错，json等资源的释放交由调用者处理
 * @note 已进行错误日志的记录
 */
static ntg_uint_t ntg_remote_message_handler(ntg_connect_t *cnt, cJSON* root) {
	cJSON* json;
	ntg_cycle_t *cycle;
	ntg_message_t msg;
	ntg_simulation_message_t *sm;
	ntg_control_message_t *ctl;

	cycle = cnt->rmt->cycle;
	ntg_memzero(&msg, sizeof(ntg_message_t));

	json = cJSON_GetObjectItem(root, "type");
	if (ntg_strcmp(json->valuestring, "simulation") == 0) {//仿真消息
		msg.type = NTG_SIMULATION_TYPE;
		sm = &msg.body.sm;
		sm->csm_id = cnt->id = cJSON_GetObjectItem(root, "csm_id")->valueint;
		sm->gp_id = cJSON_GetObjectItem(root, "gp_id")->valueint;
		sm->user_n = cJSON_GetObjectItem(root, "users")->valueint;

		/* 行为参数 */
		json = cJSON_GetObjectItem(root, "ss_num");
		sm->ss_num_type = ntg_remote_get_type(json);//0.39
		ntg_remote_get_args(json, sm->ss_num, 3);

		json = cJSON_GetObjectItem(root, "ss_itv");
		sm->ss_itv_type = ntg_remote_get_type(json);//10, 0.93, 1.67
		ntg_remote_get_args(json, sm->ss_itv, 3);

		json = cJSON_GetObjectItem(root, "clk_num");
		sm->clk_num_type = ntg_remote_get_type(json);//1.5, 0.47, 0.69
		ntg_remote_get_args(json, sm->clk_num, 3);

		json = cJSON_GetObjectItem(root, "clk_itv");
		sm->clk_itv_type = ntg_remote_get_type(json);//35, 0.49, 2.78
		ntg_remote_get_args(json, sm->clk_itv, 3);

		/* 场景参数 */
		json = cJSON_GetObjectItem(root, "scene");
		sm->is_scene = (cJSON_GetObjectItem(json, "is_scene")->type == cJSON_True ) ? 1
				: 0;
		printf("----is_scene===%d--\n", sm->is_scene);
		if (sm->is_scene) {
			ntg_remote_get_args(json, sm->scene, 24);
		}
		sm->st_point = 0;

	} else if (ntg_strcmp(json->valuestring, "control") == 0) {//控制消息
		msg.type = NTG_CONTROL_TYPE;
		ctl = &msg.body.ctl;
		ctl->csm_id = cnt->id = cJSON_GetObjectItem(root, "csm_id")->valueint;

		cJSON *type = cJSON_GetObjectItem(root, "ctl_type");
		if (ntg_strcmp(type->valuestring, "delete") == 0) {
			ctl->type = NTG_CTL_DELETE;
			ctl->gp_id = cJSON_GetObjectItem(root, "gp_id")->valueint;
		} else if (ntg_strcmp(type->valuestring, "quit") == 0) {
			ctl->type = NTG_CTL_QUIT;
		}
	}

	/* 判断是否存在 */
	if (ntg_hash_find_node(cnt->rmt->cnt_table, cnt) != NULL) {//存在返回存在消息
		ntg_remote_response(cnt->bev, NTG_REMOTE_EXIST_ERROR);
		printf("----------ntg_remote_message_handler-------1---\n");
		return NTG_ERROR;
	}

	if (ntg_hash_insert_node(cnt->rmt->cnt_table, cnt) != NTG_OK) {//超过系统支持的用户数
		ntg_remote_response(cnt->bev, NTG_REMOTE_TOO_CSM_ERROR);
		//严重错误，记录日志
		printf("----------ntg_remote_message_handler------2----\n");
		return NTG_ERROR;
	}

//	if (ntg_hash_find_node(cnt->rmt->cnt_table, cnt) != NULL) {//存在返回存在消息
//		printf("----------ntg_remote_message_handler---(%x)---pid=%d-1-2--\n", cnt->rmt->cnt_table, ntg_pid);
//		int i;
//		for(i=0; i<cnt->rmt->cnt_table->size; i++){
//			printf("...%d...(%x).......\n", i, cnt->rmt->cnt_table->buckets[i]);
//		}
//	}

	if (msg.type == NTG_SIMULATION_TYPE) {
		if (ntg_write_channel(ntg_processes[0].channel[0], &msg,
				sizeof(msg.type) + sizeof(msg.body.sm), cycle->log) != NTG_OK) {
			printf("-------message process is failed---\n");
			//后端错误
			ntg_remote_response(cnt->bev, NTG_REMOTE_END_ERROR);
			return NTG_ERROR;
		}
	} else {
		if (ntg_write_channel(ntg_processes[0].channel[0], &msg,
				sizeof(msg.type) + sizeof(msg.body.ctl), cycle->log) != NTG_OK) {
			printf("-------message process is failed---\n");
			ntg_remote_response(cnt->bev, NTG_REMOTE_END_ERROR);
			return NTG_ERROR;
		}
	}
	printf("--------ntg_remote_message_handler---finish-------\n");
	fflush(NULL);
	return NTG_OK;
}
/**
 * 处理从虚拟用户层返回的结果消息
 * @param rmt 远程对象
 * @param rm 结果消息对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
ntg_int_t ntg_remote_result_handler(ntg_remote_t *rmt, ntg_result_message_t *rm) {
	ntg_connect_t tmp;
	ntg_hash_node_t *node;
	ntg_connect_t *cnt;

	/* 查找链接对象 */
	cnt = &tmp;
	cnt->id = rm->csm_id;

	/* 判断是否存在 */
	printf("........ntg_remote_result_handler........(%x)...pid=%d...\n", rmt->cnt_table, ntg_pid);
	int i;
	for(i=0; i<rmt->cnt_table->size; i++){
		printf("...%d...(%x).......\n", i, rmt->cnt_table->buckets[i]);
	}

	node = ntg_hash_find_node(rmt->cnt_table, cnt);
	if (node == NULL) {
		//TODO 记录日志
		printf("----------ntg_remote_result_handler------3---------\n");
		fflush(NULL);
		return NTG_ERROR;
	}
	fflush(NULL);
	cnt = (ntg_connect_t*) node->data;
	if (rm->status == NTG_RST_SUCCESS) {
		printf("----------ntg_remote_result_handler--------4-------\n");
		ntg_remote_response(cnt->bev, NTG_REMOTE_SUCCESS);
	} else {
		printf("----------ntg_remote_result_handler--------5-------\n");
		ntg_remote_response(cnt->bev, NTG_REMOTE_END_ERROR);
	}
	printf("----------ntg_remote_result_handler-------6-------\n");
	fflush(NULL);
	return NTG_OK;
}

/**
 * 响应消息
 * @param bev 事件缓存对象
 * @param state_code 响应状态码
 */
static void ntg_remote_response(struct bufferevent *bev, ntg_int_t state_code) {

	int size = sizeof("{\"state_code\":100}");
	printf("ntg_remote_response......\n");
	fflush(NULL);

	switch (state_code) {
	case NTG_REMOTE_FROMAT_ERROR:
		bufferevent_write(bev, "{\"state_code\":100}", size);
		break;
	case NTG_REMOTE_EXIST_ERROR:
		bufferevent_write(bev, "{\"state_code\":101}",size);
		break;
	case NTG_REMOTE_TOO_CSM_ERROR:
		bufferevent_write(bev, "{\"state_code\":102}", size);
		break;
	case NTG_REMOTE_END_ERROR:
		bufferevent_write(bev, "{\"state_code\":103}", size);
		break;
	case NTG_REMOTE_SUCCESS:
		bufferevent_write(bev, "{\"state_code\":200}", size);
	}

	bufferevent_enable(bev, EV_WRITE);
	return;
}
/**
 * 行为函数的类型
 * @param obj json对象
 * @return 返回对应行为函数的类型标号
 */
static ntg_int_t ntg_remote_get_type(cJSON *obj) {
	cJSON *type;

	type = cJSON_GetObjectItem(obj, "type");

	if (ntg_strcmp(type->valuestring, "lognormal") == 0) {
		return NTG_MATH_LOGNORMAL_E;
	} else if (ntg_strcmp(type->valuestring, "posisson") == 0) {
		return NTG_MATH_POSISSON_E;
	} else if (ntg_strcmp(type->valuestring, "exponential") == 0) {
		return NTG_MATH_EXPONENTIAL_E;
	} else if (ntg_strcmp(type->valuestring, "normal") == 0) {
		return NTG_MATH_NORMAL_E;
	} else if (ntg_strcmp(type->valuestring, "weibull") == 0) {
		return NTG_MATH_WEIBULL_E;
	}
	//TODO 不应该到达这
	return NTG_MATH_AVERAGE_E;
}
/**
 * 解析json文件中的args信息，并存储到data中
 * @param[in] obj json对象
 * @param[out] data 保存信息的数据数组
 * @param[in] size data 数组大小
 * @note 必须保证data有足够的空间
 */
static void ntg_remote_get_args(cJSON *obj, float data[], int size) {
	cJSON *head;
	cJSON *pos;
	int index = 0;

	head = cJSON_GetObjectItem(obj, "args");

	cJSON_ArrayForEach(pos, head) {
		data[index++] = (float) pos->valuedouble;
	}

	while (index < size) {
		data[index++] = 0;
	}
}

/**
 * 计算消费者对象的key
 * @param[in] data 数据指针
 * @return 返回对象的key
 */
static ntg_uint_t ntg_remote_get_key(void *data) {
	ntg_uint_t key;
	ntg_connect_t *cnt;

	cnt = (ntg_connect_t *) data;
	key = cnt->id;

	return key;
}

/**
 * 比较两个消费者对象
 * @param[in] d1 消费者对象1
 * @param[in] d2 消费者对象2
 * @return 如果两个对象的id相等返回0
 */
static ntg_uint_t ntg_remote_compare(void *d1, void *d2) {

	ntg_connect_t *cnt1, *cnt2;

	cnt1 = (ntg_connect_t *) d1;
	cnt2 = (ntg_connect_t *) d2;

	printf("------------ntg_remote_compare---------------\n");
	return (cnt1->id - cnt2->id);
}
