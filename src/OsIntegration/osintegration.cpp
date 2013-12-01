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

#ifdef Q_OS_WIN
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
#endif

OsIntegration* OsIntegration::instance = 0;

OsIntegration::OsIntegration()
    : QObject((QObject*)0),
    m_progressMin(0), 
    m_progressMax(100)
{
	#ifdef Q_OS_WIN
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
	#ifdef Q_OS_WIN
		winProgressbar->init(mainWindow);
	#endif
}

void OsIntegration::setProgress(int value, int max)
{
	#ifdef Q_OS_WIN
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


bool OsIntegration::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifdef Q_OS_WIN
	return winProgressbar->nativeEvent(eventType, message, result);
#endif
}

bool OsIntegration::isRunningOnSameCpuPlatform() {
#if defined(_WIN32)
    // 32-bit programs run on both 32-bit and 64-bit Windows
    // so must sniff
    BOOL f64 = true;
    LPFN_ISWOW64PROCESS fnIsWow64Process;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
    if(NULL != fnIsWow64Process)
    {
        return !(fnIsWow64Process(GetCurrentProcess(),&f64) && f64);
    }
    return true;
#else
    return true;
#endif
}
