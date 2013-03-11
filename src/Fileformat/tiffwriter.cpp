/**
 * @brief Tiff facilities
 *
 * This file is a part of LuminanceHDR package.
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
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 * slightly modified by Giuseppe Rota <grota@sourceforge.net> for Luminance HDR
 * added color management support by Franco Comida <fcomida@sourceforge.net>
 */

#include "tiffwriter.h"
#include <tiffio.h>

//#include <QObject>
//#include <QSysInfo>
//#include <QFileInfo>
//#include <QFile>
//#include <QDebug>

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <limits>
#include <algorithm>
#include <stdint.h>

#include <Common/ResourceHandlerLcms.h>
#include <Libpfs/colorspace/rgbremapper.h>
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
        , tiffWriterMode_(2)        // 8bit uint
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
    ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );
    cmsUInt32Number profileSize = 0;
    cmsSaveProfileToMem (hsRGB.data(), NULL, &profileSize);	// get the size

    std::vector<char> embedBuffer(profileSize);

    cmsSaveProfileToMem(hsRGB.data(),
                        reinterpret_cast<void*>(embedBuffer.data()),
                        &profileSize);

    TIFFSetField(tif, TIFFTAG_ICCPROFILE, profileSize,
                 reinterpret_cast<void*>(embedBuffer.data()) );
}

//TiffWriter::TiffWriter(const char *filename, pfs::Frame* f):
//    tif(TIFFOpen (filename, "w")),
//    ldrimage(0),
//    pixmap(0),
//    pfsFrame(f),
//    width(f->getWidth()),
//    height(f->getHeight())
//{
//    if (!tif)
//    {
//        throw std::runtime_error ("TIFF: could not open file for writing.");
//    }

//    writeCommonHeader();

//    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);
//}

//TiffWriter::TiffWriter(const char *filename, const quint16 * pix, int w, int h):
//    tif(TIFFOpen (filename, "w")),
//    ldrimage(0),
//    pixmap(pix),
//    pfsFrame(0),
//    width(w),
//    height(h)
//{
//    if (!tif)
//    {
//        throw std::runtime_error ("TIFF: could not open file for writing.");
//    }

//    writeCommonHeader();

//    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);
//}

//TiffWriter::TiffWriter(const char *filename, QImage * f):
//    tif(TIFFOpen (filename, "w")),
//    ldrimage(f),
//    pixmap(0),
//    pfsFrame(0),
//    width(f->width()),
//    height(f->height())
//{
//    if (!tif)
//    {
//        throw std::runtime_error ("TIFF: could not open file for writing.");
//    }

//    writeCommonHeader();

//    uint16 extras[1];
//    extras[0] = EXTRASAMPLE_ASSOCALPHA;

//    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)4);
//    TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, (uint16_t)1, &extras);
//}

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
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(uint8_t));
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
    for (unsigned int s = 0; s < stripsNum; s++)
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
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(uint16_t));
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

    for (unsigned int s = 0; s < stripsNum; s++)
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
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(float));
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
    RGBRemapper rgbRemapper(params.minLuminance_,
                                params.maxLuminance_,
                                params.luminanceMapping_); // maybe I have to force to be linear?!
    for (unsigned int s = 0; s < stripsNum; s++)
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

// write LogLUv Tiff from pfs::Frame
// bool writeLogLuvTiff()
// {
//    if ( !pfsFrame )
//    {
//        std::runtime_error("TiffWriter: Invalid writeLogLuvTiff function, "\
//                           "pfsFrame is not set correctly");
//    }

//    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG);
//    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LOGLUV);
//    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
//    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
//                  (uint16_t)8*(uint16_t)sizeof(float));
//    TIFFSetField (tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
//    TIFFSetField (tif, TIFFTAG_STONITS, 1.);	/* not known */

//    pfs::Channel* Xc;
//    pfs::Channel* Yc;
//    pfs::Channel* Zc;

//    pfsFrame->getXYZChannels(Xc, Yc, Zc);

//    const float *X = Xc->getRawData();
//    const float *Y = Yc->getRawData();
//    const float *Z = Zc->getRawData();

//    tsize_t strip_size = TIFFStripSize (tif);
//    tstrip_t strips_num = TIFFNumberOfStrips (tif);
//    float *strip_buf = (float *) _TIFFmalloc (strip_size);	// enough space for a strip
//    if (!strip_buf)
//    {
//		TIFFClose(tif);
//        throw std::runtime_error ("TIFF: error allocating buffer.");
//    }

