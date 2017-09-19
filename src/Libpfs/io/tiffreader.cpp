/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
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

#include <Libpfs/io/tiffcommon.h>
#include <Libpfs/io/tiffreader.h>

#include <Libpfs/fixedstrideiterator.h>
#include <Libpfs/frame.h>
#include <Libpfs/strideiterator.h>

#include <Libpfs/colorspace/cmyk.h>
#include <Libpfs/colorspace/copy.h>
#include <Libpfs/colorspace/lcms.h>
#include <Libpfs/colorspace/xyz.h>

#include <Libpfs/utils/resourcehandlerlcms.h>
#include <Libpfs/utils/transform.h>

#include <tiffio.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

using namespace pfs;
using namespace pfs::utils;
using namespace boost::assign;

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "TiffReader: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

//! \brief This code is taken from tificc.c from libcms distribution and
//! sligthly modified
//! \ref
//! http://svn.ghostscript.com/ghostscript/trunk/gs/lcms2/utils/tificc/tificc.c
static cmsHPROFILE GetTIFFProfile(TIFF *in, uint16 bps) {
    cmsHPROFILE hProfile;
    void *iccProfilePtr;
    cmsUInt32Number iccProfileSize;

    if (TIFFGetField(in, TIFFTAG_ICCPROFILE, &iccProfileSize, &iccProfilePtr)) {
        PRINT_DEBUG("iccProfileSize: " << iccProfileSize);
        hProfile = cmsOpenProfileFromMem(iccProfilePtr, iccProfileSize);

        if (hProfile) return hProfile;
    }

    PRINT_DEBUG("No color profile");

    // Try to see if "colorimetric" tiff.data()
    cmsCIExyYTRIPLE primaries;
    cmsCIExyY whitePoint;
    cmsToneCurve *curve[3];

    cmsFloat32Number *chr;
    if (!TIFFGetField(in, TIFFTAG_PRIMARYCHROMATICITIES, &chr)) {
        return 0;
    }
    primaries.Red.x = chr[0];
    primaries.Red.y = chr[1];
    primaries.Green.x = chr[2];
    primaries.Green.y = chr[3];
    primaries.Blue.x = chr[4];
    primaries.Blue.y = chr[5];

    PRINT_DEBUG(primaries.Red.x);
    PRINT_DEBUG(primaries.Red.y);
    PRINT_DEBUG(primaries.Green.x);
    PRINT_DEBUG(primaries.Green.y);
    PRINT_DEBUG(primaries.Blue.x);
    PRINT_DEBUG(primaries.Blue.y);

    primaries.Red.Y = primaries.Green.Y = primaries.Blue.Y = 1.0;

    cmsFloat32Number *wp;
    if (!TIFFGetField(in, TIFFTAG_WHITEPOINT, &wp)) {
        return 0;
    }

    whitePoint.x = wp[0];
    whitePoint.y = wp[1];
    whitePoint.Y = 1.0;

    PRINT_DEBUG(whitePoint.x);
    PRINT_DEBUG(whitePoint.y);

    // Transferfunction is a bit harder....
    // cmsUInt16Number gmr[1 << bps];
    // cmsUInt16Number gmg[1 << bps];
    // cmsUInt16Number gmb[1 << bps];

    std::vector<cmsUInt16Number> gmr(1 << bps);
    std::vector<cmsUInt16Number> gmg(1 << bps);
    std::vector<cmsUInt16Number> gmb(1 << bps);

    // TIFFGetFieldDefaulted(in, TIFFTAG_TRANSFERFUNCTION, gmr, gmg, gmb);
    TIFFGetFieldDefaulted(in, TIFFTAG_TRANSFERFUNCTION, gmr.data(), gmg.data(),
                          gmb.data());

    // curve[0] = cmsBuildTabulatedToneCurve16(NULL, 1 << bps, gmr);
    // curve[1] = cmsBuildTabulatedToneCurve16(NULL, 1 << bps, gmg);
    // curve[2] = cmsBuildTabulatedToneCurve16(NULL, 1 << bps, gmb);
    curve[0] = cmsBuildTabulatedToneCurve16(NULL, 1 << bps, gmr.data());
    curve[1] = cmsBuildTabulatedToneCurve16(NULL, 1 << bps, gmg.data());
    curve[2] = cmsBuildTabulatedToneCurve16(NULL, 1 << bps, gmb.data());

    hProfile = cmsCreateRGBProfile(&whitePoint, &primaries, curve);

    cmsFreeToneCurve(curve[0]);
    cmsFreeToneCurve(curve[1]);
    cmsFreeToneCurve(curve[2]);

    return hProfile;
}
// End of code form tifficc.c

namespace pfs {
namespace io {

struct TiffReaderParams {};

struct TiffReaderData {
    // < photometric type, bits per sample >
    typedef boost::function<void(TiffReaderData *, Frame &,
                                 const TiffReaderParams &)>
        Callback;

