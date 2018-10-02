/**
 * @file 	ntg_md5.h
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 16, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef UTILS_NTG_MD5_H_
#define UTILS_NTG_MD5_H_

#include "../ntg_config.h"
#include "../ntg_core.h"


#if (NTG_HAVE_MD5)

#if (NTG_HAVE_OPENSSL_MD5_H)
#include <openssl/md5.h>
#else
#include <md5.h>
#endif


typedef MD5_CTX  ntg_md5_t;


#if (NTG_OPENSSL_MD5)

#define ntg_md5_init    MD5_Init
#define ntg_md5_update  MD5_Update
#define ntg_md5_final   MD5_Final

#else

#define ntg_md5_init    MD5Init
#define ntg_md5_update  MD5Update
#define ntg_md5_final   MD5Final

#endif


#else /* !NTG_HAVE_MD5 */


typedef struct {
    uint64_t  bytes;
    uint32_t  a, b, c, d;
    u_char    buffer[64];
} ntg_md5_t;


void ntg_md5_init(ntg_md5_t *ctx);
void ntg_md5_update(ntg_md5_t *ctx, const void *data, size_t size);
void ntg_md5_final(u_char result[16], ntg_md5_t *ctx);


#endif


#endif /* UTILS_NTG_MD5_H_ */
