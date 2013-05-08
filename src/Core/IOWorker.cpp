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

#include <QFileInfo>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include <QScopedPointer>
#include <stdexcept>

#include "Core/IOWorker.h"

#include "Libpfs/frame.h"
#include "Fileformat/pfs_file_format.h"
#include "Fileformat/jpegwriter.h"
#include "Viewers/GenericViewer.h"
#include "Common/LuminanceOptions.h"
#include "Core/TonemappingOptions.h"
#include "Exif/ExifOperations.h"

#include <Libpfs/io/pfswriter.h>
#include <Libpfs/io/pfsreader.h>
#include <Libpfs/io/rgbewriter.h>
#include <Libpfs/io/rgbereader.h>
#include <Libpfs/io/exrwriter.h>
#include <Libpfs/io/exrreader.h>

using namespace pfs::io;

IOWorker::IOWorker(QObject* parent)
    : QObject(parent)
{}

IOWorker::~IOWorker()
{
#ifdef QT_DEBUG
    qDebug() << "IOWorker::~IOWorker()";
#endif
}

bool IOWorker::write_hdr_frame(GenericViewer* hdr_viewer,
                               const QString& filename,
                               const pfs::Params& params)
{
    pfs::Params params2( params );
    params2.set( "min_luminance", hdr_viewer->getMinLuminanceValue() )
            ( "max_luminance", hdr_viewer->getMaxLuminanceValue() )
            ( "mapping_method", hdr_viewer->getLuminanceMappingMethod() );

    pfs::Frame* hdr_frame = hdr_viewer->getFrame();

    bool status = write_hdr_frame(hdr_frame, filename,
                                  params2);

    if ( status )
    {
        hdr_viewer->setFileName(filename);
        emit write_hdr_success(hdr_viewer, filename);
    }

    return status;
}

bool IOWorker::write_hdr_frame(pfs::Frame *hdr_frame, const QString& filename,
                               const pfs::Params& params)
{
    bool status = true;
    emit IO_init();

    QFileInfo qfi(filename);
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = QFile::encodeName(absoluteFileName);

    if (qfi.suffix().toUpper() == "EXR")
    {
        EXRWriter writer(encodedName.constData());
        writer.write(*hdr_frame, params);
    }
    else if (qfi.suffix().toUpper() == "HDR")
    {
        RGBEWriter writer(encodedName.constData());
        writer.write(*hdr_frame, params);
    }
    else if (qfi.suffix().toUpper().startsWith("TIF"))
    {
        pfs::Params writerParams(params);
        if ( !writerParams.count("tiff_mode") ) {
            LuminanceOptions lumOpts;
            // LogLuv is not implemented yet in the new TiffWriter...
            if ( lumOpts.isSaveLogLuvTiff() ) {
                writerParams.set( "tiff_mode", 3 );
            } else {
                writerParams.set( "tiff_mode", 2 );
            }
        }

        TiffWriter writer(encodedName.constData());
        status = writer.write(*hdr_frame, writerParams);
    }
    else if (qfi.suffix().toUpper() == "PFS")
    {
        pfs::io::PfsWriter writer(encodedName.constData());
        status = writer.write(*hdr_frame, pfs::Params());
    }
    else
    {
        EXRWriter writer(encodedName.constData());
        writer.write(*hdr_frame, params);
    }

    if ( status ) {
        emit write_hdr_success(hdr_frame, filename);
    } else {
        emit write_hdr_failed();
    }
    emit IO_finish();
    return status;
}

bool IOWorker::write_ldr_frame(GenericViewer* ldr_viewer,
                               const QString& filename, /*int quality,*/
                               const QString& inputFileName,
                               const QVector<float>& expoTimes,
                               TonemappingOptions* tmopts,
                               const pfs::Params& params)
{
    pfs::Frame* ldr_frame = ldr_viewer->getFrame();

    pfs::Params p2( params );
    p2.set( "min_luminance", ldr_viewer->getMinLuminanceValue() )
            ( "max_luminance", ldr_viewer->getMaxLuminanceValue() )
            ( "mapping_method", ldr_viewer->getLuminanceMappingMethod() );

    bool status = write_ldr_frame(ldr_frame,
                                  filename, /*quality,*/
                                  inputFileName,
                                  expoTimes,
                                  tmopts, p2);

    if ( status ) {
        if ( !ldr_viewer->isHDR() ) {
            ldr_viewer->setFileName(filename);
        }

        emit write_ldr_success(ldr_viewer, filename);
    }
    return status;
}


