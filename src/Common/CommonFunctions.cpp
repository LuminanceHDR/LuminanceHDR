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
    qDebug() << QString("Loading data for %1").arg(currentItem.alignedFilename());

    try
    {
        QFileInfo qfi(currentItem.filename());
        QByteArray filePath = QFile::encodeName(qfi.filePath());

        qDebug() << QString("Loading data for %1").arg(filePath.constData());

        FrameReaderPtr reader = FrameReaderFactory::open(filePath.constData());
        reader->read( *currentItem.frame(), getRawSettings() );

        // read Average Luminance
        pfs::exif::ExifData exifData(currentItem.filename().toStdString());
        currentItem.setAverageLuminance(exifData.getAverageSceneLuminance());

        // read Exposure Time
        currentItem.setExposureTime(ExifOperations::getExposureTime(filePath.constData()));

        qDebug() << QString("HdrCreationItem: Average Luminance for %1 is %2")
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
        qDebug() << QString("Cannot load %1: %2")
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

    qDebug() << QString("Saving data for %1 to %2 on %3").arg(inputFilename).arg(outputFilename).arg(tempdir);

    // save pfs::Frame as tiff 16bits or 32bits
    try
    {
        Params p;
        p.set("tiff_mode", m_mode); // 16bits or 32bits
        p.set("min_luminance", m_minLum); 
        p.set("max_luminance", m_maxLum); 
        p.set("deflateCompression", m_deflateCompression); 
        
        FrameWriterPtr writer = FrameWriterFactory::open(
                    QFile::encodeName(outputFilename).constData());
        writer->write( *currentItem.frame(), p );

    }
    catch (std::runtime_error& err)
    {
        qDebug() << QString("Cannot save %1: %2")
                    .arg(currentItem.filename())
                    .arg(QString::fromStdString(err.what()));
    }
}

void RefreshPreview::operator()(HdrCreationItem& currentItem)
{
    qDebug() << QString("Refresh preview for %1").arg(currentItem.filename());

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
        if (m != 0.0f || M != 1.0f) {
            Channel redNorm(currentItem.frame()->getWidth(),
                            currentItem.frame()->getHeight(), "X");
            pfs::colorspace::Normalizer normalize(m, M);
            ConvertToQRgb convert(2.2f);
            std::transform(red->begin(), red->end(), redNorm.begin(), normalize);
            utils::transform(redNorm.begin(), redNorm.end(), redNorm.begin(), redNorm.begin(),
                             qimageData, convert);
        }
        else
            utils::transform(red->begin(), red->end(), green->begin(), blue->begin(),
                             qimageData, ConvertToQRgb());

        currentItem.qimage().swap( tempImage );
    }
    catch (std::runtime_error& err)
    {
        qDebug() << QString("Cannot load %1: %2")
                    .arg(currentItem.filename())
                    .arg(QString::fromStdString(err.what()));
    }
}


