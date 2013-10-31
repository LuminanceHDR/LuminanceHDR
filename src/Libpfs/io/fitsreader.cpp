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

#include <cmath>
#include <math.h>
#ifndef NDEBUG
#include <boost/algorithm/minmax_element.hpp>
#endif
#include <Libpfs/io/fitsreader.h>
#include <Libpfs/frame.h>
#include <Libpfs/colorspace/normalizer.h>

using namespace std;

namespace pfs {
namespace io {

FitsReader::FitsReader(const std::string& filename)
    : FrameReader(filename)
{
    FitsReader::open();
}

void FitsReader::open()
{
    m_file.reset( new CCfits::FITS(filename(), CCfits::Read, true) );
    if ( !m_file ) {
        throw InvalidFile("Cannot open file " + filename());
    }
}

void FitsReader::close()
{
    setWidth(0);
    setHeight(0);
    m_file.reset();
}

void FitsReader::read(Frame &frame, const Params &/*params*/)
{
    if ( !isOpen() ) open();

    std::valarray<float>  contents;
    int ax1, ax2;

    CCfits::PHDU& image = m_file->pHDU();
    image.readAllKeys();
    //std::cout << *image << std::endl;

    if (!(image.axes() == 2 || image.axes() == 3))
    {
        const CCfits::ExtMap& extensions = m_file->extension();
        CCfits::ExtMap::const_iterator it = extensions.begin();
        CCfits::ExtMap::const_iterator itEnd = extensions.end();
        for (; it != itEnd; it++)
        {
            //std::cout << it->first << std::endl;
            it->second->readAllKeys();
            //std::cout << *it->second << std::endl;
            if (!(it->second->axes() == 2 || it->second->axes() == 3))
            {
                continue;
            }
            else
            {
                it->second->read(contents);
                ax1 = it->second->axis(0);
                ax2 = it->second->axis(1);
                break;
            }
        }
        if (it == itEnd)
        {
            throw InvalidFile("No image in file " + filename());
        }
    }
    else
    {
        image.read(contents);
        ax1 = image.axis(0);
        ax2 = image.axis(1); 
    }

#ifndef NDEBUG
    std::cout << "size (" << ax1 << ", " << ax2 << ")";
    std::cout << "contents.size (pixels) = " << contents.size();
#endif

    Frame tempFrame(ax1, ax2);
    Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    float max = std::pow(2.f, std::floor(log2f(contents.max()) + 1)) - 1;

    std::transform(&contents[0], &contents[0] + contents.size(),
                   Xc->begin(), Normalizer(0.f, max));
    std::copy(Xc->begin(), Xc->end(), Yc->begin());
    std::copy(Xc->begin(), Xc->end(), Zc->begin());

#ifndef NDEBUG
    std::pair<pfs::Array2Df::const_iterator, pfs::Array2Df::const_iterator> minmax =
            boost::minmax_element(Xc->begin(), Xc->end());

    std::cout << "FITS min luminance: " << *minmax.first << " (" << contents.min() << ")";
    std::cout << "FITS max luminance: " << *minmax.second << " (" << contents.max() << ")";
#endif

    frame.swap(tempFrame);
}

}   // io
}   // pfs
