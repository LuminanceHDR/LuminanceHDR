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

#ifndef LIBHDR_FUSION_FUSIONOPERATOR_H
#define LIBHDR_FUSION_FUSIONOPERATOR_H

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date 28 April 2013
//! \brief This headers defines the base interface for the fusion operator
//! fusion operators take as input a set of images and trasform them into an hdr
//! merged image, ready for tonemap or other processing
//! \note This the first header written specifically for LibHDR (milestone!)

#include <HdrCreation/responses.h>
#include <HdrCreation/weights.h>
#include <Libpfs/frame.h>

namespace libhdr {
namespace fusion {

//! \brief This class contains a (shared) pointer to a frame, plus its average
//! luminance, to be used during the fusion process
class FrameEnhanced {
   public:
    FrameEnhanced(const pfs::FramePtr &frame, float averageLuminance)
        : m_frame(frame), m_averageLuminance(averageLuminance) {}

    const pfs::FramePtr &frame() const { return m_frame; }
    float averageLuminance() const { return m_averageLuminance; }

   private:
    pfs::FramePtr m_frame;
    float m_averageLuminance;
};

enum FusionOperator { DEBEVEC = 0, ROBERTSON = 1, ROBERTSON_AUTO = 2 };

class IFusionOperator;

typedef std::shared_ptr<IFusionOperator> FusionOperatorPtr;

class IFusionOperator {
   public:
    virtual ~IFusionOperator() {}

    //! \brief create an instance of the IFusionOperator from a member of
    //! the \c FusionOperator enum
    static FusionOperatorPtr build(FusionOperator type);

    //! \brief retrieve the right \c FusionOperator value for the input string.
    //! Valid values are "debevec", "robertson" and "robertson-auto"
    static FusionOperator fromString(const std::string &type);

    pfs::Frame *computeFusion(ResponseCurve &response, WeightFunction &weight,
                              const std::vector<FrameEnhanced> &frames);

    virtual FusionOperator getType() const = 0;

   protected:
    IFusionOperator();

    virtual void computeFusion(ResponseCurve &response, WeightFunction &weight,
                               const std::vector<FrameEnhanced> &frames,
                               pfs::Frame &outFrame) = 0;
};

typedef vector<float *> DataList;

void fillDataLists(const vector<FrameEnhanced> &frames, DataList &redChannels,
                   DataList &greenChannels, DataList &blueChannels);

}  // fusion
}  // libhdr

#endif  // LIBHDR_FUSION_FUSIONOPERATOR_H
