/**
 * This file is a part of Qtpfsgui package.
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

#ifndef TRANSPLANT_IMPL_H
#define TRANSPLANT_IMPL_H

#include <QDialog>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include "../generated_uic/ui_transplantexifdialog.h"

class TransplantExifDialog : public QDialog, private Ui::TransplantExifDialog {
Q_OBJECT
public:
	TransplantExifDialog(QWidget *);
	~TransplantExifDialog();
private:
	int start_left,stop_left,start_right,stop_right;
	QStringList from,to;
	bool done;
	void updateinterval(bool);
	QSettings settings;
	QString RecentDirEXIFfrom;
	QString RecentDirEXIFto;
	//fuction that adds a log message to the model
	void add_log_message(const QString &);
	//the class that performs regexp filtering
	QSortFilterProxyModel *log_filter;
	//the model that holds the data
	QStringListModel *full_Log_Model;
private slots:
	void transplant_requested();
	void help_requested();
	void moveup_left();
	void moveup_right();
	void movedown_left();
	void movedown_right();
	void remove_left();
	void remove_right();
	void append_left();
	void append_right();
	void filterChanged(const QString&);
	void filterComboBoxActivated(int);
};
#endif
