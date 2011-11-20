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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  MainWindowTM implements TM functionalities of the MainWindow, decoupling dependencies
 *  and acting like an adapter. Some of the slots pass thru, will some are intercepted to
 *  improve tonemap progress.
 */

#ifndef MAINWINDOWTM_H
#define MAINWINDOWTM_H

#include <QObject>

// Forward declaration
namespace pfs {
    class Frame;
}

class MainWindow;
class TMWorker;
class TonemappingOptions;
class TMOProgressIndicator;

class MainWindowTM : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowTM(MainWindow* mw, QObject *parent = 0);

signals:
    void getTonemappedFrame(pfs::Frame*, TonemappingOptions*);
    void tonemapSuccess(pfs::Frame*, TonemappingOptions*);
    void tonemapFailed(QString);

public slots:

private slots:
    void tonemapBegin();
    void tonemapEnd();

private:
    MainWindow* m_MainWindows;
    TMWorker*   m_TMWorker;
    QThread*    m_TMThread;
    TMOProgressIndicator* m_TMProgressBar;
};

#endif // MAINWINDOWTM_H
