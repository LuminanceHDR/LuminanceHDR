/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2008 Pierre Marchand <pierremarc@oep-h.com>
 * Copyright (C) 2009-2013 Davide Anastasia, Franco Comida, Daniel Kaneider
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
 *
 * Adapted to Luminance HDR
 *
 */
/* Get all useful paths in one place with the hope that */
/* finally we will not need resource.qrc anymore */
#ifndef LUMINANCEPATHS_H
#define LUMINANCEPATHS_H

#include <QDir>
#include <QLocale>
#include <QMap>
#include <QString>

class LuminancePaths {
    QMap<QString, QString> LuminancePathsDB;
    LuminancePaths() {}
    static LuminancePaths *instance;
    static LuminancePaths *getThis();

   public:
    static QString HelpDir();
    static QString LocalizedDirPath(
        const QString &base, const QString &fallback = QStringLiteral("en"));
};
#endif
