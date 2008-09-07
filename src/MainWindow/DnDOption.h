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

#ifndef DNDOPTION_IMPL_H
#define DNDOPTION_IMPL_H

#include <QDialog>
#include <QStringList>
#include "../../generated_uic/ui_dndoption.h"

class DnDOptionDialog : public QDialog
{
Q_OBJECT
public:
	DnDOptionDialog(QWidget *parent, QStringList files);
	~DnDOptionDialog();

protected slots:
	void on_btnCancel_clicked();
	void on_btnCreateNewHDR_clicked();
	void on_btnOpenHDR_clicked();
public:
	int result; // 1=newHDR, 2=openHDR
private:
	bool matchesLdrFilename(QString file);
	bool matchesHdrFilename(QString file);
	Ui::DnDOption ui;

};
#endif
