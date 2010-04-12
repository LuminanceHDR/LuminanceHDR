/**
 * @brief Generic algorithm for applying response curve and combining images.
 *
 * This file is a part of LuminanceHDR package.
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

#ifndef _gerneric_applyResponse_h_
#define _gerneric_applyResponse_h_
#include <QList>
#include <QImage>
#include "responses.h"

/**
 * @brief Create HDR image by applying response curve to given images taken with different exposures
 *
 * @param sumf function used for calculating the dividend when averaging over all images
 * @param divf function used for calculating the divisor when averaging over all images
 * @param outf function used for transforming the resulting quotient to the output range
 * @param Rout [out] HDR image channel 1
 * @param Gout [out] HDR image channel 2
 * @param Bout [out] HDR image channel 3
 * @param arrayofexptime array of floats containing equivalent exposure time (computed from time,f-value and ISO)
 * @param Ir response curve for channel 1
 * @param Ig response curve for channel 2
 * @param Ib response curve for channel 3
 * @param w  array of weights
 * @param M  number of levels of the input images
 * @param ldrinput if true listldr is used for the input images otherwise listhdrR, listhdrG, listhdrB are used
 * @param listldr stack of input images in QImage format (may be null if ldrinput is false)
 * @param listhdrR stack of input images for red chanel in PFS-Format (may be null if ldrinput is true)
 * @param listhdrG stack of input images for green chanel in PFS-Format (may be null if ldrinput is true)
 * @param listhdrB stack of input images for blue chanel in PFS-Format (may be null if ldrinput is true)
 */
void generic_applyResponse( 
    float (*sumf)(float, float, float, float), float (*divf)(float, float, float, float), float (*outf)(float), 
    pfs::Array2D* Rout, pfs::Array2D* Gout, pfs::Array2D* Bout, const float * arrayofexptime, 
    const float* Ir,  const float* Ig, const float* Ib, const float* w, const int M, 
    const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB);

#endif /* #ifndef _gerneric_applyResponse_h_ */
