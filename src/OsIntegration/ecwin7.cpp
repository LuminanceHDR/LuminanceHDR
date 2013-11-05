/* EcWin7 - Support library for integrating Windows 7 taskbar features
 * into any Qt application
 * Copyright (C) 2010 Emanuele Colombo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "ecwin7.h"

#include <QtGui/5.1.1/QtGui/qpa/qplatformnativeinterface.h>
#include <QGuiApplication>

// Windows only GUID definitions
#if defined(Q_OS_WIN)
DEFINE_GUID(CLSID_TaskbarList,0x56fdf344,0xfd6d,0x11d0,0x95,0x8a,0x0,0x60,0x97,0xc9,0xa0,0x90);
DEFINE_GUID(IID_ITaskbarList3,0xea1afb91,0x9e28,0x4b86,0x90,0xE9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf);
#endif

// Constructor: variabiles initialization
EcWin7::EcWin7()
{
#ifdef Q_OS_WIN
    mTaskbar = NULL;
    mOverlayIcon = NULL;
    mTaskbarMessageId = RegisterWindowMessage(L"TaskbarButtonCreated");
    mWindowId = NULL;
#endif
}

// Init taskbar communication
void EcWin7::init(const QWidget* widget)
{
    mWindowId = getHWNDForWidget(widget);
}

QWindow* EcWin7::windowForWidget(const QWidget* widget) 
{
    QWindow* window = widget->windowHandle();
    if (window)
        return window;
    const QWidget* nativeParent = widget->nativeParentWidget();
    if (nativeParent) 
        return nativeParent->windowHandle();
    return 0; 
}

HWND EcWin7::getHWNDForWidget(const QWidget* widget)
{
    QWindow* window = windowForWidget(widget);
    if (window)
    {
        QPlatformNativeInterface* interfacep = QGuiApplication::platformNativeInterface();
        return static_cast<HWND>(interfacep->nativeResourceForWindow(QByteArrayLiteral("handle"), window));
    }
    return 0; 
}


// Windows event handler callback function
// (handles taskbar communication initial message)
#ifdef Q_OS_WIN
bool EcWin7::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    if (msg->message == mTaskbarMessageId)
    {
        HRESULT hr = CoCreateInstance(CLSID_TaskbarList,
                                      0,
                                      CLSCTX_INPROC_SERVER,
                                      IID_ITaskbarList3,

                                      reinterpret_cast<void**> (&(mTaskbar)));
        *result = hr;
        return true;
    }
    return false;
}
#endif

void EcWin7::addRecentFile(const QString& filename)
{
#ifdef Q_OS_WIN
    SHAddToRecentDocs(SHARD_PATHW, filename.toStdWString().c_str());
#endif
}

// Set progress bar current value
void EcWin7::setProgressValue(int value, int max)
{
#ifdef Q_OS_WIN
    if (!mTaskbar) return;
    mTaskbar->SetProgressValue(mWindowId, value, max);
#endif
}

// Set progress bar current state (active, error, pause, ecc...)
void EcWin7::setProgressState(ToolBarProgressState state)
{
#ifdef Q_OS_WIN
    if (!mTaskbar) return;
    mTaskbar->SetProgressState(mWindowId, (TBPFLAG)state);
#endif
}

// Set new overlay icon and corresponding description (for accessibility)
// (call with iconName == "" and description == "" to remove any previous overlay icon)
void EcWin7::setOverlayIcon(QString iconName, QString description)
{
#ifdef Q_OS_WIN
    if (!mTaskbar) return;
    HICON oldIcon = NULL;
    if (mOverlayIcon != NULL) oldIcon = mOverlayIcon;
    if (iconName == "")
    {
        mTaskbar->SetOverlayIcon(mWindowId, NULL, NULL);
        mOverlayIcon = NULL;
    }
    else
    {
        mOverlayIcon = (HICON) LoadImage(GetModuleHandle(NULL),
                                 iconName.toStdWString().c_str(),
                                 IMAGE_ICON,
                                 0,
                                 0,
                                 NULL);
        mTaskbar->SetOverlayIcon(mWindowId, mOverlayIcon, description.toStdWString().c_str());
    }
    if ((oldIcon != NULL) && (oldIcon != mOverlayIcon))
    {
        DestroyIcon(oldIcon);
    }
#endif
}
