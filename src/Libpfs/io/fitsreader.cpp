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

#include <QDebug>

#include <Libpfs/io/fitsreader.h>
#include <Libpfs/frame.h>

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

    m_image = &m_file->pHDU();
    m_image->readAllKeys();
}

void FitsReader::close()
{
    setWidth(0);
    setHeight(0);
    m_file.reset();
}

void FitsReader::read(Frame &frame, const Params &/*params*/)
{
    if (m_image->axes() != 2)
        throw InvalidFile("No image in file " + filename());

    if ( !isOpen() ) open();

    std::valarray<float>  contents;
    m_image->read(contents);

    qDebug() << "contents.size = " << contents.size();

    int ax1 = m_image->axis(0);
    int ax2 = m_image->axis(1); 
    qDebug() << "ax1 = " << ax1 << " , ax2 = " << ax2;

    Frame tempFrame(ax1, ax2);
    Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    for (long i = 0; i < ax1*ax2; i++) 
    {
        (*Yc)(i) = (*Zc)(i) = (*Xc)(i) = contents[i];;
    }     

    qDebug() << "FITS min luminance: " << *std::min_element(Xc->begin(), Xc->end());
    qDebug() << "FITS max luminance: " << *std::max_element(Xc->begin(), Xc->end());
    frame.swap( tempFrame );
}

}   // io
}   // pfs
