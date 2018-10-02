/*
 * ntg_linux.h
 *
 *  Created on: Oct 30, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_LINUX_H_
#define UTILS_NTG_LINUX_H_


ntg_chain_t *ntg_linux_sendfile_chain(ntg_connection_t *c, ntg_chain_t *in,
    off_t limit);

extern int ntg_linux_rtsig_max;

#endif /* UTILS_NTG_LINUX_H_ */
