/**
 * This file is a part of Qtpfsgui package.
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QFileDialog>
#include <QSettings>
#include <QTextStream>
#include "batch_dialog_impl.h"
#include "../config.h"
#include "../Libpfs/pfs.h"
#include "../Threads/io_threads.h"
#include "../Threads/tonemapper_thread.h"


BatchTMDialog::BatchTMDialog(QWidget *p, qtpfsgui_opts *opts) : QDialog(p), start_left(-1), stop_left(-1), start_right(-1), stop_right(-1), running_threads(0), qtpfsgui_options(opts), done(false) {
	setupUi(this);
	assert(opts!=NULL);

	QSettings settings("Qtpfsgui", "Qtpfsgui");
	RecentDirHDRSetting=settings.value(KEY_RECENT_PATH_LOAD_SAVE_HDR, QDir::currentPath()).toString();
	RecentPathLoadSaveTmoSettings=settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString();
	recentPathSaveLDR=settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString();
	settings.beginGroup(GROUP_TONEMAPPING);
	desired_number_of_threads=settings.value(KEY_NUM_BATCH_THREADS,1).toInt();
	desired_format=settings.value(KEY_BATCH_LDR_FORMAT,"JPEG").toString();
	settings.endGroup();

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

	qDebug("BATCH: using %d threads",desired_number_of_threads);
	qDebug("BATCH: saving using fileformat: %s",desired_format.toAscii().constData());
	add_log_message(QString(tr("Using %1 thread(s)")).arg(desired_number_of_threads));
	add_log_message(tr("Saving using fileformat: ")+desired_format);
}

BatchTMDialog::~BatchTMDialog() {
	QFile::remove(qtpfsgui_options->tempfilespath+"/original.pfs");
	QFile::remove(qtpfsgui_options->tempfilespath+"/after_pregamma.pfs");
	QApplication::restoreOverrideCursor();
	while (!tm_opt_list.isEmpty())
		delete (tm_opt_list.takeFirst()).first;
}

void BatchTMDialog::add_dir_HDRs() {
	QString dirname=QFileDialog::getExistingDirectory(this, tr("Choose a directory"), RecentDirHDRSetting);
	if (!dirname.isEmpty()) {
		QStringList filters;
		filters << "*.exr" << "*.hdr" << "*.pic" << "*.tiff" << "*.tif" << "*.pfs" << "*.crw" << "*.cr2" << "*.nef" << "*.dng" << "*.mrw" << "*.orf" << "*.kdc" << "*.dcr" << "*.arw" << "*.raf" << "*.ptx" << "*.pef" << "*.x3f" << "*.raw";
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
	QString filetypes = tr("All Hdr formats ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw)" ;
	QStringList onlyhdrs=QFileDialog::getOpenFileNames(this, tr("Select the input images"), RecentDirHDRSetting, filetypes);
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
	QStringList onlytxts=QFileDialog::getOpenFileNames(this, tr("Load the tonemapping settings text files..."), RecentPathLoadSaveTmoSettings, tr("Qtpfsgui tonemapping settings text file (*.txt)"));
	add_view_model_TM_OPTs(onlytxts);
}

tonemapping_options* BatchTMDialog::parse_tm_opt_file(QString fname) {
	QFile file(fname);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size()==0) {
		add_log_message(tr("ERROR: cannot load Tone Mapping Setting file: ")+fname);
		return NULL;
	}

	tonemapping_options *toreturn=new tonemapping_options;
	//specifying -2 as size, and passing -2 as the original size in the thread constructor below, we basically bypass the resize step in the thread.
	//-1 cannot be used because the global variable xsize is -1 by default, so we would sooner or later end up loading the file resized.pfs (which never gets written to disk).
	toreturn->xsize=-2;

	QTextStream in(&file);
	QString field,value;

	while (!in.atEnd()) {
		QString line = in.readLine();
		//skip comments
		if (line.startsWith('#'))
			continue;

		field=line.section('=',0,0); //get the field
		value=line.section('=',1,1); //get the value
		if (field=="TMOSETTINGSVERSION") {
			if (value != TMOSETTINGSVERSION) {
				add_log_message(tr("ERROR: File too old, cannot parse Tone Mapping Setting file: ")+fname);
				delete toreturn;
				return NULL;
			}
		} else if (field=="TMO") {
			if (value=="Ashikhmin02") {
				toreturn->tmoperator=ashikhmin;
			} else if (value == "Drago03") {
				toreturn->tmoperator=drago;
			} else if (value == "Durand02") {
				toreturn->tmoperator=durand;
			} else if (value == "Fattal02") {
				toreturn->tmoperator=fattal;
			} else if (value == "Pattanaik00") {
				toreturn->tmoperator=pattanaik;
			} else if (value == "Reinhard02") {
				toreturn->tmoperator=reinhard02;
			} else if (value == "Reinhard04") {
				toreturn->tmoperator=reinhard04;
			} else if (value == "Mantiuk06") {
				toreturn->tmoperator=mantiuk;
			}
		} else if (field=="CONTRASTFACTOR") {
			toreturn->operator_options.mantiukoptions.contrastfactor=value.toFloat();
		} else if (field=="SATURATIONFACTOR") {
			toreturn->operator_options.mantiukoptions.saturationfactor=value.toFloat();
		} else if (field=="CONTRASTEQUALIZATION") {
			toreturn->operator_options.mantiukoptions.contrastequalization=(value == "YES");
		} else if (field=="SIMPLE") {
			toreturn->operator_options.ashikhminoptions.simple= (value == "YES") ? true : false;
		} else if (field=="EQUATION") {
			toreturn->operator_options.ashikhminoptions.eq2= (value=="2") ? true : false;
		} else if (field=="CONTRAST") {
			toreturn->operator_options.ashikhminoptions.lct=value.toFloat();
		} else if (field=="BIAS") {
			toreturn->operator_options.dragooptions.bias=value.toFloat();
		} else if (field=="SPATIAL") {
			toreturn->operator_options.durandoptions.spatial=value.toFloat();
		} else if (field=="RANGE") {
			toreturn->operator_options.durandoptions.range=value.toFloat();
		} else if (field=="BASE") {
			toreturn->operator_options.durandoptions.base=value.toFloat();
		} else if (field=="ALPHA") {
			toreturn->operator_options.fattaloptions.alpha=value.toFloat();
		} else if (field=="BETA") {
			toreturn->operator_options.fattaloptions.beta=value.toFloat();
		} else if (field=="COLOR") {
			toreturn->operator_options.fattaloptions.color=value.toFloat();
		} else if (field=="NOISE") {
			toreturn->operator_options.fattaloptions.noiseredux=value.toFloat();
		} else if (field=="OLDFATTAL") {
			toreturn->operator_options.fattaloptions.newfattal= (value == "NO");
		} else if (field=="MULTIPLIER") {
			toreturn->operator_options.pattanaikoptions.multiplier=value.toFloat();
		} else if (field=="LOCAL") {
			toreturn->operator_options.pattanaikoptions.local= (value=="YES");
		} else if (field=="AUTOLUMINANCE") {
			toreturn->operator_options.pattanaikoptions.autolum= (value=="YES");
		} else if (field=="CONE") {
			toreturn->operator_options.pattanaikoptions.cone=value.toFloat();
		} else if (field=="ROD") {
			toreturn->operator_options.pattanaikoptions.rod=value.toFloat();
		} else if (field=="KEY") {
			toreturn->operator_options.reinhard02options.key=value.toFloat();
		} else if (field=="PHI") {
			toreturn->operator_options.reinhard02options.phi=value.toFloat();
		} else if (field=="SCALES") {
			toreturn->operator_options.reinhard02options.scales= (value=="YES") ? true : false;
		} else if (field=="RANGE") {
			toreturn->operator_options.reinhard02options.range=value.toInt();
		} else if (field=="LOWER") {
			toreturn->operator_options.reinhard02options.lower=value.toInt();
		} else if (field=="UPPER") {
			toreturn->operator_options.reinhard02options.upper=value.toInt();
		} else if (field=="BRIGHTNESS") {
			toreturn->operator_options.reinhard04options.brightness=value.toFloat();
		} else if (field=="CHROMATICADAPTATION") {
			toreturn->operator_options.reinhard04options.chromaticAdaptation=value.toFloat();
		} else if (field=="LIGHTADAPTATION") {
			toreturn->operator_options.reinhard04options.lightAdaptation=value.toFloat();
		} else if (field=="PREGAMMA") {
			toreturn->pregamma=value.toFloat();
		} else {
			add_log_message(tr("ERROR: cannot parse Tone Mapping Setting file: ")+fname);
			delete toreturn;
			return NULL;
		}
	}
	return toreturn;
}

void BatchTMDialog::check_enable_start() {
	//at least 1 hdr AND at least 1 tm_opt AND output_dir not empty
	BatchGoButton->setEnabled(!out_folder_widgets->text().isEmpty() && listWidget_HDRs->count()!=0 && listWidget_TMopts->count()!=0);
}

void BatchTMDialog::out_folder_clicked() {
	QString dirname=QFileDialog::getExistingDirectory(this, tr("Choose a directory"), recentPathSaveLDR);
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
		tonemapping_options *i_th_tm_opt=parse_tm_opt_file(*it);
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
// 		qDebug("L %d-%d",start_left,stop_left);
	} else {
		start_right=listWidget_TMopts->count();
		stop_right=-1;
		for (int i=0; i<listWidget_TMopts->count(); i++) {
			if (listWidget_TMopts->isItemSelected(listWidget_TMopts->item(i))) {
				start_right= (start_right>i) ? i : start_right;
				stop_right= (stop_right<i) ? i : stop_right;
			}
		}
// 		qDebug("R %d-%d",start_right,stop_right);
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
}

//function used for ordering based on the value of pregamma
//somehow I was not able to embed this in the BatchTMDialog class :)
bool order_based_on_pregamma(const QPair<tonemapping_options*,bool> &s1, const QPair<tonemapping_options*,bool> &s2) {
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
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	BatchGoButton->setText(tr("Processing..."));
	qSort(tm_opt_list.begin(), tm_opt_list.end(), order_based_on_pregamma);
	//start the process: load the HDR.
	conditional_loadthread();
}

void BatchTMDialog::conditional_loadthread() {
	if (!HDRs_list.isEmpty()) {
		qDebug("BATCH: conditional_loadthread: creating and starting load-hdr thread...");
		LoadHdrThread *loadthread = new LoadHdrThread(HDRs_list.takeFirst(), RecentDirHDRSetting, qtpfsgui_options);
		connect(loadthread, SIGNAL(finished()), loadthread, SLOT(deleteLater()));
		connect(loadthread, SIGNAL(load_failed(QString)), this, SLOT(load_HDR_failed(QString)));
		connect(loadthread, SIGNAL(hdr_ready(pfs::Frame*,QString)), this, SLOT(finished_loading_hdr(pfs::Frame*,QString)));
		loadthread->start();
	} else {
		done=true;
		QApplication::restoreOverrideCursor();
		BatchGoButton->setText(tr("&Done"));
		BatchGoButton->setEnabled(true);
		qDebug("BATCH: conditional_loadthread: FINISHED ALL HDR");
	}
}

void BatchTMDialog::load_HDR_failed(QString fname) {
	add_log_message(tr("ERROR: Failed loading HDR file: ")+fname);
	qDebug("BATCH: Failed loading HDR file: %s", fname.toAscii().constData());
	overallProgressBar->setValue(overallProgressBar->value()+listWidget_TMopts->count());
	conditional_loadthread();
}

extern float pregamma;
void BatchTMDialog::finished_loading_hdr(pfs::Frame* loaded_hdr, QString filename) {
	pfs::DOMIO pfsio;
	qDebug("BATCH: LOADED HDR, now swapping it to ./original.pfs");
	add_log_message(tr("Starting to tone map HDR file: ")+filename);
	pfsio.writeFrame(loaded_hdr, qtpfsgui_options->tempfilespath+"/original.pfs");
	pregamma=-1;
	QFile::remove(qtpfsgui_options->tempfilespath+"/after_pregamma.pfs");
	pfsio.freeFrame(loaded_hdr);
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
			BatchGoButton->setText(tr("Done"));
			BatchGoButton->setEnabled(true);
			add_log_message(tr("All tasks completed."));
			QApplication::restoreOverrideCursor();
			done=true;
		}
		return;
	}

	//if the TM_opts list has still to get processed,
	if (first_not_started!=-1) {
		while (running_threads < desired_number_of_threads && first_not_started < tm_opt_list.size()) {
			qDebug("BATCH: conditional_TMthread: creating TM_opts thread");
			tm_opt_list[first_not_started].second=true;
			TonemapperThread *thread = new TonemapperThread(-2, qtpfsgui_options->tempfilespath, NULL);

			connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
			qRegisterMetaType<QImage>("QImage");
			connect(thread, SIGNAL(ImageComputed(const QImage&,tonemapping_options*)), this, SLOT(newResult(const QImage&,tonemapping_options*)));
			//start thread
			thread->ComputeImage(*(tm_opt_list.at(first_not_started).first));

			first_not_started++;
			running_threads++;
		}
	} else {//if we are done processing the TM_opts list,
		//if there are still threads running
		if (running_threads > 0){
			qDebug("BATCH: conditional_TMthread: there are running_threads, waiting.");
			return;
		}
		qDebug("BATCH: conditional_TMthread: all TM_opts completed, resetting list to false and load (conditionally) a new hdr");
		add_log_message(tr("Done tone mapping the current HDR."));
		//re-set all of them to false
		for (int j = 0; j < tm_opt_list.size(); j++) {
			tm_opt_list[j].second=false;
		}
		conditional_loadthread();
	}
	
}

void BatchTMDialog::newResult(const QImage& newimage,tonemapping_options* opts) {
	qDebug("BATCH: newResult: Thread ended, it had pregamma=%g, save prefix is %s", opts->pregamma, current_hdr_fname.toAscii().constData());
	running_threads--;
	QString postfix=QString("_pregamma_%1_").arg(opts->pregamma);
	switch (opts->tmoperator) {
	case mantiuk: {
		postfix+="mantiuk_";
		float contrastfactor=opts->operator_options.mantiukoptions.contrastfactor;
		float saturationfactor=opts->operator_options.mantiukoptions.saturationfactor;
		bool contrast_eq=opts->operator_options.mantiukoptions.contrastequalization;
		if (contrast_eq) {
			postfix+="contrast_equalization_";
		} else {
			postfix+=QString("contrast_mapping_%1_").arg(contrastfactor);
		}
		postfix+=QString("saturation_factor_%1").arg(saturationfactor);
		}
		break;
	case fattal: {
		if (!opts->operator_options.fattaloptions.newfattal)
			postfix+="v1_";
		postfix+="fattal_";
		float alpha=opts->operator_options.fattaloptions.alpha;
		float beta=opts->operator_options.fattaloptions.beta;
		float saturation2=opts->operator_options.fattaloptions.color;
		float noiseredux=opts->operator_options.fattaloptions.noiseredux;
		postfix+=QString("alpha_%1_").arg(alpha);
		postfix+=QString("beta_%1_").arg(beta);
		postfix+=QString("saturation_%1_").arg(saturation2);
		postfix+=QString("noiseredux_%1").arg(noiseredux);
		}
		break;
	case ashikhmin: {
		postfix+="ashikhmin_";
		if (opts->operator_options.ashikhminoptions.simple) {
			postfix+="-simple";
		} else {
			if (opts->operator_options.ashikhminoptions.eq2) {
				postfix+="-eq2_";
			} else {
				postfix+="-eq4_";
			}
			postfix+=QString("local_%1").arg(opts->operator_options.ashikhminoptions.lct);
		}
		}
		break;
	case drago: {
		postfix+="drago_";
		postfix+=QString("bias_%1").arg(opts->operator_options.dragooptions.bias);
		}
		break;
	case durand: {
		float spatial=opts->operator_options.durandoptions.spatial;
		float range=opts->operator_options.durandoptions.range;
		float base=opts->operator_options.durandoptions.base;
		postfix+="durand_";
		postfix+=QString("spatial_%1_").arg(spatial);
		postfix+=QString("range_%1_").arg(range);
		postfix+=QString("base_%1").arg(base);
		}
		break;
	case pattanaik: {
		float multiplier=opts->operator_options.pattanaikoptions.multiplier;
		float cone=opts->operator_options.pattanaikoptions.cone;
		float rod=opts->operator_options.pattanaikoptions.rod;
		postfix+="pattanaik00_";
		postfix+=QString("mul_%1_").arg(multiplier);
		if (opts->operator_options.pattanaikoptions.local) {
			postfix+="local";
		} else if (opts->operator_options.pattanaikoptions.autolum) {
			postfix+="autolum";
		} else {
			postfix+=QString("cone_%1_").arg(cone);
			postfix+=QString("rod_%1_").arg(rod);
		}
		}
		break;
	case reinhard02: {
		float key=opts->operator_options.reinhard02options.key;
		float phi=opts->operator_options.reinhard02options.phi;
		int range=opts->operator_options.reinhard02options.range;
		int lower=opts->operator_options.reinhard02options.lower;
		int upper=opts->operator_options.reinhard02options.upper;
		postfix+="reinhard02_";
		postfix+=QString("key_%1_").arg(key);
		postfix+=QString("phi_%1").arg(phi);
		if (opts->operator_options.reinhard02options.scales) {
			postfix+=QString("_scales_");
			postfix+=QString("range_%1_").arg(range);
			postfix+=QString("lower%1_").arg(lower);
			postfix+=QString("upper%1").arg(upper);
		}
		}
		break;
	case reinhard04: {
		float brightness=opts->operator_options.reinhard04options.brightness;
		float chromaticAdaptation= opts->operator_options.reinhard04options.chromaticAdaptation;
		float lightAdaptation=opts->operator_options.reinhard04options.lightAdaptation;
		postfix+="reinhard04_";
		postfix+=QString("brightness_%1_").arg(brightness);
		postfix+=QString("chromatic_adaptation_%1_").arg(chromaticAdaptation);
		postfix+=QString("light_adaptation_%1").arg(lightAdaptation);
		}
		break;
	}
	if (!newimage.save(current_hdr_fname+postfix+"."+desired_format, desired_format.toAscii().constData(), 100)) {
		qDebug("BATCH: newResult: Cannot save to %s",(current_hdr_fname+postfix+"."+desired_format).toAscii().constData());
		add_log_message(tr("ERROR: Cannot save to file: ")+current_hdr_fname+postfix+"."+desired_format);
	} else {
		add_log_message(tr("Successfully saved LDR file: ")+current_hdr_fname+postfix+"."+desired_format);
	}
	overallProgressBar->setValue(overallProgressBar->value()+1);
	conditional_TMthread();
}

void BatchTMDialog::add_log_message(const QString& message) {
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
