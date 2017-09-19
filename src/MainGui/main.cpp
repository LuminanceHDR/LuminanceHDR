/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2012 Davide Anastasia
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 */

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QStringList>

#include "BatchHDR/BatchHDRDialog.h"
#include "BatchTM/BatchTMDialog.h"
#include "Common/TranslatorManager.h"
#include "Common/config.h"
#include "Common/global.h"
#include "MainWindow/DonationDialog.h"
#include "MainWindow/MainWindow.h"

namespace {
QStringList getCliFiles(const QStringList &arguments) {
    // empty QStringList;
    QStringList fileList;

    // check if any of the parameters is a proper file on the file system
    // I skip the first value of the list because it is the name of the
    // executable
    for (int i = 1; i < arguments.size(); ++i) {
        QFile file(arguments.at(i).toLocal8Bit());

        if (file.exists()) {
            fileList.push_back(arguments.at(i).toLocal8Bit());
        }
    }

    return fileList;
}

bool check_db() {
    LuminanceOptions options;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(options.getDatabaseFileName());
    db.setHostName(QStringLiteral("localhost"));
    bool ok = db.open();
    if (!ok) {
        QTextStream out(stdout);
        out << QObject::tr(
                   "The database used for saving TM parameters cannot "
                   "be opened.\n"
                   "Error: %1")
                   .arg(db.lastError().databaseText());
    }
    return ok;
}
}

#if defined(WIN32) || defined(__APPLE__)
void customMessageHandler(QtMsgType type, const QMessageLogContext &context,
                          const QString &msg) {
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
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream ts(&outFile);
        ts << txt << endl;
    }
}
#endif

int main(int argc, char **argv) {
    QCoreApplication::setApplicationName(LUMINANCEAPPLICATION);
    QCoreApplication::setOrganizationName(LUMINANCEORGANIZATION);

#ifdef WIN32  // TODO: there are problems with HiDPI on X11, let's enable this
              // only on Windows by now
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    Q_INIT_RESOURCE(icons);
    QApplication application(argc, argv);

#ifdef WIN32
    // qInstallMessageHandler(customMessageHandler);

    QIcon::setThemeSearchPaths(QStringList()
                               << QGuiApplication::applicationDirPath() +
                                      QString("/icons/luminance-hdr"));
    QIcon::setThemeName("luminance-hdr");
#endif
#ifdef __APPLE__
    QIcon::setThemeSearchPaths(
        QStringList() << QCoreApplication::applicationDirPath() +
                             QString("/../Resources/icons/luminance-hdr"));
    QIcon::setThemeName("luminance-hdr");
#endif

    LuminanceOptions::isCurrentPortableMode =
        QDir(QGuiApplication::applicationDirPath())
            .exists(QStringLiteral("PortableMode.txt"));
    LuminanceOptions::checkHomeFolder();

    if (LuminanceOptions::isCurrentPortableMode) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           QDir::currentPath());
    }

    LuminanceOptions::conditionallyDoUpgrade();
    TranslatorManager::setLanguage(LuminanceOptions().getGuiLang());

    LuminanceOptions().applyTheme(true);

    QStringList arguments = application.arguments();

    QString appname = arguments.at(0);

    bool isBatchHDR = false;
    bool isBatchTM = false;

    foreach (QString arg, arguments) {
        if (arg.startsWith("--batchhdr")) isBatchHDR = true;
        if (arg.startsWith("--batchtm")) isBatchTM = true;
    }

    if (appname.contains("luminance-hdr") && (!isBatchHDR) && (!isBatchTM)) {
        DonationDialog::showDonationDialog();

        // TODO: create update checker...
        // TODO: pass update checker to MainWindow
        MainWindow *mainWindow = new MainWindow;

        mainWindow->show();
        mainWindow->openFiles(getCliFiles(application.arguments()));

        return application.exec();
    } else if (appname.contains("batch-tonemapping") || isBatchTM) {
        if (!check_db()) return EXIT_FAILURE;

        BatchTMDialog *tmdialog = new BatchTMDialog;

        tmdialog->exec();
    } else if (appname.contains("batch-hdr") || isBatchHDR) {
        if (!check_db()) return EXIT_FAILURE;

        BatchHDRDialog *hdrdialog = new BatchHDRDialog;

        hdrdialog->exec();
    }

    return EXIT_SUCCESS;
}