bool IOWorker::write_ldr_frame(pfs::Frame* ldr_input,
                               const QString& filename,
                               const QString& inputFileName,
                               const QVector<float>& expoTimes,
                               TonemappingOptions* tmopts,
                               const pfs::Params& params)
{
    /*
     // in the future I would like it to be like this:

     try {
        FrameWriterPtr fr = FrameWriter::create( filename );

        fr->write( frame, params);

        // ...do some more stuff!

        retur true;
     } catch ( Exception& e) {

        return false;
     }

     */

    bool status = true;
    emit IO_init();

    QScopedPointer<TMOptionsOperations> operations;
    if (tmopts != NULL) {
        operations.reset(new TMOptionsOperations(tmopts));
    }
    
    QFileInfo qfi(filename);
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = QFile::encodeName(absoluteFileName);

    if (qfi.suffix().toUpper().startsWith("TIF"))
    {
        TiffWriter writer( encodedName.constData() );
        status = writer.write(*ldr_input, params);
    }
    else if (qfi.suffix().toUpper().startsWith("JP"))
    {
        JpegWriter writer( encodedName.constData() );
        status = writer.write(*ldr_input, params);
	}
    else if (qfi.suffix().toUpper().startsWith("PNG"))
    {
        PngWriter writer(filename.toStdString());
        status = writer.write(*ldr_input, params);
	}
    else
    {
        QString format = qfi.suffix();
        // QScopedPointer will call delete when this object goes out of scope
        QScopedPointer<QImage> image(fromLDRPFStoQImage(ldr_input, 0.f, 1.f));
        status = image->save(filename, format.toLocal8Bit(), -1);
    }

    if ( status )
    {
        // copy EXIF tags from the 1st bracketed image
        if ( !inputFileName.isEmpty() )
        {
            QFileInfo fileinfo(inputFileName);
            QString absoluteInputFileName = fileinfo.absoluteFilePath();
            QByteArray encodedInputFileName = QFile::encodeName(absoluteInputFileName);
            QString comment = operations->getExifComment();
            if ( !expoTimes.empty() ) {
                comment += "\nBracketed images exposure times:\n";
                foreach (float e, expoTimes) {
                    comment += QString("%1").arg(e) + "\n";
                }
            }

            ExifOperations::copyExifData(encodedInputFileName.constData(),
                                         encodedName.constData(),
                                         false,
                                         comment.toStdString(),
                                         true, false);
        }

        emit write_ldr_success(ldr_input, filename);
    }
    else
    {
        emit write_ldr_failed();
    }

    emit IO_finish();
    return status;
}

pfs::Frame* IOWorker::read_hdr_frame(const QString& filename)
{
    emit IO_init();

    if ( filename.isEmpty() )
        return NULL;

    QFileInfo qfi(filename);
    if ( !qfi.isReadable() )
    {
#ifdef QT_DEBUG
        qDebug("File %s is not readable.", qPrintable(filename));
#endif
        emit read_hdr_failed(tr("ERROR: The following file is not readable: %1").arg(filename));
        return NULL;
    }

    pfs::Frame* hdrpfsframe = NULL;
    QStringList rawextensions;
    rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF"
                  << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF"
                  << "X3F" << "RAW" << "SR2" << "3FR" << "RW2" << "MEF"
                  << "MOS" << "ERF" << "NRW" << "SRW";

    try
    {
        LuminanceOptions luminanceOptions;

        QString extension = qfi.suffix().toUpper();
        QByteArray TempPath = QFile::encodeName(luminanceOptions.getTempDir());
        QByteArray encodedFileName = QFile::encodeName(qfi.absoluteFilePath());

        if ( extension=="EXR" )
        {
            hdrpfsframe = new pfs::Frame(0, 0); // < To improve!
            pfs::io::EXRReader reader(encodedFileName.constData());
            reader.read( *hdrpfsframe, pfs::Params() );
            reader.close();
        }
        else if ( extension=="HDR" )
        {
            hdrpfsframe = new pfs::Frame(0, 0); // < To improve!
            pfs::io::RGBEReader reader(encodedFileName.constData());
            reader.read( *hdrpfsframe, pfs::Params() );
            reader.close();
        }
        else if (extension=="PFS")
        {
            hdrpfsframe = new pfs::Frame(0, 0); // < To improve!
            pfs::io::PfsReader reader(encodedFileName.constData());
            reader.read( *hdrpfsframe, pfs::Params() );
            reader.close();
        }
        else if (extension.startsWith("TIF"))
        {
            // from 8,16,32,logluv to pfs::Frame
            TiffReader reader(encodedFileName, TempPath, false );
            connect(&reader, SIGNAL(maximumValue(int)), this, SIGNAL(setMaximum(int)));
            connect(&reader, SIGNAL(nextstep(int)), this, SIGNAL(setValue(int)));
            hdrpfsframe = reader.readIntoPfsFrame();
        }
        else if ( rawextensions.indexOf(extension) != -1 )
        {
            // raw file detected
            try {
                hdrpfsframe = readRawIntoPfsFrame(encodedFileName, TempPath, &luminanceOptions, false, progress_cb, this);
            }
            catch (QString& err)
            {
                qDebug("TH: catched exception");
                emit read_hdr_failed((err + " : %1").arg(filename));
                return NULL;
            }
        }
        else
        {
#ifdef QT_DEBUG
            qDebug("TH: File %s has unsupported extension.", qPrintable(filename));
#endif
            emit read_hdr_failed(tr("ERROR: File %1 has unsupported extension.").arg(filename));
            return NULL;
        }

        if (hdrpfsframe == NULL)
        {
            return NULL;
        }
    }
	catch (const std::runtime_error& err)
	{
		qDebug() << err.what();
		emit read_hdr_failed(err.what()); 
		return NULL;
	}
    catch (...)
    {
        qDebug("TH: catched exception");
        emit read_hdr_failed(tr("ERROR: Failed loading file: %1").arg(filename));
        return NULL;
    }
    emit read_hdr_success(hdrpfsframe, filename);

    emit IO_finish();

    return hdrpfsframe;
}

void IOWorker::emitNextStep(int iteration)
{
    emit setValue(iteration);
}

void IOWorker::emitMaximumValue(int expected)
{
    emit setMaximum(expected);
}

int progress_cb(void *data,enum LibRaw_progress p,int iteration, int expected)
{
    IOWorker *ptr = (IOWorker *) data;
    ptr->emitMaximumValue(expected);
    ptr->emitNextStep(iteration);
    return 0;
}

