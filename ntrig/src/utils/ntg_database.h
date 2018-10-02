/**
 * @file ntg_database.h
 * @brief
 * @details
 * @author tzh
 * @date Jun 13, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_DATABASE_H_
#define NTG_DATABASE_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "../message/ntg_message.h"

/**
 * mysql 数据库对象
 */
struct ntg_db_mysql_s {
	ntg_cycle_t *cycle;
	MYSQL	mysql;
	MYSQL_RES *res;
	MYSQL_FIELD *fd;
	MYSQL_ROW	row;
};

ntg_int_t ntg_db_mydql_init(ntg_cycle_t *cycle);
ntg_int_t ntg_db_mysql_insert(ntg_db_mysql_t *db,ntg_record_message_t *rcd);

#endif /* NTG_DATABASE_H_ */
