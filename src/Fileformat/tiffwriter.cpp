/**
 * @brief Tiff facilities
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006 Giuseppe Rota
 * Copyright (C) 2012 Davide Anastasia
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

#include <QObject>
#include <QSysInfo>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QScopedPointer>

#include <cmath>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <limits>

#include "Common/ResourceHandlerLcms.h"
#include "Libpfs/frame.h"
#include "Libpfs/array2d.h"

void TiffWriter::writeCommonHeader()
{
    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, (uint32_t)width);
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, (uint32_t)height);
    TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (uint32_t)1);
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
}


TiffWriter::TiffWriter(const char *filename, pfs::Frame* f):
    tif(TIFFOpen (filename, "w")),
    ldrimage(0),
    pixmap(0),
    pfsFrame(f),
    width(f->getWidth()),
    height(f->getHeight())
{ 
    if (!tif)
    {
        throw std::runtime_error ("TIFF: could not open file for writing.");
    }

    writeCommonHeader();

    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);
}

TiffWriter::TiffWriter(const char *filename, const quint16 * pix, int w, int h):
    tif(TIFFOpen (filename, "w")),
    ldrimage(0),
    pixmap(pix),
    pfsFrame(0),
    width(w),
    height(h)
{
    if (!tif)
    {
        throw std::runtime_error ("TIFF: could not open file for writing.");
    }

    writeCommonHeader();

    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)3);
}

TiffWriter::TiffWriter(const char *filename, QImage * f):
    tif(TIFFOpen (filename, "w")),
    ldrimage(f),
    pixmap(0),
    pfsFrame(0),
    width(f->width()),
    height(f->height())
{
    if (!tif)
    {
        throw std::runtime_error ("TIFF: could not open file for writing.");
    }

    writeCommonHeader();

    uint16 extras[1];
    extras[0] = EXTRASAMPLE_ASSOCALPHA;

    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)4);
    TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, (uint16_t)1, &extras);
}

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

}

//write 32 bit float Tiff from pfs::Frame
int
TiffWriter::writeFloatTiff()
{
    if ( !pfsFrame )
    {
        std::runtime_error("TiffWriter: Invalid writeFloatTiff function, "\
                           "pfsFrame is not set correctly");
    }

    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(float));

    size_t numPixels = pfsFrame->getHeight()*pfsFrame->getWidth();

    pfs::Channel* Xc;
    pfs::Channel* Yc;
    pfs::Channel* Zc;

    pfsFrame->getXYZChannels(Xc, Yc, Zc);

    const float *X = Xc->getRawData();
    const float *Y = Yc->getRawData();
    const float *Z = Zc->getRawData();

    ComputeMinMax xMinMax = std::for_each(X, X + numPixels, ComputeMinMax());
    ComputeMinMax yMinMax = std::for_each(Y, Y + numPixels, ComputeMinMax());
    ComputeMinMax zMinMax = std::for_each(Z, Z + numPixels, ComputeMinMax());

    Normalizer normalizer(std::min(xMinMax.min(),
                                   std::min(yMinMax.min(),
                                            zMinMax.min())),
                          std::max(xMinMax.max(),
                                   std::max(yMinMax.max(),
                                            zMinMax.max())));

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);
    // enough space for a strip (row)
    float *strip_buf = (float *) _TIFFmalloc (strip_size);
    if (!strip_buf)
    {
		TIFFClose(tif);
        throw std::runtime_error ("TIFF: error allocating buffer.");
    }

    emit maximumValue (strips_num);	// for QProgressDialog

    for (unsigned int s = 0; s < strips_num; s++)
    {
        for (unsigned int col = 0; col < width; col++)
        {
            strip_buf[3 * col + 0] = normalizer( X[s * width + col] );	//(*X)(col,s);
            strip_buf[3 * col + 1] = normalizer( Y[s * width + col] );	//(*Y)(col,s);
            strip_buf[3 * col + 2] = normalizer( Z[s * width + col] );	//(*Z)(col,s);
        }
        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
        {
            qDebug ("error writing strip");
			TIFFClose(tif);

            return -1;
        }
        else
        {
            emit nextstep (s);	// for QProgressDialog
        }
    }
    _TIFFfree (strip_buf);
	TIFFClose(tif);

    return 0;
}

// write LogLUv Tiff from pfs::Frame
int
TiffWriter::writeLogLuvTiff()
{
    if ( !pfsFrame )
    {
        std::runtime_error("TiffWriter: Invalid writeLogLuvTiff function, "\
                           "pfsFrame is not set correctly");
    }

    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG);
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LOGLUV);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(float));
    TIFFSetField (tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
    TIFFSetField (tif, TIFFTAG_STONITS, 1.);	/* not known */

    pfs::Channel* Xc;
    pfs::Channel* Yc;
    pfs::Channel* Zc;

    pfsFrame->getXYZChannels(Xc, Yc, Zc);

    const float *X = Xc->getRawData();
    const float *Y = Yc->getRawData();
    const float *Z = Zc->getRawData();

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);
    float *strip_buf = (float *) _TIFFmalloc (strip_size);	// enough space for a strip
    if (!strip_buf)
    {
		TIFFClose(tif);
        throw std::runtime_error ("TIFF: error allocating buffer.");
    }

    emit maximumValue (strips_num);	// for QProgressDialog

    for (unsigned int s = 0; s < strips_num; s++)
    {
        for (unsigned int col = 0; col < width; col++)
        {
            strip_buf[3 * col + 0] = X[s * width + col];	//(*X)(col,s);
            strip_buf[3 * col + 1] = Y[s * width + col];	//(*Y)(col,s);
            strip_buf[3 * col + 2] = Z[s * width + col];	//(*Z)(col,s);
        }
        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
        {
            qDebug ("error writing strip");
			TIFFClose(tif);
            return -1;
        }
        else
        {
            emit nextstep (s);	// for QProgressDialog
        }
    }
    _TIFFfree (strip_buf);
	TIFFClose(tif);

    return 0;
}

