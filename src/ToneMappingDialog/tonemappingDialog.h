/**
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007,2008 Giuseppe Rota
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

#ifndef TONEMAPPINGDIALOG_IMPL_H
#define TONEMAPPINGDIALOG_IMPL_H

#include <QMdiArea>

#include "../generated_uic/ui_tonemappingdialog.h"
#include "../Common/global.h"
#include "tonemapping_widget.h"

class TonemappingWindow : public QMainWindow, public Ui::TonemappingWindow
{
Q_OBJECT

public:
	TonemappingWindow(QWidget *parent, pfs::Frame* &f, QString prefixname);
	~TonemappingWindow();
protected:
	void closeEvent ( QCloseEvent * );
signals:
	void closing();
private:
	QMdiArea* mdiArea;
	QString recentPathSaveLDR, prefixname;

private slots:
	void addMDIresult(const QImage&,tonemapping_options*);
	void LevelsRequested(bool);
	void levels_closed();
	void updateActions(QMdiSubWindow *);
	void viewAllAsThumbnails();
	void current_ldr_fit_to_win(bool);
	void on_actionClose_All_triggered();
	void on_actionSaveAll_triggered();
	void on_actionSave_triggered();
	void enterWhatsThis();
};
#endif
