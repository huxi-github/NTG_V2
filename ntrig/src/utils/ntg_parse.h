/*
 * ntg_parse.h
 *
 *  Created on: Sep 1, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_PARSE_H_
#define UTILS_NTG_PARSE_H_
#include "../ntg_config.h"
#include "ntg_string.h"

//解析大小
ssize_t 	ntg_parse_size(ntg_str_t *line);
//解析偏移
off_t 		ntg_parse_offset(ntg_str_t *line);
//解析时间
ntg_int_t 	ntg_parse_time(ntg_str_t *line, ntg_uint_t is_sec);

#endif /* UTILS_NTG_PARSE_H_ */
