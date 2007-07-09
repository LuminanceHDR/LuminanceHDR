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


#ifndef ALIGN_IMPL_H
#define ALIGN_IMPL_H

#include <QDialog>
#include "../generated_uic/ui_aligndialog.h"
#include "show_image.h"
class AlignDialog : public QDialog, private Ui::AlignDialog
{
Q_OBJECT
public:
	AlignDialog(QWidget *);
	~AlignDialog();
	QList<Image *> listimages;
	QList<QPair<CP*,CP*> > listcps;
// public slots:
signals:
	void clear_temp_CP();
private:
	QPair<CP*,CP*> *current_new_pair; //pointer, so that I can allocate new ones.
	Image* current_master;
	QList<QPair<CP*,CP*> > of_two_imgs;
	bool prevclickleft, prevclickright;
	int sel_start,sel_stop;
	bool update_tabs, update_graphical_list;
	void create_tabs();
	void updateinterval();
	void append_pair_tothe_table(QPair<CP*,CP*> *newpair,bool);
	void remove_pairs_belonging_to(int);
	void refresh_graphical_list();
	void update_pairs_of_two_imgs();
private slots:
	void moveup_files();
	void movedown_files();
	void remove_files();
	void append_files();
	void del_cps_of_files();
	void setmaster();
	void progress_tab(int);
	void slide_left();
	void slide_right();
	void point_finished_moving(CP*);
	void remove_current_pair();
	void newpoint_in_rightarea(int,int);
	void newpoint_in_leftarea(int,int);
	void image_tab_changed(int);
};
#endif
