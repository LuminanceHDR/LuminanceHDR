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

#include "tiffreader.h"

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

#include "Common/LuminanceOptions.h"

#include <Libpfs/frame.h>
#include <Libpfs/colorspace/xyz.h>
#include <Libpfs/utils/resourcehandlerlcms.h>

using namespace pfs;
using namespace pfs::utils;

namespace
{

//! \brief This code is taken from tificc.c from libcms distribution and
//! sligthly modified
//! \ref http://svn.ghostscript.com/ghostscript/trunk/gs/lcms2/utils/tificc/tificc.c
cmsHPROFILE
GetTIFFProfile(TIFF* in)
{
    cmsHPROFILE hProfile;
    void* iccProfilePtr;
    cmsUInt32Number iccProfileSize;

    if (TIFFGetField(in, TIFFTAG_ICCPROFILE, &iccProfileSize, &iccProfilePtr))
    {
        qDebug () << "iccProfileSize: " << iccProfileSize;
        hProfile = cmsOpenProfileFromMem(iccProfilePtr, iccProfileSize);

        if (hProfile) return hProfile;
    }

    // Try to see if "colorimetric" tiff.data()
    cmsCIExyYTRIPLE primaries;
    cmsCIExyY whitePoint;
    cmsToneCurve* curve[3];

    cmsFloat32Number* chr;
    if (TIFFGetField(in, TIFFTAG_PRIMARYCHROMATICITIES, &chr))
    {
        primaries.Red.x     = chr[0];
        primaries.Red.y     = chr[1];
        primaries.Green.x   = chr[2];
        primaries.Green.y   = chr[3];
        primaries.Blue.x    = chr[4];
        primaries.Blue.y    = chr[5];

        primaries.Red.Y = primaries.Green.Y = primaries.Blue.Y = 1.0;

        cmsFloat32Number* wp;
        if (TIFFGetField (in, TIFFTAG_WHITEPOINT, &wp))
        {
            whitePoint.x = wp[0];
            whitePoint.y = wp[1];
            whitePoint.Y = 1.0;

            // Transferfunction is a bit harder....
            cmsUInt16Number *gmr;
            cmsUInt16Number *gmg;
            cmsUInt16Number *gmb;

            TIFFGetFieldDefaulted(in, TIFFTAG_TRANSFERFUNCTION, &gmr, &gmg, &gmb);

            curve[0] = cmsBuildTabulatedToneCurve16(NULL, 256, gmr);
            curve[1] = cmsBuildTabulatedToneCurve16(NULL, 256, gmg);
            curve[2] = cmsBuildTabulatedToneCurve16(NULL, 256, gmb);

            hProfile = cmsCreateRGBProfile (&whitePoint, &primaries, curve);

            cmsFreeToneCurve(curve[0]);
            cmsFreeToneCurve(curve[1]);
            cmsFreeToneCurve(curve[2]);

            return hProfile;
        }
    }

    return 0;
}
// End of code form tifficc.c

void
transform_to_rgb(unsigned char *ScanLineIn, unsigned char *ScanLineOut, uint32 size, int nSamples)
{
  for (uint32 i = 0; i < size; i += nSamples)
    {
      unsigned char C = *(ScanLineIn + i + 0);
      unsigned char M = *(ScanLineIn + i + 1);
      unsigned char Y = *(ScanLineIn + i + 2);
      unsigned char K = *(ScanLineIn + i + 3);
      *(ScanLineOut + i + 0) = ((255 - C) * (255 - K)) / 255;
      *(ScanLineOut + i + 1) = ((255 - M) * (255 - K)) / 255;
      *(ScanLineOut + i + 2) = ((255 - Y) * (255 - K)) / 255;
      *(ScanLineOut + i + 3) = 255;
    }
}

void
transform_to_rgb_16(uint16 *ScanLineIn, uint16 *ScanLineOut, uint32 size, int nSamples)
{
  for (uint32 i = 0; i < size/2; i += nSamples)
    {
      uint16 C = *(ScanLineIn + i + 0);
      uint16 M = *(ScanLineIn + i + 1);
      uint16 Y = *(ScanLineIn + i + 2);
      uint16 K = *(ScanLineIn + i + 3);
      *(ScanLineOut + i + 0) = ((65535 - C) * (65535 - K)) / 65535;
      *(ScanLineOut + i + 1) = ((65535 - M) * (65535 - K)) / 65535;
      *(ScanLineOut + i + 2) = ((65535 - Y) * (65535 - K)) / 65535;
      *(ScanLineOut + i + 3) = 65535;
    }
}

// The way QRgb is stored is a bit weird, hence the strange way to store the
// final value
void
cmyk_to_bgra_qimage(const unsigned char *inVector, unsigned char *outVector,
                    uint32 size, int nSamples)
{
    for (uint32 i = 0; i < size; i += nSamples)
    {
        unsigned char C = *(inVector + 0);
        unsigned char M = *(inVector + 1);
        unsigned char Y = *(inVector + 2);
        unsigned char K = *(inVector + 3);
        *(outVector + 2) = ((255 - C) * (255 - K)) / 255;   // RED
        *(outVector + 1) = ((255 - M) * (255 - K)) / 255;   // GREEN
        *(outVector + 0) = ((255 - Y) * (255 - K)) / 255;   // BLUE
        *(outVector + 3) = 255;

        inVector += 4;
        outVector += 4;
    }
}

const float DIV_255 = 1.f/255.f;
const float DIV_256 = 1.f/256.f;
}

