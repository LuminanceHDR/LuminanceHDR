/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * This class provides I/O of HDR images
 * This class is inspired by LoadHdrThread and borrow most of its code
 * but it is not derived from QThread
 *
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <sstream>
#include <stdexcept>

#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QScopedPointer>
#include <QString>

#include <Core/IOWorker.h>
#include <Libpfs/frame.h>
#include <Common/LuminanceOptions.h>
#include <Core/TonemappingOptions.h>
#include <Exif/ExifOperations.h>
#include <Fileformat/pfsoutldrimage.h>
#include <Viewers/GenericViewer.h>
#include <Libpfs/io/exrwriter.h>  // default for HDR saving
#include <Libpfs/io/framereaderfactory.h>
#include <Libpfs/io/framewriterfactory.h>

using namespace pfs;
using namespace pfs::io;
using namespace std;

IOWorker::IOWorker(QObject *parent) : QObject(parent) {}

IOWorker::~IOWorker() {
#ifdef QT_DEBUG
    qDebug() << "IOWorker::~IOWorker()";
#endif
}

bool IOWorker::write_hdr_frame(GenericViewer *hdr_viewer,
                               const QString &filename,
                               const pfs::Params &params) {
    pfs::Params params2(params);
    params2.set("min_luminance", hdr_viewer->getMinLuminanceValue())(
        "max_luminance", hdr_viewer->getMaxLuminanceValue())(
        "mapping_method", hdr_viewer->getLuminanceMappingMethod());

    pfs::Frame *hdr_frame = hdr_viewer->getFrame();

    bool status = write_hdr_frame(hdr_frame, filename, params2);

    if (status) {
        hdr_viewer->setFileName(filename);
        emit write_hdr_success(hdr_viewer, filename);
    }

    return status;
}

bool IOWorker::write_hdr_frame(pfs::Frame *hdr_frame, const QString &filename,
                               const pfs::Params &params) {
    bool status = true;
    emit IO_init();

    QFileInfo qfi(filename);
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = QFile::encodeName(absoluteFileName);

    // add parameters for TiffWriter HDR
    pfs::Params writerParams(params);
    if (!writerParams.count("tiff_mode")) {
        writerParams.set("tiff_mode", 2);
    }

    try {
        FrameWriterPtr writer =
            FrameWriterFactory::open(encodedName.constData(), writerParams);
        writer->write(*hdr_frame, writerParams);
    } catch (pfs::io::InvalidFile &exInvalid) {
        qDebug() << "Unsupported format for " << exInvalid.what();

        EXRWriter writer(encodedName.constData());
        writer.write(*hdr_frame, writerParams);
    } catch (std::runtime_error &ex) {
        qDebug() << ex.what();
        status = false;
    }

    if (status) {
        emit write_hdr_success(hdr_frame, filename);
    } else {
        emit write_hdr_failed(filename);
    }
    emit IO_finish();
    return status;
}

bool IOWorker::write_ldr_frame(GenericViewer *ldr_viewer,
                               const QString &filename, /*int quality,*/
                               const QString &inputFileName,
                               const QVector<float> &expoTimes,
                               TonemappingOptions *tmopts,
                               const pfs::Params &params) {
    pfs::Frame *ldr_frame = ldr_viewer->getFrame();

    pfs::Params p2(params);
    p2.set("min_luminance", ldr_viewer->getMinLuminanceValue())(
        "max_luminance", ldr_viewer->getMaxLuminanceValue())(
        "mapping_method", ldr_viewer->getLuminanceMappingMethod());

    bool status = write_ldr_frame(ldr_frame, filename, /*quality,*/
                                  inputFileName, expoTimes, tmopts, p2);

    if (status) {
        if (!ldr_viewer->isHDR()) {
            ldr_viewer->setFileName(filename);
        }

        emit write_ldr_success(ldr_viewer, filename);
    }
    return status;
}

bool IOWorker::write_ldr_frame(pfs::Frame *ldr_input, const QString &filename,
                               const QString &inputFileName,
                               const QVector<float> &expoTimes,
                               TonemappingOptions *tmopts,
                               const pfs::Params &params) {
    bool status = true;
    emit IO_init();

    QScopedPointer<TMOptionsOperations> operations;
    if (tmopts != NULL) {
        operations.reset(new TMOptionsOperations(tmopts));
    }

    QFileInfo qfi(filename);
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = QFile::encodeName(absoluteFileName);

    try {
        FrameWriterPtr writer =
            FrameWriterFactory::open(encodedName.constData(), params);
        writer->write(*ldr_input, params);
    } catch (pfs::io::UnsupportedFormat &exUnsupported) {
        qDebug() << "Exception: " << exUnsupported.what();

        QString format = qfi.suffix();
        // QScopedPointer will call delete when this object goes out of scope
        QScopedPointer<QImage> image(fromLDRPFStoQImage(ldr_input, 0.f, 1.f));
        status = image->save(filename, format.toLocal8Bit(), -1);
    } catch (pfs::io::InvalidFile & /*exInvalid*/) {
        status = false;
    } catch (pfs::io::WriteException & /*exWrite*/) {
        status = false;
    }

    if (status) {
        // this come from an existing HDR file, we do not have exif data
        // let's write our own tags only
        if (inputFileName == "FromHdrFile") {
            QString comment;

            if (operations != NULL) {
                comment = operations->getExifComment();
            } else {
                comment = QObject::tr("HDR Preview");
            }

            ExifOperations::copyExifData("", encodedName.constData(), false,
                                         comment.toStdString(), true, false);
        }
        // copy EXIF tags from the 1st bracketed image
        if (!inputFileName.isEmpty() && inputFileName != "FromHdrFile") {
            QFileInfo fileinfo(inputFileName);
            QString absoluteInputFileName = fileinfo.absoluteFilePath();
            QByteArray encodedInputFileName =
                QFile::encodeName(absoluteInputFileName);
            QString comment;

            if (operations != NULL) {
                comment = operations->getExifComment();
            } else {
                comment = QObject::tr("HDR Preview");
            }

            if (!expoTimes.empty()) {
                comment +=
                    QLatin1String("\nBracketed images exposure times:\n");
                foreach (float e, expoTimes) {
                    comment += QStringLiteral("%1").arg(e) + "\n";
                }
            }

            ExifOperations::copyExifData(encodedInputFileName.constData(),
                                         encodedName.constData(), false,
                                         comment.toStdString(), true, false);
        }

        emit write_ldr_success(ldr_input, filename);
    } else {
        emit write_ldr_failed(filename);
    }

    emit IO_finish();
    return status;
}

