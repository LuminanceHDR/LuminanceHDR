/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \date October 20th, 2012

#include <boost/program_options.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include <iostream>
#include <cstddef>
#include <limits>
#include <iosfwd>
#include <cfloat>
#include "arch/math.h"
#include <sstream>

#include <Libpfs/frame.h>
#include <Libpfs/io/framereaderfactory.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;
namespace po = boost::program_options;

void getParameters(int argc, char** argv, std::string& input)
{
    po::options_description desc("Allowed options: ");
    desc.add_options()
            ("input,i", po::value<std::string>(&input)->required(), "input file")
            ;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
}

class ImageInspectorStats
{
public:
    ImageInspectorStats()
        : m_parsedSamples(0)
        , m_max(-std::numeric_limits<float>::max())
        , m_min(std::numeric_limits<float>::max())
        , m_numInf(0)
        , m_numNan(0)
    {}

    void operator()(float value)
    {
        m_parsedSamples++;

        if ( boost::math::isnan(value) )
        {
            m_numNan++;
            return;
        }

        if ( !finite(value) )
        {
            m_numInf++;
            return;
        }

        if ( value > m_max )
            m_max = value;
        else if ( value < m_min )
            m_min = value;
    }

    friend std::ostream& operator<<(std::ostream& out, const ImageInspectorStats& stats);

private:
    size_t m_parsedSamples;

    float m_max;
    float m_min;

    size_t m_numInf;
    size_t m_numNan;
};

std::ostream& operator<<(std::ostream& out, const ImageInspectorStats& stats)
{
    stringstream ss;

    ss << "{ Samples: " << stats.m_parsedSamples << ", ";
    ss << "Min: " << stats.m_min << ", ";
    ss << "Max: " << stats.m_max << ", ";
    ss << "NaN: " << stats.m_numNan << ", ";
    ss << "Inf: " << stats.m_numInf << "}";

    return (out << ss.str());
}

int main(int argc, char ** argv)
{
    std::string filename;
    getParameters(argc, argv, filename);

    Frame myFrame(0,0);

    FrameReaderPtr reader = FrameReaderFactory::open(filename);
    reader->read(myFrame, pfs::Params());
    reader->close();

    // print stats
    std::cout << "Size: "
              << myFrame.getWidth() << " x "
              << myFrame.getHeight() << endl;

    ChannelContainer& channels = myFrame.getChannels();
    std::cout << "Channels: " << channels.size() << endl;

    for (ChannelContainer::const_iterator it = channels.begin(), itEnd = channels.end();
         it != itEnd;
         ++it)
    {
        const float* dataStart = (*it)->data();
        const float* dataEnd = dataStart + ((*it)->getWidth()*(*it)->getHeight());

        ImageInspectorStats t = std::for_each(dataStart, dataEnd, ImageInspectorStats());

        std::cout << "Channel: " << (*it)->getName() << std::endl;
        std::cout << t << std::endl;
    }

    return 0;
}