    TiffReaderData()
        : hasAlpha_(false),
          stonits_(1.0),
          currentCallback_(boost::bind(&TiffReaderData::doNothing, _1, _2, _3)),
          hsRGB_(cmsCreate_sRGBProfile()) {}

    // public members...
    ScopedTiffFile file_;

    uint32 height_;
    uint32 width_;

    uint16 compressionType_;  // compression type
    uint16 photometricType_;  // type of photometric data

    uint16 bitsPerSample_;    // bits per sample
    uint16 samplesPerPixel_;  // number of channels in tiff file (only 1-3 are
                              // used)
    bool hasAlpha_;

    double stonits_;  // scale factor to get nit values

    Callback currentCallback_;

    ScopedCmsProfile hsRGB_;  // (  );
    ScopedCmsProfile hIn_;    // ( GetTIFFProfile(tif) );

    // public functions
    inline TIFF *handle() { return file_.data(); }

    void read(Frame &frame, const Params & /*params*/) {
        currentCallback_(this, frame, TiffReaderParams());
    }

    void initReader() {
        typedef std::pair<uint16, uint16> RegistryKey;
        typedef std::map<RegistryKey, Callback> Registry;

        /*static*/ Registry sm_registry = map_list_of
            // (RegistryKey(PHOTOMETRIC_LOGLUV, 32),
            // boost::bind(&TiffReaderData::readLogLuv, _1, _2, _3)) // <
            (RegistryKey(PHOTOMETRIC_LOGLUV, 16),
             boost::bind(&TiffReaderData::readLogLuv, _1, _2, _3))(
                RegistryKey(PHOTOMETRIC_SEPARATED, 8),
                boost::bind(&TiffReaderData::readCMYK<uint8_t>, _1, _2, _3))(
                RegistryKey(PHOTOMETRIC_SEPARATED, 16),
                boost::bind(&TiffReaderData::readCMYK<uint16_t>, _1, _2, _3))
            // (RegistryKey(PHOTOMETRIC_SEPARATED, 32),
            // boost::bind(&TiffReaderData::readCMYK<float>, _1, _2, _3))
            (RegistryKey(PHOTOMETRIC_RGB, 8),
             boost::bind(&TiffReaderData::readRGB<uint8_t>, _1, _2, _3))(
                RegistryKey(PHOTOMETRIC_RGB, 16),
                boost::bind(&TiffReaderData::readRGB<uint16_t>, _1, _2, _3))(
                RegistryKey(PHOTOMETRIC_RGB, 32),
                boost::bind(&TiffReaderData::readRGB<float>, _1, _2, _3));

        Registry::iterator it =
            sm_registry.find(RegistryKey(photometricType_, bitsPerSample_));
        if (it == sm_registry.end()) {
            throw pfs::io::InvalidHeader(
                std::string("TiffReader: cannot find the right function to "
                            "parse the data ") +
                "bits per sample: " +
                boost::lexical_cast<std::string>(bitsPerSample_) +
                " photometric type: " +
                boost::lexical_cast<std::string>(photometricType_));
        }
        currentCallback_ = it->second;
    }

    // private stuff...
   private:
    cmsHTRANSFORM getColorSpaceTransform() {
        if (!hIn_) {
            PRINT_DEBUG("No available input profile");
            return NULL;
        }

        PRINT_DEBUG("Available ICC Profile, building LCMS Transform");

        cmsUInt32Number cmsInputFormat = TYPE_RGB_8;
        cmsUInt32Number cmsOutputFormat = TYPE_RGB_FLT;
        cmsUInt32Number cmsIntent = INTENT_PERCEPTUAL;

        switch (photometricType_) {
            case PHOTOMETRIC_RGB: {
                switch (bitsPerSample_) {
                    case 32: {
                        if (hasAlpha_) {
                            cmsInputFormat = TYPE_RGBA_FLT;
                        } else {
                            cmsInputFormat = TYPE_RGB_FLT;
                        }
                    } break;
                    case 16: {
                        if (hasAlpha_) {
                            cmsInputFormat = TYPE_RGBA_16;
                        } else {
                            cmsInputFormat = TYPE_RGB_16;
                        }
                    } break;
                    default:
                    case 8: {
                        if (hasAlpha_) {
                            cmsInputFormat = TYPE_RGBA_8;
                        } else {
                            cmsInputFormat = TYPE_RGB_8;
                        }
                    }
                }  // switch
            } break;
            case PHOTOMETRIC_SEPARATED: {
                switch (bitsPerSample_) {
                    case 32: {
                        cmsInputFormat = TYPE_CMYK_FLT;
                    } break;
                    case 16: {
                        cmsInputFormat = TYPE_CMYK_16;
                    } break;
                    default:
                    case 8: {
                        cmsInputFormat = TYPE_CMYK_8;
                    }
                }  // switch
            } break;
        }

        return cmsCreateTransform(hIn_.data(), cmsInputFormat, hsRGB_.data(),
                                  cmsOutputFormat, cmsIntent, 0);
    }

