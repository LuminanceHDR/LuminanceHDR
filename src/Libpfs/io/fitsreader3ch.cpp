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
#include <Libpfs/utils/minmax.h>

namespace {

inline
void rgb2hsl(float r, float g, float b, float& h, float& s, float& l)
{
    float v, m, vm, r2, g2, b2;
    h = 0.0f;
    s = 0.0f;
    l = 0.0f;

    pfs::utils::minmax(r, g, b, m, v);

    l = (m + v) / 2.0f;
    if (l <= 0.0f)
        return;
    vm = v - m;
    s = vm;
    //if (s >= 0.0f)
    if (s > 0.0f)
        s /= (l <= 0.5f) ? (v + m) : (2.0f - v - m);
    else return;
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    else if (g == v)
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    else
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    h /= 6.0f;
}

inline
void hsl2rgb(float h, float sl, float l, float& r, float& g, float& b)
{
    float v;
    r = l;
    g = l;
    b = l;
    v = (l <= 0.5f) ? (l * (1.0f + sl)) : (l + sl - l * sl);
    if (v > 0.0f)
    {
        float m;
        float sv;
        int sextant;
        float fract, vsf, mid1, mid2;
        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0f;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant)
        {
        case 0:
            r = v;
            g = mid1;
            b = m;
            break;
        case 1:
            r = mid2;
            g = v;
            b = m;
            break;
        case 2:
            r = m;
            g = v;
            b = mid1;
            break;
        case 3:
            r = m;
            g = mid2;
            b = v;
            break;
        case 4:
            r = mid1;
            g = m;
            b = v;
            break;
        case 5:
            r = v;
            g = m;
            b = mid2;
            break;
        }
    }
}

}

namespace pfs {
namespace io {

FitsReader3Ch::FitsReader3Ch(const std::string& luminosityChannel,
                             const std::string& redChannel, 
                             const std::string& greenChannel, 
                             const std::string& blueChannel,
                             const std::string& hChannel) :
    m_luminosityChannel(luminosityChannel), 
    m_redChannel(redChannel), 
    m_greenChannel(greenChannel), 
    m_blueChannel(blueChannel),
    m_hChannel(hChannel)
{
    FitsReader3Ch::open();
}

void FitsReader3Ch::open()
{
    m_fileLuminosity.reset( new CCfits::FITS(m_luminosityChannel, CCfits::Read, true) );
    if ( !m_fileLuminosity ) {
        throw InvalidFile("Cannot open file " + m_luminosityChannel);
    }

    m_imageLuminosity = &m_fileLuminosity->pHDU();
    m_imageLuminosity->readAllKeys();

    m_fileRed.reset( new CCfits::FITS(m_redChannel, CCfits::Read, true) );
    if ( !m_fileRed ) {
        throw InvalidFile("Cannot open file " + m_redChannel);
    }

    m_imageRed = &m_fileRed->pHDU();
    m_imageRed->readAllKeys();

    m_fileGreen.reset( new CCfits::FITS(m_greenChannel, CCfits::Read, true) );
    if ( !m_fileGreen ) {
        throw InvalidFile("Cannot open file " + m_greenChannel);
    }

    m_imageGreen = &m_fileGreen->pHDU();
    m_imageGreen->readAllKeys();

    m_fileBlue.reset( new CCfits::FITS(m_blueChannel, CCfits::Read, true) );
    if ( !m_fileBlue ) {
        throw InvalidFile("Cannot open file " + m_blueChannel);
    }

    m_imageBlue = &m_fileBlue->pHDU();
    m_imageBlue->readAllKeys();

    if (m_hChannel != "") {
        m_fileH.reset( new CCfits::FITS(m_hChannel, CCfits::Read, true) );
        if ( !m_fileBlue ) {
            throw InvalidFile("Cannot open file " + m_hChannel);
        }

        m_imageH = &m_fileH->pHDU();
        m_imageH->readAllKeys();
    }
}

void FitsReader3Ch::close()
{
    m_fileLuminosity.reset();
    m_fileRed.reset();
    m_fileGreen.reset();
    m_fileBlue.reset();
    m_fileH.reset();
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
    if (m_hChannel != "") {
        if (m_imageH->axes() != 2)
            throw InvalidFile("No image in file " + m_hChannel);
    }

    if (m_imageLuminosity->axis(0) != m_imageRed->axis(0) || m_imageRed->axis(0) != m_imageGreen->axis(0) || m_imageRed->axis(0) != m_imageBlue->axis(0))
         throw InvalidFile("Images have different size");

    if (m_imageLuminosity->axis(1) != m_imageRed->axis(1) || m_imageRed->axis(1) != m_imageGreen->axis(1) || m_imageRed->axis(1) != m_imageBlue->axis(1))
         throw InvalidFile("Images have different size");

    if ( !isOpen() ) open();

    std::valarray<float>  contentsLuminosity;
    std::valarray<float>  contentsRed;
    std::valarray<float>  contentsGreen;
    std::valarray<float>  contentsBlue;
    std::valarray<float>  contentsH;
    m_imageLuminosity->read(contentsLuminosity);
    m_imageRed->read(contentsRed);
    m_imageGreen->read(contentsGreen);
    m_imageBlue->read(contentsBlue);

    int ax1 = m_imageRed->axis(0);
    int ax2 = m_imageRed->axis(1); 
    qDebug() << "ax1 = " << ax1 << " , ax2 = " << ax2;

    if (m_hChannel != "") 
        m_imageH->read(contentsH);
    else {
        contentsH.resize(ax1*ax2);
        for (int i = 0; i < ax1*ax2; i++)
            contentsH[i] = 0.0f;
    }

    Frame tempFrame(ax1, ax2);
    Channel *Xc, *Yc, *Zc;
    tempFrame.createXYZChannels(Xc, Yc, Zc);

    for (long i = 0; i < ax1*ax2; i++) 
    {
        float r, g, b, h, s, l;
        rgb2hsl(contentsRed[i], contentsGreen[i], contentsBlue[i], h, s, l);
        hsl2rgb(h,s, contentsLuminosity[i], r, g, b);
        (*Xc)(i) = r + contentsH[i];
        (*Yc)(i) = g;
        (*Zc)(i) = b;
    }     

    frame.swap( tempFrame );
}

}   // io
}   // pfs
