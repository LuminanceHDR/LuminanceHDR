/*
 * This file is a part of Luminance package, based on pfstmo.
 *
 * common configuration file for tone mapping operators
 */


#ifndef __TMO_CONFIG_H
#define __TMO_CONFIG_H

#ifdef BRANCH_PREDICTION
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#endif