TiffReader::TiffReader(const char *filename, const char *tempfilespath, bool wod):
    tif( TIFFOpen(filename, "r") ),
    writeOnDisk(wod),
    fileName(filename),
    tempFilesPath(tempfilespath)
{
    if (!tif)
        throw std::runtime_error ("TIFF: could not open file for reading.");
    // read header containing width and height from file
    //--- image size
    TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &height);

    if (width * height <= 0)
    {
        throw std::runtime_error ("TIFF: illegal image size.");
    }

    //--- image parameters
    uint16 planar;
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planar);
    qDebug() << "Planar configuration: " << planar;
    if (planar != PLANARCONFIG_CONTIG)
    {
        throw std::runtime_error ("TIFF: unsupported planar configuration");
    }

    if (!TIFFGetField(tif, TIFFTAG_COMPRESSION, &comp))	// compression type
        comp = COMPRESSION_NONE;

    // type of photometric data
    if (!TIFFGetFieldDefaulted (tif, TIFFTAG_PHOTOMETRIC, &phot))
    {
        throw std::runtime_error ("TIFF: unspecified photometric type");
    }

    qDebug () << "Photometric type : " << phot;

    uint16 *extra_sample_types = 0;
    uint16 extra_samples_per_pixel = 0;
    switch (phot)
    {
    case PHOTOMETRIC_LOGLUV:
    {
        qDebug ("Photometric data: LogLuv");
        if (comp != COMPRESSION_SGILOG && comp != COMPRESSION_SGILOG24)
        {
            throw std::runtime_error ("TIFF: only support SGILOG compressed LogLuv data");
        }
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nSamples);
        TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
        TypeOfData = FLOATLOGLUV;
    }
        break;
    case PHOTOMETRIC_RGB:
    {
        qDebug("Photometric data: RGB");
        // read extra samples (# of alpha channels)
        if (TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &extra_samples_per_pixel, &extra_sample_types) != 1)
        {
            extra_samples_per_pixel = 0;
        }
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nSamples);
        bps = nSamples - extra_samples_per_pixel;
        has_alpha = (extra_samples_per_pixel == 1);
        // qDebug("nSamples=%d extra_samples_per_pixel=%d",nSamples,extra_samples_per_pixel);
        // qDebug("has alpha? %s", has_alpha ? "true" : "false");
        if (bps != 3)
        {
            qDebug ("TIFF: unsupported samples per pixel for RGB");
            throw std::runtime_error ("TIFF: unsupported samples per pixel for RGB");
        }
        if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps) || (bps != 8 && bps != 16 && bps != 32))
        {
            qDebug ("TIFF: unsupported bits per sample for RGB");
            throw std::runtime_error ("TIFF: unsupported bits per sample for RGB");
        }

        switch (bps)
        {
        case 8:
            TypeOfData = BYTE;
            qDebug ("8bit per channel");
            break;
        case 16:
            TypeOfData = WORD;
            qDebug ("16bit per channel");
            break;
        default:
            TypeOfData = FLOAT;
            qDebug ("32bit float per channel");
            break;
        }
        ColorSpace = RGB;
    }
        break;
    case PHOTOMETRIC_SEPARATED:
    {
        qDebug("Photometric data: CMYK");
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nSamples);
        qDebug() << "nSamples: " << nSamples;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);

        switch (bps)
        {
        case 8:
            TypeOfData = BYTE;
            qDebug ("8bit per channel");
            break;
        case 16:
            TypeOfData = WORD;
            qDebug ("16bit per channel");
            break;
        default:
            TypeOfData = FLOAT;
            qDebug ("32bit float per channel");
            break;
        }
        ColorSpace = CMYK;
    }
        break;
    default:
        //qFatal("Unsupported photometric type: %d",phot);
        throw std::runtime_error ("TIFF: unsupported photometric type");
    }

    if (!TIFFGetField(tif, TIFFTAG_STONITS, &stonits))
        stonits = 1.;
}

