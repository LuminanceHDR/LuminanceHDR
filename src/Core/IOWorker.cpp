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
#ifdef QT_DEBUG
#include <QDebug>
#endif
#include <QScopedPointer>
#include <stdexcept>

#include "Core/IOWorker.h"
#include "Fileformat/pfs_file_format.h"
#include "Libpfs/domio.h"
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

bool IOWorker::write_hdr_frame(GenericViewer* hdr_viewer, QString filename)
{
    pfs::Frame* hdr_frame = hdr_viewer->getFrame();

    bool status = write_hdr_frame(hdr_frame, filename);

    if ( status )
    {
        hdr_viewer->setFileName(filename);
        emit write_hdr_success(hdr_viewer, filename);
    }

    return status;
}

bool IOWorker::write_hdr_frame(pfs::Frame *hdr_frame, QString filename)
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
        LuminanceOptions LuminanceOptions;

        TiffWriter tiffwriter(encodedName, hdr_frame);
        connect(&tiffwriter, SIGNAL(maximumValue(int)), this, SIGNAL(setMaximum(int)));
        connect(&tiffwriter, SIGNAL(nextstep(int)), this, SIGNAL(setValue(int)));
        if (LuminanceOptions.isSaveLogLuvTiff() )
        {
            tiffwriter.writeLogLuvTiff();
        }
        else
        {
            tiffwriter.writeFloatTiff();
        }
    }
    else if (qfi.suffix().toUpper() == "PFS")
    {
        FILE *fd = fopen(encodedName, "w");
        pfs::DOMIO pfsio;
        pfsio.writeFrame(hdr_frame, fd);
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

bool IOWorker::write_ldr_frame(GenericViewer* ldr_viewer, QString filename, int quality, TonemappingOptions* tmopts)
{
    pfs::Frame* ldr_frame = ldr_viewer->getFrame();

    bool status = write_ldr_frame(ldr_frame, filename, quality, tmopts, ldr_viewer->getMinLuminanceValue(), ldr_viewer->getMaxLuminanceValue());

    if ( status )
    {
        if ( !ldr_viewer->isHDR() )
            ldr_viewer->setFileName(filename);

        emit write_ldr_success(ldr_viewer, filename);
    }

    return status;
}


bool IOWorker::write_ldr_frame(pfs::Frame* ldr_input, QString filename, int quality, TonemappingOptions* tmopts, float min_luminance, float max_luminance)
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

    if (qfi.suffix().toUpper().startsWith("TIF"))
    {
        // QScopedArrayPointer will call delete [] when this object goes out of scope
        QScopedArrayPointer<quint16> pixmap(fromLDRPFSto16bitsPixmap(ldr_input, min_luminance, max_luminance));
        int width = ldr_input->getWidth();
        int height = ldr_input->getHeight();
        try
        {
            TiffWriter tiffwriter(encodedName, pixmap.data(), width, height);
            connect(&tiffwriter, SIGNAL(maximumValue(int)), this, SIGNAL(setMaximum(int)));
            connect(&tiffwriter, SIGNAL(nextstep(int)), this, SIGNAL(setValue(int)));
            tiffwriter.write16bitTiff();

            if (tmopts != NULL)
                ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

            emit write_ldr_success(ldr_input, filename);
        }
        catch(...)
        {
            status = false;
            emit write_ldr_failed();
        }
    }
	else if (qfi.suffix().toUpper().startsWith("JP")) {
        QImage *image(fromLDRPFStoQImage(ldr_input, min_luminance, max_luminance));
		JpegWriter writer(image, filename, quality);
		if (writer.writeQImageToJpeg()) 
		{
			if (tmopts != NULL)
				ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

			emit write_ldr_success(ldr_input, filename);
			delete image;
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
            if (tmopts != NULL)
                ExifOperations::writeExifData(encodedName.constData(), operations->getExifComment().toStdString());

            emit write_ldr_success(ldr_input, filename);
        }
        else
        {
            status = false;
            emit write_ldr_failed();
        }
    }
    emit IO_finish();

    return status;
}

pfs::Frame* IOWorker::read_hdr_frame(QString filename)
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

            pfs::DOMIO pfsio;
            hdrpfsframe = pfsio.readFrame(fd);
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

