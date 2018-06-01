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

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRgb>
#include <QUuid>
#include <valarray>

#include <Core/IOWorker.h>
#include <Exif/ExifOperations.h>
#include <Libpfs/colorspace/colorspace.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/frame.h>
#include <Libpfs/io/framereader.h>
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/framewriterfactory.h>
#include <Libpfs/io/tiffreader.h>
#include <Libpfs/io/tiffwriter.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/manip/cut.h>
#include <Libpfs/manip/rotate.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/params.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/transform.h>
#include <Libpfs/exif/exifdata.hpp>
#include <Common/CommonFunctions.h>
#include <Common/LuminanceOptions.h>

#include <boost/algorithm/minmax_element.hpp>
#include <omp.h>

using namespace std;
using namespace pfs;
using namespace pfs::io;
using namespace libhdr::fusion;

static void build_histogram(valarray<float> &hist, const valarray<uint8_t> &src) {
    const size_t size = src.size();

    size_t numThreads = 1;

#ifdef _OPENMP
    // Because we have an overhead in the critical region of the main loop for each thread
    // we make a rough calculation to reduce the number of threads for small data size.
    const size_t maxThreads = omp_get_max_threads();
    while (size > numThreads * numThreads * 16384 && numThreads < maxThreads) {
        ++numThreads;
    }
#endif

    // Original version used a valarray<float>
    // We use a valarray<uint32_t>, because incrementing a float value of 0.f by 1.f saturates at 16777215.f
    valarray<uint32_t> histInt(0u, hist.size());

#pragma omp parallel num_threads(numThreads)
{
    valarray<uint32_t> histThr(0u, hist.size());

    #pragma omp for nowait
    for (size_t i = 0; i < size; i++) {
        histThr[src[i]]++;
    }

    // add per thread histogram to global histogram
    #pragma omp critical
    for(size_t i = 0; i < hist.size(); i++) {
        histInt[i] += histThr[i];
    }
}

    // copy to float histogram
    for(size_t i = 0; i < hist.size(); i++) {
        hist[i] = histInt[i];
    }

    // find max
    float hist_max = hist.max();

    // normalize in the range [0...1]
    hist /= hist_max;
}

static void compute_histogram_minmax(const valarray<float> &hist,
                                     const float threshold, float &minHist,
                                     float &maxHist) {
    // Scaling factor: range [0..1]
    const float scalingFactor = 1.f / 255.f;

    const int hist_size = hist.size();
    float CUMUL = hist.sum();

    // Start from max hist
    float hist_max = 0.f;
    int xmax = 0;
    for (int i = 0; i < hist_size; i++) {
        if (hist[i] > hist_max) {
            hist_max = hist[i];
            xmax = i;
        }
    }

    if (xmax >= hist_size - 1) {
        xmax = hist_size - 2;
    }
    if (xmax == 0) {
        xmax = 1;
    }

    int xa = xmax - 1;
    int xb = xmax + 1;

    float CUMUL_A = hist[slice(0, xmax, 1)].sum();
    float CUMUL_B = CUMUL - CUMUL_A;

    const float Threshold_A = threshold * CUMUL_A;
    const float Threshold_B = threshold * CUMUL_B;

    float count = 0.f;
    int counter = 0;
    while (true) {
        if (counter > hist_size) break;
        
        count = hist[slice(xa, xmax - xa, 1)].sum();
        
        if (count >= Threshold_A) break;
        xa--;

        if (xa <= 0) {
            xa = 0;
            break;
        }

        counter++;
    }

    counter = 0;
    while (true) {
        if (counter > hist_size) break;

        count = hist[slice(xmax, xb - xmax, 1)].sum();

        if (count >= Threshold_B) break;

        xb++;

        if (xb >= hist_size - 1) {
            xb = hist_size - 1;
            break;
        }

        counter++;
    }
    minHist = scalingFactor * xa;
    maxHist = scalingFactor * xb;
}

