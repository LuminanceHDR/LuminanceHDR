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
#include "Libpfs/domio.h"
#include "Fileformat/pfs_file_format.h"
#include "Fileformat/jpegwriter.h"
#include "Viewers/GenericViewer.h"
#include "Common/LuminanceOptions.h"
#include "Fileformat/pfsout16bitspixmap.h"
#include "Fileformat/pfsoutldrimage.h"
#include "Core/TonemappingOptions.h"
#include "Exif/ExifOperations.h"

IOWorker::IOWorker(QObject* parent):
    QObject(parent)
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
    params2.insert( "min_luminance", hdr_viewer->getMinLuminanceValue() );
    params2.insert( "max_luminance", hdr_viewer->getMaxLuminanceValue() );
    params2.insert( "mapping_method", hdr_viewer->getLuminanceMappingMethod() );

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
        writeEXRfile(hdr_frame, encodedName);
    }
    else if (qfi.suffix().toUpper() == "HDR")
    {
        writeRGBEfile(hdr_frame, encodedName);
    }
    else if (qfi.suffix().toUpper().startsWith("TIF"))
    {
        // DAVIDE_TIFF
//        LuminanceOptions LuminanceOptions;
        pfs::Params writerParams(params);

        // LogLuv is not implemented yet in the new TiffWriter...
//        if ( LuminanceOptions.isSaveLogLuvTiff() ) {
//            writerParams.insert( "tiff_mode", 3 );
//        }
//        else {
            writerParams.insert( "tiff_mode", 2 );
//        }

        TiffWriter writer(encodedName);
        writer.write(*hdr_frame, writerParams);

    }
    else if (qfi.suffix().toUpper() == "PFS")
    {
        FILE *fd = fopen(encodedName, "w");
        pfs::DOMIO::writeFrame(hdr_frame, fd);
        fclose(fd);
    }
    else
    {
        // Default as EXR
        writeEXRfile(hdr_frame, QFile::encodeName(absoluteFileName + ".exr"));
    }
    emit write_hdr_success(hdr_frame, filename);

    emit IO_finish();

    return status;
}

bool IOWorker::write_ldr_frame(GenericViewer* ldr_viewer,
                               const QString& filename, int quality,
                               const QString& inputFileName,
                               const QVector<float>& expoTimes,
                               TonemappingOptions* tmopts)
{
    pfs::Frame* ldr_frame = ldr_viewer->getFrame();

    bool status = write_ldr_frame(ldr_frame,
                                  filename, quality,
                                  inputFileName,
                                  expoTimes,
                                  tmopts,
                                  ldr_viewer->getMinLuminanceValue(),
                                  ldr_viewer->getMaxLuminanceValue(),
                                  ldr_viewer->getLuminanceMappingMethod());

    if ( status )
    {
        if ( !ldr_viewer->isHDR() )
            ldr_viewer->setFileName(filename);

        emit write_ldr_success(ldr_viewer, filename);
    }

    return status;
}


bool IOWorker::write_ldr_frame(pfs::Frame* ldr_input,
                               const QString& filename,
                               int quality,
                               const QString& inputFileName,
                               const QVector<float>& expoTimes,
                               TonemappingOptions* tmopts,
                               float min_luminance,
                               float max_luminance,
                               RGBMappingType mapping_method)
{
    bool status = true;
    emit IO_init();

    QScopedPointer<TMOptionsOperations> operations;

    if (tmopts != NULL)
        operations.reset(new TMOptionsOperations(tmopts));
    
    QFileInfo qfi(filename);
    QString format = qfi.suffix();
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = QFile::encodeName(absoluteFileName);

    pfs::Params writerParams;
    writerParams.insert( "quality", (size_t)quality );
    writerParams.insert( "min_luminance", min_luminance );
    writerParams.insert( "max_luminance", max_luminance );
    writerParams.insert( "mapping_method", mapping_method );

    if (qfi.suffix().toUpper().startsWith("TIF"))
    {
        TiffWriter writer( encodedName.constData() );
        if ( writer.write(*ldr_input, writerParams) )
        {
//            if (tmopts != NULL)
//                ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

            emit write_ldr_success(ldr_input, filename);
        }
        else
        {
            status = false;
            emit write_ldr_failed();
        }
    }
    else if (qfi.suffix().toUpper().startsWith("JP"))
    {
        JpegWriter writer(filename.toStdString());
        if (writer.write(*ldr_input, writerParams))
		{
//			if (tmopts != NULL)
//				ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

			emit write_ldr_success(ldr_input, filename);
		}
		else
		{
            status = false;
            emit write_ldr_failed();
		}
	}
    else if (qfi.suffix().toUpper().startsWith("PNG"))
    {
//        QScopedPointer<QImage> image(fromLDRPFStoQImage(ldr_input,
//                                                        min_luminance,
//                                                        max_luminance,
//                                                        mapping_method));
        PngWriter writer(filename.toStdString());
        if (writer.write(*ldr_input, writerParams))
		{
//			if (tmopts != NULL)
//				ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

			emit write_ldr_success(ldr_input, filename);
		}
		else
		{
            status = false;
            emit write_ldr_failed();
		}
	}
    else
    {
        // QScopedPointer will call delete when this object goes out of scope
        QScopedPointer<QImage> image(fromLDRPFStoQImage(ldr_input, min_luminance, max_luminance));
        if ( image->save(filename, format.toLocal8Bit(), quality) )
        {
//            if (tmopts != NULL)
//                ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

            emit write_ldr_success(ldr_input, filename);
        }
        else
        {
            status = false;
            emit write_ldr_failed();
        }
    }
    // copy EXIF tags from the 1st bracketed image
    if (inputFileName != "")
    {
        QFileInfo fileinfo(inputFileName);
        QString absoluteInputFileName = fileinfo.absoluteFilePath();
        QByteArray encodedInputFileName = QFile::encodeName(absoluteInputFileName);
        QString comment = operations->getExifComment();
        comment += "\nBracketed images exposure times:\n\n";
        foreach (float e, expoTimes) {
            comment += QString("%1").arg(e) + "\n";
        }
        
        ExifOperations::copyExifData(encodedInputFileName.constData(), 
                                     encodedName.constData(), 
                                     false, 
                                     comment.toStdString(),
                                     true, false);
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
    rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF" << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF" << "X3F" << "RAW" << "SR2" << "3FR" << "RW2" << "MEF" << "MOS" << "ERF" << "NRW" << "SRW";

    try
    {
        LuminanceOptions luminanceOptions;

        QString extension = qfi.suffix().toUpper();
        QByteArray TempPath = QFile::encodeName(luminanceOptions.getTempDir());
        QByteArray encodedFileName = QFile::encodeName(qfi.absoluteFilePath());

        if (extension=="EXR")
        {
            hdrpfsframe = readEXRfile(encodedFileName);
        }
        else if (extension=="HDR")
        {
            hdrpfsframe = readRGBEfile(encodedFileName);
        }
        else if (extension=="PFS")
        {
            //TODO : check this code and make it smoother
            FILE *fd = fopen(encodedFileName, "rb");
            if (!fd) throw;

            hdrpfsframe = pfs::DOMIO::readFrame(fd);
            fclose(fd);
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
		catch (QString err)
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

