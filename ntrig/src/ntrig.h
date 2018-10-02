/*
 * ntrig.h
 *
 *  Created on: Oct 12, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTRIG_H_
#define UTILS_NTRIG_H_

#define ntrig_version      1008000
#define NTRIG_VERSION      "1.0"
#define NTRIG_VER          "ntrig/" NTRIG_VERSION

#ifdef NTG_BUILD
#define NTRIG_VER_BUILD    NTRIG_VER " (" NGX_BUILD ")"
#else
#define NTRIG_VER_BUILD    NTRIG_VER
#endif

#define NTRIG_VAR          "NTRIG"
#define NTG_OLDPID_EXT     ".oldbin"



#endif /* UTILS_NTRIG_H_ */
