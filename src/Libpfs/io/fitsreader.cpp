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

#ifndef NDEBUG
#include <boost/algorithm/minmax_element.hpp>
#endif

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <Libpfs/frame.h>
#include <Libpfs/colorspace/normalizer.h>

// include windows.h to avoid TBYTE define clashes with fitsio.h
#ifdef Q_OS_WIN
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <windows.h>
#endif

#include <fitsio.h>

using namespace std;
using namespace boost;

namespace pfs {
namespace io {

class FitsReaderData
{
public:
    FitsReaderData()
        : m_status(0)
        , m_ptr(NULL)
    {}

    ~FitsReaderData()
    {
        if (m_ptr != NULL)
        {
            fits_close_file(m_ptr, &m_status);
        }
    }

    float normalize(float input)
    {
        return (input/m_datamax);
    }

    int m_status;
    long m_datamax;

    fitsfile* m_ptr;
};

FitsReader::FitsReader(const std::string& filename)
    : FrameReader(filename)
{
    FitsReader::open();
}

FitsReader::~FitsReader()
{}

void FitsReader::open()
{
    m_data.reset(new FitsReaderData());

    // open stream
    if ( fits_open_file(&m_data->m_ptr, filename().c_str(), READONLY, &m_data->m_status) )
    {
        throw InvalidFile("Cannot open file " + filename() +
                          "FITS error " + boost::lexical_cast<std::string>(m_data->m_status));
    }

    // read image size
    int nfound;
    long naxes[2];

    if ( fits_read_keys_lng(m_data->m_ptr, "NAXIS", 1, 2, naxes, &nfound, &m_data->m_status) )
    {
        throw InvalidHeader("Could not find the size of the data range");
    }

    setWidth(naxes[0]);
    setHeight(naxes[1]);

    // read data range
    if ( fits_read_key_lng(m_data->m_ptr, "DATAMAX", &m_data->m_datamax, NULL, &m_data->m_status) )
    {
        long bitpix;
        // I couldn't read DATAMAX, so I read BITPIX?
        if ( fits_read_key_lng(m_data->m_ptr, "BITPIX", &bitpix, NULL, &m_data->m_status) )
        {
            throw InvalidHeader("Could not read the bit width of the sample data");
        }
        else
        {
            m_data->m_datamax = ((1 << bitpix) - 1);
        }
    }

    std::cout << "Size: w: " << width() << " height = " << height() << " "
                 << " datamax = " << m_data->m_datamax << std::endl;
}


void FitsReader::close()
{
    setWidth(0);
    setHeight(0);
    m_data.reset();
}

void FitsReader::read(Frame &frame, const Params&)
{
    if ( !isOpen() ) open();

#ifndef NDEBUG
    std::cout << "size (" << width() << ", " << height() << ")";
    std::cout << "contents.size (pixels) = " << width()*height() << std::endl;
#endif

    long fpixel = 1;
    float nullval = 0; /* don't check for null values in the image */
    long nbuffer = width();
    int anynull;

    Frame tempFrame(width(), height());
    Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    std::vector<float> buffer(width());
    Channel::iterator it = Xc->begin();
    for (size_t i = 0; i < height(); ++i, fpixel += nbuffer)
    {
        if (fits_read_img(m_data->m_ptr, TFLOAT, fpixel, nbuffer, &nullval,
                          buffer.data(), &anynull, &m_data->m_status) )
        {
            throw std::runtime_error("Cannot read strip " +
                                     boost::lexical_cast<std::string>(i));
        }

        it = std::transform(buffer.begin(), buffer.end(), it,
                            boost::bind(&FitsReaderData::normalize, m_data.get(), _1));
    }

    // copy into other channels
    std::copy(Xc->begin(), Xc->end(), Yc->begin());
    std::copy(Xc->begin(), Xc->end(), Zc->begin());

#ifndef NDEBUG
    std::pair<pfs::Array2Df::const_iterator, pfs::Array2Df::const_iterator> minmax =
            boost::minmax_element(Xc->begin(), Xc->end());

    std::cout << "FITS min luminance: " << *minmax.first;
    std::cout << " FITS max luminance: " << *minmax.second;
#endif

    frame.swap(tempFrame);
}

}   // io
}   // pfs