pfs::Frame*
TiffReader::readIntoPfsFrame()
{
    qDebug() << "TiffReader::readIntoPfsFrame()";

    bool doTransform = false;
    // LuminanceOptions luminance_opts;
    // int camera_profile_opt = luminance_opts.getCameraProfile ();

    // will get automatigically cleaned on return of this function!
    ScopedCmsTransform xform;

//    if (camera_profile_opt == 1) // embedded
//    {
    ScopedCmsProfile hIn( GetTIFFProfile(tif) );

    if (hIn)
    {
        qDebug () << "Found ICC profile";

        ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );

        cmsUInt32Number cmsInputFormat = TYPE_CMYK_8;
        cmsUInt32Number cmsOutputFormat = TYPE_RGBA_8;
        cmsUInt32Number cmsIntent = INTENT_PERCEPTUAL;

        if (ColorSpace == RGB && TypeOfData == WORD) {
            if (has_alpha) {
                cmsInputFormat = TYPE_RGBA_16;
                cmsOutputFormat = TYPE_RGBA_16;
            } else {
                cmsInputFormat = TYPE_RGB_16;
                cmsOutputFormat = TYPE_RGB_16;
            }
        }
        else if (ColorSpace == RGB && TypeOfData == BYTE) {
            if (has_alpha) {
                cmsInputFormat = TYPE_RGBA_8;
                cmsOutputFormat = TYPE_RGBA_8;
            } else {
                cmsInputFormat = TYPE_RGB_8;
                cmsOutputFormat = TYPE_RGB_8;
            }
        }
        else if (ColorSpace == CMYK && TypeOfData == WORD) {
            cmsInputFormat = TYPE_CMYK_16;
            cmsOutputFormat = TYPE_RGBA_16;
        }
        else {
            cmsInputFormat = TYPE_CMYK_8;
            cmsOutputFormat = TYPE_RGBA_8;
        }

        xform.reset( cmsCreateTransform (hIn.data(), cmsInputFormat, hsRGB.data(), cmsOutputFormat, cmsIntent, 0) );
        if (xform)
        {
            doTransform = true;
            qDebug () << "Created transform";
        }
    }
#ifdef QT_DEBUG
    else
    {
        qDebug () << "No embedded profile found";
    }
#endif
//    }
//    else if (camera_profile_opt == 2)   // from file
//    {
//        QString profile_fname = luminance_opts.getCameraProfileFileName ();
//        qDebug () << "Camera profile: " << profile_fname;

//        if (!profile_fname.isEmpty ())
//        {
//            QByteArray ba( QFile::encodeName( profile_fname ) );

//            ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );
//            ScopedCmsProfile hIn( cmsOpenProfileFromFile (ba.data (), "r") );

//            if ( hsRGB && hIn )
//            {
//                if (ColorSpace == RGB && TypeOfData == WORD)
//                    xform.reset( cmsCreateTransform (hIn.data(), TYPE_RGB_16, hsRGB.data(), TYPE_RGB_16, INTENT_PERCEPTUAL, 0) );
//                else if (ColorSpace == RGB && TypeOfData == BYTE)
//                    xform.reset( cmsCreateTransform (hIn.data(), TYPE_RGB_8, hsRGB.data(), TYPE_RGB_8, INTENT_PERCEPTUAL, 0) );
//                else if (ColorSpace == CMYK && TypeOfData == WORD)
//                    xform.reset( cmsCreateTransform (hIn.data(), TYPE_CMYK_16, hsRGB.data(), TYPE_RGBA_16, INTENT_PERCEPTUAL, 0) );
//                else
//                    xform.reset( cmsCreateTransform (hIn.data(), TYPE_CMYK_8, hsRGB.data(), TYPE_RGBA_8, INTENT_PERCEPTUAL, 0) );
//            }
//            doTransform = true;

