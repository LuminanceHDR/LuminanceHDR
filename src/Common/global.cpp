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

#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QUrl>
#include <QTranslator>
#include <QCoreApplication>
#include <QScopedPointer>
#include <iostream>

#include "Common/config.h"

#include "Common/LuminanceOptions.h"
#include "Common/global.h"
#include "global.hxx"

bool matchesLdrFilename(QString file)
{
	QRegExp exp(".*\\.(jpeg|jpg|tiff|tif|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|rw2)$", Qt::CaseInsensitive);
	return exp.exactMatch(file);
}

bool matchesHdrFilename(QString file)
{
	QRegExp exp(".*\\.(exr|hdr|pic|tiff|tif|pfs|crw|cr2|nef|dng|mrw|orf|kdc|dcr|arw|raf|ptx|pef|x3f|raw|sr2|rw2)$", Qt::CaseInsensitive);
	return exp.exactMatch(file);
}

bool matchesValidHDRorLDRfilename(QString file)
{
	return matchesLdrFilename(file) || matchesHdrFilename(file);
}

QStringList convertUrlListToFilenameList(QList<QUrl> urls)
{
    QStringList files;
    for (int i = 0; i < urls.size(); ++i)
    {
        QString localFile = urls.at(i).toLocalFile();
        if (matchesValidHDRorLDRfilename(localFile))
        {
            files.append(localFile);
        }
    }
    return files;
}

namespace
{
typedef QScopedPointer<QTranslator> ScopedQTranslator;

ScopedQTranslator lastGuiTranslator;
ScopedQTranslator lastQtTranslator;
}

void installTranslators(const QString& lang, bool installQtTranslations, bool isPortable)
{
    if (lastGuiTranslator)
    {
        QCoreApplication::removeTranslator(lastGuiTranslator.data());
        lastGuiTranslator.reset();
	}
    if (installQtTranslations && lastQtTranslator)
    {
        QCoreApplication::removeTranslator(lastQtTranslator.data());
        lastQtTranslator.reset();
	}
    if (lang != "en")
    {
        ScopedQTranslator guiTranslator( new QTranslator() );

        if (isPortable)
            guiTranslator->load(QString("lang_") + lang, QDir::currentPath() + QDir::separator() + "i18n");
        else
            guiTranslator->load(QString("lang_") + lang, I18NDIR);
        QCoreApplication::installTranslator(guiTranslator.data());
        lastGuiTranslator.swap( guiTranslator );

        if (installQtTranslations)
        {
            ScopedQTranslator qtTranslator( new QTranslator() );

            if (isPortable)
			    qtTranslator->load(QString("qt_") + lang, QDir::currentPath() + QDir::separator() + "i18n");
            else
			    qtTranslator->load(QString("qt_") + lang, I18NDIR);
            QCoreApplication::installTranslator(qtTranslator.data());
            lastQtTranslator.swap( qtTranslator );
	    }
	}
}

void installTranslators(bool installQtTranslations, bool isPortable)
{
	LuminanceOptions luminance_options;
	installTranslators(luminance_options.getGuiLang(), installQtTranslations, isPortable);
}
