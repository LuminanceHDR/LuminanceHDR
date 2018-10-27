/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2014 Davide Anastasia
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

#include "DonationDialog.h"

#include <QDesktopServices>
#include <QUrl>

#include "MainWindow/ui_DonationDialog.h"
#include "Common/LuminanceOptions.h"
#include "ui_DonationDialog.h"

namespace {
static const QString DONATION_DIALOG_KEY = QStringLiteral("DonationDialogShow");
}

DonationDialog::DonationDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::DonationDialog) {
    m_ui->setupUi(this);

    connect(m_ui->yesButton, &QAbstractButton::clicked, this,
            &DonationDialog::onYesButtonClicked);
    connect(m_ui->noButton, &QAbstractButton::clicked, this,
            &DonationDialog::onNoButtonClicked);
    connect(m_ui->askMeLaterButton, &QAbstractButton::clicked, this,
            &QWidget::close);
}

void DonationDialog::onYesButtonClicked() {
    openDonationPage();
    emit close();
}

void DonationDialog::onNoButtonClicked() {
    LuminanceOptions().setValue(DONATION_DIALOG_KEY, false);
    emit close();
}

DonationDialog::~DonationDialog() {}

void DonationDialog::showDonationDialog() {
    if (LuminanceOptions().value(DONATION_DIALOG_KEY, true).toBool()) {
        QScopedPointer<DonationDialog> dd(new DonationDialog);

        dd->exec();
    }
}

void DonationDialog::openDonationPage() {
    QDesktopServices::openUrl(
        QUrl(QStringLiteral("https://www.paypal.com/cgi-bin/\
                            webscr?cmd=_s-xclick&hosted_button_id=\
                            10037712")));  // Franco Comida
}
