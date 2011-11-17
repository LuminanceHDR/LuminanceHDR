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
#include <QLocale>
#include <QTranslator>
#include <QObject>
#include <QDebug>

#include "Common/commandline.h"
#include "MainWindow/MainWindow.h"

#ifdef WIN32
#include <QMessageBox>
#include <windows.h>
#endif

int main( int argc, char ** argv )
{
    //QCoreApplication::setOrganizationName("Luminance");
    //QCoreApplication::setApplicationName("Luminance");

    // Make sure an Q*Application exists before instantiating the QSettings
    // Without this some systems will deadlock
    QCoreApplication *cliApplication = new QCoreApplication( argc, argv );
    LuminanceOptions luminance_options;

    CommandLineInterfaceManager cli( argc, argv );

    if (cli.isCommandLineApp())
    {
        // Command Line Application

        QTranslator translator;
        translator.load(QString("lang_") + luminance_options.getGuiLang(), I18NDIR);
        cliApplication->installTranslator(&translator);
        cli.execCommandLineParams();
        cliApplication->connect(&cli, SIGNAL(finishedParsing()), cliApplication, SLOT(quit()));

        int ret_value = cliApplication->exec();
        delete cliApplication;
        return ret_value;
    }
    else
    {
        // GUI application

        // Only one Q*Application may exist at a time
        delete cliApplication;

#ifdef WIN32
        FreeConsole();
#endif
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
        qDebug() << "i18n folder = " << I18NDIR;
        //qDebug() << "QDir::currentPath() = " << QDir::currentPath();
        //qDebug() << "QCoreApplication::applicationDirPath() = " << QCoreApplication::applicationDirPath();
#endif

        QTranslator guiTranslator;
        QTranslator qtTranslator;
        qtTranslator.load(QString("qt_") + luminance_options.getGuiLang(), I18NDIR);
        guiTranslator.load(QString("lang_") + luminance_options.getGuiLang(), I18NDIR);
        application.installTranslator(&qtTranslator);
        application.installTranslator(&guiTranslator);

        MainWindow* MW = new MainWindow;
        MW->setInputFiles(cli.files());
        MW->show();

        return application.exec();
    }
}

