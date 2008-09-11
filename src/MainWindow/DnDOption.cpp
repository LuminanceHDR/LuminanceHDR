/**
 * This file is a part of Qtpfsgui package.
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

#include "DnDOption.h"
#include "../Common/global.h"
#include <QRegExp>

DnDOptionDialog::DnDOptionDialog(QWidget *p, QStringList files) : QDialog(p) {
	ui.setupUi(this);
	result = -1;
	bool areAllHDRs = true;
	bool areAllLDRs = true;
	foreach (QString file, files) {
		areAllHDRs = areAllHDRs && matchesHdrFilename(file);
		areAllLDRs = areAllLDRs && matchesLdrFilename(file);
	}
	ui.btnCreateNewHDR->setEnabled(areAllLDRs);
	ui.btnOpenHDR->setEnabled(areAllHDRs);
	if (!(areAllHDRs || areAllLDRs))
		QDialog::accept();
	else
		activateWindow();
}

DnDOptionDialog::~DnDOptionDialog() {
}

void DnDOptionDialog::on_btnCancel_clicked() {
	QDialog::accept();
}

void DnDOptionDialog::on_btnCreateNewHDR_clicked() {
	result = 1;
	QDialog::accept();
}

void DnDOptionDialog::on_btnOpenHDR_clicked() {
	result = 2;
	QDialog::accept();
}