void computeAutolevels(const QImage *data, const float threshold,
                       float &minHist, float &maxHist, float &gamma) {

    const int COLOR_DEPTH = 256;
    const QRgb *src = reinterpret_cast<const QRgb *>(data->bits());
    const int width = data->width();
    const int height = data->height();
    const int ELEMENTS = width * height;

    float minL, maxL;
    float minR, maxR;
    float minG, maxG;
    float minB, maxB;

    // Convert to lightness
    valarray<uint8_t> lightness(ELEMENTS);

    #pragma omp parallel for
    for (int i = 0; i < ELEMENTS; i++) {
        lightness[i] = QColor::fromRgb(src[i]).toHsl().lightness();
    }

    // Build histogram
    valarray<float> histL(0.f, COLOR_DEPTH);
    build_histogram(histL, lightness);
    compute_histogram_minmax(histL, threshold, minL, maxL);

    // get Red, Gree, Blue
    valarray<uint8_t> red(ELEMENTS);
    valarray<uint8_t> green(ELEMENTS);
    valarray<uint8_t> blue(ELEMENTS);

    #pragma omp parallel for
    for (int i = 0; i < ELEMENTS; i++) {
        red[i] = qRed(src[i]);
        green[i] = qGreen(src[i]);
        blue[i] = qBlue(src[i]);
    }

    // Build histogram
    valarray<float> histR(0.f, COLOR_DEPTH);
    build_histogram(histR, red);
    compute_histogram_minmax(histR, threshold, minR, maxR);

    // Build histogram
    valarray<float> histG(0.f, COLOR_DEPTH);
    build_histogram(histG, green);
    compute_histogram_minmax(histG, threshold, minG, maxG);

    // Build histogram
    valarray<float> histB(0.f, COLOR_DEPTH);
    build_histogram(histB, blue);
    compute_histogram_minmax(histB, threshold, minB, maxB);

    minHist = min(min(minL, minR), min(minG, minB));
    maxHist = max(max(maxL, maxR), max(maxG, maxB));

    // const float meanL = accumulate(lightness, lightness + ELEMENTS,
    // 0.f)/ELEMENTS;
    // float midrange = minHist + .5f*(maxHist - minHist);
    // gamma = log10(midrange*255.f)/log10(meanL);
    gamma = 1.f;  // TODO Let's return gamma = 1
}

ConvertToQRgb::ConvertToQRgb(float gamma) : gamma(1.0f / gamma) {}

void ConvertToQRgb::operator()(float r, float g, float b, QRgb &rgb) const {
    uint8_t r8u;
    uint8_t g8u;
    uint8_t b8u;

    if (gamma == 1.0f) {
        r8u = colorspace::convertSample<uint8_t>(r);
        g8u = colorspace::convertSample<uint8_t>(g);
        b8u = colorspace::convertSample<uint8_t>(b);
    } else {
        r8u = colorspace::convertSample<uint8_t>(pow(r, gamma));
        g8u = colorspace::convertSample<uint8_t>(pow(g, gamma));
        b8u = colorspace::convertSample<uint8_t>(pow(b, gamma));
    }

    rgb = qRgb(r8u, g8u, b8u);
}

void LoadFile::operator()(HdrCreationItem &currentItem) {
    if (currentItem.filename().isEmpty()) {
        return;
    }

    QFileInfo qfi(currentItem.alignedFilename());

    try {
        QByteArray filePath = QFile::encodeName(qfi.filePath());

        qDebug() << QStringLiteral("LoadFile: Loading data for %1")
                        .arg(filePath.constData());

        FrameReaderPtr reader = FrameReaderFactory::open(filePath.constData());
        reader->read(*currentItem.frame(), getRawSettings());

        // read Average Luminance
        pfs::exif::ExifData exifData(currentItem.filename().toStdString());
        currentItem.setAverageLuminance(exifData.getAverageSceneLuminance());

        // read Exposure Time
        currentItem.setExposureTime(
            ExifOperations::getExposureTime(filePath.constData()));

        qDebug() << QStringLiteral("LoadFile: Average Luminance for %1 is %2")
                        .arg(currentItem.filename())
                        .arg(currentItem.getAverageLuminance());

        // build QImage
        QImage tempImage(currentItem.frame()->getWidth(),
                         currentItem.frame()->getHeight(),
                         QImage::Format_ARGB32_Premultiplied);

        QRgb *qimageData = reinterpret_cast<QRgb *>(tempImage.bits());

        Channel *red;
        Channel *green;
        Channel *blue;
        currentItem.frame()->getXYZChannels(red, green, blue);

        if (red == NULL || green == NULL || blue == NULL) {
            throw std::runtime_error("Null frame");
        }

        // If frame comes from HdrWizard it has already been normalized,
        // if it comes from fitsreader it's not and all channels are equal so I
        // calculate min and max of red channel only.
        std::pair<pfs::Array2Df::const_iterator, pfs::Array2Df::const_iterator>
            minmaxRed = boost::minmax_element(red->begin(), red->end());

        float minRed = *minmaxRed.first;
        float maxRed = *minmaxRed.second;

        // Only useful for FitsImporter. Is there another way???
        currentItem.setMin(minRed);
        currentItem.setMax(maxRed);

#ifndef NDEBUG
        std::cout << "LoadFile:datamin = " << minRed << std::endl;
        std::cout << "LoadFile:datamax = " << maxRed << std::endl;
#endif

        // if ((fabs(minRed) > std::numeric_limits<float>::epsilon()) ||
        // (fabs(maxRed - 1.f) > std::numeric_limits<float>::epsilon()))
        if (m_fromFITS) {
#ifndef NDEBUG
            std::cout << "LoadFile: Normalizing data" << std::endl;
#endif

            // Let's normalize thumbnails for FitsImporter. Again, all channels
            // are
            // equal
            Channel c(currentItem.frame()->getWidth(),
                      currentItem.frame()->getHeight(), "X");
            pfs::colorspace::Normalizer normalize(minRed, maxRed);

            std::transform(red->begin(), red->end(), c.begin(), normalize);
            utils::transform(c.begin(), c.end(), c.begin(), c.begin(),
                             qimageData, ConvertToQRgb(2.2f));
        } else  // OK, already in [0..1] range.
        {
            utils::transform(red->begin(), red->end(), green->begin(),
                             blue->begin(), qimageData, ConvertToQRgb());
        }

        currentItem.qimage().swap(tempImage);
    } catch (std::runtime_error &err) {
        qDebug() << QStringLiteral("LoadFile: Cannot load %1: %2")
                        .arg(currentItem.filename(),
                             QString::fromStdString(err.what()));
        throw;
    }
}

