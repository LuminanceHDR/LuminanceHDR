/**
 * @brief Tiff facilities
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006 Giuseppe Rota
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
 * slightly modified by Giuseppe Rota <grota@sourceforge.net> for luminance
 */

#include <cmath>
#include <QObject>
#include <QSysInfo>
#include <QFileInfo>
#include <iostream>
#include <assert.h>

#include "pfstiff.h"

#include "Libpfs/frame.h"
#include "Libpfs/domio.h"

TiffReader::TiffReader( const char* filename, const char *tempfilespath, bool wod)
{
    // read header containing width and height from file
    fileName = QString(filename);
    tempFilesPath = QString(tempfilespath);
    writeOnDisk = wod;

    tif = TIFFOpen(filename, "r");
    if( !tif )
        throw pfs::Exception("TIFF: could not open file for reading.");

    //--- image size
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    if ( width*height<=0 )
    {
        TIFFClose(tif);
        throw pfs::Exception("TIFF: illegal image size");
    }

    //--- image parameters
    if (!TIFFGetField(tif, TIFFTAG_COMPRESSION, &comp)) // compression type
        comp = COMPRESSION_NONE;

    // type of photometric data
    if(!TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &phot))
        throw pfs::Exception("TIFF: unspecified photometric type");

    uint16 * extra_sample_types=0;
    uint16 extra_samples_per_pixel=0;
    switch(phot)
    {
    case PHOTOMETRIC_LOGLUV:
        qDebug("Photometric data: LogLuv");
        if (comp != COMPRESSION_SGILOG && comp != COMPRESSION_SGILOG24)
        {
            TIFFClose(tif);
            throw pfs::Exception("TIFF: only support SGILOG compressed LogLuv data");
        }
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nSamples);
        TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
        TypeOfData = FLOATLOGLUV;
        break;
    case PHOTOMETRIC_RGB:
        //       qDebug("Photometric data: RGB");
        // read extra samples (# of alpha channels)
        if (TIFFGetField( tif, TIFFTAG_EXTRASAMPLES,
                          &extra_samples_per_pixel, &extra_sample_types )!=1)
        {
            extra_samples_per_pixel=0;
        }
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nSamples);
        bps = nSamples - extra_samples_per_pixel;
        has_alpha=(extra_samples_per_pixel==1);
        //       qDebug("nSamples=%d extra_samples_per_pixel=%d",nSamples,extra_samples_per_pixel);
        //       qDebug("has alpha? %s", has_alpha ? "true" : "false");
        if (bps!=3)
        {
            qDebug("TIFF: unsupported samples per pixel for RGB");
            TIFFClose(tif);
            throw pfs::Exception("TIFF: unsupported samples per pixel for RGB");
        }
        if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps) || (bps!=8 && bps!=16 && bps!=32))
        {
            qDebug("TIFF: unsupported bits per sample for RGB");
            TIFFClose(tif);
            throw pfs::Exception("TIFF: unsupported bits per sample for RGB");
        }

        if( bps==8 )
        {
            TypeOfData = BYTE;
            qDebug("8bit per channel");
        }
        else if( bps==16 )
        {
            TypeOfData = WORD;
            qDebug("16bit per channel");
        }
        else
        {
            TypeOfData = FLOAT;
            qDebug("32bit float per channel");
        }
        break;
    default:
        //qFatal("Unsupported photometric type: %d",phot);
        TIFFClose(tif);
        throw pfs::Exception("TIFF: unsupported photometric type");
    }

    if (!TIFFGetField(tif, TIFFTAG_STONITS, &stonits))
        stonits = 1.;
}

