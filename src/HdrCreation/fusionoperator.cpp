/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#include "fusionoperator.h"
#include "debevec.h"

#include <cassert>
#include <boost/make_shared.hpp>

namespace libhdr {
namespace fusion {

IFusionOperator::IFusionOperator()
    : m_response(new ResponseLinear)
    , m_weight(new WeightPlateau) // Triangular) // Gaussian) // Triangular)
{}

// questa e' da sistemare...
pfs::Frame* IFusionOperator::computeFusion(const std::vector<FrameEnhanced>& frames) const
{
    pfs::Frame* frame = new pfs::Frame;
    computeFusion(frames, *frame);
    return frame;
}

FusionOperatorPtr IFusionOperator::build(FusionOperator type) {
    switch (type) {
    case ROBERTSON02_NEW:
        assert(false);
        break;
    case DEBEVEC_NEW:
    default:
        return boost::make_shared<DebevecOperator>();
        break;
    }
}

}   // fusion
}   // libhdr
