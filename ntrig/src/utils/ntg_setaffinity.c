/*
 * ntg_setaffinity.c
 *
 *  Created on: Oct 30, 2015
 *      Author: tzh
 */


#include "../ntg_config.h"
#include "../ntg_core.h"

/**
 * 设置进程的cpu亲和力
 * @param[in] cpus 亲和集
 * @param[in] rank 亲和序号
 * @param[in] log 日志对象
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */

ntg_int_t
ntg_setaffinity(cpu_set_t *cpus, ntg_int_t rank, ntg_log_t *log)
{
#if (NTG_CPU_AFFINITY)

	ntg_uint_t count;
    count = tmc_cpus_count(cpus);

    /* 判断合法性 */
    if(count <= 0){//没有指定cpu亲和性
    	return NTG_OK;
    }

	if ( count < rank ) {
		tmc_task_die("Insufficient cpus (%d < %d).", (int)count, (int)rank);
		ntg_log_error(NTG_LOG_ALERT, log, 0,"insufficient cpus (%d < %d).",
				count, rank);
		return NTG_ERROR;
	}

    ntg_log_error(NTG_LOG_NOTICE, log, 0, "process(%d) set the affinity in no.(%d) position",
    		ntg_pid, rank);

	if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(cpus, rank)) < 0){
		tmc_task_die("Failure in 'tmc_cpus_set_my_cpu()'.");
		ntg_log_error(NTG_LOG_ALERT, log, 0 ,"failure in 'tmc_cpus_set_my_cpu()");
		return NTG_ERROR;
	}
#endif
	return NTG_OK;
}

