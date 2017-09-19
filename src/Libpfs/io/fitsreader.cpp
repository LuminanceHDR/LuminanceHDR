/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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

#include <Libpfs/io/fitsreader.h>

#include <boost/algorithm/minmax_element.hpp>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/frame.h>

#include <qglobal.h>
// include windows.h to avoid TBYTE define clashes with fitsio.h
#ifdef Q_OS_WIN
#define _WINSOCKAPI_  // stops windows.h including winsock.h
#include <windows.h>
#endif

#include <fitsio.h>

using namespace std;
using namespace pfs::colorspace;
using namespace boost;

namespace pfs {
namespace io {

class FitsReaderData {
   public:
    FitsReaderData()
        : m_format(0), m_status(0), m_bscale(1.f), m_bzero(0.f), m_ptr(NULL) {}

    ~FitsReaderData() {
        if (m_ptr != NULL) {
            fits_close_file(m_ptr, &m_status);
        }
    }

    short int m_format;
    int m_status;
    float m_bscale;
    float m_bzero;

    fitsfile *m_ptr;
};

FitsReader::FitsReader(const std::string &filename) : FrameReader(filename) {
    FitsReader::open();
}

FitsReader::~FitsReader() {}

void FitsReader::open() {
    m_data.reset(new FitsReaderData());

    // open stream
    if (fits_open_file(&m_data->m_ptr, filename().c_str(), READONLY,
                       &m_data->m_status)) {
        throw InvalidFile("Cannot open file " + filename() + "FITS error " +
                          boost::lexical_cast<std::string>(m_data->m_status));
    }

    // read image size
    int nfound;
    long naxes[2];

    if (fits_read_keys_lng(m_data->m_ptr, "NAXIS", 1, 2, naxes, &nfound,
                           &m_data->m_status)) {
        fits_close_file(m_data->m_ptr, &m_data->m_status);
        throw InvalidHeader("Could not find the size of the data range");
    }

    if (nfound == 0) {
        fits_close_file(m_data->m_ptr, &m_data->m_status);
        throw InvalidHeader("No image data array present");
    }
    setWidth(naxes[0]);
    setHeight(naxes[1]);

    float bscale;
    float bzero;
    long bitpix;
    int status = 0;
    char error_string[FLEN_ERRMSG];

    fits_read_key_flt(m_data->m_ptr, "BSCALE", &bscale, NULL, &status);
    if (status) {
        fits_get_errstatus(status, error_string);
#ifndef NDEBUG
        std::cout << "BSCALE: " << error_string << std::endl;
#endif
        bscale = 1.f;
        status = 0;
    }

    fits_read_key_flt(m_data->m_ptr, "BZERO", &bzero, NULL, &status);
    if (status) {
        fits_get_errstatus(status, error_string);
#ifndef NDEBUG
        std::cout << "BZERO: " << error_string << std::endl;
#endif
        bzero = 0.f;
        status = 0;
    }

    fits_read_key_lng(m_data->m_ptr, "BITPIX", &bitpix, NULL, &status);
    if (status) {
        fits_get_errstatus(status, error_string);
#ifndef NDEBUG
        std::cout << "BITPIX: " << error_string << std::endl;
#endif
        fits_close_file(m_data->m_ptr, &status);
        throw InvalidHeader(error_string);
    }

    m_data->m_bscale = bscale;
    m_data->m_bzero = bzero;
    m_data->m_format = bitpix;
    m_data->m_status = status;

#ifndef NDEBUG
    std::cout << "Size: w: " << width() << " height = " << height()
              << " bitpix = " << bitpix << " bscale = " << bscale
              << " bzero = " << bzero << " status = " << m_data->m_status
              << std::endl;
#endif
}

void FitsReader::close() {
    setWidth(0);
    setHeight(0);
    m_data.reset();
}

void FitsReader::read(Frame &frame, const Params &) {
    if (!isOpen()) open();

#ifndef NDEBUG
    std::cout << "size (" << width() << ", " << height() << ")";
    std::cout << "contents.size (pixels) = " << width() * height() << std::endl;
#endif

    long fpixel = 1;
    long nbuffer = width();
    int anynull;

    Frame tempFrame(width(), height());
    Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    Channel::iterator it = Xc->begin();

    float bscale = m_data->m_bscale;
    float bzero = m_data->m_bzero;

    if (m_data->m_format == FLOAT_IMG) {
        std::vector<float> buffer(width());
        float nullval = 0;  // don't check for null values in the image
        for (size_t i = 0; i < height(); ++i, fpixel += nbuffer) {
            if (fits_read_img(m_data->m_ptr, TFLOAT, fpixel, nbuffer, &nullval,
                              buffer.data(), &anynull, &m_data->m_status)) {
                char error_string[FLEN_ERRMSG];
                fits_get_errstatus(m_data->m_status, error_string);
                fits_close_file(m_data->m_ptr, &m_data->m_status);
                throw std::runtime_error("FLOAT: Cannot read strip " +
                                         boost::lexical_cast<std::string>(i) +
                                         ". " + error_string);
            }

            std::transform(
                buffer.begin(), buffer.end(), buffer.begin(),
                [bscale, bzero](float c) { return bscale * c + bzero; });
            it = std::copy(buffer.begin(), buffer.end(), it);
        }
    } else if (m_data->m_format == DOUBLE_IMG) {
        std::vector<double> buffer(width());
        double nullval = 0;  // don't check for null values in the image
        for (size_t i = 0; i < height(); ++i, fpixel += nbuffer) {
            if (fits_read_img(m_data->m_ptr, TDOUBLE, fpixel, nbuffer, &nullval,
                              buffer.data(), &anynull, &m_data->m_status)) {
                char error_string[FLEN_ERRMSG];
                fits_get_errstatus(m_data->m_status, error_string);
                fits_close_file(m_data->m_ptr, &m_data->m_status);
                throw std::runtime_error("DOUBLE: Cannot read strip " +
                                         boost::lexical_cast<std::string>(i) +
                                         ". " + error_string);
            }

            std::transform(
                buffer.begin(), buffer.end(), buffer.begin(),
                [bscale, bzero](double c) { return bscale * c + bzero; });
            it = std::copy(buffer.begin(), buffer.end(), it);
        }
    } else if (m_data->m_format == BYTE_IMG) {
        std::vector<short> buffer(width());
        std::vector<float> buffer_flt(width());
        short nullval = 0;  // don't check for null values in the image
        for (size_t i = 0; i < height(); ++i, fpixel += nbuffer) {
            if (fits_read_img(m_data->m_ptr, TINT, fpixel, nbuffer, &nullval,
                              buffer.data(), &anynull, &m_data->m_status)) {
                char error_string[FLEN_ERRMSG];
                fits_get_errstatus(m_data->m_status, error_string);
                fits_close_file(m_data->m_ptr, &m_data->m_status);
                throw std::runtime_error("BYTE: Cannot read strip " +
                                         boost::lexical_cast<std::string>(i) +
                                         ". " + error_string);
            }

            std::transform(
                buffer.begin(), buffer.end(), buffer_flt.begin(),
                [bscale, bzero](short c) { return bscale * c + bzero; });
            it = std::copy(buffer_flt.begin(), buffer_flt.end(), it);
        }
    } else if (m_data->m_format == SHORT_IMG) {
        std::vector<int> buffer(width());
        std::vector<float> buffer_flt(width());
        int nullval = 0;  // don't check for null values in the image
        for (size_t i = 0; i < height(); ++i, fpixel += nbuffer) {
            if (fits_read_img(m_data->m_ptr, TINT, fpixel, nbuffer, &nullval,
                              buffer.data(), &anynull, &m_data->m_status)) {
                char error_string[FLEN_ERRMSG];
                fits_get_errstatus(m_data->m_status, error_string);
                fits_close_file(m_data->m_ptr, &m_data->m_status);
                throw std::runtime_error("SHORT: Cannot read strip " +
                                         boost::lexical_cast<std::string>(i) +
                                         ". " + error_string);
            }

            std::transform(
                buffer.begin(), buffer.end(), buffer_flt.begin(),
                [bscale, bzero](int c) { return bscale * c + bzero; });
            it = std::copy(buffer_flt.begin(), buffer_flt.end(), it);
        }
    } else if (m_data->m_format == LONG_IMG) {
        std::vector<long> buffer(width());
        std::vector<float> buffer_flt(width());
        long nullval = 0;  // don't check for null values in the image
        for (size_t i = 0; i < height(); ++i, fpixel += nbuffer) {
            if (fits_read_img(m_data->m_ptr, TLONG, fpixel, nbuffer, &nullval,
                              buffer.data(), &anynull, &m_data->m_status)) {
                char error_string[FLEN_ERRMSG];
                fits_get_errstatus(m_data->m_status, error_string);
                fits_close_file(m_data->m_ptr, &m_data->m_status);
                throw std::runtime_error("LONG: Cannot read strip " +
                                         boost::lexical_cast<std::string>(i) +
                                         ". " + error_string);
            }

            std::transform(
                buffer.begin(), buffer.end(), buffer_flt.begin(),
                [bscale, bzero](long c) { return bscale * c + bzero; });
            it = std::copy(buffer_flt.begin(), buffer_flt.end(), it);
        }
    } else if (m_data->m_format == LONGLONG_IMG) {
        std::vector<long long> buffer(width());
        std::vector<float> buffer_flt(width());
        long long nullval = 0;  // don't check for null values in the image
        for (size_t i = 0; i < height(); ++i, fpixel += nbuffer) {
            if (fits_read_img(m_data->m_ptr, TLONGLONG, fpixel, nbuffer,
                              &nullval, buffer.data(), &anynull,
                              &m_data->m_status)) {
                char error_string[FLEN_ERRMSG];
                fits_get_errstatus(m_data->m_status, error_string);
                fits_close_file(m_data->m_ptr, &m_data->m_status);
                throw std::runtime_error("LONG: Cannot read strip " +
                                         boost::lexical_cast<std::string>(i) +
                                         ". " + error_string);
            }

            std::transform(
                buffer.begin(), buffer.end(), buffer_flt.begin(),
                [bscale, bzero](long long c) { return bscale * c + bzero; });
            it = std::copy(buffer_flt.begin(), buffer_flt.end(), it);
        }
    }

#ifndef NDEBUG
    std::pair<pfs::Array2Df::const_iterator, pfs::Array2Df::const_iterator>
        minmax = boost::minmax_element(Xc->begin(), Xc->end());
    std::cout << "FITS min luminance = " << *minmax.first << std::endl;
    std::cout << "FITS max luminance = " << *minmax.second << std::endl;
#endif

    // copy into other channels
    std::copy(Xc->begin(), Xc->end(), Yc->begin());
    std::copy(Xc->begin(), Xc->end(), Zc->begin());

    frame.swap(tempFrame);
}

}  // io
}  // pfs
