/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2016 Franco Comida
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
 * @author Franco Comida <francocomida@users.sourceforge.net>
 */

#include <QtGlobal>
#include <boost/foreach.hpp>
#include "UI/SupportedCamerasDialog.h"
#include "UI/ui_SupportedCamerasDialog.h"
#include "Common/GitSHA1.h"
#include "Common/config.h"

#ifdef Q_OS_MACOS
#include <libraw.h>
#else
#include <libraw/libraw.h>
#endif

SupportedCamerasDialog::SupportedCamerasDialog(QWidget *parent)
    : QDialog(parent), m_Ui(new Ui::SupportedCamerasDialog) {
    m_Ui->setupUi(this);

    m_Ui->labelVersion->setText(QStringLiteral(LUMINANCEVERSION) + " [Build " +
                                QString(g_GIT_SHA1).left(6) + "]");

    const char *version = LibRaw::version();
    int count = LibRaw::cameraCount();
    m_Ui->labelLibRawVersion->setText(m_Ui->labelLibRawVersion->text() + " " +
                                      QString(version).remove("-Release"));
    m_Ui->labelCount->setText(QStringLiteral("%1").arg(count) + " " +
                              m_Ui->labelCount->text());

    const char **list = LibRaw::cameraList();
    while (*list) {
        QListWidgetItem *item = new QListWidgetItem(*list++);
        m_Ui->listWidget->addItem(item);
        m_items.append(new QListWidgetItem(*item));
    }

    connect(m_Ui->lineEdit, &QLineEdit::textEdited, this,
            &SupportedCamerasDialog::search);
}

SupportedCamerasDialog::~SupportedCamerasDialog() {
    while (!m_items.isEmpty()) delete m_items.takeFirst();
}

// I REALLY HATE this implementation!!!
void SupportedCamerasDialog::search(const QString &text) {
    m_Ui->listWidget->clear();

    // for (auto i : m_items)
    BOOST_FOREACH (QListWidgetItem *i, m_items) {
        m_Ui->listWidget->addItem(new QListWidgetItem(
            *i));  // listWidget takes ownership of its items
    }
    QList<QListWidgetItem *> tmp_found = m_Ui->listWidget->findItems(
        text, Qt::MatchContains);  // still owned by listWidget
    QList<QListWidgetItem *> found;
    // for (auto i : tmp_found)
    BOOST_FOREACH (QListWidgetItem *i, tmp_found) {
        found.append(new QListWidgetItem(*i));  // new copy owned by found QList
    }
    m_Ui->listWidget->clear();
    // for (auto i : found)
    BOOST_FOREACH (QListWidgetItem *i, found) {
        m_Ui->listWidget->addItem(
            i);  // ownership transferred by found to listWidget
    }
}
