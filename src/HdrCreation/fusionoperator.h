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

#include <boost/scoped_ptr.hpp>

#include <Libpfs/frame.h>
#include <HdrCreation/responses.h>
#include <HdrCreation/weights.h>

namespace libhdr {
namespace fusion {

//! \brief This class contains a (shared) pointer to a frame, plus its average
//! luminance, to be used during the fusion process
class FrameEnhanced
{
public:
    FrameEnhanced(const pfs::FramePtr& frame, float averageLuminance)
        : m_frame(frame)
        , m_averageLuminance(averageLuminance)
    {}

    const pfs::FramePtr& frame() const { return m_frame; }
    float averageLuminance() const { return m_averageLuminance; }

private:
    pfs::FramePtr m_frame;
    float m_averageLuminance;
};

enum FusionOperator
{
    DEBEVEC = 0,
    ROBERTSON = 1,
    ROBERTSON_AUTO = 2
};

class IFusionOperator;

typedef boost::shared_ptr<IFusionOperator> FusionOperatorPtr;

class IFusionOperator
{
public:
    virtual ~IFusionOperator() {}

    //! \brief create an instance of the IFusionOperator from a member of
    //! the \c FusionOperator enum
    static FusionOperatorPtr build(FusionOperator type);
    // //! \brief create an instance of the \c IFusionOperator from a string. Valid
    // //! values are "debevec" and "robertson02". Useful in a CLI interface
    // static FusionOperatorPtr build(const std::string& name);

    bool setResponseFunction(ResponseFunction responseFunction);
    bool setResponseFunctionInputFile(const std::string& fileName);
    ResponseFunction getResponseFunction() const { return m_response->getType(); }

    bool setWeightFunction(WeightFunction weightFunction);
    WeightFunction getWeightFunction() const { return m_weight->getType(); }

    pfs::Frame* computeFusion(const std::vector<FrameEnhanced>& frames) const;

    void writeResponsesToFile(const std::string& fileName) const
    {
        m_response->writeToFile(fileName);
    }

protected:
    IFusionOperator();

    inline float response(float in, ResponseChannel channel = RESPONSE_CHANNEL_RED) const
    { return m_response->getResponse(in, channel); }

    virtual void computeFusion(const std::vector<FrameEnhanced>& frames, pfs::Frame& outFrame) const = 0;

    inline float weight(float in) const { return m_weight->getWeight(in); }
    inline float minTrustedValue() const  { return m_weight->minTrustedValue(); }
    inline float maxTrustedValue() const  { return m_weight->maxTrustedValue(); }

    boost::scoped_ptr<IResponseFunction> m_response;
    boost::scoped_ptr<IWeightFunction> m_weight;
};

typedef vector<float*> DataList;

void fillDataLists(const vector<FrameEnhanced> &frames,
                   DataList& redChannels, DataList& greenChannels, DataList& blueChannels);

}   // fusion
}   // libhdr

#endif // LIBHDR_FUSION_FUSIONOPERATOR_H
