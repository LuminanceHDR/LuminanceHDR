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
#include <QFile>
#include <QString>
#include <QStringList>

#include "Common/global.h"
#include "Common/config.h"
#include "MainWindow/MainWindow.h"

#ifdef WIN32
#include <QMessageBox>
#include <windows.h>
#endif

namespace
{
QStringList getCliFiles(const QStringList& arguments)
{
    QStringList return_value;   // empty QStringList;

    // check if any of the parameters is a proper file on the file system
    // I skip the first value of the list because it is the name of the executable
    for (int i = 1; i < arguments.size(); ++i)
    {
        QFile file( arguments.at(i).toLocal8Bit() );

        if ( file.exists() ) return_value.push_back( arguments.at(i).toLocal8Bit() );
    }

    return return_value;
}
}

#ifdef WIN32
inline void customMessageHandler(QtMsgType type, const char *msg)
{
	QString txt;
	switch (type) {
	case QtDebugMsg:
		txt = QString("Debug: %1").arg(msg);
		break;
	case QtWarningMsg:
		txt = QString("Warning: %1").arg(msg);
	break;
	case QtCriticalMsg:
		txt = QString("Critical: %1").arg(msg);
	break;
	case QtFatalMsg:
		txt = QString("Fatal: %1").arg(msg);
		abort();
	}

	QFile outFile("debuglog.txt");
	outFile.open(QIODevice::WriteOnly | QIODevice::Append);
	QTextStream ts(&outFile);
	ts << txt << endl;
}
#endif

int main( int argc, char ** argv )
{
    Q_INIT_RESOURCE(icons);
    QApplication application( argc, argv );

#ifdef WIN32
    //qInstallMsgHandler(customMessageHandler);

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

    QCoreApplication::setApplicationName(LUMINANCEAPPLICATION);
    QCoreApplication::setOrganizationName(LUMINANCEORGANIZATION);

    LuminanceOptions::isCurrentPortableMode = QDir(QApplication::applicationDirPath()).exists("PortableMode.txt");

    if (LuminanceOptions::isCurrentPortableMode) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::currentPath());
    }

    installTranslators(true);

    MainWindow* MW = new MainWindow;

    MW->setInputFiles( getCliFiles( application.arguments() ) );

    MW->show();

    return application.exec();
}

