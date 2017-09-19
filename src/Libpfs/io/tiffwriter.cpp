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

#include <Libpfs/io/tiffcommon.h>
#include <Libpfs/io/tiffwriter.h>

#include <tiffio.h>

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#include <Libpfs/array2d.h>
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/colorspace/xyz.h>
#include <Libpfs/fixedstrideiterator.h>
#include <Libpfs/frame.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/utils/chain.h>
#include <Libpfs/utils/clamp.h>
#include <Libpfs/utils/resourcehandlerlcms.h>

using namespace std;
using namespace boost;
using namespace pfs;

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "TiffReader: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

namespace pfs {
namespace io {

struct TiffWriterParams {
    TiffWriterParams()
        : quality_(100),
          minLuminance_(0.f),
          maxLuminance_(1.f),
          luminanceMapping_(MAP_LINEAR),
          tiffWriterMode_(0)  // 8bit uint by default
          ,
          deflateCompression_(true) {}

    void parse(const Params &params) {
        for (Params::const_iterator it = params.begin(), itEnd = params.end();
             it != itEnd; ++it) {
            if (it->first == "quality") {
                quality_ = it->second.as<size_t>(quality_);
                continue;
            }
            if (it->first == "min_luminance") {
                minLuminance_ = it->second.as<float>(minLuminance_);
                continue;
            }
            if (it->first == "max_luminance") {
                maxLuminance_ = it->second.as<float>(maxLuminance_);
                continue;
            }
            if (it->first == "mapping_method") {
                luminanceMapping_ =
                    it->second.as<RGBMappingType>(luminanceMapping_);
                continue;
            }
            if (it->first == "tiff_mode") {
                tiffWriterMode_ = it->second.as<int>(tiffWriterMode_);
                continue;
            }
            if (it->first == "deflateCompression") {
                deflateCompression_ = it->second.as<bool>(deflateCompression_);
                // continue;
            }
        }
    }

    size_t quality_;
    float minLuminance_;
    float maxLuminance_;
    RGBMappingType luminanceMapping_;
    int tiffWriterMode_;
    bool deflateCompression_;
};

ostream &operator<<(ostream &out, const TiffWriterParams &params) {
    stringstream ss;
    ss << "TiffWriterParams: [";
    ss << "mode: " << params.tiffWriterMode_ << ", ";
    ss << "quality: " << params.quality_ << ", ";
    ss << "min_luminance: " << params.minLuminance_ << ", ";
    ss << "max_luminance: " << params.maxLuminance_ << ", ";
    ss << "mapping_method: " << params.luminanceMapping_ << "]";
    ss << "deflateCompression: " << params.deflateCompression_ << "]";

    return (out << ss.str());
}

void writeCommonHeader(TIFF *tif, uint32_t width, uint32_t height) {
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32_t)width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32_t)height);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, (uint32_t)1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
}

void writeSRGBProfile(TIFF *tif) {
    utils::ScopedCmsProfile hsRGB(cmsCreate_sRGBProfile());
    cmsUInt32Number profileSize = 0;
    cmsSaveProfileToMem(hsRGB.data(), NULL, &profileSize);  // get the size

    std::vector<char> embedBuffer(profileSize);

    cmsSaveProfileToMem(hsRGB.data(),
                        reinterpret_cast<void *>(embedBuffer.data()),
                        &profileSize);

    TIFFSetField(tif, TIFFTAG_ICCPROFILE, profileSize,
                 reinterpret_cast<void *>(embedBuffer.data()));
}

// Info: if you want to write the alpha channel, please use this!
//    uint16 extras[1] = { EXTRASAMPLE_ASSOCALPHA };
//    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)4);
//    TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, (uint16_t)1, &extras);

