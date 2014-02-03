/**
 * @brief Robertson02 algorithm for automatic self-calibration.
 *
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2004 Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
 * Copytight (C) 2013 Davide Anastasia
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
 * $Id: robertson02.h,v 1.3 2006/09/13 11:52:56 gkrawczyk Exp $
 */

#ifndef ROBERTSON02_H
#define ROBERTSON02_H

//! \author Grzegorz Krawczyk, <gkrawczyk@users.sourceforge.net>
//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Adaptation for Luminance HDR

#include <QList>
#include <QImage>

#include "HdrCreation/createhdr_common.h"

#include <Libpfs/array2d.h>
#include <Libpfs/frame.h>
#include <HdrCreation/fusionoperator.h>

namespace libhdr {
namespace fusion {

//! \brief Debevec Radiance Map operator
class RobertsonOperator : public IFusionOperator
{
public:
    RobertsonOperator()
        : IFusionOperator()
    {}

private:
    void computeFusion(const std::vector<FrameEnhanced>& frames, pfs::Frame& frame) const;

protected:
    void applyResponse(ResponseChannel channel,
                       const DataList& inputData, float* outputData,
                       size_t width, size_t height,
                       float minAllowedValue, float maxAllowedValue,
                       const float* arrayofexptime) const;
};

class RobertsonOperatorAuto : public RobertsonOperator
{
public:
    RobertsonOperatorAuto()
        : RobertsonOperator()
    {}

private:
    void computeFusion(const std::vector<FrameEnhanced>& frames, pfs::Frame& outFrame) const;

    void computeResponse(ResponseChannel channel,
                         const DataList& inputData, float* outputData,
                         size_t width, size_t height,
                         float minAllowedValue, float maxAllowedValue,
                         const float* arrayofexptime) const;
};

}   // fusion
}   // libhdr

//
//  @brief Create HDR image by applying response curve to given images taken with different exposures
//
//  @param xj [out] HDR image
//  @param imgs reference to vector containing source exposures
//  @param I camera response function (array size of M)
//  @param w weighting function for camera output values (array size of M)
//  @param M number of camera output levels
//  @return number of saturated pixels in the HDR image (0: all OK)
//! \note HDR version
int robertson02_applyResponse(pfs::Array2Df& Rj,  pfs::Array2Df& Gj,  pfs::Array2Df& Bj,
                              const float* arrayofexptime,
                              const float* Ir, const float* Ig, const float* Ib,
                              const float* w, int M,
                              const Array2DfList& listhdrR, const Array2DfList& listhdrG, const Array2DfList& listhdrB);
//! \note LDR version
int robertson02_applyResponse(pfs::Array2Df& Rj, pfs::Array2Df& Gj, pfs::Array2Df& Bj,
                              const float* arrayofexptime,
                              const float* Ir, const float* Ig, const float* Ib,
                              const float* w, int M,
                              const QList<QImage*>& listldr);

//*
//  @brief Calculate camera response using Robertson02 algorithm
//
//  @param xj [out]  estimated luminance values
//  @param imgs reference to vector containing source exposures
//  @param I [out] array to put response function
//  @param w weights
//  @param M max camera output (no of discrete steps)
//  @return number of saturated pixels in the HDR image (0: all OK)
//
int robertson02_getResponse(pfs::Array2Df& Rj, pfs::Array2Df& Gj, pfs::Array2Df& Bj,
                            const float* arrayofexptime,
                            float *Ir, float *Ig, float *Ib,
                            const float* w, int M,
                            const QList<QImage*>& list);

int robertson02_getResponse(pfs::Array2Df& Rj, pfs::Array2Df& Gj, pfs::Array2Df& Bj,
                            const float* arrayofexptime,
                            float *Ir, float *Ig, float *Ib,
                            const float* w, int M,
                            const Array2DfList& listhdrR, const Array2DfList& listhdrG, const Array2DfList& listhdrB);

#endif // ROBERTSON02_H
