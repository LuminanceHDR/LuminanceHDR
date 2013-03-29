/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
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
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfOutputFile.h>
#include <ImfRgbaFile.h>
#include <ImfStringAttribute.h>
#include <ImfStandardAttributes.h>

#include <string>
#include <cmath>

#include <Libpfs/frame.h>
#include <Libpfs/io/exrwriter.h>

// #define min(x,y) ( (x)<(y) ? (x) : (y) )

using namespace Imf;
using namespace Imath;
using namespace std;

namespace pfs {
namespace io {

EXRWriter::EXRWriter(const string &filename)
    : FrameWriter(filename)
{}

bool EXRWriter::write(const Frame &frame, const Params &params)
{
    const pfs::Channel *R, *G, *B;
    // Channels are named X Y Z but contain R G B data
    frame.getXYZChannels(R, G, B);

    string luminanceTag = frame.getTags().getTag("LUMINANCE");

    Header header(frame.getWidth(),
                  frame.getHeight(),
                  1,                      // aspect ratio
                  Imath::V2f (0, 0),      // screenWindowCenter
                  1,                      // screenWindowWidth
                  INCREASING_Y,           // lineOrder
                  PIZ_COMPRESSION );

    // Define channels in Header
    header.channels().insert( "R", Imf::Channel(HALF) );
    header.channels().insert( "G", Imf::Channel(HALF) );
    header.channels().insert( "B", Imf::Channel(HALF) );

    // Copy tags to attributes
    pfs::TagContainer::const_iterator it = frame.getTags().begin();
    pfs::TagContainer::const_iterator itEnd = frame.getTags().end();

    for ( ; it != itEnd; ++it)
    {
        header.insert( it->first.c_str(), StringAttribute(it->second) );
    }

    // Copy all channel tags
    const pfs::ChannelContainer& channels = frame.getChannels();

    for (pfs::ChannelContainer::const_iterator ch = channels.begin();
         ch != channels.end();
         ++ch)
    {
        pfs::TagContainer::const_iterator it = (*ch)->getTags().begin();
        pfs::TagContainer::const_iterator itEnd = (*ch)->getTags().end();

        for ( ; it != itEnd; ++it ) {
            header.insert( string((*ch)->getName() + ":" + it->first).c_str(),
                           StringAttribute(it->second) );
        }
    }

    FrameBuffer frameBuffer;

    // Create channels in FrameBuffer
    std::vector<half> halfR( frame.getWidth()*frame.getHeight() );
    std::vector<half> halfG( frame.getWidth()*frame.getHeight() );
    std::vector<half> halfB( frame.getWidth()*frame.getHeight() );

    frameBuffer.insert( "R",                                      // name
                        Slice( HALF,                               // type
                               (char*)halfR.data(),                        // base
                               sizeof(half) * 1,                    // xStride
                               sizeof(half) * frame.getWidth()) );	// yStride

    frameBuffer.insert( "G",                                      // name
                        Slice( HALF,                               // type
                               (char*)halfG.data(),                        // base
                               sizeof(half) * 1,                    // xStride
                               sizeof(half) * frame.getWidth()) );	// yStride

    frameBuffer.insert( "B",                                      // name
                        Slice( HALF,                               // type
                               (char*)halfB.data(),                        // base
                               sizeof(half) * 1,                    // xStride
                               sizeof(half) * frame.getWidth()) );	// yStride

    int pixelCount = frame.getHeight()*frame.getWidth();

    // Check if pixel values do not exceed maximum HALF value
    float maxValue = -1;
    for( int i = 0; i < pixelCount; i++ )
    {
        if( (*R)(i) > maxValue ) maxValue = (*R)(i);
        if( (*G)(i) > maxValue ) maxValue = (*G)(i);
        if( (*B)(i) > maxValue ) maxValue = (*B)(i);
        // if( (*X)(i) > maxValue ) maxValue = (*X)(i);
        // if( (*Y)(i) > maxValue ) maxValue = (*Y)(i);
        // if( (*Z)(i) > maxValue ) maxValue = (*Z)(i);
    }

    bool maxHalfExceeded = maxValue > HALF_MAX;

    if ( maxHalfExceeded )
    {
        // Rescale and copy pixels to half-type buffers
        float scaleFactor = HALF_MAX/maxValue;
        for( int i = 0; i < pixelCount; i++ )
        {
            halfR[i] = (half)((*R)(i)*scaleFactor);
            halfG[i] = (half)((*G)(i)*scaleFactor);
            halfB[i] = (half)((*B)(i)*scaleFactor);
            // halfR[i] = (half)((*X)(i)*scaleFactor);
            // halfG[i] = (half)((*Y)(i)*scaleFactor);
            // halfB[i] = (half)((*Z)(i)*scaleFactor);
        }
        // Store scale factor as WhileLuminance standard sttribute
        // in order to restore absolute values later
        addWhiteLuminance( header, 1/scaleFactor );
    }
    else
    {
        // Copy pixels to half-type buffers
        for( int i = 0; i < pixelCount; i++ )
        {
            halfR[i] = std::min( (half)(*R)(i), (half)HALF_MAX );
            halfG[i] = std::min( (half)(*G)(i), (half)HALF_MAX );
            halfB[i] = std::min( (half)(*B)(i), (half)HALF_MAX );
            // halfR[i] = min( (*X)(i), HALF_MAX );
            // halfG[i] = min( (*Y)(i), HALF_MAX );
            // halfB[i] = min( (*Z)(i), HALF_MAX );
        }
        if ( !luminanceTag.empty() && luminanceTag != "ABSOLUTE" )
        {
            addWhiteLuminance( header, 1 );
        }
    }

    OutputFile file(filename().c_str(), header);
    file.setFrameBuffer( frameBuffer );
    file.writePixels( frame.getHeight() );

    return true;
}

}   // pfs
}   // io
