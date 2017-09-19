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

#include <QApplication>
#include <QDebug>
#include <QUuid>
#include <QtConcurrentFilter>
#include <QtConcurrentMap>

#include <boost/bind.hpp>

#include "Align.h"
#include "Common/CommonFunctions.h"
#include "Exif/ExifOperations.h"

Align::Align(HdrCreationItemContainer &data, bool fromCommadLine,
             int savingMode, float minLum, float maxLum)
    : m_data(data),
      m_fromCommandLine(fromCommadLine),
      m_savingMode(savingMode),
      m_minLum(minLum),
      m_maxLum(maxLum),
      m_ais(0) {}

Align::~Align() {}

void Align::align_with_ais(bool ais_crop_flag) {
    m_ais.reset(new QProcess(this));
    if (m_ais == NULL) exit(1);  // TODO: exit gracefully
    if (!m_fromCommandLine) {
        m_ais->setWorkingDirectory(m_luminance_options.getTempDir());
    }
#ifndef WIN32
    QStringList env = QProcess::systemEnvironment();
    QString separator(QStringLiteral(":"));
    env.replaceInStrings(
        QRegExp("^PATH=(.*)", Qt::CaseInsensitive),
        "PATH=\\1" + separator + QCoreApplication::applicationDirPath());
    m_ais->setEnvironment(env);
#endif
    connect(m_ais.data(),
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                &QProcess::finished),
            this, &Align::ais_finished);
    connect(m_ais.data(), &QProcess::errorOccurred, this,
            &Align::ais_failed_slot);
    connect(m_ais.data(), &QIODevice::readyRead, this, &Align::readData);

    QStringList ais_parameters =
        m_luminance_options.getAlignImageStackOptions();

    if (ais_crop_flag) {
        ais_parameters << QStringLiteral("-C");
    }

    QFutureWatcher<void> futureWatcher;

// Start the computation.
#ifdef WIN32
    const bool deflateCompression =
        false;  // AIS is misconfigured (see hugin bug #1265480)
#else
    const bool deflateCompression = true;
#endif
    SaveFile saveFile(m_savingMode, m_minLum, m_maxLum, deflateCompression);
    futureWatcher.setFuture(
        QtConcurrent::map(m_data.begin(), m_data.end(), saveFile));
    futureWatcher.waitForFinished();

    if (futureWatcher.isCanceled()) return;

    QString uuidStr = QUuid::createUuid().toString();
    QString tempDir(m_luminance_options.getTempDir());
    m_ais->setWorkingDirectory(tempDir);
    // ais_parameters << "-a" << tempDir + "/" + uuidStr;
    ais_parameters << QStringLiteral("-a") << uuidStr;
    int i = 0;
    for (auto &it : m_data) {
        if (it.convertedFilename().isEmpty()) continue;

        QFileInfo qfi(it.convertedFilename());
        QString filename = qfi.completeBaseName() + ".tif";
        // QString tempDir(m_luminance_options.getTempDir());
        // QString completeFilename = tempDir + "/" + filename;
        // QString completeFilename = filename;
        // ais_parameters << completeFilename;
        ais_parameters << filename;

        QString alignedFilename =
            tempDir + "/" + uuidStr +
            QStringLiteral("%1").arg(i++, 4, 10, QChar('0')) + ".tif";
        it.setAlignedFilename(alignedFilename);
    }
    qDebug() << "ais_parameters " << ais_parameters;
#ifdef Q_OS_MAC
    qDebug() << QCoreApplication::applicationDirPath() + "/align_image_stack";
    m_ais->start(QCoreApplication::applicationDirPath() + "/align_image_stack",
                 ais_parameters);
#elif defined Q_OS_WIN
    QFileInfo huginPath("hugin/align_image_stack.exe");
    m_ais->start(huginPath.canonicalFilePath(), ais_parameters);
#else
    m_ais->start(QStringLiteral("align_image_stack"), ais_parameters);
#endif
    qDebug() << "ais started";
}

void Align::ais_finished(int exitcode, QProcess::ExitStatus exitstatus) {
    if (exitstatus != QProcess::NormalExit) {
        qDebug() << "ais failed";
        // emit ais_failed(QProcess::Crashed);
        removeTempFiles();
        return;
    }
    if (exitcode == 0) {
        // TODO: try-catch
        // DAVIDE _ HDR CREATION
        for (const auto &it : m_data) {
            QString inputFilename = it.alignedFilename();
            if (!inputFilename.isEmpty()) {
                ExifOperations::copyExifData(it.filename().toStdString(),
                                             inputFilename.toStdString(), true,
                                             "", false, false);
            }
        }

        // parallel load of the data...
        // Start the computation.
        connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
                &Align::alignedFilesLoaded, Qt::DirectConnection);
        m_futureWatcher.setFuture(
            QtConcurrent::map(m_data.begin(), m_data.end(), LoadFile()));
    } else {
        qDebug() << "align_image_stack exited with exit code " << exitcode;
        removeTempFiles();
        emit finishedAligning(exitcode);
    }
}

void Align::alignedFilesLoaded() {
    disconnect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
               &Align::alignedFilesLoaded);
    for (const auto &it : m_data) {
        if (it.filename().isEmpty()) continue;
        QFile::remove(QFile::encodeName(it.convertedFilename()).constData());
        QFile::remove(QFile::encodeName(it.alignedFilename()).constData());
        qDebug() << "void Align::ais_finished: remove "
                 << it.convertedFilename();
        qDebug() << "void Align::ais_finished: remove " << it.alignedFilename();
    }

    QFile::remove(m_luminance_options.getTempDir() +
                  "/hugin_debug_optim_results.txt");
    emit finishedAligning(0);
}

void Align::ais_failed_slot(QProcess::ProcessError error) {
    qDebug() << "align_image_stack failed";
    removeTempFiles();
    emit failedAligning(error);
}

void Align::readData() {
    QByteArray data = m_ais->readAll();
    emit dataReady(data);
}

void Align::reset() {
    if (m_ais != NULL && m_ais->state() != QProcess::NotRunning) {
        m_ais->kill();
    }
}

void Align::removeTempFiles() {
    for (const auto &it : m_data) {
        if (!it.alignedFilename().isEmpty()) {
            QFile::remove(
                QFile::encodeName(it.convertedFilename()).constData());
            QFile::remove(QFile::encodeName(it.alignedFilename()).constData());
            qDebug() << "void HdrCreationManager::ais_finished: remove "
                     << it.convertedFilename();
            qDebug() << "void HdrCreationManager::ais_finished: remove "
                     << it.alignedFilename();
        }
    }
}
