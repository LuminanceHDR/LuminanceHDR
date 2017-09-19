/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 */

#include <QCoreApplication>

#include "Common/LuminanceOptions.h"
#include "Common/TranslatorManager.h"
#include "Common/config.h"

#include "MainCli/commandline.h"

int main(int argc, char **argv) {
    QCoreApplication::setApplicationName(LUMINANCEAPPLICATION);
    QCoreApplication::setOrganizationName(LUMINANCEORGANIZATION);
    QCoreApplication application(argc, argv);
    LuminanceOptions lumOpts;

    TranslatorManager::setLanguage(lumOpts.getGuiLang(), false);

    CommandLineInterfaceManager cli(argc, argv);

    try {
        int result = cli.execCommandLineParams();
        if (result != 0) return result;
    } catch (...) {
        return -1;
    }
    application.connect(&cli, SIGNAL(finishedParsing()), &application,
                        SLOT(quit()));

    return application.exec();
}