    void doNothing(Frame & /*frame*/, const TiffReaderParams & /*params*/) {}

    template <typename InputDataType, typename Converter>
    void read3Components(Frame &frame, const TiffReaderParams & /*params*/,
                         const Converter &conv) {
        assert(samplesPerPixel_ >= 3);
        Frame tempFrame(width_, height_);

        pfs::Channel *Xc;
        pfs::Channel *Yc;
        pfs::Channel *Zc;
        tempFrame.createXYZChannels(Xc, Yc, Zc);

        std::vector<InputDataType> tempBuffer(width_ * samplesPerPixel_);
        for (uint32 row = 0; row < height_; row++) {
            TIFFReadScanline(handle(), tempBuffer.data(), row);

            utils::transform(StrideIterator<InputDataType *>(tempBuffer.data(),
                                                             samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + width_ * samplesPerPixel_,
                                 samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + 1, samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + 2, samplesPerPixel_),
                             Xc->row_begin(row), Yc->row_begin(row),
                             Zc->row_begin(row), conv);
        }

        tempFrame.swap(frame);
    }

    template <typename InputDataType, typename Converter>
    void read4Components(Frame &frame, const TiffReaderParams & /*params*/,
                         const Converter &conv) {
        assert(samplesPerPixel_ >= 4);
        Frame tempFrame(width_, height_);

        pfs::Channel *Xc;
        pfs::Channel *Yc;
        pfs::Channel *Zc;
        tempFrame.createXYZChannels(Xc, Yc, Zc);

        std::vector<InputDataType> tempBuffer(width_ * samplesPerPixel_);
        for (uint32 row = 0; row < height_; row++) {
            TIFFReadScanline(handle(), tempBuffer.data(), row);

            utils::transform(StrideIterator<InputDataType *>(tempBuffer.data(),
                                                             samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + width_ * samplesPerPixel_,
                                 samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + 1, samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + 2, samplesPerPixel_),
                             StrideIterator<InputDataType *>(
                                 tempBuffer.data() + 3, samplesPerPixel_),
                             Xc->row_begin(row), Yc->row_begin(row),
                             Zc->row_begin(row), conv);
        }

        tempFrame.swap(frame);
    }

    template <typename InputDataType>  //, typename ConversionOperator>
    void readRGB(Frame &frame, const TiffReaderParams &params) {
#ifndef NDEBUG
        std::cout << BOOST_CURRENT_FUNCTION << typeid(InputDataType).name()
                  << std::endl;
        assert(samplesPerPixel_ >= 3);
#endif

        ScopedCmsTransform xform(getColorSpaceTransform());
        if (xform) {
            PRINT_DEBUG("ICC Profile Available");
            if (hasAlpha_) {
                read4Components<InputDataType>(
                    frame, params, colorspace::Convert4LCMS3(xform.data()));
            } else {
                read3Components<InputDataType>(
                    frame, params, colorspace::Convert3LCMS3(xform.data()));
            }
        } else {
            read3Components<InputDataType>(frame, params, colorspace::Copy());
        }
    }

    void readLogLuv(Frame &frame, const TiffReaderParams &params) {
#ifndef NDEBUG
        std::cout << BOOST_CURRENT_FUNCTION << std::endl;
        assert(samplesPerPixel_ == 3);
#endif

        read3Components<float>(frame, params, colorspace::ConvertXYZ2RGB());
    }