SaveFile::SaveFile(int mode, float minLum, float maxLum,
                   bool deflateCompression)
    : m_mode(mode),
      m_minLum(minLum),
      m_maxLum(maxLum),
      m_deflateCompression(deflateCompression) {}

void SaveFile::operator()(HdrCreationItem &currentItem) {
    QUuid uuid = QUuid::createUuid();
    QString inputFilename = currentItem.filename();

    QString tempdir = LuminanceOptions().getTempDir();

    QString outputFilename = tempdir + "/" + uuid.toString() + ".tif";
    currentItem.setConvertedFilename(outputFilename);

    qDebug() << QStringLiteral("SaveFile: Saving data for %1 to %2 on %3")
                    .arg(inputFilename, outputFilename, tempdir);

    // save pfs::Frame as tiff 16bits or 32bits
    try {
        Params p;
        p.set("tiff_mode", m_mode);  // 16bits or 32bits
        p.set("min_luminance", m_minLum);
        p.set("max_luminance", m_maxLum);
        p.set("deflateCompression", m_deflateCompression);

        FrameWriterPtr writer = FrameWriterFactory::open(
            QFile::encodeName(outputFilename).constData(), p);
        writer->write(*currentItem.frame(), p);

    } catch (std::runtime_error &err) {
        qDebug() << QStringLiteral("SaveFile: Cannot save %1: %2")
                        .arg(currentItem.filename(),
                             QString::fromStdString(err.what()));
    }
}

void RefreshPreview::operator()(HdrCreationItem &currentItem) {
    qDebug() << QStringLiteral("RefreshPreview: Refresh preview for %1")
                    .arg(currentItem.filename());

    try {
        // build QImage
        QImage tempImage(currentItem.frame()->getWidth(),
                         currentItem.frame()->getHeight(),
                         QImage::Format_ARGB32_Premultiplied);

        QRgb *qimageData = reinterpret_cast<QRgb *>(tempImage.bits());

        Channel *red;
        Channel *green;
        Channel *blue;
        currentItem.frame()->getXYZChannels(red, green, blue);

        float m = *std::min_element(red->begin(), red->end());
        float M = *std::max_element(red->begin(), red->end());
        if (m != 0.0f || M != 1.0f) {
            Channel redNorm(currentItem.frame()->getWidth(),
                            currentItem.frame()->getHeight(), "X");
            pfs::colorspace::Normalizer normalize(m, M);
            ConvertToQRgb convert(2.2f);
            std::transform(red->begin(), red->end(), redNorm.begin(),
                           normalize);
            utils::transform(redNorm.begin(), redNorm.end(), redNorm.begin(),
                             redNorm.begin(), qimageData, convert);
        } else {
            utils::transform(red->begin(), red->end(), green->begin(),
                             blue->begin(), qimageData, ConvertToQRgb());
        }

        currentItem.qimage().swap(tempImage);
    } catch (std::runtime_error &err) {
        qDebug() << QStringLiteral("RefreshPreview: Cannot load %1: %2")
                        .arg(currentItem.filename(),
                             QString::fromStdString(err.what()));
    }
}

QString getQString(libhdr::fusion::FusionOperator fo) {
    switch (fo) {
        case DEBEVEC:
            return QObject::tr("Debevec");
        case ROBERTSON:
            return QObject::tr("Robertson");
        case ROBERTSON_AUTO:
            return QObject::tr("Robertson Response Calculation");
    }

    return QString();
}

QString getQString(libhdr::fusion::WeightFunctionType wf) {
    switch (wf) {
        case WEIGHT_TRIANGULAR:
            return QObject::tr("Triangular");
        case WEIGHT_PLATEAU:
            return QObject::tr("Plateau");
        case WEIGHT_GAUSSIAN:
            return QObject::tr("Gaussian");
        case WEIGHT_FLAT:
            return QObject::tr("Flat");
    }

    return QString();
}

QString getQString(libhdr::fusion::ResponseCurveType rf) {
    switch (rf) {
        case RESPONSE_LINEAR:
            return QObject::tr("Linear");
        case RESPONSE_GAMMA:
            return QObject::tr("Gamma");
        case RESPONSE_LOG10:
            return QObject::tr("Logarithmic");
        case RESPONSE_SRGB:
            return QObject::tr("sRGB");
        case RESPONSE_CUSTOM:
            return QObject::tr("From Calibration/Input File");
            // case FROM_FILE:
            //    return tr("From File: ") +
            //    m_hdrCreationManager->fusionOperatorConfig.inputResponseCurveFilename;
    }

    return QString();
}
