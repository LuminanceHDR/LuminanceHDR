/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006 Giuseppe Rota
 * Copyright (C) 2012-2013 Davide Anastasia
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

//! \brief TIFF facilities
//! \author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
//! Original author for PFSTOOLS
//! \author Giuseppe Rota <grota@sourceforge.net>
//! slightly modified by for Luminance HDR
//! \author Franco Comida <fcomida@sourceforge.net>
//! added color management support by Franco Comida
//! \author Davide Anastasia <davideanastasia@sourceforge.net>
//! Complete rewrite/refactoring

#include "tiffwriter.h"
#include <Fileformat/tiffcommon.h>
#include <tiffio.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <limits>
#include <algorithm>
#include <stdint.h>

#include <Libpfs/utils/resourcehandlerlcms.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/colorspace/xyz.h>
#include <Fileformat/pfsoutldrimage.h>
#include <Libpfs/frame.h>
#include <Libpfs/array2d.h>

using namespace std;
using namespace boost;
using namespace pfs;

struct TiffWriterParams
{
    TiffWriterParams()
        : quality_(100)
        , minLuminance_(0.f)
        , maxLuminance_(1.f)
        , luminanceMapping_(MAP_LINEAR)
        , tiffWriterMode_(0)        // 8bit uint by default
    {}

    void parse(const Params& params)
    {
        for ( Params::const_iterator it = params.begin(), itEnd = params.end();
              it != itEnd; ++it )
        {
            if ( it->first == "quality" ) {
                quality_ = it->second.as<size_t>(quality_);
                continue;
            }
            if ( it->first == "min_luminance" ) {
                minLuminance_ = it->second.as<float>(minLuminance_);
                continue;
            }
            if ( it->first == "max_luminance" ) {
                maxLuminance_ = it->second.as<float>(maxLuminance_);
                continue;
            }
            if ( it->first == "mapping_method" ) {
                luminanceMapping_ = it->second.as<RGBMappingType>(luminanceMapping_);
                continue;
            }
            if ( it->first == "tiff_mode" ) {
                tiffWriterMode_ = it->second.as<int>(tiffWriterMode_);
            }
        }
    }

    size_t quality_;
    float minLuminance_;
    float maxLuminance_;
    RGBMappingType luminanceMapping_;
    int tiffWriterMode_;
};

ostream& operator<<(ostream& out, const TiffWriterParams& params)
{
    stringstream ss;
    ss << "TiffWriterParams: [";
    ss << "mode: " << params.tiffWriterMode_ << ", ";
    ss << "quality: " << params.quality_ << ", ";
    ss << "min_luminance: " << params.minLuminance_ << ", ";
    ss << "max_luminance: " << params.maxLuminance_ << ", ";
    ss << "mapping_method: " << params.luminanceMapping_ << "]";

    return (out << ss.str());
}

void writeCommonHeader(TIFF* tif, uint32_t width, uint32_t height)
{
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32_t)width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32_t)height);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, (uint32_t)1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
}

void writeSRGBProfile(TIFF* tif)
{
    utils::ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );
    cmsUInt32Number profileSize = 0;
    cmsSaveProfileToMem(hsRGB.data(), NULL, &profileSize);	// get the size

    std::vector<char> embedBuffer(profileSize);

    cmsSaveProfileToMem(hsRGB.data(),
                        reinterpret_cast<void*>(embedBuffer.data()),
                        &profileSize);

    TIFFSetField(tif, TIFFTAG_ICCPROFILE, profileSize,
                 reinterpret_cast<void*>(embedBuffer.data()) );
}

// Info: if you want to write the alpha channel, please use this!
//    uint16 extras[1] = { EXTRASAMPLE_ASSOCALPHA };
//    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)4);
//    TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, (uint16_t)1, &extras);

bool writeUint8(TIFF* tif, const Frame& frame, const TiffWriterParams& params)
{
#ifndef NDEBUG
    cout << __func__ << endl;
#endif

    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);
    writeSRGBProfile(tif);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)8*(uint16_t)sizeof(uint8_t));
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);

    tsize_t stripSize = TIFFStripSize(tif);
    assert( width*3 == stripSize );
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel* rChannel;
    const Channel* gChannel;
    const Channel* bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    const float* redData = rChannel->getRawData();
    const float* greenData = gChannel->getRawData();
    const float* blueData = bChannel->getRawData();

    std::vector<uint8_t> stripBuffer( stripSize );
    RGBRemapper rgbRemapper(params.minLuminance_, params.maxLuminance_,
                                params.luminanceMapping_);
    for (tstrip_t s = 0; s < stripsNum; s++)
    {
        planarToInterleaved(redData + s*width,
                            greenData + s*width,
                            blueData + s*width,
                            stripBuffer.data(), RGB_FORMAT, width,
                            rgbRemapper);

        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) != stripSize)
        {
            qDebug ("error writing strip");
            return false;
        }
    }
    return true;
}