bool writeUint8(TIFF *tif, const Frame &frame, const TiffWriterParams &params) {
#ifndef NDEBUG
    cout << BOOST_CURRENT_FUNCTION << endl;
#endif

    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);
    writeSRGBProfile(tif);

    if (params.deflateCompression_)
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
                 (uint16_t)8 * (uint16_t)sizeof(uint8_t));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);

    tsize_t stripSize = TIFFStripSize(tif);
    assert(width * 3 == stripSize);
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel *rChannel;
    const Channel *gChannel;
    const Channel *bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    std::vector<uint8_t> stripBuffer(stripSize);
    for (tstrip_t s = 0; s < stripsNum; s++) {
        utils::transform(
            rChannel->row_begin(s), rChannel->row_end(s),
            gChannel->row_begin(s), bChannel->row_begin(s),
            FixedStrideIterator<uint8_t *, 3>(stripBuffer.data()),
            FixedStrideIterator<uint8_t *, 3>(stripBuffer.data() + 1),
            FixedStrideIterator<uint8_t *, 3>(stripBuffer.data() + 2),
            utils::chain(colorspace::Normalizer(params.minLuminance_,
                                                params.maxLuminance_),
                         utils::CLAMP_F32,
                         Remapper<uint8_t>(params.luminanceMapping_)));

        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) !=
            stripSize) {
            throw pfs::io::WriteException("TiffWriter: Error writing strip " +
                                          boost::lexical_cast<std::string>(s));
            return false;
        }
    }
    return true;
}

bool writeUint16(TIFF *tif, const Frame &frame,
                 const TiffWriterParams &params) {
#ifndef NDEBUG
    cout << BOOST_CURRENT_FUNCTION << endl;
#endif
    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);
    writeSRGBProfile(tif);

    if (params.deflateCompression_) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    }
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
                 (uint16_t)8 * (uint16_t)sizeof(uint16_t));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);

    tsize_t stripSize = TIFFStripSize(tif);
    assert(width * 3 * 2 == stripSize);
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel *rChannel;
    const Channel *gChannel;
    const Channel *bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    std::vector<uint16_t> stripBuffer(width * 3);

    typedef utils::Chain<colorspace::Normalizer,
                         utils::Chain<utils::Clamp<float>, Remapper<uint16_t>>>
        TiffRemapper;

    TiffRemapper remapper(
        colorspace::Normalizer(params.minLuminance_, params.maxLuminance_),
        utils::Chain<utils::Clamp<float>, Remapper<uint16_t>>(
            utils::Clamp<float>(0.f, 1.f),
            Remapper<uint16_t>(params.luminanceMapping_)));
    for (tstrip_t s = 0; s < stripsNum; s++) {
        utils::transform(
            rChannel->row_begin(s), rChannel->row_end(s),
            gChannel->row_begin(s), bChannel->row_begin(s),
            FixedStrideIterator<uint16_t *, 3>(stripBuffer.data()),
            FixedStrideIterator<uint16_t *, 3>(stripBuffer.data() + 1),
            FixedStrideIterator<uint16_t *, 3>(stripBuffer.data() + 2),
            remapper);
        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) !=
            stripSize) {
            throw pfs::io::WriteException("TiffWriter: Error writing strip " +
                                          boost::lexical_cast<std::string>(s));
            return false;
        }
    }

    return true;
}