//            qDebug () << "Created transform";
//        }
//    }

    pfs::Frame* frame = new pfs::Frame (width, height);

    pfs::Channel* Xc;
    pfs::Channel* Yc;
    pfs::Channel* Zc;
    frame->createXYZChannels (Xc, Yc, Zc);

    float* X = Xc->data();
    float* Y = Yc->data();
    float* Z = Zc->data();

    //--- image length
    uint32 imagelength;
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imagelength);

    emit maximumValue(imagelength);	//for QProgressDialog

    //--- image scanline size
    uint32 scanlinesize = TIFFScanlineSize(tif);
    std::vector<uchar> buf( scanlinesize );
    std::vector<uchar> buf2;
    if ( xform )
    {
        buf2.resize( scanlinesize );
    }

    qDebug () << "scanlinesize: " << scanlinesize;
    //--- read scan lines
    const int  image_width = width;

    for (uint32 row = 0; row < height; row++)
    {
        switch (TypeOfData)
        {
        // float 32bit/channel
        case FLOAT:
        {
#ifndef NDEBUG
            std::clog << "TIFF float 32bit/channel Reader";
#endif
            float* buf_fp = reinterpret_cast<float*>(buf.data());

            TIFFReadScanline (tif, buf_fp, row);
            for (int i = 0; i < image_width; i++)
            {
                X[row * image_width + i] = buf_fp[i * nSamples];
                Y[row * image_width + i] = buf_fp[i * nSamples + 1];
                Z[row * image_width + i] = buf_fp[i * nSamples + 2];
            }
        }
            break;
        // float LogLuv
        case FLOATLOGLUV:
        {
#ifndef NDEBUG
            std::clog << "TIFF LogLuv Reader";
#endif
            pfs::colorspace::ConvertXYZ2RGB xyz2rgb;

            float* buf_fp = reinterpret_cast<float*>(buf.data());

            TIFFReadScanline (tif, buf_fp, row);
            for (int i = 0; i < image_width; i++) {
                xyz2rgb(buf_fp[i * nSamples],
                        buf_fp[i * nSamples + 1],
                        buf_fp[i * nSamples + 2],
                        X[row * image_width + i],
                        Y[row * image_width + i],
                        Z[row * image_width + i]);
            }
        }
            break;
        case WORD:
        {
            uint16* buf_wp = reinterpret_cast<uint16*>(buf.data());

            TIFFReadScanline(tif, buf_wp, row);
            if (doTransform)
            {
                uint16* buf_wp_2 = reinterpret_cast<uint16*>(buf2.data());

                cmsDoTransform(xform.data(), buf_wp, buf_wp_2, image_width);

                ::std::swap(buf_wp, buf_wp_2);
            }
            else if (ColorSpace == CMYK)
            {
                transform_to_rgb_16(buf_wp, buf_wp, scanlinesize, nSamples);
            }
            for (int i = 0; i < image_width; i++)
            {
                X[row * image_width + i] = buf_wp[i * nSamples];
                Y[row * image_width + i] = buf_wp[i * nSamples + 1];
                Z[row * image_width + i] = buf_wp[i * nSamples + 2];
            }
        }
            break;
        case BYTE:
        {
            uint8* buf_bp = reinterpret_cast<uint8*>(buf.data());

            TIFFReadScanline(tif, buf_bp, row);
            if (doTransform)
            {
                uint8* buf_bp_2 = reinterpret_cast<uint8*>(buf2.data());

                cmsDoTransform(xform.data(), buf_bp, buf_bp_2, image_width);

                ::std::swap(buf_bp, buf_bp_2);
            }
            else if (ColorSpace == CMYK)
            {
                transform_to_rgb(buf_bp, buf_bp, scanlinesize, nSamples);
            }
            for (int i = 0; i < image_width; i++)
            {
                X[row * image_width + i] = powf(buf_bp[i * nSamples] * DIV_255, 2.2f); // why?
                Y[row * image_width + i] = powf(buf_bp[i * nSamples + 1] * DIV_255, 2.2f); // why?
                Z[row * image_width + i] = powf(buf_bp[i * nSamples + 2] * DIV_255, 2.2f); // why?
            }
        }
            break;
        }
        emit nextstep (row);	//for QProgressDialog
    }

    if (writeOnDisk)
    {
        assert (TypeOfData != FLOAT);
        assert (TypeOfData != FLOATLOGLUV);

        float scaleFactor = DIV_256;
        if ( TypeOfData == BYTE) scaleFactor = 1.0f;

        pfs::Channel *Xc, *Yc, *Zc;
        frame->createXYZChannels (Xc, Yc, Zc);

        float* X = Xc->data();
        float* Y = Yc->data();
        float* Z = Zc->data();

        QImage remapped( image_width, imagelength, QImage::Format_RGB32);

        for (uint32 row = 0; row < height; ++row)
        {
            QRgb* line =  reinterpret_cast<QRgb*>(remapped.scanLine(row));
            for (uint32 col = 0; col < width; ++col)
            {
                line[col] = qRgb(static_cast<char>(*X * scaleFactor),
                                 static_cast<char>(*Y * scaleFactor),
                                 static_cast<char>(*Z * scaleFactor));

                X++; Y++; Z++;
            }
        }
        QFileInfo fi (fileName);
        QString fname = fi.completeBaseName () + ".thumb.jpg";

        remapped.scaledToHeight(imagelength / 10).save(tempFilesPath + "/" + fname);
    }

    //if (TypeOfData==FLOATLOGLUV)
    //  pfs::transformColorSpace( pfs::CS_XYZ, X,Y,Z, pfs::CS_RGB, X,Y,Z );
	TIFFClose(tif);

    return frame;
}