bool writeUint16(TIFF* tif, const Frame& frame, const TiffWriterParams& params)
{
#ifndef NDEBUG
    cout << __func__ << endl;
#endif
    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);
    writeSRGBProfile(tif);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)8*(uint16_t)sizeof(uint16_t));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);

    tsize_t stripSize = TIFFStripSize(tif);
    assert( width*3*2 == stripSize );
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel* rChannel;
    const Channel* gChannel;
    const Channel* bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    const float* redData = rChannel->getRawData();
    const float* greenData = gChannel->getRawData();
    const float* blueData = bChannel->getRawData();

    std::vector<uint16_t> stripBuffer( width*3 );
    RGBRemapper rgbRemapper(params.minLuminance_, params.maxLuminance_,
                                params.luminanceMapping_);

    for (tstrip_t s = 0; s < stripsNum; s++)
    {
        planarToInterleaved(redData + s*width,
                            greenData + s*width,
                            blueData + s*width,
                            stripBuffer.data(), RGB_FORMAT, width,
                            rgbRemapper);
        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) != stripSize)
        {
            qDebug ("error writing strip");
            return false;
        }
    }

    return true;
}

// write 32 bit float Tiff from pfs::Frame ... to finish!
bool writeFloat32(TIFF* tif, const Frame& frame, const TiffWriterParams& params)
{
#ifndef NDEBUG
    cout << __func__ << endl;
#endif
    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);
    // writeSRGBProfile(tif);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)8*(uint16_t)sizeof(float));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);

    tsize_t stripSize = TIFFStripSize(tif);
    assert( sizeof(float)*width*3 == stripSize );
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel* rChannel;
    const Channel* gChannel;
    const Channel* bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    const float* redData = rChannel->getRawData();
    const float* greenData = gChannel->getRawData();
    const float* blueData = bChannel->getRawData();

    std::vector<float> stripBuffer( width*3 );
    RGBRemapper rgbRemapper(params.minLuminance_, params.maxLuminance_,
                            MAP_LINEAR); // maybe I have to force to be linear?!
    for (tstrip_t s = 0; s < stripsNum; s++)
    {
        planarToInterleaved(redData + s*width,
                            greenData + s*width,
                            blueData + s*width,
                            stripBuffer.data(), RGB_FORMAT, width,
                            rgbRemapper);
        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) == 0)
        {
            qDebug ("error writing strip");

            return -1;
        }
    }

    return true;
}

#include <Libpfs/utils/chain.h>

// write LogLUv Tiff from pfs::Frame
bool writeLogLuv(TIFF* tif, const Frame& frame, const TiffWriterParams& params)
{
#ifndef NDEBUG
    cout << __func__ << endl;
#endif
    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LOGLUV);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)8*(uint16_t)sizeof(float));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);
    TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
    TIFFSetField(tif, TIFFTAG_STONITS, 1.);	/* not known */

    tsize_t stripSize = TIFFStripSize(tif);
    assert( sizeof(float)*width*3 == stripSize );
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel* rChannel;
    const Channel* gChannel;
    const Channel* bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    const float* redData = rChannel->getRawData();
    const float* greenData = gChannel->getRawData();
    const float* blueData = bChannel->getRawData();

    std::vector<float> stripBuffer( width*3 );
#ifndef NDEBUG
    float* p = stripBuffer.data();
#endif

    utils::Chain<RGBRemapper, colorspace::ConvertRGB2XYZ>
            // remap to [0, 1] + transform to colorspace XYZ
            func(RGBRemapper(params.minLuminance_, params.maxLuminance_, MAP_LINEAR),
                 colorspace::ConvertRGB2XYZ());

     // maybe I have to force to be linear?!
    for (tstrip_t s = 0; s < stripsNum; s++)
    {
        utils::transform(redData + s*width, redData + (s+1)*width,
                         greenData + s*width,
                         blueData + s*width,
                         stripBuffer.data(),
                         stripBuffer.data()+1,
                         stripBuffer.data()+2,
                         func, 3);
#ifndef NDEBUG
        assert(p == stripBuffer.data());
#endif
        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) != stripSize)
        {
            qDebug ("error writing strip");

            return -1;
        }
    }

    return true;
}

TiffWriter::TiffWriter(const std::string& filename)
    : m_filename(filename)
{}

TiffWriter::~TiffWriter()
{}

bool TiffWriter::write(const pfs::Frame& frame, const pfs::Params& params)
{
    TiffWriterParams p;
    p.parse(params);

#ifndef NDEBUG
    cout << p << endl;
#endif

    ScopedTiffFile tif( TIFFOpen (m_filename.c_str(), "w") );

    bool status = true;
    switch (p.tiffWriterMode_) {
    case 1:
        status = writeUint16(tif.data(), frame, p);
        break;
    case 2:
        status = writeFloat32(tif.data(), frame, p);
        break;
    case 3:
        status = writeLogLuv(tif.data(), frame, p);
        break;
    case 0:
    default:
        status = writeUint8(tif.data(), frame, p);
        break;
    }

    assert( status == true );
    return status;
}
