/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#ifndef _icip06_h_
#define _icip06_h_
#include <math.h>
#include <QList>
#include <QImage>
#include "responses.h"

/**
 * @brief icip06 anti-ghosting algorithm
 *
 * @param arrayofexptime array of floats containing equivalent exposure time (computed from time,f-value and ISO)
 * @param xj [out] HDR image channel 1
 * @param yj [out] HDR image channel 2
 * @param zj [out] HDR image channel 3
 * @param Ir response curve for channel 1, to be found with robertson02
 * @param Ig response curve for channel 2, to be found with robertson02
 * @param Ib response curve for channel 3, to be found with robertson02
 * @param w initial exposure weights, has to be computed with exposure_weights_icip06()
 * @param iterations number of iterations, from commandline
 * @param Ptemp   width*height*#exposures temporary reference array of weights
 * @param P [out] width*height*#exposures reference array of weights
 * @param ldrinput if true we cast v to a Qt list (ldr data), otherwise we cast to a Array2DList (hdr data)
 */
void icip06_applyResponse(    const float * arrayofexptime,
                             pfs::Array2D* Xj,
                             pfs::Array2D* Yj,
                             pfs::Array2D* Zj,
                             const float* Ir,
                             const float* Ig,
                             const float* Ib,
                             const float* w, const int iterations,
                             Array2DList &Ptemp,
                             Array2DList &P, const bool ldrinput, ... );

/**
 * @brief Reinhard06 anti-ghosting algorithm
 *
 * @param list reference to input Qt list containing source exposures, channels RGB
 * @param w initial exposure weights, has to be computed with exposure_weights_icip06()
 * @param iteration number of iterations for which you want the algorithm to run
 * @param Ptemp   width*height*#exposures temporary reference array of weights
 * @param P [out] width*height*#exposures reference array of weights
 */
void reinhard06_anti_ghosting(  const float* w, const int iterations,
                                Array2DList &Ptemp,
                                Array2DList &P,
                                const bool ldrinput, ... );
#endif /* #ifndef _reinhard06_h_ */
