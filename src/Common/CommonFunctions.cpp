/**
 * This file is a part of LuminanceHDR package.
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
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#include <QUuid>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QRgb>
#include <QByteArray>

#include "CommonFunctions.h"
#include "LuminanceOptions.h"
#include "Exif/ExifOperations.h"
#include <Libpfs/frame.h>
#include <Libpfs/params.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/io/tiffwriter.h>
#include <Libpfs/io/tiffreader.h>
#include <Libpfs/io/framereader.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/exif/exifdata.hpp>
#include <Libpfs/utils/transform.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/manip/rotate.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/manip/cut.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/colorspace/normalizer.h>

#include <boost/scoped_ptr.hpp>

#include "Core/IOWorker.h"


using namespace std;
using namespace pfs;
using namespace pfs::io;

void computeAutolevels(QImage* data, float &minL, float &maxL, float &gammaL)
{
    const QRgb* src = reinterpret_cast<const QRgb*>(data->bits());
    const int width = data->width();
    const int height = data->height();
    const int ELEMENTS = width * height;

    //Convert to lightness
    int *lightness = new int[ELEMENTS];

    for (int i = 0;  i < ELEMENTS; i++)
    {
        lightness[i] = QColor::fromRgb(src[i]).toHsl().lightness();
    }

    //Build histogram
    int hist[256];
    for (int i = 0; i < 256; ++i) hist[i] = 0;

    for (int i = 0; i < ELEMENTS; ++i)
    {
        hist[ lightness[i] ] += 1;
    }

    //Scaling factor: range [0..1]
    const float factor = 1.f/255.f;

    //Compute mean and threshold
    const float meanL = accumulate(lightness, lightness + ELEMENTS, 0.f)/ELEMENTS;
    const int threshold = ceil(meanL/10.f);

    //Start from midrange
    int xa = 120;
    int xb = 132;

    if (hist[0] >= threshold)
        xa = 0;
    if (hist[255] >= threshold)
        xb = 255;

    while (true)
    {
        int count = 0;
        for (int i = xa; i <= xb; i++)
        {
            if (hist[i] >= threshold)
                count += hist[i];
        }

        if ((float)count/(float)ELEMENTS > 0.9f)
            break;
        else {
            if (hist[xa] > threshold)
                xa--;
            else if (hist[xa] == threshold)
                continue;
            else
                xa++;
            if (hist[xb] > threshold)
                xb++;
            else if (hist[xb] == threshold)
                continue;
            else
                xb--;
        }
        if (xa < 0)
            xa = 0;
        if (xb > 255)
            xb = 255;
    }
    minL = factor*xa;
    maxL = factor*xb;
    float midrange = minL + .5f*(maxL - minL);
    if (abs(midrange-.5f) < 1e-3)
        gammaL = 1.f;
    else
        gammaL = log10(midrange)/log10(factor*meanL);

#ifndef NDEBUG
    cout << "minL: " << minL << ", maxL: " << maxL << ", gammaL: " << gammaL << endl;
    cout << "ithreshold: " << threshold << ", threshold: " << factor*threshold << endl;
    cout << "midrange: " << midrange << endl;
#endif

    delete[] lightness;
}


ConvertToQRgb::ConvertToQRgb(float gamma)
    : gamma(1.0f/gamma)
{
}

void ConvertToQRgb::operator()(float r, float g, float b, QRgb& rgb) const
{
    uint8_t r8u;
    uint8_t g8u;
    uint8_t b8u;
    if (gamma == 1.0f)
    {
        r8u = colorspace::convertSample<uint8_t>(r);
        g8u = colorspace::convertSample<uint8_t>(g);
        b8u = colorspace::convertSample<uint8_t>(b);
    }
    else
    {
        r8u = colorspace::convertSample<uint8_t>(pow(r, gamma));
        g8u = colorspace::convertSample<uint8_t>(pow(g, gamma));
        b8u = colorspace::convertSample<uint8_t>(pow(b, gamma));
    }

    rgb = qRgb(r8u, g8u, b8u);
}

void LoadFile::operator()(HdrCreationItem& currentItem)
{
    if (currentItem.filename().isEmpty())
    {
        return;
    }

    QFileInfo qfi(currentItem.alignedFilename());

    try
    {
        QByteArray filePath = QFile::encodeName(qfi.filePath());

        qDebug() << QString("LoadFile: Loading data for %1").arg(filePath.constData());

        FrameReaderPtr reader = FrameReaderFactory::open(filePath.constData());
        reader->read( *currentItem.frame(), getRawSettings() );

        // read Average Luminance
        pfs::exif::ExifData exifData(currentItem.filename().toStdString());
        currentItem.setAverageLuminance(exifData.getAverageSceneLuminance());

        // read Exposure Time
        currentItem.setExposureTime(ExifOperations::getExposureTime(filePath.constData()));

        qDebug() << QString("LoadFile: Average Luminance for %1 is %2")
                    .arg(currentItem.filename())
                    .arg(currentItem.getAverageLuminance());

        // build QImage
        QImage tempImage(currentItem.frame()->getWidth(),
                         currentItem.frame()->getHeight(),
                         QImage::Format_ARGB32_Premultiplied);

        QRgb* qimageData = reinterpret_cast<QRgb*>(tempImage.bits());

        Channel* red;
        Channel* green;
        Channel* blue;
        currentItem.frame()->getXYZChannels(red, green, blue);

        if (red == NULL || green == NULL || blue == NULL)
        {
            throw std::runtime_error("Null frame");
        }

        utils::transform(red->begin(), red->end(), green->begin(), blue->begin(),
                         qimageData, ConvertToQRgb());

        currentItem.qimage().swap( tempImage );
    }
    catch (std::runtime_error& err)
    {
        qDebug() << QString("LoadFile: Cannot load %1: %2")
                    .arg(currentItem.filename())
                    .arg(QString::fromStdString(err.what()));
    }
}

SaveFile::SaveFile(int mode, float minLum, float maxLum, bool deflateCompression) : 
    m_mode(mode),
    m_minLum(minLum),
    m_maxLum(maxLum),
    m_deflateCompression(deflateCompression)
{
}

void SaveFile::operator()(HdrCreationItem& currentItem)
{
    QUuid uuid = QUuid::createUuid();
    QString inputFilename = currentItem.filename();

    QString tempdir = LuminanceOptions().getTempDir();

    QString outputFilename = tempdir + "/" + uuid.toString() + ".tif";
    currentItem.setConvertedFilename(outputFilename);

    qDebug() << QString("SaveFile: Saving data for %1 to %2 on %3").arg(inputFilename).arg(outputFilename).arg(tempdir);

    // save pfs::Frame as tiff 16bits or 32bits
    try
    {
        Params p;
        p.set("tiff_mode", m_mode); // 16bits or 32bits
        p.set("min_luminance", m_minLum); 
        p.set("max_luminance", m_maxLum); 
        p.set("deflateCompression", m_deflateCompression); 
        
        FrameWriterPtr writer = FrameWriterFactory::open(
				QFile::encodeName(outputFilename).constData(), p);
        writer->write( *currentItem.frame(), p );

    }
    catch (std::runtime_error& err)
    {
        qDebug() << QString("SaveFile: Cannot save %1: %2")
                    .arg(currentItem.filename())
                    .arg(QString::fromStdString(err.what()));
    }
}

void RefreshPreview::operator()(HdrCreationItem& currentItem)
{
    qDebug() << QString("RefreshPreview: Refresh preview for %1").arg(currentItem.filename());

    try
    {
        // build QImage
        QImage tempImage(currentItem.frame()->getWidth(),
                         currentItem.frame()->getHeight(),
                         QImage::Format_ARGB32_Premultiplied);

        QRgb* qimageData = reinterpret_cast<QRgb*>(tempImage.bits());

        Channel* red;
        Channel* green;
        Channel* blue;
        currentItem.frame()->getXYZChannels(red, green, blue);

        float m = *std::min_element(red->begin(), red->end());
        float M = *std::max_element(red->begin(), red->end());
        if (m != 0.0f || M != 1.0f)
        {
            Channel redNorm(currentItem.frame()->getWidth(),
                            currentItem.frame()->getHeight(), "X");
            pfs::colorspace::Normalizer normalize(m, M);
            ConvertToQRgb convert(2.2f);
            std::transform(red->begin(), red->end(), redNorm.begin(), normalize);
            utils::transform(redNorm.begin(), redNorm.end(), redNorm.begin(), redNorm.begin(),
                             qimageData, convert);
        }
        else
        {
            utils::transform(red->begin(), red->end(), green->begin(), blue->begin(),
                             qimageData, ConvertToQRgb());
        }

        currentItem.qimage().swap( tempImage );
    }
    catch (std::runtime_error& err)
    {
        qDebug() << QString("RefreshPreview: Cannot load %1: %2")
                    .arg(currentItem.filename())
                    .arg(QString::fromStdString(err.what()));
    }
}


