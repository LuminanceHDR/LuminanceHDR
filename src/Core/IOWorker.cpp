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

#include "Core/IOWorker.h"
#include "Fileformat/pfs_file_format.h"
#include "Libpfs/domio.h"

IOWorker::IOWorker(QObject* parent): QObject(parent)
{
    luminance_options = LuminanceOptions::getInstance();
}

IOWorker::~IOWorker()
{
    qDebug() << "IOWorker::~IOWorker()";
}

bool IOWorker::write_hdr_frame(HdrViewer* hdr_input, QString filename)
{
    bool status = true;
    emit IO_init();

    QFileInfo qfi(filename);
    QString absoluteFileName = qfi.absoluteFilePath();
    QByteArray encodedName = QFile::encodeName(absoluteFileName);

    pfs::Frame* hdr_frame = hdr_input->getHDRPfsFrame();

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
        TiffWriter tiffwriter(encodedName, hdr_frame);
        connect(&tiffwriter, SIGNAL(maximumValue(int)), this, SIGNAL(setMaximum(int)));
        connect(&tiffwriter, SIGNAL(nextstep(int)), this, SIGNAL(setValue(int)));
        if (luminance_options->saveLogLuvTiff)
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
    emit write_hdr_success(hdr_input, filename);

    emit IO_finish();

    return status;
}

void IOWorker::write_ldr_frame(LdrViewer* ldr_input, QString filename, int quality)
{
    emit IO_init();
    const QImage* image = ldr_input->getQImage();

    QFileInfo qfi(filename);
    QString format = qfi.suffix();

    if ( image->save(filename, format.toLocal8Bit(), quality) )
    {
        emit write_ldr_success(ldr_input, filename);
    }
    else
    {
        emit write_ldr_failed();
    }

    emit IO_finish();
}

void IOWorker::read_frame(QString filename)
{
    emit IO_init();

    get_frame(filename);

    emit IO_finish();
}

void IOWorker::read_frames(QStringList filenames)
{
    emit IO_init();
    foreach (QString filename, filenames)
    {
        get_frame(filename);
    }
    emit IO_finish();
}

void IOWorker::get_frame(QString fname)
{
    if ( fname.isEmpty() )
        return;

    QFileInfo qfi(fname);
    if ( !qfi.isReadable() )
    {
        qDebug("File %s is not readable.", qPrintable(fname));
        emit read_failed(tr("ERROR: The following file is not readable: %1").arg(fname));
        return;
    }

    pfs::Frame* hdrpfsframe = NULL;
    QStringList rawextensions;
    rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF" << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF" << "X3F" << "RAW" << "SR2" << "3FR" << "RW2" << "MEF" << "MOS" << "ERF" << "NRW";

    try
    {
        QString extension = qfi.suffix().toUpper();
        QByteArray TempPath = QFile::encodeName(luminance_options->tempfilespath);
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
            //const char *fname = encodedFileName;
            FILE *fd = fopen(encodedFileName, "rb");
            if (!fd) {
                emit read_failed(tr("ERROR: Cannot open file: %1").arg(fname));
                return;
            }
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
            hdrpfsframe = readRawIntoPfsFrame(encodedFileName, TempPath, luminance_options, false, progress_cb, this);
        }
        else
        {
            qDebug("TH: File %s has unsupported extension.", qPrintable(fname));
            emit read_failed(tr("ERROR: File %1 has unsupported extension.").arg(fname));
            return;
        }

        if (hdrpfsframe == NULL)
        {
            throw "Error loading file";
        }
    }
    catch(pfs::Exception e)
    {
        emit read_failed(tr("ERROR: %1").arg(e.getMessage()));
        return;
    }
    catch (...)
    {
        qDebug("TH: catched exception");
        emit read_failed(tr("ERROR: Failed loading file: %1").arg(fname));
        return;
    }
    emit read_success(hdrpfsframe, fname);
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