pfs::Frame *IOWorker::read_hdr_frame(const QString &filename) {
    emit IO_init();

    if (filename.isEmpty()) {
        return NULL;
    }

    QFileInfo qfi(filename);
    if (!qfi.isReadable()) {
        emit read_hdr_failed(
            tr("IOWorker: The following file is not readable: %1")
                .arg(filename));
        return NULL;
    }

    QScopedPointer<pfs::Frame> hdrpfsframe(new pfs::Frame());
    try {
        QByteArray encodedFileName = QFile::encodeName(qfi.absoluteFilePath());

        pfs::Params params = getRawSettings();
        FrameReaderPtr reader =
            FrameReaderFactory::open(encodedFileName.constData());
        reader->read(*hdrpfsframe, params);
        reader->close();
    } catch (pfs::io::UnsupportedFormat &exUnsupported) {
        emit read_hdr_failed(
            tr("IOWorker: file %1 has unsupported extension: %2")
                .arg(filename, exUnsupported.what()));

        hdrpfsframe.reset();
    } catch (std::runtime_error &err) {
        emit read_hdr_failed(tr("IOWorker: caught exception reading %1: %2")
                                 .arg(filename, err.what()));

        hdrpfsframe.reset();
    } catch (std::bad_alloc &err) {
        emit read_hdr_failed(tr("IOWorker: failed to allocate memory %1: %2")
                                 .arg(filename, err.what()));

        hdrpfsframe.reset();
    } catch (...) {
        emit read_hdr_failed(
            tr("IOWorker: failed loading file: %1").arg(filename));

        hdrpfsframe.reset();
    }

    emit IO_finish();

    if (hdrpfsframe) {
        pfs::Frame *frame = hdrpfsframe.take();

        emit read_hdr_success(frame, filename);
        return frame;
    } else {
        return NULL;
    }
}

void IOWorker::emitNextStep(int iteration) { emit setValue(iteration); }

void IOWorker::emitMaximumValue(int expected) { emit setMaximum(expected); }

int progress_cb(void *data, enum LibRaw_progress p, int iteration,
                int expected) {
    IOWorker *ptr = (IOWorker *)data;
    ptr->emitMaximumValue(expected);
    ptr->emitNextStep(iteration);
    return 0;
}

// moves settings from LuminanceOptions into Params, for the underlying
// processing engine
pfs::Params getRawSettings(const LuminanceOptions &opts) {
    pfs::Params p;
    //    // general parameters
    //    if ( opts.isRawFourColorRGB() )
    //    { p.set("raw.four_color", 1); }
    //    if ( opts.isRawDoNotUseFujiRotate() )
    //    { p.set("raw.fuji_rotate", 0); }

    //    p.set("raw.user_quality", opts.getRawUserQuality());
    //    p.set("raw.med_passes", opts.getRawMedPasses());

    // white balance
    p.set("raw.wb_method", opts.getRawWhiteBalanceMethod());
    p.set("raw.wb_temperature", opts.getRawTemperatureKelvin());
    p.set("raw.wb_green", opts.getRawGreen());

    // highlights
    p.set("raw.highlights", opts.getRawHighlightsMode());
    p.set("raw.highlights_rebuild", opts.getRawLevel());

    // colors
    if (opts.isRawUseBlack()) {
        p.set("raw.black_level", opts.getRawUserBlack());
    }
    if (opts.isRawUseSaturation()) {
        p.set("raw.saturation", opts.getRawUserSaturation());
    }

    // brightness
    if (opts.isRawAutoBrightness()) {
        p.set("raw.auto_brightness", true);
    }

    p.set("raw.brightness", opts.getRawBrightness());
    p.set("raw.auto_brightness_threshold",
          opts.getRawAutoBrightnessThreshold());

    // noise reduction
    if (opts.isRawUseNoiseReduction()) {
        p.set("raw.noise_reduction_threshold",
              opts.getRawNoiseReductionThreshold());
    }

    if (opts.isRawUseChromaAber()) {
        p.set("raw.chroma_aber", true);
        p.set("raw.chroma_aber_0", opts.getRawAber0());
        p.set("raw.chroma_aber_1", opts.getRawAber1());
        p.set("raw.chroma_aber_2", opts.getRawAber2());
        p.set("raw.chroma_aber_3", opts.getRawAber3());
    }

    if (!opts.getRawCameraProfile().isEmpty()) {
        p.set("raw.camera_profile",
              std::string(
                  QFile::encodeName(opts.getRawCameraProfile()).constData()));
    }

    return p;
}

pfs::Params getRawSettings() { return getRawSettings(LuminanceOptions()); }
