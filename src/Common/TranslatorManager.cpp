/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
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
 */

#include "TranslatorManager.h"
#include "global.hxx"

#include <QCoreApplication>
#include <QDebug>
#include <QLibraryInfo>

TranslatorManager::ScopedQTranslator TranslatorManager::sm_appTranslator;
TranslatorManager::ScopedQTranslator TranslatorManager::sm_qtTranslator;

void TranslatorManager::setLanguage(const QString &lang,
                                    bool installQtTranslation) {
    if (lang == QLatin1String("en")) {
        cleanAppTranslator();
        cleanQtTranslator();

        return;
    }

    setAppTranslator(lang);
    if (installQtTranslation) setQtTranslator(lang);
}

void TranslatorManager::cleanAppTranslator() {
    if (sm_appTranslator) {
        QCoreApplication::removeTranslator(sm_appTranslator.data());
        sm_appTranslator.reset();
    }
}

void TranslatorManager::cleanQtTranslator() {
    if (sm_qtTranslator) {
        QCoreApplication::removeTranslator(sm_qtTranslator.data());
        sm_qtTranslator.reset();
    }
}

void TranslatorManager::setAppTranslator(const QString &lang) {
    qDebug() << "I18NDIR: " << I18NDIR;
    cleanAppTranslator();

    ScopedQTranslator appTranslator(new QTranslator());

    if (appTranslator->load(QStringLiteral("lang_") + lang,
                            QStringLiteral("i18n")) ||
        appTranslator->load(QStringLiteral("lang_") + lang, I18NDIR)) {
        QCoreApplication::installTranslator(appTranslator.data());
        sm_appTranslator.swap(appTranslator);
    }
}

void TranslatorManager::setQtTranslator(const QString &lang) {
    qDebug() << "QLibraryInfo::location(QLibraryInfo::TranslationsPath)): "
             << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    cleanQtTranslator();

    ScopedQTranslator qtTranslator(new QTranslator());

    if (qtTranslator->load(QStringLiteral("qt_") + lang,
                           QStringLiteral("i18n")) ||
        // qtTranslator->load(QString("qt_") + lang, I18NDIR) )
        qtTranslator->load(
            QStringLiteral("qt_") + lang,
            QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(qtTranslator.data());
        sm_qtTranslator.swap(qtTranslator);
    }
}
