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
#include "robertson02.h"

#include <cassert>
#include <boost/make_shared.hpp>

#include <Libpfs/frame.h>

using namespace pfs;

namespace libhdr {
namespace fusion {

IFusionOperator::IFusionOperator()
    : m_response(new ResponseLinear)
    , m_weight(new WeightGaussian)
{}

// questa e' da sistemare...
pfs::Frame* IFusionOperator::computeFusion(const std::vector<FrameEnhanced>& frames) const
{
    pfs::Frame* frame = new pfs::Frame;
    computeFusion(frames, *frame);
    return frame;
}

FusionOperatorPtr IFusionOperator::build(FusionOperator type) {
    switch (type)
    {
    case ROBERTSON02_NEW_AUTO:
        return boost::make_shared<RobertsonOperatorAuto>();
        break;
    case ROBERTSON02_NEW:
        return boost::make_shared<RobertsonOperator>();
        break;
    case DEBEVEC_NEW:
    default:
        return boost::make_shared<DebevecOperator>();
        break;
    }
}

bool IFusionOperator::setResponseFunction(ResponseFunction responseFunction)
{
    switch (responseFunction) {
    case RESPONSE_GAMMA:
        m_response.reset(new ResponseGamma);
        break;
    case RESPONSE_LINEAR:
        m_response.reset(new ResponseLinear);
        break;
    case RESPONSE_LOG10:
        m_response.reset(new ResponseLog10);
        break;
    case RESPONSE_SRGB:
    default:
        m_response.reset(new ResponseSRGB);
        break;
    }
    return true;
}

bool IFusionOperator::setWeightFunction(WeightFunction weightFunction)
{
    switch (weightFunction) {
    case WEIGHT_TRIANGULAR:
        m_weight.reset(new WeightTriangular);
        break;
    case WEIGHT_PLATEAU:
        m_weight.reset(new WeightPlateau);
        break;
    case WEIGHT_GAUSSIAN:
    default:
        m_weight.reset(new WeightGaussian);
        break;
    }
    return true;
}

void fillDataLists(const vector<FrameEnhanced> &frames,
                   DataList& redChannels, DataList& greenChannels, DataList& blueChannels)
{
    assert(frames.size() == redChannels.size());
    assert(frames.size() == greenChannels.size());
    assert(frames.size() == blueChannels.size());

    // build temporary data structure
    for ( size_t exp = 0; exp < frames.size(); ++exp )
    {
        Channel* red;
        Channel* green;
        Channel* blue;
        frames[exp].frame()->getXYZChannels(red, green, blue);

        redChannels[exp] = red->data();
        greenChannels[exp] = green->data();
        blueChannels[exp] = blue->data();
    }
}

}   // fusion
}   // libhdr
