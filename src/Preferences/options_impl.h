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

#ifndef OPTIONS_IMPL_H
#define OPTIONS_IMPL_H

#include <QDialog>
#include <QSettings>
#include "../generated_uic/ui_options.h"
#include "../options.h"

class QtpfsguiOptions : public QDialog, private Ui::OptionsDialog
{
Q_OBJECT
public:
	QtpfsguiOptions(QWidget *parent, qtpfsgui_opts *orig_opts, QSettings *s);
	~QtpfsguiOptions();
private:
	void change_color_of(QPushButton *,QColor *);
	void from_options_to_gui();
	qtpfsgui_opts *opts;
	QColor infnancolor, negcolor;
	QSettings *settings;
private slots:
	void negative_clicked();
	void infnan_clicked();
	void ok_clicked();
	void updateLineEditString();
};
#endif