pfs::Frame* TiffReader::readIntoPfsFrame()
{
    //--- scanline buffer with pointers to different data types
    union {
        float* fp;
        uint16* wp;
        uint8* bp;
        void* vp;
    } buf;

    uchar *data = NULL; // ?

    //pfs::DOMIO pfsio;
    //pfs::Frame *frame = pfsio.createFrame( width, height );
    pfs::Frame *frame = new pfs::Frame(width, height);

    pfs::Channel *Xc, *Yc, *Zc;
    frame->createXYZChannels( Xc, Yc, Zc );

    float * X = Xc->getRawData();
    float * Y = Yc->getRawData();
    float * Z = Zc->getRawData();

    //--- image length
    uint32 imagelength;
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);

    emit maximumValue( imagelength ); //for QProgressDialog

    if (writeOnDisk)
    {
        data = new uchar[width*height*4]; //this will contain the image data linearly remapped to 8bit per channel,  data must be 32-bit aligned, in Format: 0xffRRGGBB
        if (data == NULL)
            throw pfs::Exception("TIFF: Memory error");
    }

    //--- image scanline size
    uint32 scanlinesize = TIFFScanlineSize(tif);
    buf.vp = _TIFFmalloc(scanlinesize);

    //--- read scan lines
    const int image_width = width; //X->getCols();
    for(uint32 row = 0; row < imagelength; row++)
    {
        switch(TypeOfData)
        {
        case FLOAT:
        case FLOATLOGLUV:
            TIFFReadScanline(tif, buf.fp, row);
            for( int i = 0; i < image_width; i++ )
            {
                X[row*image_width + i] = buf.fp[i*nSamples];
                Y[row*image_width + i] = buf.fp[i*nSamples+1];
                Z[row*image_width + i] = buf.fp[i*nSamples+2];
            }
            break;
      case WORD:
            TIFFReadScanline(tif, buf.wp, row);
            for( int i=0; i < image_width; i++ )
            {
                X[row*image_width + i] = buf.wp[i*nSamples];
                Y[row*image_width + i] = buf.wp[i*nSamples+1];
                Z[row*image_width + i] = buf.wp[i*nSamples+2];
                if (writeOnDisk)
                {
                    *(data + 0 + (row*width+i)*4) = buf.wp[i*nSamples+2]/256.0 ;
                    *(data + 1 + (row*width+i)*4) = buf.wp[i*nSamples+1]/256.0 ;
                    *(data + 2 + (row*width+i)*4) = buf.wp[i*nSamples]/256.0 ;
                    *(data + 3 + (row*width+i)*4) = 0xff;
                }
            }
            break;
      case BYTE:
            TIFFReadScanline(tif, buf.bp, row);
            for( int i=0; i<image_width; i++ )
            {
                X[row*image_width + i] = pow( buf.bp[i*nSamples]/255.0, 2.2 );
                Y[row*image_width + i] = pow( buf.bp[i*nSamples+1]/255.0, 2.2 );
                Z[row*image_width + i] = pow( buf.bp[i*nSamples+2]/255.0, 2.2 );
            }
            break;
        }
        emit nextstep( row ); //for QProgressDialog
    }

    if (writeOnDisk)
    {
        QImage remapped(const_cast<uchar *>(data),image_width,imagelength,QImage::Format_ARGB32);

        QFileInfo fi(fileName);
        QString fname = fi.baseName() + ".thumb.jpg";
        //std::cout << fileName.toAscii().constData() << std::endl;
        //std::cout << fname.toAscii().constData() << std::endl;
        //std::cout << tempFilesPath.toAscii().constData() << std::endl;
        remapped.scaledToHeight(imagelength/10).save(tempFilesPath + "/" + fname);
    }
    //--- free buffers and close files
    _TIFFfree(buf.vp);
    TIFFClose(tif);
    //if (TypeOfData==FLOATLOGLUV)
    //  pfs::transformColorSpace( pfs::CS_XYZ, X,Y,Z, pfs::CS_RGB, X,Y,Z );
    return frame;
}

//given for granted that users of this function call it only after checking that TypeOfData==BYTE
QImage* TiffReader::readIntoQImage()
{
    uchar *data=new uchar[width*height*4]; //this will contain the image data: data must be 32-bit aligned, in Format: 0xffRRGGBB
    // 	qDebug("pfstiff, w=%d h=%d",width,height);
    assert(TypeOfData==BYTE);

    //--- image length
    uint32 imagelength;
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);

    //--- image scanline size
    uint32 scanlinesize = TIFFScanlineSize(tif);
    uint8* bp = (uint8 *)_TIFFmalloc(scanlinesize);

    //--- read scan lines
    for (uint y = 0; y < height; y++)
    {
        TIFFReadScanline(tif, bp, y);
        for ( uint x=0; x<width; x++ )
        {
            if (QSysInfo::ByteOrder==QSysInfo::LittleEndian)
            {
                *(data + 0 + (y*width+x)*4) = bp[x*nSamples+2] ;
                *(data + 1 + (y*width+x)*4) = bp[x*nSamples+1] ;
                *(data + 2 + (y*width+x)*4) = bp[x*nSamples] ;
                if (has_alpha)
                    *(data + 3 + (y*width+x)*4) = bp[x*nSamples+3];
                else
                    *(data + 3 + (y*width+x)*4) = 0xff;
            }
            else
            {
                *(data + 3 + (y*width+x)*4) = bp[x*nSamples+2];
                *(data + 2 + (y*width+x)*4) = bp[x*nSamples+1];
                *(data + 1 + (y*width+x)*4) = bp[x*nSamples];
                if (has_alpha)
                    *(data + 0 + (y*width+x)*4) = bp[x*nSamples+3];
                else
                    *(data + 0 + (y*width+x)*4) = 0xff;
            }
        }
    }
    //--- free buffers and close files
    _TIFFfree(bp);
    TIFFClose(tif);

    QImage *toreturn = new QImage(const_cast<uchar *>(data),width,height,QImage::Format_ARGB32);
    return toreturn;
}

TiffWriter::TiffWriter( const char* filename, pfs::Frame *f ) : tif((TIFF *)NULL)
{
    f->getXYZChannels(Xc, Yc, Zc);
    width   = Xc->getWidth();
    height  = Yc->getHeight();

    //X = Xc->getChannelData();
    //Y = Yc->getChannelData();
    //Z = Zc->getChannelData();

    // 	qDebug("width=%d, heigh=%d",width,height);
    tif = TIFFOpen(filename, "w");
    if( !tif )
        throw pfs::Exception("TIFF: could not open file for writing.");

    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, 1);
}

