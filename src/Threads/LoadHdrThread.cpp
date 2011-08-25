/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 */

#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QCoreApplication>
#include <iostream>

#include "LoadHdrThread.h"
#include "Fileformat/pfs_file_format.h"
#include "Libpfs/domio.h"

LoadHdrThread::LoadHdrThread(QString fname, QString RecentDirHDRSetting) : QThread(0), fname(fname), RecentDirHDRSetting(RecentDirHDRSetting)
{
    luminance_options = LuminanceOptions::getInstance();
}

LoadHdrThread::LoadHdrThread(QString fname) : QThread(0), fname(fname), RecentDirHDRSetting(QString())
{
    luminance_options = LuminanceOptions::getInstance();
}

LoadHdrThread::~LoadHdrThread()
{
    this->wait();   // waits that all the signals have been served

    std::cout << "LoadHdrThread::~LoadHdrThread()" << std::endl;
}

void LoadHdrThread::run()
{
    if( fname.isEmpty() )
        return;

    QFileInfo qfi(fname);
    if (!qfi.isReadable())
    {
        qDebug("File %s is not readable.", qPrintable(fname));
        emit load_failed(tr("ERROR: The following file is not readable: %1").arg(fname));
        return;
    }
    // if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings->
    if ( (!RecentDirHDRSetting.isNull()) && (RecentDirHDRSetting != qfi.path()) )
    {
        emit updateRecentDirHDRSetting(qfi.path());
    }
    pfs::Frame* hdrpfsframe = NULL;
    QStringList rawextensions;
    rawextensions << "CRW" << "CR2" << "NEF" << "DNG" << "MRW" << "ORF" << "KDC" << "DCR" << "ARW" << "RAF" << "PTX" << "PEF" << "X3F" << "RAW" << "SR2" << "3FR" << "RW2" << "MEF" << "MOS" << "ERF" << "NRW";
    QString extension = qfi.suffix().toUpper();
    bool rawinput = (rawextensions.indexOf(extension) != -1);
    try
    {
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
            //TODO
            //const char *fname = encodedFileName;
            FILE *fd = fopen(encodedFileName, "rb");
            if (!fd) {
                emit load_failed(tr("ERROR: Cannot open file: %1").arg(fname));
                return;
            }
            pfs::DOMIO pfsio;
            hdrpfsframe = pfsio.readFrame(fd);
            fclose(fd);
        }
        else if (extension.startsWith("TIF"))
        {
            TiffReader reader(encodedFileName, TempPath, false );
            connect(&reader, SIGNAL(maximumValue(int)), this, SIGNAL(maximumValue(int)));
            connect(&reader, SIGNAL(nextstep(int)), this, SIGNAL(nextstep(int)));
            hdrpfsframe = reader.readIntoPfsFrame();
            //from 8,16,32,logluv to pfs::Frame
        }
        else if (rawinput)
        {
            hdrpfsframe = readRawIntoPfsFrame(encodedFileName, TempPath, luminance_options, false, prog_cb, this);
        } //raw file detected
        else
        {
            qDebug("TH: File %s has unsupported extension.", qPrintable(fname));
            emit load_failed(tr("ERROR: File %1 has unsupported extension.").arg(fname));
            return;
        }
        if (hdrpfsframe == NULL)
        {
            throw "Error loading file";
        }
    }
    catch(pfs::Exception e)
    {
        emit load_failed(tr("ERROR: %1").arg(e.getMessage()));
        return;
    }
    catch (...)
    {
        qDebug("TH: catched exception");
        emit load_failed(tr("ERROR: Failed loading file: %1").arg(fname));
        return;
    }
    emit hdr_ready(hdrpfsframe, fname);
}

void LoadHdrThread::emitNextStep(int iteration)
{
	emit nextstep(iteration);
}

void LoadHdrThread::emitMaximumValue(int expected)
{
	emit maximumValue(expected);
}

int prog_cb(void *data,enum LibRaw_progress p,int iteration, int expected)
{
	LoadHdrThread *ptr = (LoadHdrThread *) data;
	ptr->emitMaximumValue(expected);
	ptr->emitNextStep(iteration);
	return 0;
}

