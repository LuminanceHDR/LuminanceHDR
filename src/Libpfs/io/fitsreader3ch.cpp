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

#include <Libpfs/io/fitsreader3ch.h>
#include <Libpfs/frame.h>

namespace pfs {
namespace io {

FitsReader3Ch::FitsReader3Ch(const std::string& luminosityChannel,
                             const std::string& redChannel, 
                             const std::string& greenChannel, 
                             const std::string& blueChannel) :
    m_luminosityChannel(luminosityChannel), 
    m_redChannel(redChannel), 
    m_greenChannel(greenChannel), 
    m_blueChannel(blueChannel)
{
    FitsReader3Ch::open();
}

void FitsReader3Ch::open()
{
    m_fileLuminosity.reset( new FITS(m_luminosityChannel, Read, true) );
    if ( !m_fileLuminosity ) {
        throw InvalidFile("Cannot open file " + m_luminosityChannel);
    }

    m_imageLuminosity = &m_fileLuminosity->pHDU();
    m_imageLuminosity->readAllKeys();

    m_fileRed.reset( new FITS(m_redChannel, Read, true) );
    if ( !m_fileRed ) {
        throw InvalidFile("Cannot open file " + m_redChannel);
    }

    m_imageRed = &m_fileRed->pHDU();
    m_imageRed->readAllKeys();

    m_fileGreen.reset( new FITS(m_greenChannel, Read, true) );
    if ( !m_fileGreen ) {
        throw InvalidFile("Cannot open file " + m_greenChannel);
    }

    m_imageGreen = &m_fileGreen->pHDU();
    m_imageGreen->readAllKeys();

    m_fileBlue.reset( new FITS(m_blueChannel, Read, true) );
    if ( !m_fileBlue ) {
        throw InvalidFile("Cannot open file " + m_blueChannel);
    }

    m_imageBlue = &m_fileBlue->pHDU();
    m_imageBlue->readAllKeys();
}

void FitsReader3Ch::close()
{
    m_fileLuminosity.reset();
    m_fileRed.reset();
    m_fileGreen.reset();
    m_fileBlue.reset();
}

void FitsReader3Ch::read(Frame &frame)
{
    if (m_imageLuminosity->axes() != 2)
        throw InvalidFile("No image in file " + m_luminosityChannel);
    if (m_imageRed->axes() != 2)
        throw InvalidFile("No image in file " + m_redChannel);
    if (m_imageGreen->axes() != 2)
        throw InvalidFile("No image in file " + m_greenChannel);
    if (m_imageBlue->axes() != 2)
        throw InvalidFile("No image in file " + m_blueChannel);


    if (m_imageLuminosity->axis(0) != m_imageRed->axis(0) || m_imageRed->axis(0) != m_imageGreen->axis(0) || m_imageRed->axis(0) != m_imageBlue->axis(0))
         throw InvalidFile("Images have different size");

    if (m_imageLuminosity->axis(1) != m_imageRed->axis(1) || m_imageRed->axis(1) != m_imageGreen->axis(1) || m_imageRed->axis(1) != m_imageBlue->axis(1))
         throw InvalidFile("Images have different size");

    if ( !isOpen() ) open();

    std::valarray<float>  contentsLuminosity;
    std::valarray<float>  contentsRed;
    std::valarray<float>  contentsGreen;
    std::valarray<float>  contentsBlue;
    m_imageLuminosity->read(contentsLuminosity);
    m_imageRed->read(contentsRed);
    m_imageGreen->read(contentsGreen);
    m_imageBlue->read(contentsBlue);

    int ax1 = m_imageRed->axis(0);
    int ax2 = m_imageRed->axis(1); 
    qDebug() << "ax1 = " << ax1 << " , ax2 = " << ax2;

    Frame tempFrame(ax1, ax2);
    Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    for (long i = 0; i < ax1*ax2; i++) 
    {
            (*Xc)(i) = contentsRed[i];
            (*Yc)(i) = contentsGreen[i];
            (*Zc)(i) = contentsBlue[i];
    }     

    frame.swap( tempFrame );
}

}   // io
}   // pfs
