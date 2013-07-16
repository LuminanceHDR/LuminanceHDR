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

#include "osintegration.h"

OsIntegration* OsIntegration::instance = 0;

OsIntegration::OsIntegration()
    : QObject((QObject*)0),
    m_progressMin(0), 
    m_progressMax(100)
{
	#ifdef Q_WS_WIN
		winProgressbar = new EcWin7();
	#endif
}

OsIntegration::~OsIntegration()
{ }

OsIntegration& OsIntegration::getInstance() {
	if (!instance)
		instance = new OsIntegration();
	return *instance;
}

OsIntegration* OsIntegration::getInstancePtr() {
	if (!instance)
		instance = new OsIntegration();
	return instance;
}

void OsIntegration::init(QWidget* mainWindow) {
	#ifdef Q_WS_WIN
		winProgressbar->init(mainWindow->winId());
	#endif
}

void OsIntegration::setProgress(int value, int max)
{
	#ifdef Q_WS_WIN
		if (value < 0)
			winProgressbar->setProgressState(EcWin7::NoProgress);
		else {
			winProgressbar->setProgressState(EcWin7::Normal);
			winProgressbar->setProgressValue(value, max);
		}
	#endif
}

void OsIntegration::setProgressValue(int value)
{
#ifdef Q_WS_WIN
    winProgressbar->setProgressState(EcWin7::Normal);
    winProgressbar->setProgressValue(value, m_progressMax - m_progressMin);
#endif
}

void OsIntegration::setProgressRange(int min, int max)
{
    m_progressMin = min;
    m_progressMax = max;
}

void OsIntegration::addRecentFile(const QString& filename)
	{
#ifdef Q_OS_WIN
	winProgressbar->addRecentFile(filename);
#endif
	}

#ifdef Q_WS_WIN
	bool OsIntegration::winEvent(MSG * message, long * result)
	{
		return winProgressbar->winEvent(message, result);
	}
#endif
