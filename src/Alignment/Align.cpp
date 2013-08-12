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

#include <QDebug>
#include <QApplication>
#include <QtConcurrentMap>
#include <QtConcurrentFilter>

#include <boost/bind.hpp>

#include "Align.h"
#include "Common/CommonFunctions.h"
#include "Exif/ExifOperations.h"


Align::Align(HdrCreationItemContainer* data, bool fromCommadLine, int savingMode, float minLum, float maxLum) :
    m_data(data),
    m_fromCommandLine(fromCommadLine),
    m_savingMode(savingMode),
    m_minLum(minLum),
    m_maxLum(maxLum)
{
}

Align::~Align()
{
    if (m_ais)
        delete m_ais;
}

void Align::align_with_ais(bool ais_crop_flag)
{
    m_ais = new QProcess(this);
    if (m_ais == NULL) exit(1);       // TODO: exit gracefully
    if (!m_fromCommandLine) {
        m_ais->setWorkingDirectory(m_luminance_options.getTempDir());
    }
    QStringList env = QProcess::systemEnvironment();
#ifdef WIN32
    QString separator(";");
#else
    QString separator(":");
#endif
    env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=\\1"+separator+QCoreApplication::applicationDirPath());
    m_ais->setEnvironment(env);
    connect(m_ais, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(ais_finished(int,QProcess::ExitStatus)));
    connect(m_ais, SIGNAL(error(QProcess::ProcessError)), this, SIGNAL(ais_failed(QProcess::ProcessError)));
    connect(m_ais, SIGNAL(readyRead()), this, SLOT(readData()));
    
    QStringList ais_parameters = m_luminance_options.getAlignImageStackOptions();

    if (ais_crop_flag) { ais_parameters << "-C"; }

    QFutureWatcher<void> futureWatcher;

    // Start the computation.
    SaveFile saveFile(m_savingMode, m_minLum, m_maxLum);
    futureWatcher.setFuture( QtConcurrent::map(m_data->begin(), m_data->end(), saveFile) );
    futureWatcher.waitForFinished();

    if (futureWatcher.isCanceled()) return;

    for ( HdrCreationItemContainer::const_iterator it = m_data->begin(), 
          itEnd = m_data->end(); it != itEnd; ++it) {
        QFileInfo qfi(it->filename());
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = m_luminance_options.getTempDir();
        QString completeFilename = tempdir + "/" + filename;
        ais_parameters << completeFilename; 
    }
    qDebug() << "ais_parameters " << ais_parameters;
#ifdef Q_WS_MAC
    qDebug() << QCoreApplication::applicationDirPath()+"/align_image_stack";
    m_ais->start(QCoreApplication::applicationDirPath()+"/align_image_stack", ais_parameters );
#else
    m_ais->start("align_image_stack", ais_parameters );
#endif
    qDebug() << "ais started";
}

void Align::ais_finished(int exitcode, QProcess::ExitStatus exitstatus)
{
    if (exitstatus != QProcess::NormalExit)
    {
        qDebug() << "ais failed";
        //emit ais_failed(QProcess::Crashed);
        return;
    }
    if (exitcode == 0)
    {
        // TODO: try-catch
        // DAVIDE _ HDR CREATION
        m_tmpdata.clear();
        int i = 0;
        for ( HdrCreationItemContainer::iterator it = m_data->begin(), 
              itEnd = m_data->end(); it != itEnd; ++it) {
            QString inputFilename = it->filename(), filename;
            if (!m_fromCommandLine)
                filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            else
                filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
            
            m_tmpdata.push_back( HdrCreationItem(filename) );
            ExifOperations::copyExifData(inputFilename.toStdString(), filename.toStdString(), false, "", false, true); 
            i++;
        }

        // parallel load of the data...
        // Start the computation.
        connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(alignedFilesLoaded()), Qt::DirectConnection);
        m_futureWatcher.setFuture( QtConcurrent::map(m_tmpdata.begin(), m_tmpdata.end(), LoadFile()) );
    }
    else
    {
        qDebug() << "align_image_stack exited with exit code " << exitcode;
        emit finishedAligning(exitcode);
    }
}

void Align::alignedFilesLoaded()
{
    disconnect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(alignedFilesLoaded()));
    for ( HdrCreationItemContainer::iterator it = m_data->begin(), 
          itEnd = m_data->end(); it != itEnd; ++it) {
        QFileInfo qfi(it->filename());
        QString base = qfi.completeBaseName(); 
        QString filename = base + ".tif";
        QString tempdir = m_luminance_options.getTempDir();
        QString completeFilename = tempdir + "/" + filename;
        QFile::remove(QFile::encodeName(completeFilename).constData());
        qDebug() << "void Align::ais_finished: remove " << filename;
    }

    m_data->swap(m_tmpdata);
    QFile::remove(m_luminance_options.getTempDir() + "/hugin_debug_optim_results.txt");
    emit finishedAligning(0);
}

void Align::ais_failed_slot(QProcess::ProcessError error)
{
    qDebug() << "align_image_stack failed";
    emit failedAligning(error);
}

void Align::readData()
{
    QByteArray data = m_ais->readAll();
    emit dataReady(data);
}

void Align::reset()
{
    if (m_ais != NULL && m_ais->state() != QProcess::NotRunning) {
        m_ais->kill();
    }
}

void Align::removeTempFiles()
{
    int i = 0;
    for ( HdrCreationItemContainer::iterator it = m_data->begin(), 
          itEnd = m_data->end(); it != itEnd; ++it) {
        QString filename;
        if (!m_fromCommandLine) {
            filename = QString(m_luminance_options.getTempDir() + "/aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
        }
        else {
            filename = QString("aligned_" + QString("%1").arg(i,4,10,QChar('0'))+".tif");
        }
        QFile::remove(filename);
        qDebug() << "void HdrCreationManager::ais_finished: remove " << filename;
        ++i;
    }
}