TiffWriter::TiffWriter( const char* filename, QImage *f ) : tif((TIFF *)NULL)
{
    ldrimage=f;
    width   =f->width();
    height  =f->height();
    tif = TIFFOpen(filename, "w");

    if( !tif )
        throw pfs::Exception("TIFF: could not open file for writing.");

    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, f->width());
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, f->height());
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, EXTRASAMPLE_ASSOCALPHA);
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, 1);
}

//write 32 bit float Tiff from pfs::Frame
int TiffWriter::writeFloatTiff()
{ 
    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE); // TODO what about others?
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);

    const float* X = Xc->getRawData();
    const float* Y = Yc->getRawData();
    const float* Z = Zc->getRawData();

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);
    float* strip_buf = (float*)_TIFFmalloc(strip_size); //enough space for a strip (row)
    if (!strip_buf)
        throw pfs::Exception("TIFF: error allocating buffer.");

    emit maximumValue( strips_num );  // for QProgressDialog

    for (unsigned int s = 0; s < strips_num; s++)
    {
        for (unsigned int col = 0; col < width; col++)
        {
            strip_buf[3*col + 0] = X[s*width + col]; //(*X)(col,s);
            strip_buf[3*col + 1] = Y[s*width + col]; //(*Y)(col,s);
            strip_buf[3*col + 2] = Z[s*width + col]; //(*Z)(col,s);
        }
        if (TIFFWriteEncodedStrip (tif, s, strip_buf, strip_size) == 0)
        {
            qDebug("error writing strip");
            return -1;
        }
        else
        {
            emit nextstep( s );  // for QProgressDialog
        }
    }
    _TIFFfree(strip_buf);
    TIFFClose(tif);
    return 0;
}

//write LogLUv Tiff from pfs::Frame
int TiffWriter::writeLogLuvTiff()
{ 
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_SGILOG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LOGLUV);
    TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
    TIFFSetField(tif, TIFFTAG_STONITS, 1.);   /* not known */

    const float* X = Xc->getRawData();
    const float* Y = Yc->getRawData();
    const float* Z = Zc->getRawData();

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);
    float* strip_buf=(float*)_TIFFmalloc(strip_size); //enough space for a strip
    if (!strip_buf)
        throw pfs::Exception("TIFF: error allocating buffer.");

    emit maximumValue( strips_num ); // for QProgressDialog

    for (unsigned int s=0; s<strips_num; s++)
    {
        for (unsigned int col=0; col<width; col++)
        {
            strip_buf[3*col + 0] = X[s*width + col]; //(*X)(col,s);
            strip_buf[3*col + 1] = Y[s*width + col]; //(*Y)(col,s);
            strip_buf[3*col + 2] = Z[s*width + col]; //(*Z)(col,s);
        }
        if (TIFFWriteEncodedStrip (tif, s, strip_buf, strip_size) == 0)
        {
            qDebug("error writing strip");
            return -1;
        }
        else
        {
            emit nextstep( s ); // for QProgressDialog
        }
    }

    _TIFFfree(strip_buf);
    TIFFClose(tif);
    return 0;
}

int TiffWriter::write8bitTiff()
{
    if (ldrimage == NULL)
        throw pfs::Exception("TIFF: QImage was not set correctly");

    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE); // TODO what about others?
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);

    tsize_t strip_size = TIFFStripSize (tif);
    tstrip_t strips_num = TIFFNumberOfStrips (tif);

    char* strip_buf=(char*)_TIFFmalloc(strip_size); //enough space for a strip
    if (!strip_buf)
        throw pfs::Exception("TIFF: error allocating buffer");

    QRgb *ldrpixels = reinterpret_cast<QRgb*>(ldrimage->bits());

    emit maximumValue( strips_num ); // for QProgressDialog
    for (unsigned int s=0; s<strips_num; s++)
    {
        for (unsigned int col=0; col<width; col++)
        {
            //qRed  (*( (QRgb*)( ldrimage->bits() ) + width*s + col ));
            strip_buf[4*col+0] = qRed(ldrpixels[width*s + col]);
            //qGreen(*( (QRgb*)( ldrimage->bits() ) + width*s + col ));
            strip_buf[4*col+1] = qGreen(ldrpixels[width*s + col]);
            //qBlue (*( (QRgb*)( ldrimage->bits() ) + width*s + col ));
            strip_buf[4*col+2] = qBlue(ldrpixels[width*s + col]);
            //qAlpha(*( (QRgb*)( ldrimage->bits() ) + width*s + col ));
            strip_buf[4*col+3] = qAlpha(ldrpixels[width*s + col]);
        }
        if (TIFFWriteEncodedStrip (tif, s, strip_buf, strip_size) == 0)
        {
            qDebug("error writing strip");
            return -1;
        }
        else
        {
            emit nextstep( s ); // for QProgressDialog
        }
    }
    _TIFFfree(strip_buf);
    TIFFClose(tif);
    return 0;
}
