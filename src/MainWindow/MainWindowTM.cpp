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

#include "MainWindow/MainWindowTM.h"
#include "MainWindow/MainWindow.h"
#include "Core/TMWorker.h"

MainWindowTM::MainWindowTM(MainWindow* mw, QObject *parent) :
    QObject(parent),
    m_MainWindows(mw),
    m_TMWorker(new TMWorker),
    m_TMThread(new QThread)
{
    m_TMWorker->moveToThread(m_TMThread);

    // Memory Management
    connect(this, SIGNAL(destroyed()), m_TMWorker, SLOT(deleteLater()));
    connect(m_TMWorker, SIGNAL(destroyed()), m_TMThread, SLOT(deleteLater()));
}
