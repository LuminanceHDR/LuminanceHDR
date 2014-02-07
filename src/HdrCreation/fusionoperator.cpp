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
    : m_weight(new WeightGaussian)
{}

// TODO: fix this to return a shared_ptr
pfs::Frame* IFusionOperator::computeFusion(
        ResponseCurve& response, const std::vector<FrameEnhanced>& frames) const
{
    pfs::Frame* frame = new pfs::Frame;
    computeFusion(response, frames, *frame);
    return frame;
}

FusionOperatorPtr IFusionOperator::build(FusionOperator type) {
    switch (type)
    {
    case ROBERTSON_AUTO:
        return boost::make_shared<RobertsonOperatorAuto>();
        break;
    case ROBERTSON:
        return boost::make_shared<RobertsonOperator>();
        break;
    case DEBEVEC:
    default:
        return boost::make_shared<DebevecOperator>();
        break;
    }
}

//bool IFusionOperator::setResponseFunction(ResponseCurveType responseCurve)
//{
//    m_response->setType(responseCurve);
//    return true;
//}

//bool IFusionOperator::setResponseFunctionInputFile(const string &fileName)
//{
//    m_response->setType(RESPONSE_CUSTOM);
//    return m_response->readFromFile(fileName);
//}

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
