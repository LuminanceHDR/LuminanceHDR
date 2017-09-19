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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QCoreApplication>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QScopedPointer>
#include <QTranslator>
#include <QUrl>
#include <iostream>

#include "Common/LuminanceOptions.h"
#include "Common/config.h"
#include "Common/global.h"
#include "Common/global.hxx"

bool matchesLdrFilename(const QString &file) {
    QRegExp exp(
        ".*\\.(jpeg|jpg|tiff|tif|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|"
        "raf|ptx|pef|"
        "x3f|raw|sr2|rw2)$",
        Qt::CaseInsensitive);
    return exp.exactMatch(file);
}

bool matchesHdrFilename(const QString &file) {
#ifdef HAVE_CFITSIO
    QRegExp exp(
        ".*\\.(exr|hdr|pic|tiff|tif|fit|fits|pfs|crw|cr2|nef|dng|mrw|"
        "orf|kdc|dcr|"
        "arw|raf|ptx|pef|x3f|raw|sr2|rw2)$",
        Qt::CaseInsensitive);
#else
    QRegExp exp(
        ".*\\.(exr|hdr|pic|tiff|tif|pfs|crw|cr2|nef|dng|mrw|orf|kdc|"
        "dcr|arw|raf|"
        "ptx|pef|x3f|raw|sr2|rw2)$",
        Qt::CaseInsensitive);
#endif
    return exp.exactMatch(file);
}

QStringList getAllHdrFileExtensions() {
    QStringList listAll;
    QStringList list;
    list << QStringLiteral(".exr") << QStringLiteral(".hdr")
         << QStringLiteral(".pic") << QStringLiteral(".tiff")
         << QStringLiteral(".tif");
#if HAVE_CFITSIO
    list << QStringLiteral(".fit") << QStringLiteral(".fits");
#endif
    list << QStringLiteral(".pfs") << QStringLiteral(".crw")
         << QStringLiteral(".cr2") << QStringLiteral(".nef")
         << QStringLiteral(".dng") << QStringLiteral(".mrw")
         << QStringLiteral(".orf") << QStringLiteral(".kdc")
         << QStringLiteral(".dcr") << QStringLiteral(".arw")
         << QStringLiteral(".raf") << QStringLiteral(".ptx")
         << QStringLiteral(".pef") << QStringLiteral(".x3f")
         << QStringLiteral(".raw") << QStringLiteral(".rw2")
         << QStringLiteral(".sr2") << QStringLiteral(".3fr")
         << QStringLiteral(".mef") << QStringLiteral(".mos")
         << QStringLiteral(".erf") << QStringLiteral(".nrw")
         << QStringLiteral(".srw");

    foreach (const QString &s, list) {
        listAll << s;
        listAll << s.toUpper();
    }
    return listAll;
}

bool matchesValidHDRorLDRfilename(const QString &file) {
    return matchesLdrFilename(file) || matchesHdrFilename(file);
}

QStringList convertUrlListToFilenameList(const QList<QUrl> &urls) {
    QStringList files;
    for (int i = 0; i < urls.size(); ++i) {
        QString localFile = urls.at(i).toLocalFile();
        if (matchesValidHDRorLDRfilename(localFile)) {
            files.append(localFile);
        }
    }
    return files;
}
