/**
 * This file is a part of Qtpfsgui package, based on pfstmo.
 * @file tmo_reinhard02.h
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard02.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef _tmo_reinhard02_h_
#define _tmo_reinhard02_h_

#include "../../Common/progressHelper.h"
/*
 * @brief Photographic tone-reproduction
 *
 * @param width image width
 * @param height image height
 * @param Y input luminance
 * @param L output tonemapped intensities
 * @param use_scales true: local version, false: global version of TMO
 * @param key maps log average luminance to this value (default: 0.18)
 * @param phi sharpening parameter (defaults to 1 - no sharpening)
 * @param num number of scales to use in computation (default: 8)
 * @param low size in pixels of smallest scale (should be kept at 1)
 * @param high size in pixels of largest scale (default 1.6^8 = 43)
 */
void tmo_reinhard02(unsigned int width, unsigned int height, 
  const float *Y, float *L, 
  bool use_scales, float key, float phi, 
  int num, int low, int high, bool temporal_coherent, ProgressHelper *ph );

#endif /* _tmo_reinhard02_h_ */
