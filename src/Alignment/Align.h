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

#ifndef ALIGN_H
#define ALIGN_H

#include <QFuture>
#include <QFutureWatcher>
#include <QProcess>
#include <QScopedPointer>

#include "Common/LuminanceOptions.h"
#include "HdrWizard/HdrCreationItem.h"

class Align : public QObject {
    Q_OBJECT

   public:
    Align(HdrCreationItemContainer &data, bool fromCommandLine,
          int savingMode = 1, float minLum = 0.0f, float maxLum = 1.0f);
    ~Align();

    void align_with_ais(bool ais_crop_flag);
    void reset();
    void removeTempFiles();

   signals:
    void finishedAligning(int);
    void failedAligning(QProcess::ProcessError);
    void dataReady(QByteArray &);

   protected slots:
    void ais_finished(int exitcode, QProcess::ExitStatus exitstatus);
    void alignedFilesLoaded();
    void ais_failed_slot(QProcess::ProcessError error);
    void readData();

   protected:
    HdrCreationItemContainer m_data;
    bool m_fromCommandLine;
    int m_savingMode;
    float m_minLum;
    float m_maxLum;
    QScopedPointer<QProcess> m_ais;
    LuminanceOptions m_luminance_options;
    QFutureWatcher<void> m_futureWatcher;
};

#endif
