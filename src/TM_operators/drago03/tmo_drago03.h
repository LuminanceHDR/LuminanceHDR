/**
 * @brief Frederic Drago logmapping operator
 * 
 * This file is a part of Qtpfsgui package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 * 
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_drago03.h,v 1.3 2008/09/04 12:46:48 julians37 Exp $
 */

#ifndef _tmo_drago03_h_
#define _tmo_drago03_h_

/**
 * @brief Frederic Drago Logmapping Algorithm
 *
 * Implementation obtained from source code provided
 * by Frederic Drago on 16 May 2003.
 *
 * @param width image width
 * @param height image height
 * @param Y [in] image luminance values
 * @param L [out] tone mapped values
 * @param maxLum maximum luminance in the image
 * @param avLum logarithmic average of luminance in the image
 * @param bias bias parameter of tone mapping algorithm (eg 0.85)
 */
#include "../../Common/progressHelper.h"

void tmo_drago03(unsigned int width, unsigned int height,
  const float* Y, float* L,
  float maxLum, float avLum, float bias, ProgressHelper *ph);

/**
 * @brief Find average and maximum luminance in an image
 * 
 * @param Y [in] image luminance values
 * @param avLum [out] average luminance
 * @param maxLum [out] maximum luminance
 */
void calculateLuminance(unsigned int width, unsigned int height, const float* Y, float& avLum, float& maxLum );

#endif
