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
 */

#include <QStatusBar>

#include "MainWindow/MainWindowTM.h"
#include "MainWindow/MainWindow.h"
#include "Core/TMWorker.h"
#include "TonemappingPanel/TMOProgressIndicator.h"

MainWindowTM::MainWindowTM(MainWindow* mw, QObject *parent) :
    QObject(parent),
    m_MainWindows(mw),
    m_TMWorker(new TMWorker),
    m_TMThread(new QThread),
    m_TMProgressBar(new TMOProgressIndicator)
{
    m_TMWorker->moveToThread(m_TMThread);

    // Memory Management
    connect(this, SIGNAL(destroyed()), m_TMWorker, SLOT(deleteLater()));
    connect(m_TMWorker, SIGNAL(destroyed()), m_TMThread, SLOT(deleteLater()));
    connect(this, SIGNAL(destroyed()), m_TMProgressBar, SLOT(deleteLater()));

    // connect TMOProgressIndicator slots
    connect(m_TMWorker, SIGNAL(tonemapBegin()), this, SLOT(tonemapBegin()));
    connect(m_TMWorker, SIGNAL(tonemapEnd()), this, SLOT(tonemapEnd()));
    connect(m_TMWorker, SIGNAL(tonemapSetValue(int)), m_TMProgressBar, SLOT(setValue(int)));
    connect(m_TMWorker, SIGNAL(tonemapSetMaximum(int)), m_TMProgressBar, SLOT(setMaximum(int)));
    connect(m_TMWorker, SIGNAL(tonemapSetMinimum(int)), m_TMProgressBar, SLOT(setMinimum(int)));
    connect(m_TMProgressBar, SIGNAL(terminate()), m_TMWorker, SIGNAL(tonemapRequestTermination()));

    // connect IOWorker engine
    connect(this, SIGNAL(getTonemappedFrame(pfs::Frame*,TonemappingOptions*)),
            m_TMWorker, SLOT(computeTonemap(pfs::Frame*,TonemappingOptions*)));
    connect(m_TMWorker, SIGNAL(tonemapFailed(QString)),
            this, SIGNAL(tonemapFailed(QString)));
    connect(m_TMWorker, SIGNAL(tonemapSuccess(pfs::Frame*,TonemappingOptions*)),
            this, SIGNAL(tonemapSuccess(pfs::Frame*,TonemappingOptions*)));

    m_TMThread->start();
}

void MainWindowTM::tonemapBegin()
{
    // Insert TMOProgressIndicator
    m_MainWindows->statusBar()->addWidget(m_TMProgressBar);
    m_TMProgressBar->show();
}

void MainWindowTM::tonemapEnd()
{
    m_MainWindows->statusBar()->removeWidget(m_TMProgressBar);
    m_TMProgressBar->reset();
}
