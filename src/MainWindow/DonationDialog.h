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

#ifndef DONATIONDIALOG_H
#define DONATIONDIALOG_H

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
class DonationDialog;
}

class DonationDialog : public QDialog {
    Q_OBJECT
   public:
    static void showDonationDialog();
    static void openDonationPage();

   private:
    QScopedPointer<Ui::DonationDialog> m_ui;

    explicit DonationDialog(QWidget *parent = 0);

   private slots:
    void onYesButtonClicked();
    void onNoButtonClicked();

   public:
    ~DonationDialog();
};

#endif  // DONATIONDIALOG_H