int
TiffWriter::write8bitTiff()
{
    if ( !ldrimage )
        throw std::runtime_error ("TIFF: QImage was not set correctly");

    ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );
    cmsUInt32Number profileSize = 0;
    cmsSaveProfileToMem (hsRGB.data(), NULL, &profileSize);	// get the size

    std::vector<char> embedBuffer(profileSize);

    cmsSaveProfileToMem(hsRGB.data(),
                        reinterpret_cast<void*>(embedBuffer.data()),
                        &profileSize);

    TIFFSetField(tif, TIFFTAG_ICCPROFILE, profileSize,
                 reinterpret_cast<void*>(embedBuffer.data()) );


    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);	// TODO what about others?
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(uint8_t));

    tsize_t strip_size = TIFFStripSize(tif);
    tstrip_t strips_num = TIFFNumberOfStrips(tif);

    char *strip_buf = (char *)_TIFFmalloc (strip_size);	//enough space for a strip
    if (!strip_buf)
    {
		TIFFClose(tif);
        throw std::runtime_error ("TIFF: error allocating buffer");
    }

    QRgb *ldrpixels = reinterpret_cast<QRgb*>(ldrimage->bits ());

    emit maximumValue (strips_num);	// for QProgressDialog
    for (unsigned int s = 0; s < strips_num; s++)
    {
        for (unsigned int col = 0; col < width; col++)
        {
            strip_buf[4 * col + 0] = qRed(ldrpixels[width * s + col]);
            strip_buf[4 * col + 1] = qGreen(ldrpixels[width * s + col]);
            strip_buf[4 * col + 2] = qBlue(ldrpixels[width * s + col]);
            strip_buf[4 * col + 3] = qAlpha(ldrpixels[width * s + col]);
        }
        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
        {
            qDebug ("error writing strip");
			TIFFClose(tif);
            return -1;
        }
        else
        {
            emit nextstep (s);	// for QProgressDialog
        }
    }
    _TIFFfree (strip_buf);
	TIFFClose(tif);

    return 0;
}

int
TiffWriter::write16bitTiff()
{
    assert(pixmap != NULL);

    if (pixmap == NULL)
    {
		TIFFClose(tif);
        throw std::runtime_error ("TIFF: 16 bits pixmap was not set correctly");
    }

    ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );
    cmsUInt32Number profileSize = 0;
    cmsSaveProfileToMem (hsRGB.data(), NULL, &profileSize);	// get the size

    std::vector<char> embedBuffer(profileSize);

    cmsSaveProfileToMem(hsRGB.data(),
                        reinterpret_cast<void*>(embedBuffer.data()),
                        &profileSize);

    TIFFSetField(tif, TIFFTAG_ICCPROFILE, profileSize,
                 reinterpret_cast<void*>(embedBuffer.data()) );

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);	// TODO what about others?
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE,
                  (uint16_t)8*(uint16_t)sizeof(uint16_t));

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);

    quint16 *strip_buf = (quint16 *) _TIFFmalloc (strip_size);	//enough space for a strip
    if (!strip_buf)
    {
		TIFFClose(tif);
        throw std::runtime_error ("TIFF: error allocating buffer");
    }

    emit maximumValue (strips_num);	// for QProgressDialog

    for (unsigned int s = 0; s < strips_num; s++)
    {
        for (unsigned int col = 0; col < width; col++)
        {
            strip_buf[3 * col]      = pixmap[3 * (width * s + col)];
            strip_buf[3 * col + 1]  = pixmap[3 * (width * s + col) + 1];
            strip_buf[3 * col + 2]  = pixmap[3 * (width * s + col) + 2];
        }
        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
        {
            qDebug ("error writing strip");
			TIFFClose(tif);

            return -1;
        }
        else
        {
            emit nextstep (s);	// for QProgressDialog
        }
    }
    _TIFFfree (strip_buf);
	TIFFClose(tif);

    return 0;
}

// double check this? (Sept 24, 2012)
int
TiffWriter::writePFSFrame16bitTiff()
{
    assert (pfsFrame != 0);

    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);	// TODO what about others?
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);

    pfs::Channel* Xc;
    pfs::Channel* Yc;
    pfs::Channel* Zc;

    pfsFrame->getXYZChannels(Xc, Yc, Zc);

    const float *X = Xc->getRawData();
    const float *Y = Yc->getRawData();
    const float *Z = Zc->getRawData();

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);
    quint16 *strip_buf = (quint16 *) _TIFFmalloc (strip_size);	//enough space for a strip (row)
    if (!strip_buf)
    {
		TIFFClose(tif);
        throw std::runtime_error ("TIFF: error allocating buffer.");
    }

    emit maximumValue (strips_num);	// for QProgressDialog

    for (unsigned int s = 0; s < strips_num; s++)
    {
        for (unsigned int col = 0; col < width; col++)
        {
            strip_buf[3 * col + 0] = (qint16) X[s * width + col];	//(*X)(col,s);
            strip_buf[3 * col + 1] = (qint16) Y[s * width + col];	//(*Y)(col,s);
            strip_buf[3 * col + 2] = (qint16) Z[s * width + col];	//(*Z)(col,s);
        }
        if (TIFFWriteEncodedStrip(tif, s, strip_buf, strip_size) == 0)
        {
            qDebug ("error writing strip");
			TIFFClose(tif);

            return -1;
        }
        else
        {
            emit nextstep (s);	// for QProgressDialog
        }
    }
    _TIFFfree (strip_buf);
	TIFFClose(tif);

    return 0;
}

