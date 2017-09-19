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
 * @author Daniel Kaneider <danielkaneider@users.sourceforge.net>
 */

#ifndef OSINTEGRATION_H
#define OSINTEGRATION_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QWidget>

#ifdef Q_OS_WIN
#include "ecwin7.h"
#endif

class OsIntegration : public QObject {
    Q_OBJECT

   public:
    static OsIntegration &getInstance();
    static OsIntegration *getInstancePtr();

    void init(QWidget *mainWindow);
    void setProgress(int value, int max = 100);

    ~OsIntegration();

    bool isRunningOnSameCpuPlatform();

    void addRecentFile(const QString &filename);

   public Q_SLOTS:
    void setProgressValue(int value);
    void setProgressRange(int min, int max);

   private:
    OsIntegration();
    OsIntegration(const OsIntegration &);
    OsIntegration &operator=(const OsIntegration &);

    static OsIntegration *instance;
    int m_progressMin;
    int m_progressMax;

#ifdef Q_OS_WIN
    EcWin7 *m_winProgressbar;
#endif
};
#endif  // OSINTEGRATION_H
