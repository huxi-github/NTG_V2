/*
 * ntg_setproctitle.h
 *
 *  Created on: Oct 19, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_SETPROCTITLE_H_
#define UTILS_NTG_SETPROCTITLE_H_

#include "../ntg_config.h"
#include "ntg_log.h"

#define NTG_SETPROCTITLE_USES_ENV  1
#define NTG_SETPROCTITLE_PAD       '\0'

ntg_int_t ntg_init_setproctitle(ntg_log_t *log);
void ntg_setproctitle(char *title);

#endif /* UTILS_NTG_SETPROCTITLE_H_ */
