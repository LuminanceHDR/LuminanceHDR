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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <QDir>
#include <QProcess>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include <Core/TonemappingOptions.h>
#include <HdrWizard/HdrCreationManager.h>
#include <Libpfs/frame.h>
#include <Libpfs/params.h>
#include "ezETAProgressBar.hpp"

class CommandLineInterfaceManager : public QObject {
    Q_OBJECT
   public:
    CommandLineInterfaceManager(const int argc, char **argv);
    int execCommandLineParams();

   private:
    const int argc;
    char **argv;

    enum operation_mode {
        CREATE_HDR_MODE,
        LOAD_HDR_MODE,
        UNKNOWN_MODE
    } operationMode;

    enum align_mode { AIS_ALIGN, MTB_ALIGN, NO_ALIGN } alignMode;

    QList<float> ev;
    QScopedPointer<HdrCreationManager> hdrCreationManager;
    QString saveHdrFilename;
    QString saveLdrFilename;
    QScopedPointer<pfs::Frame> HDR;
    void saveHDR();
    void printHelp(char *progname);
    QScopedPointer<TonemappingOptions> tmopts;
    QScopedPointer<pfs::Params> tmofileparams;
    bool verbose;
    FusionOperatorConfig hdrcreationconfig;
    QString loadHdrFilename;
    QStringList inputFiles;
    ez::ezETAProgressBar progressBar;
    int oldValue;
    int maximum;
    bool started;
    float threshold;
    bool isAutolevels;
    bool isHtml;
    bool isHtmlDone;
    int htmlQuality;
    bool isProposedLdrName;
    bool isProposedHdrName;
    std::string pageName;
    std::string imagesDir;
    std::string ldrExtension;
    std::string hdrExtension;
    QString saveAlignedImagesPrefix;
    QStringList validLdrExtensions;
    QStringList validHdrExtensions;

    void generateHTML();
    void startTonemap();

   private slots:
    void finishedLoadingInputFiles();
    void ais_failed(QProcess::ProcessError);
    void errorWhileLoading(const QString &);
    void createHDR(int);
    void execCommandLineParamsSlot();
    void setProgressBar(int);
    void updateProgressBar(int);
    void readData(const QByteArray &);
    void tonemapFailed(const QString &);

   signals:
    void finishedParsing();
};

#endif
