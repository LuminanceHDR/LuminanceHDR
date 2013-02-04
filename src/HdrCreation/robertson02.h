/**
 * @brief Robertson02 algorithm for automatic self-calibration.
 *
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2004 Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
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
 * @author Grzegorz Krawczyk, <gkrawczyk@users.sourceforge.net>
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * $Id: robertson02.h,v 1.3 2006/09/13 11:52:56 gkrawczyk Exp $
 */

#ifndef ROBERTSON02_H
#define ROBERTSON02_H

#include <QList>
#include <QImage>
#include "responses.h"

/**
 * @brief Create HDR image by applying response curve to given images taken with different exposures
 *
 * @param xj [out] HDR image
 * @param imgs reference to vector containing source exposures
 * @param I camera response function (array size of M)
 * @param w weighting function for camera output values (array size of M)
 * @param M number of camera output levels
 * @return number of saturated pixels in the HDR image (0: all OK)
 */
int robertson02_applyResponse(pfs::Array2D* xj, const float * arrayofexptime,
  const float* I, const float* w, int M, int chan, bool ldrinput, ... );

/**
 * @brief Calculate camera response using Robertson02 algorithm
 *
 * @param xj [out]  estimated luminance values
 * @param imgs reference to vector containing source exposures
 * @param I [out] array to put response function
 * @param w weights
 * @param M max camera output (no of discrete steps)
 * @return number of saturated pixels in the HDR image (0: all OK)
 */
int robertson02_getResponse(pfs::Array2D* xj, const float * arrayofexptime,
  float* I, const float* w, int M, int chan, bool ldrinput, ... );

#endif // ROBERTSON02_H
