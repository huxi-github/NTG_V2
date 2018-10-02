/*
 * ntg_setaffinity.h
 *
 *  Created on: Oct 30, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_SETAFFINITY_H_
#define UTILS_NTG_SETAFFINITY_H_

#include "../ntg_config.h"
#include "../ntg_core.h"


ntg_int_t
ntg_setaffinity(cpu_set_t *cpus, ntg_int_t rank, ntg_log_t *log);



#endif /* UTILS_NTG_SETAFFINITY_H_ */