//    for (unsigned int s = 0; s < strips_num; s++)
//    {
//        for (unsigned int col = 0; col < width; col++)
//        {
//            strip_buf[3 * col + 0] = X[s * width + col];	//(*X)(col,s);
//            strip_buf[3 * col + 1] = Y[s * width + col];	//(*Y)(col,s);
//            strip_buf[3 * col + 2] = Z[s * width + col];	//(*Z)(col,s);
//        }
//        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
//        {
//            qDebug ("error writing strip");
//			TIFFClose(tif);
//            return -1;
//        }
//    }
//    _TIFFfree (strip_buf);
//	TIFFClose(tif);
//
//    return 0;
// }

namespace
{
// It can be done much better, but we can keep it for a bit :)
struct ComputeMinMax
{
    ComputeMinMax()
        : m_min( std::numeric_limits<float>::max() )
        , m_max( -std::numeric_limits<float>::max() )
    {}

    void operator()(const float& value)
    {
        if ( value > m_max ) m_max = value;
        else if ( value < m_min ) m_min = value;
    }

    inline
    float min()
    { return m_min; }

    inline
    float max()
    { return m_max; }

private:
    float m_min;
    float m_max;
};

struct Normalizer
{
    Normalizer(float min, float max)
        : m_min(min + std::numeric_limits<float>::epsilon())
        // , m_max(max)
        , m_normalizer( max - m_min )
    {}

    float operator()(const float& value) const
    {
        return (value - m_min)/m_normalizer;
    }
private:
    float m_min;
    // float m_max;
    float m_normalizer;
};

//    ComputeMinMax xMinMax = std::for_each(X, X + numPixels, ComputeMinMax());
//    ComputeMinMax yMinMax = std::for_each(Y, Y + numPixels, ComputeMinMax());
//    ComputeMinMax zMinMax = std::for_each(Z, Z + numPixels, ComputeMinMax());

//    Normalizer normalizer(std::min(xMinMax.min(),
//                                   std::min(yMinMax.min(),
//                                            zMinMax.min())),
//                          std::max(xMinMax.max(),
//                                   std::max(yMinMax.max(),
//                                            zMinMax.max())));

}

// double check this? (Sept 24, 2012)
// bool writePFSFrame16bitTiff()
// {
//    assert (pfsFrame != 0);

//    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);	// TODO what about others?
//    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
//    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);

//    pfs::Channel* Xc;
//    pfs::Channel* Yc;
//    pfs::Channel* Zc;

//    pfsFrame->getXYZChannels(Xc, Yc, Zc);

//    const float *X = Xc->getRawData();
//    const float *Y = Yc->getRawData();
//    const float *Z = Zc->getRawData();

//    tsize_t strip_size = TIFFStripSize (tif);
//    tstrip_t strips_num = TIFFNumberOfStrips (tif);
//    quint16 *strip_buf = (quint16 *) _TIFFmalloc (strip_size);	// enough space for a strip (row)
//    if (!strip_buf)
//    {
//		TIFFClose(tif);
//        throw std::runtime_error ("TIFF: error allocating buffer.");
//    }

//    for (unsigned int s = 0; s < strips_num; s++)
//    {
//        for (unsigned int col = 0; col < width; col++)
//        {
//            strip_buf[3 * col + 0] = (qint16) X[s * width + col];	//(*X)(col,s);
//            strip_buf[3 * col + 1] = (qint16) Y[s * width + col];	//(*Y)(col,s);
//            strip_buf[3 * col + 2] = (qint16) Z[s * width + col];	//(*Z)(col,s);
//        }
//        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
//        {
//            qDebug ("error writing strip");
//			TIFFClose(tif);

//            return -1;
//        }
//    }
//    _TIFFfree (strip_buf);
//	TIFFClose(tif);

//    return 0;
//}

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

    // get handle to file ... better done with a smart pointer!
    TIFF* tif = TIFFOpen (m_filename.c_str(), "w");

    bool status = true;
    switch (p.tiffWriterMode_) {
    case 1:
        status = writeUint16(tif, frame, p);
        break;
    case 2:
        status = writeFloat32(tif, frame, p);
        break;
    case 3:
//        status = writeLogLuvTiff();
//        break;
    case 0:
    default:
        status = writeUint8(tif, frame, p);
        break;
    }
    TIFFClose(tif);
    assert( status == true );
    return status;
}
