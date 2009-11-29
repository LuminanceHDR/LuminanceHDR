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

#include <QFileDialog>
#include <QTextStream>

#include "BatchTMDialog.h"
#include "Common/config.h"
#include "Libpfs/pfs.h"
#include "Threads/LoadHdrThread.h"
#include "Threads/TMOFactory.h"
#include "Exif/ExifOperations.h"
#include "Filter/pfscut.h"

BatchTMDialog::BatchTMDialog(QWidget *p) : QDialog(p), start_left(-1), stop_left(-1), start_right(-1), stop_right(-1), running_threads(0), done(false), workingFrame(NULL) {
	setupUi(this);

	luminance_options=LuminanceOptions::getInstance();

	Log_Widget->setWordWrap(true);
	RecentDirHDRSetting=settings.value(KEY_RECENT_PATH_LOAD_SAVE_HDR, QDir::currentPath()).toString();
	RecentPathLoadSaveTmoSettings=settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString();
	recentPathSaveLDR=settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString();

	connect(add_dir_HDRs_Button, SIGNAL(clicked()), this, SLOT(add_dir_HDRs()));
	connect(add_HDRs_Button, SIGNAL(clicked()), this, SLOT(add_HDRs()));
	connect(add_dir_TMopts_Button, SIGNAL(clicked()), this, SLOT(add_dir_TMopts()));
	connect(add_TMopts_Button, SIGNAL(clicked()), this, SLOT(add_TMopts()));
	connect(out_folder_Button, SIGNAL(clicked()), this, SLOT(out_folder_clicked()));
	connect(remove_HDRs_Button, SIGNAL(clicked()), this, SLOT(remove_HDRs()));
	connect(remove_TMOpts_Button, SIGNAL(clicked()), this, SLOT(remove_TMOpts()));
	connect(BatchGoButton, SIGNAL(clicked()), this, SLOT(start_called()));
	connect(filterLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(filterChanged(const QString&)));
	connect(filterComboBox, SIGNAL(activated(int)), this, SLOT(filterComboBoxActivated(int)));

	full_Log_Model=new QStringListModel();
	log_filter=new QSortFilterProxyModel(this);
	log_filter->setDynamicSortFilter(true);
	log_filter->setSourceModel(full_Log_Model);
	Log_Widget->setModel(log_filter);

	add_log_message(tr("Using %1 thread(s)").arg(luminance_options->num_threads));
	add_log_message(tr("Saving using file format: ")+luminance_options->batch_ldr_format);

	qRegisterMetaType<QImage>("QImage");
}

BatchTMDialog::~BatchTMDialog() {
	//pfs::DOMIO pfsio;
	//pfsio.freeFrame(workingFrame[running_threads]);
	QApplication::restoreOverrideCursor();
	while (!tm_opt_list.isEmpty())
		delete (tm_opt_list.takeFirst()).first;
}

void BatchTMDialog::add_dir_HDRs() {
	QString dirname=QFileDialog::getExistingDirectory(this, tr("Choose a directory"), RecentDirHDRSetting);
	if (!dirname.isEmpty()) {
		QStringList filters;
		filters << "*.exr" << "*.hdr" << "*.pic" << "*.tiff" << "*.tif" << "*.pfs" << "*.crw" << "*.cr2" << "*.nef" << "*.dng" << "*.mrw" << "*.orf" << "*.kdc" << "*.dcr" << "*.arw" << "*.raf" << "*.ptx" << "*.pef" << "*.x3f" << "*.raw" << "*.sr2" << "*.rw2";
		filters << "*.EXR" << "*.HDR" << "*.PIC" << "*.TIFF" << "*.TIF" << "*.PFS" << "*.CRW" << "*.CR2" << "*.NEF" << "*.DNG" << "*.MRW" << "*.ORF" << "*.KDC" << "*.DCR" << "*.ARW" << "*.RAF" << "*.PTX" << "*.PEF" << "*.X3F" << "*.RAW" << "*.SR2" << "*.RW2";
		QDir chosendir(dirname);
		chosendir.setFilter(QDir::Files);
		chosendir.setNameFilters(filters);
		QStringList onlyhdrs=chosendir.entryList();
		//hack to prepend to this list the path as prefix.
		onlyhdrs.replaceInStrings(QRegExp("(.+)"), chosendir.path()+"/\\1");
		add_view_model_HDRs(onlyhdrs);
	}
}

