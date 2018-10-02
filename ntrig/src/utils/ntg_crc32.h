/**
 * @file 	ntg_crc32.h
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 16, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#ifndef UTILS_NTG_CRC32_H_
#define UTILS_NTG_CRC32_H_

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_crc32.h"


extern uint32_t  *ntg_crc32_table_short;
extern uint32_t   ntg_crc32_table256[];


static ntg_inline uint32_t
ntg_crc32_short(u_char *p, size_t len)
{

    u_char    c;
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--) {
        c = *p++;
        crc = ntg_crc32_table_short[(crc ^ (c & 0xf)) & 0xf] ^ (crc >> 4);
        crc = ntg_crc32_table_short[(crc ^ (c >> 4)) & 0xf] ^ (crc >> 4);
    }

    return crc ^ 0xffffffff;
}


static ntg_inline uint32_t
ntg_crc32_long(u_char *p, size_t len)
{
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--) {
        crc = ntg_crc32_table256[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    }

    return crc ^ 0xffffffff;
}


#define ntg_crc32_init(crc)                                                   \
    crc = 0xffffffff


static ntg_inline void
ntg_crc32_update(uint32_t *crc, u_char *p, size_t len)
{
    uint32_t  c;

    c = *crc;

    while (len--) {
        c = ntg_crc32_table256[(c ^ *p++) & 0xff] ^ (c >> 8);
    }

    *crc = c;
}


#define ntg_crc32_final(crc)                                                  \
    crc ^= 0xffffffff


ntg_int_t ntg_crc32_table_init(void);


#endif /* UTILS_NTG_CRC32_H_ */