// given for granted that users of this function call it only after checking that TypeOfData==BYTE
QImage*
TiffReader::readIntoQImage()
{
#ifdef QT_DEBUG
    qDebug() << "TiffReader::readIntoQImage()";
#endif
    assert(TypeOfData == BYTE);

    bool doTransform = false;

    ScopedCmsProfile hIn( GetTIFFProfile(tif) );
    ScopedCmsProfile hsRGB( cmsCreate_sRGBProfile() );

    ScopedCmsTransform xform;

    if ( hIn && hsRGB )
    {
        qDebug () << "Found ICC profile";

        cmsUInt32Number cmsInputFormat = TYPE_CMYK_8;
        const cmsUInt32Number cmsOutputFormat = TYPE_BGRA_8; // RGBA_8;
        const cmsUInt32Number cmsIntent = INTENT_PERCEPTUAL;

        if (has_alpha && ColorSpace == RGB && TypeOfData == BYTE)
        {
            cmsInputFormat = TYPE_RGBA_8;
        }
        else if (!has_alpha && ColorSpace == RGB && TypeOfData == BYTE)
        {
            cmsInputFormat = TYPE_RGB_8;
        }
        else if (ColorSpace == CMYK && TypeOfData == BYTE)
        {
            cmsInputFormat = TYPE_CMYK_8;
        } else
        {
			TIFFClose(tif);
            throw std::runtime_error("TiffReader: Unsupported colorspace combination");
        }

        xform.reset( cmsCreateTransform (hIn.data(), cmsInputFormat,
                                         hsRGB.data(), cmsOutputFormat,
                                         cmsIntent, 0) );
        if ( xform )
        {
            doTransform = true;
        }
        else
        {
            doTransform = false;
        }
    }
    else
    {
        doTransform = false;
#ifdef QT_DEBUG
        qDebug () << "No embedded profile found";
#endif
    }

    QScopedPointer<QImage> toReturn( new QImage(width, height, QImage::Format_RGB32) );

    //--- image length
    uint32 imagelength;
    TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &imagelength);

    //--- image scanline size
    uint32 scanlinesize = TIFFScanlineSize(tif);

    qDebug() << "Scanlinesize:" << scanlinesize;

    std::vector<uint8> buffer(scanlinesize);
    std::vector<uint8> bufferConverted;
    if ( doTransform )
    {
        bufferConverted.resize((width << 2));
    }

    qDebug() << "Do Transform: " << doTransform;

    //--- read scan lines
    if ( doTransform )
    {
        // color-converted branch!
        for (uint y = 0; y < height; ++y)
        {
            TIFFReadScanline(tif, buffer.data(), y);

            cmsDoTransform(xform.data(),
                           buffer.data(),
                           toReturn->scanLine(y), width);
        }
    }
    else
    {
        // no color-conversion
        switch (ColorSpace)
        {
        case CMYK:
        {
            for (uint y = 0; y < height; ++y)
            {
                TIFFReadScanline(tif, buffer.data(), y);

                cmyk_to_bgra_qimage(buffer.data(),
                                    toReturn->scanLine(y),
                                    scanlinesize,
                                    nSamples);
            }
        } break;
        case RGB:
        {
            for (uint y = 0; y < height; ++y)
            {
                QRgb* qImageData = reinterpret_cast<QRgb*>(toReturn->scanLine(y));
                TIFFReadScanline(tif, buffer.data(), y);

                // it's not really efficient, but I hope it doesn't get used
                // too many times in a real world scenario!
                for (uint x = 0; x < width; x++)
                {
                    qImageData[x] = qRgba(buffer[(x * nSamples)],
                                          buffer[(x * nSamples) + 1],
                                          buffer[(x * nSamples) + 2],
                                          has_alpha ? buffer[(x * nSamples) + 3] : 0xFF);
                }
            }
        } break;
        }
    }
	TIFFClose(tif);

    return toReturn.take();
}


