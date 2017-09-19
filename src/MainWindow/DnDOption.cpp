/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
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
 * @author Daniel Kaneider
 */

#include <QRegExp>

#include "Common/global.h"
#include "MainWindow/DnDOption.h"
#include "MainWindow/ui_DnDOption.h"

DnDOptionDialog::DnDOptionDialog(QWidget *p, QStringList files, bool areAllHDRs,
                                 bool areAllLDRs)
    : QDialog(p), ui(new Ui::DnDOption) {
    ui->setupUi(this);
    result = ACTION_INVALID;
    ui->btnCreateNewHDR->setEnabled(areAllLDRs);
    ui->btnOpenHDR->setEnabled(areAllHDRs);
    if (!(areAllHDRs || areAllLDRs))
        QDialog::accept();
    else
        activateWindow();
}

DnDOptionDialog::~DnDOptionDialog() {}

void DnDOptionDialog::on_btnCancel_clicked() { QDialog::accept(); }

void DnDOptionDialog::on_btnCreateNewHDR_clicked() {
    result = ACTION_NEW_HDR;
    QDialog::accept();
}

void DnDOptionDialog::on_btnOpenHDR_clicked() {
    result = ACTION_OPEN_HDR;
    QDialog::accept();
}

int DnDOptionDialog::showDndDialog(QWidget *parent, QStringList files) {
    bool areAllHDRs = true;
    bool areAllLDRs = true;
    foreach (const QString &file, files) {
        areAllHDRs = areAllHDRs && matchesHdrFilename(file);
        areAllLDRs = areAllLDRs && matchesLdrFilename(file);
    }
    if (areAllHDRs && files.size() == 1)
        return ACTION_OPEN_HDR;  // just open the files without dialog
    else {
        DnDOptionDialog dndOption(parent, files, areAllHDRs, areAllLDRs);
        dndOption.exec();
        return dndOption.result;
    }
}
