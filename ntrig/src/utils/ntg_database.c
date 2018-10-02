/**
 * @file ntg_database.c
 * @brief
 * @details
 * @author tzh
 * @date Jun 13, 2016
 * @version V0.1
 * @copyright tzh
 */
#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../message/ntg_message.h"
#include "ntg_database.h"

/**
 * 初始化mysql数据库
 * @param cycle 循环对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t ntg_db_mydql_init(ntg_cycle_t *cycle){
	ntg_core_conf_t *ccf;
	ntg_db_mysql_t *db;
	ntg_database_conf_t *db_conf;
	char *query = "set character set gb2312";
	char value = 1;

	db = (ntg_db_mysql_t*) ntg_palloc(cycle->pool, sizeof(ntg_db_mysql_t));
	if (db == NULL) {
		printf("-------------db == NULL)---------------\n");
		return NTG_ERROR;
	}
	db->cycle = cycle;

	ccf = (ntg_core_conf_t *) ntg_get_conf(cycle->conf_ctx, ntg_core_module);
	db_conf = &ccf->database;

	if (!db_conf->type.data) {//不存在
		printf("-------------!db_conf->type.data---------------\n");
		return NTG_ERROR;
	}
	if(ntg_strncmp("mysql",db_conf->type.data, db_conf->type.len) != 0){
		printf("-----------!db_conf->type.data-----------------\n");
		return NTG_ERROR;
	}

	/* 初始化数据库 */
	mysql_init(&db->mysql);
	mysql_options(&db->mysql, MYSQL_OPT_RECONNECT, (char*)&value);

	MYSQL *		STDCALL mysql_real_connect(MYSQL *mysql, const char *host,
	   const char *user,
	   const char *passwd,
	   const char *db,
	   unsigned int port,
	   const char *unix_socket,
	   unsigned long clientflag);

	if(!mysql_real_connect(&db->mysql, (const char *)db_conf->host.data,
			(const char *)db_conf->user.data, (const char *)db_conf->password.data,
			(const char *)db_conf->name.data, (unsigned int)db_conf->port, NULL, 0)){
		//error
		printf("-----mysql_real_connect-------(%s)----------\n", mysql_error(&db->mysql));
		return NTG_ERROR;
	}
	if (mysql_real_query(&db->mysql, query, (unsigned int)strlen(query))){
		printf("字符集设置出错:%s\n", mysql_error(&db->mysql));
		return NTG_ERROR;
	}
	cycle->db = db;

	return NTG_OK;
}
/**
 * 将记录插入到日志表中
 * @param db 数据库对象
 * @param rcd 日志记录对象
 * @return 成功返回NTG_OK,否则返回NTG_ERROR
 */
ntg_int_t ntg_db_mysql_insert(ntg_db_mysql_t *db, ntg_record_message_t *rcd){
	struct timeval tv;
	char query[512];
	int n=0;

	mysql_ping(&db->mysql);

	if (evutil_gettimeofday(&tv, NULL) < 0){
		printf("------------evutil_gettimeofday-----------\n");
		return NTG_ERROR;
	}

	n = sprintf(query, "insert into log_t (log_id, csm_id, group_id, user_id, url_id,"
			" state_code, send_size, recv_size, time) value(null, '%d','%d','%d','%d','%d',"
			"'%d','%d','%ld')",
			rcd->csm_id, rcd->gp_id, (int)rcd->user_id, (int)rcd->url_id, (int)rcd->state_code,
			(int)rcd->send, (int)rcd->receive, (long int)tv.tv_sec);
	query[n] = '\0';
//	printf("query====%s\n", query);

	if (mysql_real_query(&db->mysql, query, (unsigned int)strlen(query))){
		printf("插入日志出错:%s\n", mysql_error(&db->mysql));
		return NTG_ERROR;
	}

	return NTG_OK;
}




