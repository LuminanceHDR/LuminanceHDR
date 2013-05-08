/*
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
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
 */

#ifndef LIBHDR_FUSION_DEBEVEC_H
#define LIBHDR_FUSION_DEBEVEC_H

#include <HdrCreation/fusionoperator.h>

//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Adaptation for Luminance HDR

namespace libhdr {
namespace fusion {

//! \brief Debevec Radiance Map operator
class DebevecOperator : public IFusionOperator
{
public:
    DebevecOperator()
        : IFusionOperator()
    {}

private:
    void computeFusion(const std::vector<FrameEnhanced> &frames, pfs::Frame& frame) const;
};

}   // fusion
}   // libhdr

/*
// old stuff!
#include <QList>
#include <QImage>
#include "HdrCreation/createhdr_common.h"

//! \brief Create HDR image by applying response curve to given images using Debevec model, simple model, using array of weights P, not checking for under/over exposed pixel values.
//!
//! \param list reference to input Qt list containing source exposures, channels RGB
//! \param arrayofexptime array of floats containing equivalent exposure time (computed from time,f-value and ISO)
//! \param xj [out] HDR image channel 1
//! \param yj [out] HDR image channel 2
//! \param zj [out] HDR image channel 3
//! \param I1 response curve for channel 1, to be found with robertson02
//! \param I2 response curve for channel 2, to be found with robertson02
//! \param I3 response curve for channel 3, to be found with robertson02
//! \param P  width*height*#exposures array of weights
//!
//int debevec_applyResponse( const float *arrayofexptime,
//			pfs::Array2D* xj, const float* I1,
//			pfs::Array2D* yj, const float* I2,
//			pfs::Array2D* zj, const float* I3,
//			const Array2DList &P, const bool ldrinput, ... );

//! \brief Create HDR image by applying response curve to given images using Debevec model, checking for under/over exposed pixel values.
//!
//! \param list reference to input Qt list containing source exposures, channels RGB
//! \param arrayofexptime array of floats containing equivalent exposure time (computed from time,f-value and ISO)
//! \param xj [out] HDR image channel 1
//! \param yj [out] HDR image channel 2
//! \param zj [out] HDR image channel 3
//! \param Ir response curve for channel 1, to be found with robertson02
//! \param Ig response curve for channel 2, to be found with robertson02
//! \param Ib response curve for channel 3, to be found with robertson02
//! \param w  array of weights
//! \param M  lenght of w
//!
//! \note HDR version
int debevec_applyResponse(pfs::Array2Df& xj,  pfs::Array2Df& yj,  pfs::Array2Df& zj,
                          const float* arrayofexptime,
                          const float* Ir, const float* Ig, const float* Ib,
                          const float* w, int M,
                          const Array2DfList& listhdrR, const Array2DfList& listhdrG, const Array2DfList& listhdrB);
//! \note LDR version
int debevec_applyResponse(pfs::Array2Df& xj, pfs::Array2Df& yj, pfs::Array2Df& zj,
                          const float* arrayofexptime,
                          const float* Ir, const float* Ig, const float* Ib,
                          const float* w, int M,
                          const QList<QImage*>& list);
*/

#endif // LIBHDR_FUSION_DEBEVEC_H
