/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
 * Copyright (C) 2017 Franco Comida
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
//! \author Franco Comida <fcomida@users.sourceforge.net>
//! New Debevec implementation

namespace libhdr {
namespace fusion {

//! \brief Debevec Radiance Map operator
class DebevecOperator : public IFusionOperator {
   public:
    DebevecOperator() : IFusionOperator() {}

    FusionOperator getType() const { return DEBEVEC; }

   private:
    void computeFusion(ResponseCurve &response, WeightFunction &weight,
                       const std::vector<FrameEnhanced> &frames,
                       pfs::Frame &frame);
};

}  // fusion
}  // libhdr

#endif  // LIBHDR_FUSION_DEBEVEC_H
