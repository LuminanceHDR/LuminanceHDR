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

#include <boost/assign.hpp>
#include <cassert>
#include <map>

#include <Libpfs/frame.h>
#include <Libpfs/utils/string.h>

using namespace pfs;
using namespace std;
using namespace boost::assign;

namespace libhdr {
namespace fusion {

IFusionOperator::IFusionOperator() {}

// TODO: fix this to return a shared_ptr
pfs::Frame *IFusionOperator::computeFusion(
    ResponseCurve &response, WeightFunction &weight,
    const std::vector<FrameEnhanced> &frames) {
    pfs::Frame *frame = new pfs::Frame;
    computeFusion(response, weight, frames, *frame);
    return frame;
}

FusionOperatorPtr IFusionOperator::build(FusionOperator type) {
    switch (type) {
        case ROBERTSON_AUTO:
            return std::make_shared<RobertsonOperatorAuto>();
            break;
        case ROBERTSON:
            return std::make_shared<RobertsonOperator>();
            break;
        case DEBEVEC:
        default:
            return std::make_shared<DebevecOperator>();
            break;
    }
}

FusionOperator IFusionOperator::fromString(const std::string &type) {
    typedef map<string, FusionOperator, pfs::utils::StringUnsensitiveComp> Dict;
    static Dict v = map_list_of("debevec", DEBEVEC)("robertson", ROBERTSON)(
        "robertson-auto", ROBERTSON_AUTO);

    Dict::const_iterator it = v.find(type);
    if (it != v.end()) {
        return it->second;
    }
    return DEBEVEC;
}

void fillDataLists(const vector<FrameEnhanced> &frames, DataList &redChannels,
                   DataList &greenChannels, DataList &blueChannels) {
    assert(frames.size() == redChannels.size());
    assert(frames.size() == greenChannels.size());
    assert(frames.size() == blueChannels.size());

    // build temporary data structure
    for (size_t exp = 0; exp < frames.size(); ++exp) {
        Channel *red;
        Channel *green;
        Channel *blue;
        frames[exp].frame()->getXYZChannels(red, green, blue);

        redChannels[exp] = red->data();
        greenChannels[exp] = green->data();
        blueChannels[exp] = blue->data();
    }
}

}  // fusion
}  // libhdr
