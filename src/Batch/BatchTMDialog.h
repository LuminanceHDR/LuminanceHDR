/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Giuseppe Rota
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing 
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef BATCH_IMPL_H
#define BATCH_IMPL_H

#include <QVector>
#include <QStringListModel>
#include <QSortFilterProxyModel>

#include "ui_BatchTMDialog.h"
#include "Common/options.h"
#include "Common/global.h"

namespace pfs {
class Frame;
}

class BatchTMDialog : public QDialog, public Ui::BatchTMDialog
{
Q_OBJECT

public:
	BatchTMDialog(QWidget *parent=0);
	~BatchTMDialog();
// protected:
private slots:
	void add_dir_HDRs();
	void add_HDRs();
	void add_dir_TMopts();
	void add_TMopts();
	void out_folder_clicked();
	void check_enable_start();
	void remove_HDRs();
	void remove_TMOpts();
	void start_called();
	void conditional_loadthread();
	void conditional_TMthread();
	void load_HDR_failed(QString);
	void finished_loading_hdr(pfs::Frame*,QString);
	void newResult(const QImage&,TonemappingOptions*);
	void newResult(const QImage&);
	void filterChanged(const QString&);
	void filterComboBoxActivated(int);
protected:
// 	void closeEvent(QCloseEvent *);
private:
	//selection start/stop left/right.
	int start_left,stop_left,start_right,stop_right;
	// number of threads currently running in parallel.
	int running_threads;
	//required for the cache path and for the dcraw opts required by the hdr-load thread.
	LuminanceOptions *luminance_options;
	//the filename for the hdr we are currently working on, used when a LDR result comes up.
	QString current_hdr_fname;
	//Application-wide settings, loaded via QSettings.
	QString RecentDirHDRSetting, RecentPathLoadSaveTmoSettings, recentPathSaveLDR;
	//data structure (model) for left-side list: HDRs.
	QStringList HDRs_list;
	//data structure (model) for right-side list: tone mapping options.
	QList< QPair<TonemappingOptions*,bool> > tm_opt_list;
	//when removing we cycle through the list to grab the selected interval.
	void update_selection_interval(bool left);
	//updates graphica widget (view) and data structure (model) for HDR list.
	void add_view_model_HDRs(QStringList);
	//updates graphica widget (view) and data structure (model) for TM_opts list.
	void add_view_model_TM_OPTs(QStringList);
	//Parses a TM_opts file (return NULL on error).
	TonemappingOptions* parse_tm_opt_file(QString filename);
	//set to true once we are done processing.
	bool done;
	//fuction that adds a log message to the model
	void add_log_message(const QString &);
	//the class that performs regexp filtering
	QSortFilterProxyModel *log_filter;
	//the model that holds the data
	QStringListModel *full_Log_Model;
	QVector<pfs::Frame*> workingFrame;
	int job_num;
	//QVector< Qvector<pfs::Frame*> >  workingFrame;
	int row,colon;
	TonemappingOptions *opts;
};
#endif
