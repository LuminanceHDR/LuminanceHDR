/**
 * This file is a part of Luminance HDR package.
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QApplication>
#include <QObject>
#include <QDebug>

#include "Common/global.h"
#include "Common/commandline.h"
#include "MainWindow/MainWindow.h"

#ifdef WIN32
#include <QMessageBox>
#include <windows.h>
#endif

int main( int argc, char ** argv )
{
    Q_INIT_RESOURCE(icons);
    QApplication application( argc, argv );

#ifdef WIN32
    bool found_DLL = false;
    foreach (QString path, application.libraryPaths())
    {
        if ( QFile::exists(path+"/imageformats/qjpeg4.dll") )
        {
            found_DLL = true;
        }
    }
    if (!found_DLL)
    {
        QMessageBox::critical(NULL,
                              QObject::tr("Aborting..."),
                              QObject::tr("Cannot find Qt's JPEG Plugin...<br>Please unzip the DLL package with the option \"use folder names\" activated."));
        return 1;
    }
#endif

#ifdef QT_DEBUG
    //qDebug() << "i18n folder = " << I18NDIR;
    //qDebug() << "QDir::currentPath() = " << QDir::currentPath();
    //qDebug() << "QCoreApplication::applicationDirPath() = " << QCoreApplication::applicationDirPath();
#endif
    installTranslators(true);
    MainWindow* MW = new MainWindow;
#ifndef Q_WS_MAC
    MW->setInputFiles(cli.files());
#endif // Q_WS_MAC
    MW->show();

    return application.exec();
}

