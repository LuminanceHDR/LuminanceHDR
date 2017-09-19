/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * Copyright (C) 2009 Franco Comida
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
 * Copied from fontmatrix.
 * C++ Implementation: fmpaths
 * Adapted to Luminance HDR
 *
 */

#include "HelpBrowser/LuminancePaths.h"
#include "HelpBrowser/help-path.hxx"

#include <QApplication>
#include <iostream>

LuminancePaths *LuminancePaths::instance = 0;
LuminancePaths *LuminancePaths::getThis() {
    if (!instance) instance = new LuminancePaths;
    return instance;
}

namespace {
const QString dirsep(QDir::separator());
}

QString LuminancePaths::HelpDir() {
    if (getThis()->LuminancePathsDB.contains(QStringLiteral("HelpDir"))) {
        QString hf = getThis()->LuminancePathsDB[QStringLiteral("HelpDir")];
        if (!hf.isEmpty() && QDir(hf).exists()) {
            return hf;
        }
    }

#ifdef Q_OS_MAC
    QString hf = LocalizedDirPath(QApplication::applicationDirPath() + dirsep +
                                  "../Resources/help/en" + dirsep);
#elif _WIN32
    QString hf = LocalizedDirPath(QApplication::applicationDirPath() + dirsep +
                                  "help" + dirsep);
// no fall-back
#else  // UNIX
    // hf = LocalizedDirPath( PREFIX + dirsep + "share" + dirsep + "fontmatrix"
    // +
    // dirsep + "help" + dirsep );
    // hf = LocalizedDirPath("usr" + dirsep + "share" + dirsep + "luminance-hdr"
    // +
    // dirsep + "help" + dirsep);
    QString hf = LocalizedDirPath(HELPDIR + dirsep);
#endif

    getThis()->LuminancePathsDB[QStringLiteral("HelpDir")] = hf;

    return hf;
}

QString LuminancePaths::LocalizedDirPath(const QString &base,
                                         const QString &fallback) {
    QString sep(QStringLiteral("_"));
    QStringList l_c(QLocale::system().name().split(sep));
    QString langcode(l_c.first());
    QString countrycode(l_c.last());

    QStringList names;
    if ((!langcode.isEmpty()) || (!countrycode.isEmpty())) {
        names << base + langcode + sep + countrycode;
        names << base + langcode;
    }
    names << base + fallback;
    names << base;

    foreach (const QString &t, names) {
        QDir d(t);
        if (d.exists()) return d.absolutePath() + QString(QDir::separator());
    }

    return QString();
}
