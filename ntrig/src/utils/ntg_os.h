/**
 * @file ntg_os.h
 * @brief
 * @details
 * @author tzh
 * @date Mar 12, 2016
 * @version V0.1
 * @copyright tzh
 */

#ifndef NTG_OS_H_
#define NTG_OS_H_

#include "../ntg_config.h"
#include "../ntg_core.h"

ntg_int_t ntg_os_init(ntg_log_t *log);
void ntg_os_status(ntg_log_t *log);
ntg_int_t ntg_os_specific_init(ntg_log_t *log);
void ntg_os_specific_status(ntg_log_t *log);
ntg_int_t ntg_daemon(ntg_log_t *log);
ntg_int_t ntg_os_signal_process(ntg_cycle_t *cycle, char *sig, ntg_int_t pid);


#endif /* NTG_OS_H_ */