    template <typename InputDataType>
    void readCMYK(Frame &frame, const TiffReaderParams &params) {
#ifndef NDEBUG
        std::cout << BOOST_CURRENT_FUNCTION << typeid(InputDataType).name()
                  << std::endl;
        assert(samplesPerPixel_ == 4);
#endif

        ScopedCmsTransform xform(getColorSpaceTransform());
        if (xform) {
            PRINT_DEBUG("ICC Profile Available");

            read4Components<InputDataType>(
                frame, params, colorspace::Convert4LCMS3(xform.data()));

        } else {
            read4Components<InputDataType>(frame, params,
                                           colorspace::ConvertCMYK2RGB());
        }
    }
};

TiffReader::TiffReader(const string &filename)
    : FrameReader(filename), m_data(new TiffReaderData) {
    TiffReader::open();
}

TiffReader::~TiffReader() { TiffReader::close(); }

bool TiffReader::isOpen() const { return m_data->file_; }

void TiffReader::close() { m_data.reset(new TiffReaderData); }

void TiffReader::open() {
    m_data->file_.reset(TIFFOpen(filename().c_str(), "r"));
    if (!m_data->file_) {
        throw pfs::io::InvalidFile("TiffReader: cannot open file " +
                                   filename());
    }

    TIFFGetField(m_data->handle(), TIFFTAG_IMAGEWIDTH, &m_data->width_);
    TIFFGetField(m_data->handle(), TIFFTAG_IMAGELENGTH, &m_data->height_);

    if (m_data->width_ <= 0 || m_data->height_ <= 0) {
        throw pfs::io::InvalidHeader("TiffReader: invalid image size");
    }
    setWidth(m_data->width_);
    setHeight(m_data->height_);

    // check if planar... maybe in the future we can add support for tiled
    // images?
    uint16 planarConfig;
    TIFFGetField(m_data->handle(), TIFFTAG_PLANARCONFIG, &planarConfig);
    if (planarConfig != PLANARCONFIG_CONTIG) {
        throw pfs::io::InvalidHeader(
            "TiffReader: Unsopported planar configuration");
    }

    // compression type
    if (!TIFFGetField(m_data->handle(), TIFFTAG_COMPRESSION,
                      &m_data->compressionType_)) {
        m_data->compressionType_ = COMPRESSION_NONE;
    }
    // photometric type
    if (!TIFFGetFieldDefaulted(m_data->handle(), TIFFTAG_PHOTOMETRIC,
                               &m_data->photometricType_)) {
        throw pfs::io::InvalidHeader(
            "TiffReader: unspecified photometric type");
    }

    if (!TIFFGetField(m_data->handle(), TIFFTAG_STONITS, &m_data->stonits_)) {
        m_data->stonits_ = 1.;
    }

    // bits per sample
    if (!TIFFGetField(m_data->handle(), TIFFTAG_BITSPERSAMPLE,
                      &m_data->bitsPerSample_)) {
        throw pfs::io::InvalidHeader("TiffReader: unspecified bits per sample");
    }
    if (m_data->bitsPerSample_ != 8 && m_data->bitsPerSample_ != 16 &&
        m_data->bitsPerSample_ != 32) {
        throw pfs::io::InvalidHeader(
            "TiffReader: invalid bits per sample (read: " +
            boost::lexical_cast<std::string>(m_data->bitsPerSample_) + ")");
    }

    // samples per pixel
    if (!TIFFGetField(m_data->handle(), TIFFTAG_SAMPLESPERPIXEL,
                      &m_data->samplesPerPixel_)) {
        throw pfs::io::InvalidHeader(
            "TiffReader: unspecified samples per pixel");
    }

    // parse photometric type
    switch (m_data->photometricType_) {
        case PHOTOMETRIC_LOGLUV: {
            if (m_data->compressionType_ != COMPRESSION_SGILOG &&
                m_data->compressionType_ != COMPRESSION_SGILOG24) {
                throw pfs::io::InvalidHeader(
                    "TiffReader: only support SGILOG compressed LogLuv data");
            }
            TIFFSetField(m_data->handle(), TIFFTAG_SGILOGDATAFMT,
                         SGILOGDATAFMT_FLOAT);
        } break;
        case PHOTOMETRIC_RGB: {
            uint16 *extraSamplesTypes = 0;
            uint16 extraSamplesPerPixel = 0;

            // read extra samples (# of alpha channels)
            if (!TIFFGetField(m_data->handle(), TIFFTAG_EXTRASAMPLES,
                              &extraSamplesPerPixel, &extraSamplesTypes)) {
                if (m_data->samplesPerPixel_ == 4) {
                    extraSamplesPerPixel = 1;
                }
            }
            uint16 colorSamples =
                m_data->samplesPerPixel_ - extraSamplesPerPixel;
            m_data->hasAlpha_ = (extraSamplesPerPixel == 1);
            if (colorSamples != 3) {
                throw pfs::io::InvalidHeader(
                    "TIFF: unsupported samples per pixel for RGB");
            }
        } break;
        case PHOTOMETRIC_SEPARATED: {
            if (m_data->samplesPerPixel_ != 4) {
                throw pfs::io::InvalidHeader(
                    "TIFF: unsupported samples per pixel for CMYK");
            }
        } break;
        default: {
            throw pfs::io::InvalidHeader(
                "TiffReader: unsupported photometric type");
        } break;
    }

    // ...based on photometric type and bits per samples, will make ready the
    // right callback to read the data
    m_data->initReader();
    m_data->hIn_.reset(
        GetTIFFProfile(m_data->handle(), m_data->bitsPerSample_));
}

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

void TiffReader::read(Frame &frame, const Params &params) {
    if (!isOpen()) {
        open();
    }

    m_data->read(frame, params);
    FrameReader::read(frame, params);
}

}  // io
}  // pfs