// write 32 bit float Tiff from pfs::Frame ... to finish!
bool writeFloat32(TIFF *tif, const Frame &frame,
                  const TiffWriterParams &params) {
#ifndef NDEBUG
    cout << BOOST_CURRENT_FUNCTION << endl;
#endif
    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);
    // writeSRGBProfile(tif);

    if (params.deflateCompression_) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    }
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
                 (uint16_t)8 * (uint16_t)sizeof(float));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);

    tsize_t stripSize = TIFFStripSize(tif);
    assert((tsize_t)sizeof(float) * width * 3 == stripSize);
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel *rChannel;
    const Channel *gChannel;
    const Channel *bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    PRINT_DEBUG(params.minLuminance_);
    PRINT_DEBUG(params.maxLuminance_);

    std::vector<float> stripBuffer(width * 3);
    typedef utils::Chain<colorspace::Normalizer, utils::Clamp<float>>
        TiffRemapper;
    // Mapping is linear, so I avoid to call the Remapper class
    TiffRemapper remapper(
        colorspace::Normalizer(params.minLuminance_, params.maxLuminance_),
        utils::Clamp<float>(0.f, 1.f));
    for (tstrip_t s = 0; s < stripsNum; s++) {
        utils::transform(
            rChannel->row_begin(s), rChannel->row_end(s),
            gChannel->row_begin(s), bChannel->row_begin(s),
            FixedStrideIterator<float *, 3>(stripBuffer.data()),
            FixedStrideIterator<float *, 3>(stripBuffer.data() + 1),
            FixedStrideIterator<float *, 3>(stripBuffer.data() + 2), remapper);
        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) == 0) {
            throw pfs::io::WriteException("TiffWriter: Error writing strip " +
                                          boost::lexical_cast<std::string>(s));

            return -1;
        }
    }

    return true;
}

// write LogLUv Tiff from pfs::Frame
bool writeLogLuv(TIFF *tif, const Frame &frame,
                 const TiffWriterParams &params) {
#ifndef NDEBUG
    cout << BOOST_CURRENT_FUNCTION << endl;
#endif
    assert(tif != NULL);

    uint32_t width = frame.getWidth();
    uint32_t height = frame.getHeight();

    writeCommonHeader(tif, width, height);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LOGLUV);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,
                 (uint16_t)8 * (uint16_t)sizeof(float));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);
    TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
    TIFFSetField(tif, TIFFTAG_STONITS, 1.); /* not known */

    tsize_t stripSize = TIFFStripSize(tif);
    assert((tsize_t)sizeof(float) * width * 3 == stripSize);
    tstrip_t stripsNum = TIFFNumberOfStrips(tif);

    const Channel *rChannel;
    const Channel *gChannel;
    const Channel *bChannel;
    frame.getXYZChannels(rChannel, gChannel, bChannel);

    std::vector<float> stripBuffer(width * 3);

    // remap to [0, 1] + transform to colorspace XYZ
    // no gamma curve applied
    typedef utils::Chain<
        colorspace::Normalizer,
        utils::Chain<utils::Clamp<float>, colorspace::ConvertRGB2XYZ>>
        TiffRemapper;

    TiffRemapper remapper(
        colorspace::Normalizer(params.minLuminance_, params.maxLuminance_),
        utils::Chain<utils::Clamp<float>, colorspace::ConvertRGB2XYZ>(
            utils::Clamp<float>(0.f, 1.f), colorspace::ConvertRGB2XYZ()));

    for (tstrip_t s = 0; s < stripsNum; s++) {
        utils::transform(
            rChannel->row_begin(s), rChannel->row_end(s),
            gChannel->row_begin(s), bChannel->row_begin(s),
            FixedStrideIterator<float *, 3>(stripBuffer.data()),
            FixedStrideIterator<float *, 3>(stripBuffer.data() + 1),
            FixedStrideIterator<float *, 3>(stripBuffer.data() + 2), remapper);
        if (TIFFWriteEncodedStrip(tif, s, stripBuffer.data(), stripSize) !=
            stripSize) {
            throw pfs::io::WriteException("TiffWriter: Error writing strip " +
                                          boost::lexical_cast<std::string>(s));

            return false;
        }
    }

    return true;
}

TiffWriter::TiffWriter(const std::string &filename) : FrameWriter(filename) {}

TiffWriter::~TiffWriter() {}

bool TiffWriter::write(const pfs::Frame &frame, const pfs::Params &params) {
    TiffWriterParams p;
    p.parse(params);

#ifndef NDEBUG
    cout << p << endl;
#endif

    ScopedTiffFile tif(TIFFOpen(filename().c_str(), "w"));
    if (!tif) {
        throw pfs::io::InvalidFile("TiffWriter: cannot open " + filename());
    }

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

    return status;
}

}  // io
}  // pfs
