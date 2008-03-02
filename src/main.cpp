/**
 * This file is a part of Qtpfsgui package.
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
#include <QLocale>
#include <QTranslator>
// #include <QDir>
#include "MainWindow/mainWindow.h"
#include "Common/commandline.h"

#ifdef WIN32
#include <QMessageBox>
#endif

int main( int argc, char ** argv )
{
#ifndef Q_WS_MAC
	//CLI application
	// Do not try to run the CLI app on Mac -
	// TODO: the problem is that some args are supplied when
	// running as a gui-app, which does not work!
	if (argc>1) {
		QCoreApplication CLIapplication( argc, argv );
		CommandLineInterfaceManager cli( argc, argv );
		CLIapplication.connect(&cli, SIGNAL(finishedParsing()), &CLIapplication, SLOT(quit()));
		return CLIapplication.exec();
	}
#endif

	//GUI application
	QApplication application( argc, argv );
// 	qDebug() << "QDir::currentPath()=" << QDir::currentPath();
// 	qDebug() << "QCoreApplication::applicationDirPath()=" << QCoreApplication::applicationDirPath();

#ifdef WIN32
	bool found_DLL=false;
	foreach (QString path, application.libraryPaths()) {
		if (QFile::exists(path+"/imageformats/qjpeg4.dll"))
			found_DLL=true;
	}
	if (!found_DLL) {
		QMessageBox::critical(0,QObject::tr("Aborting..."),QObject::tr("Cannot find Qt's JPEG Plugin...<br>Please unzip the DLL package with the option \"use folder names\" activated."));
		return 1;
	}
#endif
	QString locale = QLocale::system().name();
	QTranslator translator;
// 	qDebug( "Looking for i18n files in: " I18NDIR );
	translator.load(QString("lang_") + locale, I18NDIR);
	application.installTranslator(&translator);
	MainGui maingui;
	application.connect( &application, SIGNAL(lastWindowClosed()), &application, SLOT(quit()) );
	maingui.show();
	return application.exec();
}
