/*
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copytight (C) 2013-2014 Davide Anastasia
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
 */

#ifndef ROBERTSON02_H
#define ROBERTSON02_H

//! \brief Robertson02 algorithm for automatic self-calibration.
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <QList>
#include <QImage>

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
    void computeFusion(
            ResponseCurve& response,
            const std::vector<FrameEnhanced>& frames, pfs::Frame& frame) const;

protected:
    void applyResponse(
            ResponseCurve& response,
            ResponseChannel channel,
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
    void computeFusion(
            ResponseCurve& response,
            const std::vector<FrameEnhanced>& frames, pfs::Frame& outFrame) const;

    void computeResponse(
            ResponseCurve& response,
            ResponseChannel channel,
            const DataList& inputData, float* outputData,
            size_t width, size_t height,
            float minAllowedValue, float maxAllowedValue,
            const float* arrayofexptime) const;
};

}   // fusion
}   // libhdr

#endif // ROBERTSON02_H