void BatchTMDialog::add_HDRs() {
	QString filetypes = tr("All HDR images ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 "
                      "*.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2)";
	QStringList onlyhdrs=QFileDialog::getOpenFileNames(this, tr("Select input images"), RecentDirHDRSetting, filetypes);
	add_view_model_HDRs(onlyhdrs);
}

void BatchTMDialog::add_dir_TMopts() {
	QString dirname=QFileDialog::getExistingDirectory(this, tr("Choose a directory"), RecentPathLoadSaveTmoSettings);
	if (!dirname.isEmpty()) {
		QStringList filters;
		filters << "*.txt";
		QDir chosendir(dirname);
		chosendir.setFilter(QDir::Files);
		chosendir.setNameFilters(filters);
		QStringList onlytxts=chosendir.entryList();
		//hack to prepend to this list the path as prefix.
		onlytxts.replaceInStrings(QRegExp("(.+)"), chosendir.path()+"/\\1");
		add_view_model_TM_OPTs(onlytxts);
	}
}

void BatchTMDialog::add_TMopts() {
	QStringList onlytxts=QFileDialog::getOpenFileNames(this, tr("Load tone mapping settings text files..."), RecentPathLoadSaveTmoSettings, tr("LuminanceHDR tone mapping settings text file (*.txt)"));
	add_view_model_TM_OPTs(onlytxts);
}

TonemappingOptions* BatchTMDialog::parse_tm_opt_file(QString fname) {
	try {
		return TMOptionsOperations::parseFile(fname);
	} catch (QString &e) {
		add_log_message(e);
		return NULL;
	}
}

void BatchTMDialog::check_enable_start() {
	//at least 1 hdr AND at least 1 tm_opt AND output_dir not empty
	BatchGoButton->setEnabled(!out_folder_widgets->text().isEmpty() && listWidget_HDRs->count()!=0 && listWidget_TMopts->count()!=0);
}

void BatchTMDialog::out_folder_clicked() {
	QString dirname=QFileDialog::getExistingDirectory(this, tr("Choose a directory"), recentPathSaveLDR);

	settings.setValue(KEY_RECENT_PATH_SAVE_LDR, dirname);
	recentPathSaveLDR=dirname;

	QFileInfo test(dirname);
	if (test.isWritable() && test.exists() && test.isDir() && !dirname.isEmpty()) {
		out_folder_widgets->setText(dirname);
		check_enable_start();
	}
}

void BatchTMDialog::add_view_model_HDRs(QStringList list) {
	QStringList::ConstIterator it = list.begin();
	while( it != list.end() ) {
		QFileInfo *qfi=new QFileInfo(*it);
		listWidget_HDRs->addItem(qfi->fileName()); //fill graphical list
		++it;
		delete qfi;
	}
	HDRs_list+=list;
	check_enable_start();
}

void BatchTMDialog::add_view_model_TM_OPTs(QStringList list) {
	QStringList::ConstIterator it = list.begin();
	while( it != list.end() ) {
		TonemappingOptions *i_th_tm_opt=parse_tm_opt_file(*it);
		if (i_th_tm_opt!=NULL) {
			//add to model
			tm_opt_list.append(qMakePair(i_th_tm_opt,false));
			//add to view
			QFileInfo *qfi=new QFileInfo(*it);
			listWidget_TMopts->addItem(qfi->fileName()); //fill graphical list
			delete qfi;
		}
		++it;
	}
	check_enable_start();
}

void BatchTMDialog::update_selection_interval(bool left) {
	if (left) {
		start_left=listWidget_HDRs->count();
		stop_left=-1;
		for (int i=0; i<listWidget_HDRs->count(); i++) {
			if (listWidget_HDRs->isItemSelected(listWidget_HDRs->item(i))) {
				start_left= (start_left>i) ? i : start_left;
				stop_left= (stop_left<i) ? i : stop_left;
			}
		}
	} else {
		start_right=listWidget_TMopts->count();
		stop_right=-1;
		for (int i=0; i<listWidget_TMopts->count(); i++) {
			if (listWidget_TMopts->isItemSelected(listWidget_TMopts->item(i))) {
				start_right= (start_right>i) ? i : start_right;
				stop_right= (stop_right<i) ? i : stop_right;
			}
		}
	}
}

void BatchTMDialog::remove_HDRs() {
	update_selection_interval(true);
	if (listWidget_HDRs->count()==0 || start_left==-1 || stop_left==-1)
		return;
	for (int i=stop_left-start_left+1; i>0; i--) {
		listWidget_HDRs->takeItem(start_left);
		HDRs_list.removeAt(start_left);
	}
	start_left=stop_left=-1;
	check_enable_start();
}

void BatchTMDialog::remove_TMOpts() {
	update_selection_interval(false);
	if (listWidget_TMopts->count()==0 || start_right==-1 || stop_right==-1)
		return;
	for (int i=stop_right-start_right+1; i>0; i--) {
		listWidget_TMopts->takeItem(start_right);
		delete (tm_opt_list.at(start_right)).first;
		tm_opt_list.removeAt(start_right);
	}
	start_right=stop_right=-1;
	check_enable_start();
}

//function used for ordering based on the value of pregamma
//somehow I was not able to embed this in the BatchTMDialog class :)
bool order_based_on_pregamma(const QPair<TonemappingOptions*,bool> &s1, const QPair<TonemappingOptions*,bool> &s2) {
	return ((s1.first)->pregamma < (s2.first)->pregamma);
}

void BatchTMDialog::start_called() {
	if (done) {
		accept();
		return;
	}
	overallProgressBar->setMaximum(listWidget_HDRs->count()*listWidget_TMopts->count());
	overallProgressBar->setValue(0);
	cancelbutton->setDisabled(true);
	BatchGoButton->setDisabled(true);
	out_folder_Button->setDisabled(true);
	add_dir_HDRs_Button->setDisabled(true);
	add_HDRs_Button->setDisabled(true);
	remove_HDRs_Button->setDisabled(true);
	add_dir_TMopts_Button->setDisabled(true);
	add_TMopts_Button->setDisabled(true);
	remove_TMOpts_Button->setDisabled(true);
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	BatchGoButton->setText(tr("Processing..."));
	qSort(tm_opt_list.begin(), tm_opt_list.end(), order_based_on_pregamma);
	//start the process: load the HDR.
	conditional_loadthread();
}

void BatchTMDialog::conditional_loadthread() {
	if (!HDRs_list.isEmpty()) {
		if (workingFrame != NULL) {
			std::cout << "Frame freed" << std::endl;
			pfs::DOMIO pfsio;
			pfsio.freeFrame(workingFrame);
			workingFrame = NULL;
		}
		qDebug("BATCH: conditional_loadthread: creating and starting load-hdr thread...");
		LoadHdrThread *loadthread = new LoadHdrThread(HDRs_list.takeFirst(), RecentDirHDRSetting);
		connect(loadthread, SIGNAL(finished()), loadthread, SLOT(deleteLater()));
		connect(loadthread, SIGNAL(load_failed(QString)), this, SLOT(load_HDR_failed(QString)));
		connect(loadthread, SIGNAL(hdr_ready(pfs::Frame*,QString)), this, SLOT(finished_loading_hdr(pfs::Frame*,QString)));
		loadthread->start();
	} else {
		if (workingFrame != NULL) {
			std::cout << "Frame freed" << std::endl;
			pfs::DOMIO pfsio;
			pfsio.freeFrame(workingFrame);
			workingFrame = NULL;
		}
		done=true;
		QApplication::restoreOverrideCursor();
		BatchGoButton->setText(tr("&Done"));
		BatchGoButton->setEnabled(true);
		qDebug("BATCH: conditional_loadthread: FINISHED ALL HDR");
	}
}

void BatchTMDialog::load_HDR_failed(QString error_message) {
	add_log_message(error_message);
	overallProgressBar->setValue(overallProgressBar->value()+listWidget_TMopts->count());
	conditional_loadthread();
}

//extern float pregamma;
void BatchTMDialog::finished_loading_hdr(pfs::Frame* loaded_hdr, QString filename) {
	pfs::DOMIO pfsio;
	add_log_message(tr("Starting to convert an HDR image: ")+filename);
	//pregamma=-1;
	workingFrame = loaded_hdr;
	QFileInfo qfi(filename);
	current_hdr_fname=out_folder_widgets->text() + "/" + qfi.completeBaseName();
	//now start processing the list of tone mapping settings
	conditional_TMthread();
}

void BatchTMDialog::conditional_TMthread() {
	int first_not_started=-1; 
	//look for the first that has not been started yet
	for (int i = 0; i < tm_opt_list.size(); i++) {
		if ((tm_opt_list.at(i)).second==false) {
			first_not_started=i;
			qDebug("BATCH: conditional_TMthread: found not started yet: %d",i);
			break;
		}
	}
	//we can end up in this function "conditional_TMthread" many times, called in a queued way by newResult(...). Here we bail out when TM_opt list and HDR_list are done.
	if (first_not_started==-1 && HDRs_list.isEmpty()) {
		qDebug("BATCH: conditional_TMthread: We are done, bailing out.");
		if (overallProgressBar->value()==overallProgressBar->maximum()) {
			BatchGoButton->setText(tr("&Done"));
			BatchGoButton->setEnabled(true);
			add_log_message(tr("All tasks completed."));
			QApplication::restoreOverrideCursor();
			done=true;
		}
		return;
	}

	//if the TM_opts list has still to get processed,
	if (first_not_started!=-1) {
		//while (running_threads < luminance_options->num_threads && first_not_started < tm_opt_list.size()) {
		while (running_threads < 1 && first_not_started < tm_opt_list.size()) {
			qDebug("BATCH: conditional_TMthread: creating TM_opts thread");
			tm_opt_list[first_not_started].second=true;

			//TonemappingOptions *opts = tm_opt_list.at(first_not_started).first; 
			opts = tm_opt_list.at(first_not_started).first; 
			opts->xsize = workingFrame->getWidth();
			opts->origxsize = opts->xsize;
			TMOThread *thread = TMOFactory::getTMOThread(opts->tmoperator, workingFrame, *opts);

			connect(thread, SIGNAL(imageComputed(const QImage&)), this, SLOT(newResult(const QImage&)));

			//start thread
			thread->start();

			first_not_started++;
			running_threads++;
			std::cout << "Started new thread: " << running_threads << std::endl;
		}
	} else {//if we are done processing the TM_opts list,
		//if there are still threads running
		if (running_threads > 0){
			qDebug("BATCH: conditional_TMthread: there are running_threads, waiting.");
			return;
		}
		qDebug("BATCH: conditional_TMthread: all TM_opts completed, resetting list to false and load (conditionally) a new hdr");
		add_log_message(tr("Finished tone mapping the current HDR."));
		//re-set all of them to false
		for (int j = 0; j < tm_opt_list.size(); j++) {
			tm_opt_list[j].second=false;
		}
		conditional_loadthread();
	}
}

void BatchTMDialog::newResult(const QImage& newimage, TonemappingOptions* opts) {
	qDebug("BATCH: newResult: Thread ended, it had pregamma=%g, save prefix is %s", opts->pregamma, current_hdr_fname.toAscii().constData());
	running_threads--;
	TMOptionsOperations operations(opts);
	QString postfix=operations.getPostfix();
	QString fname=current_hdr_fname+"_"+postfix+"."+luminance_options->batch_ldr_format;
	if (!newimage.save(fname, luminance_options->batch_ldr_format.toAscii().constData(), 100)) {
		add_log_message(tr("ERROR: Cannot save to file: ")+fname);
	} else {
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		ExifOperations::writeExifData(QFile::encodeName(fname).constData(),operations.getExifComment().toStdString());
		add_log_message(tr("Successfully saved LDR file: ")+fname);
	}
	overallProgressBar->setValue(overallProgressBar->value()+1);
	conditional_TMthread();
}

void BatchTMDialog::newResult(const QImage& newimage) {
	running_threads--;
	TMOptionsOperations operations(opts);
	QString postfix=operations.getPostfix();
	QString fname=current_hdr_fname+"_"+postfix+"."+luminance_options->batch_ldr_format;
	if (!newimage.save(fname, luminance_options->batch_ldr_format.toAscii().constData(), 100)) {
		add_log_message(tr("ERROR: Cannot save to file: ")+fname);
	} else {
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		ExifOperations::writeExifData(QFile::encodeName(fname).constData(),operations.getExifComment().toStdString());
		add_log_message(tr("Successfully saved LDR file: ")+fname);
	}
	overallProgressBar->setValue(overallProgressBar->value()+1);
	conditional_TMthread();
}

void BatchTMDialog::add_log_message(const QString& message) {
	qDebug(qPrintable(message));
	full_Log_Model->insertRows(full_Log_Model->rowCount(),1);
	full_Log_Model->setData(full_Log_Model->index(full_Log_Model->rowCount()-1), message, Qt::DisplayRole);
	Log_Widget->scrollToBottom();
}

void BatchTMDialog::filterChanged(const QString& newtext) {
	bool no_text=newtext.isEmpty();
	filterComboBox->setEnabled(no_text);
	filterLabel1->setEnabled(no_text);
	clearTextToolButton->setEnabled(!no_text);
	if (no_text)
		filterComboBoxActivated(filterComboBox->currentIndex());
	else
		log_filter->setFilterRegExp(QRegExp(newtext, Qt::CaseInsensitive, QRegExp::RegExp));
}

void BatchTMDialog::filterComboBoxActivated(int index) {
	QString regexp;
	switch (index) {
	case 0:
		regexp=".*";
		break;
	case 1:
		regexp="error";
		break;
	case 2:
		regexp="successful";
		break;
	}
	log_filter->setFilterRegExp(QRegExp(regexp, Qt::CaseInsensitive, QRegExp::RegExp));
}
