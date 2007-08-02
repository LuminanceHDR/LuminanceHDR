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

#include <QStringList>
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QMessageBox>
#include <QTextStream>
#if defined(__FreeBSD__) && __FreeBSD__ < 6
extern "C" {
#include "../arch/freebsd/s_exp2f.c"
}
#endif

#include "hdrwizardform_impl.h"
#include "../Fileformat/pfstiff.h"
#include "../Exif/exif_operations.h"
#include "../Fileformat/pfsindcraw.h"
#include "../config.h"

const config_triple predef_confs[6]= {
{TRIANGULAR, GAMMA, DEBEVEC,""},
{TRIANGULAR, LINEAR,DEBEVEC,""},
{PLATEAU, LINEAR,DEBEVEC,""},
{PLATEAU, GAMMA,DEBEVEC,""},
{GAUSSIAN, LINEAR,DEBEVEC,""},
{GAUSSIAN, GAMMA,DEBEVEC,""},
};


HdrWizardForm::HdrWizardForm(QWidget *p, qtpfsgui_opts *options) : QDialog(p), curvefilename(""), expotimes(NULL), ldr_tiff(false), opts(&(options->dcraw_options)) {
	setupUi(this);
	connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextpressed()));
	connect(backbutton,SIGNAL(clicked()),this,SLOT(backpressed()));
	connect(pagestack,SIGNAL(currentChanged(int)),this,SLOT(currentPageChangedInto(int)));

	connect(checkbox_load_from_file,SIGNAL(toggled(bool)),this,SLOT(update_current_config_file_or_notfile(bool)));
	connect(comboBox_model,SIGNAL(activated(int)), this,SLOT(update_current_config_model(int)));
	connect(comboBox_gamma_lin_log,SIGNAL(activated(int)), this,SLOT(update_current_config_gamma_lin_log(int)));
	connect(comboBox_tri_gau_plateau,SIGNAL(activated(int)), this,SLOT(update_current_config_weights(int)));
	connect(comboBox_PredefConfigs,SIGNAL(activated(int)), this,SLOT(update_currentconfig(int)));
	connect(loadFileButton,SIGNAL(clicked()),this,SLOT(load_response_curve_from_file()));
	connect(loadsetButton,SIGNAL(clicked()),this,SLOT(loadfiles()));
	connect(radioButton_use_Predefined,SIGNAL(clicked()),this,SLOT(fix_gui_custom_config()));
	connect(checkBoxCallCustom,SIGNAL(toggled(bool)),this,SLOT(custom_toggled(bool)));
	connect(coboxRespCurve_Antighost,SIGNAL(activated(int)),this,SLOT(update_current_antighost_curve(int)));
	connect(radio_usecalibrate_02,SIGNAL(toggled(bool)),this,SLOT(update_current_config_calibrate()));
	connect(ShowFileloadedLineEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setLoadFilename(const QString&)));

	connect(EVcomboBox, SIGNAL(activated(int)), this, SLOT(EVcomboBoxactivated(int)));
	connect(EVcomboBox, SIGNAL(highlighted(int)), this, SLOT(highlighted(int)));

	connect(listShowFiles, SIGNAL(currentRowChanged(int)), this, SLOT(fileselected(int)));
	weights_in_gui[0]=TRIANGULAR;
	weights_in_gui[1]=GAUSSIAN;
	weights_in_gui[2]=PLATEAU;
	responses_in_gui[0]=GAMMA;
	responses_in_gui[1]=LINEAR;
	responses_in_gui[2]=LOG10;
	responses_in_gui[3]=FROM_ROBERTSON;
	models_in_gui[0]=ROBERTSON;
	models_in_gui[1]=DEBEVEC;

	QSettings settings("Qtpfsgui", "Qtpfsgui");
	RecentDirInputLDRs=settings.value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR,QDir::currentPath()).toString();
	chosen_config=predef_confs[0];
	fillEVcombobox();
	need_to_transform_indices=false;
	enable_usability_jump_hack=true;
}

void HdrWizardForm::highlighted(int) {
	if (enable_usability_jump_hack)
		(EVcomboBox->view())->scrollTo((EVcomboBox->model())->index(29,0));
	enable_usability_jump_hack=false;
}

void HdrWizardForm::loadfiles() {
    QString filetypes;
    filetypes += tr("All formats (*.jpeg *.jpg *.tiff *.tif *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw);;");
    filetypes += tr("JPEG (*.jpeg *.jpg);;");
    filetypes += tr("TIFF Images (*.tiff *.tif);;");
    filetypes += tr("RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw)");
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select the input images"), RecentDirInputLDRs, filetypes );
    if (!files.isEmpty() ) {
	QFileInfo qfi(files.at(0));
	QSettings settings("Qtpfsgui", "Qtpfsgui");
	// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
	if (RecentDirInputLDRs != qfi.path()) {
		// update internal field variable
		RecentDirInputLDRs=qfi.path();
		settings.setValue(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, RecentDirInputLDRs);
	}

	numberinputfiles=files.count();
	progressBar->setMaximum(numberinputfiles);
	progressBar->setValue(0);
	expotimes=new float[numberinputfiles];
	int index_expotimes=0;
	QStringList::Iterator it = files.begin();
	while( it != files.end() ) {
		QFileInfo qfi(*it);
		expotimes[index_expotimes]=ExifOperations::obtain_expotime(qfi.filePath().toStdString()); //fill array of exposure times, -1 is error
		listShowFiles->addItem(qfi.fileName()); //fill graphical list
		QString extension=qfi.suffix().toUpper(); //get filename extension
		//now go and fill the list of image data (real payload)
		if (extension.startsWith("JP")) { //check for extension: if JPEG:
			ImagePtrList.append( new QImage(qfi.filePath()) ); // fill with image data
		} else if(extension.startsWith("TIF")) { //if tiff
			TiffReader reader(qfi.filePath().toUtf8().constData());
			if (reader.is8bitTiff()) { //if 8bit (tiff) treat as ldr
				ImagePtrList.append( reader.readIntoQImage() );
				ldr_tiff=true;
			} else if (reader.is16bitTiff()) { //if 16bit (tiff) treat as hdr
				pfs::Frame *framepointer=reader.readIntoPfsFrame();
				pfs::Channel *R, *G, *B;
				framepointer->getRGBChannels( R, G, B );
				listhdrR.push_back(R);
				listhdrG.push_back(G);
				listhdrB.push_back(B);
			} else {
				listShowFiles->clear();
				clearlists();
				delete expotimes;
				QMessageBox::critical(this,tr("Tiff error"), QString(tr("the file<br>%1<br> is not a 8 bit or 16 bit tiff")).arg(qfi.fileName()));
				return;
			}
		} else { //not a jpeg of tiff_LDR file, so it's raw input (hdr)
			pfs::Frame *framepointer=readRAWfile(qfi.filePath().toUtf8().constData(), opts);
			pfs::Channel *R, *G, *B;
			framepointer->getRGBChannels( R, G, B );
			listhdrR.push_back(R);
			listhdrG.push_back(G);
			listhdrB.push_back(B);
		}
		++it;
		index_expotimes++;
		progressBar->setValue(progressBar->value()+1); // increment progressbar
	}

// 	check if at least one image doesn't contain (the needed) exif tags
	bool all_ok=true;
	QStringList files_lacking_exif;
	for (int i=0; i<numberinputfiles; i++) {
		if (expotimes[i]==-1) {
			files_lacking_exif+="<li><font color=\"#FF9500\"><i><b>"+ listShowFiles->item(i)->text()+ "</b></i></font></li>\n";
			all_ok=false;
		}
	}

	if (all_ok) {
		Next_Finishbutton->setEnabled(TRUE);
		confirmloadlabel->setText(tr("<center><font color=\"#008400\"><h3><b>Done!</b></h3></font></center>"));
	} else {
//if we didn't find the exif data for at least one image, we need to set all the values to -1, so that the user has to fill *all* the values by hand.
		for (int j=0;j<numberinputfiles;j++) {
			expotimes[j]=-1;
		}
		EVcomboBox->setEnabled(TRUE);
		label_EV->setEnabled(TRUE);
		QString warning_message=(QString(tr("<font color=\"#FF0000\"><h3><b>WARNING:</b></h3></font>\
Qtpfsgui was not able to find the relevant <i>EXIF</i> tags\nfor the following images:\n <ul>\
%1</ul>\
<hr>You can still proceed creating an Hdr. To do so you have to insert <b>manually</b> the EV (exposure values) or stop difference values for each one of your images.\
<hr>If you want Qtfsgui to do this <b>automatically</b>, you have to load images that have at least\nthe following exif data: \
<ul><li>Shutter Speed (seconds)</li>\
<li>Aperture (f-number)</li></ul>\
<hr><b>HINT:</b> Losing EXIF data usually happens when you preprocess your pictures.<br>\
You can perform a <b>one-to-one copy of the exif data</b> between two sets of images via the <i><b>\"Tools->Copy Exif Data...\"</b></i> menu item."))).arg(files_lacking_exif.join(""));
		QMessageBox::warning(this,tr("EXIF data not found"),warning_message);
		confirmloadlabel->setText(QString(tr("<center><font color=\"#FF9500\"><h3><b>To proceed you need to manually set the exposure values.<br>%1 values still required.</b></h3></font></center>")).arg(numberinputfiles));
	}
	loadsetButton->setEnabled(FALSE);
    }
}

void HdrWizardForm::custom_toggled(bool checked) {
	if (!checked) {
		if (!checkBoxAntighosting->isChecked()) {
			label_RespCurve_Antighost->setDisabled(TRUE);
			coboxRespCurve_Antighost->setDisabled(TRUE);
			label_Iterations->setDisabled(TRUE);
			spinBoxIterations->setDisabled(TRUE);
		}
		else {
			label_predef_configs->setDisabled(TRUE);
			comboBox_PredefConfigs->setDisabled(TRUE);
			label_weights->setDisabled(TRUE);
			lineEdit_showWeight->setDisabled(TRUE);
			label_resp->setDisabled(TRUE);
			lineEdit_show_resp->setDisabled(TRUE);
			label_model->setDisabled(TRUE);
			lineEdit_showmodel->setDisabled(TRUE);
		}
	update_currentconfig(comboBox_PredefConfigs->currentIndex());
	}
}

void HdrWizardForm::fix_gui_custom_config() {
    //ENABLE load_curve_button and lineedit when "load from file" is checked.
    if (!checkbox_load_from_file->isChecked()) {
	//qDebug("not checked, disabling Load Button");
	loadFileButton->setEnabled(false);
	ShowFileloadedLineEdit->setEnabled(false);
    }
    update_current_config_file_or_notfile(checkbox_load_from_file->isChecked());
}

void HdrWizardForm::update_current_config_file_or_notfile( bool checkedfile ) {
    //if checkbox is checked AND we have a valid filename
    if (checkedfile && curvefilename != "") {
	//update chosen config
	chosen_config.response_curve=FROM_FILE;
	chosen_config.CurveFilename=curvefilename;
	//and ENABLE nextbutton
	Next_Finishbutton->setEnabled(true);
    }
    //if checkbox is checked AND no valid filename
    else  if (checkedfile && curvefilename == "") {
	// DISABLE nextbutton until situation is fixed
	Next_Finishbutton->setEnabled(false);
    }
    //checkbox not checked
    else {
	// update chosen config
	chosen_config.response_curve=responses_in_gui[comboBox_gamma_lin_log->currentIndex()];
	chosen_config.CurveFilename="";
	//and ENABLE nextbutton
	Next_Finishbutton->setEnabled(true);
    }
}

void HdrWizardForm::currentPageChangedInto(int newindex) {
/*	if      (newindex==0) { //initial page
		//can't end up here, I guess.
	}
	else*/ if (newindex==1) { //predefined configs page
		backbutton->setEnabled(FALSE);
	}
	else if (newindex==2) { //custom config
		update_currentconfig(0);
		backbutton->setEnabled(TRUE);
	}
	else if (newindex==3) { //ending page
		Next_Finishbutton->setText(tr("&Finish"));
		Next_Finishbutton->setEnabled(FALSE);
		backbutton->setEnabled(FALSE);
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		//CREATE THE HDR
		if (ImagePtrList.size() != 0)
			PfsFrameHDR=createHDR(expotimes,&chosen_config,checkBoxAntighosting->isChecked(),spinBoxIterations->value(),true,&ImagePtrList);
		else
			PfsFrameHDR=createHDR(expotimes,&chosen_config,checkBoxAntighosting->isChecked(),spinBoxIterations->value(),false,&listhdrR,&listhdrG,&listhdrB);
		QApplication::restoreOverrideCursor();
		Next_Finishbutton->setEnabled(TRUE);
		Next_Finishbutton->setFocus();
		return;
	}
	Next_Finishbutton->setText(tr("&Next >"));
}

void HdrWizardForm::update_current_antighost_curve(int fromgui) {
	update_current_config_gamma_lin_log(fromgui);
}

void HdrWizardForm::nextpressed() {
	switch (pagestack->currentIndex()) {
	case 0:
		if (need_to_transform_indices)
			transform_indices_into_values();
// 		adjustSize();
// 		showNormal();
		break;
	case 1:
		if(!checkBoxCallCustom->isChecked()) {
			pagestack->setCurrentIndex(3);
			return;
		}
		break;
	case 2:
		break;
	case 3:	
		accept();
		return;
	}
	pagestack->setCurrentIndex(pagestack->currentIndex()+1);
}

void HdrWizardForm::backpressed() {
	if (pagestack->currentIndex()==3 && !checkBoxCallCustom->isChecked())
		pagestack->setCurrentIndex(1);
	else
		pagestack->setCurrentIndex(pagestack->currentIndex()-1);
}

void HdrWizardForm::load_response_curve_from_file() {
	curvefilename = QFileDialog::getOpenFileName(
			this,
			tr("Load a camera response curve file"),
			QDir::currentPath(),
			tr("Camera response curve (*.m);;All Files (*)") );
	if (curvefilename !="")  {
		ShowFileloadedLineEdit->setText(curvefilename);
		update_current_config_file_or_notfile(checkbox_load_from_file->isChecked());
	}
}

void HdrWizardForm::update_currentconfig( int index_from_gui ) {
// 	qDebug("updating config to %d", index_from_gui);
	chosen_config=predef_confs[index_from_gui];
	lineEdit_showWeight->setText(getQStringFromConfig(1));
	lineEdit_show_resp->setText(getQStringFromConfig(2));
	lineEdit_showmodel->setText(getQStringFromConfig(3));
}

void HdrWizardForm::update_current_config_weights(int from_gui) {
	chosen_config.weights=weights_in_gui[from_gui];
}

void HdrWizardForm::update_current_config_gamma_lin_log(int from_gui) {
	chosen_config.response_curve=responses_in_gui[from_gui];
}

void HdrWizardForm::update_current_config_model(int from_gui) {
	chosen_config.model=models_in_gui[from_gui];
}

void HdrWizardForm::update_current_config_calibrate() {
	chosen_config.response_curve=FROM_ROBERTSON;;
}

void HdrWizardForm::setLoadFilename( const QString & filename_from_gui) {
	if (filename_from_gui!="") {
		chosen_config.response_curve=FROM_FILE;
		chosen_config.CurveFilename=filename_from_gui;
	}
}

pfs::Frame* HdrWizardForm::getPfsFrameHDR() {
	return PfsFrameHDR;
}

QString HdrWizardForm::getCaptionTEXT() {
	return tr("(*) Weights: ")+getQStringFromConfig(1) + tr(" - Response curve: ") + getQStringFromConfig(2) + tr(" - Model: ") + getQStringFromConfig(3);
}

QString HdrWizardForm::getQStringFromConfig( int type ) {
    if (type==1) { //return String for weights
	switch (chosen_config.weights) {
	case TRIANGULAR:
	    return tr("Triangular");
	case PLATEAU:
	    return tr("Plateau");
	case GAUSSIAN:
	    return tr("Gaussian");
	}
    } else if (type==2) {   //return String for response curve
	switch (chosen_config.response_curve) {
	case LINEAR:
	    return tr("Linear");
	case GAMMA:
	    return tr("Gamma");
	case LOG10:
	    return tr("Logarithmic");
	case FROM_ROBERTSON:
	    return tr("From Calibration");
	case FROM_FILE:
	    return tr("From File");
	}
    } else if (type==3) {   //return String for model
	switch (chosen_config.model) {
	case DEBEVEC:
	    return tr("Debevec");
	case ROBERTSON:
	    return tr("Robertson");
	}
    } else return "";
return "";
}

void HdrWizardForm::fillEVcombobox() {
	for (int i=-6*4;i<=6*4;i++){
		float ev_value=-100;
		QString stop_string="";
		switch (abs(i%4)) {
			case 0:
			ev_value=0;
			stop_string=((i>0)? "+":"") + QString("%1").arg((i/4));
			break;
			case 1:
			ev_value= (i>0)? +0.3 : -0.3;
			stop_string=((i>0)? "+":"") + QString("%1/3").arg(( (i/4)*3 + ((i>0)? +1:-1) ));
			break;
			case 2:
			ev_value= (i>0)? +0.5 : -0.5;
			stop_string=((i>0)? "+":"") + QString("%1/2").arg(( (i/4)*2 + ((i>0)? +1:-1) ));
			break;
			case 3:
			ev_value= (i>0)? +0.7 : -0.7;
			stop_string=((i>0)? "+":"") + QString("%1/3").arg(( (i/4)*3 + ((i>0)? +2:-2) ));
			break;
		}
		ev_value+=i/4;
		QString result;
		QTextStream ts(&result);
		ts << "EV "<< qSetFieldWidth(4) << left << forcesign << ev_value << " || stops " << qSetFieldWidth(4) << forcesign << left << stop_string;
		EVcomboBox->addItem(result);
	}
	EVcomboBox->setCurrentIndex(-1);
}

void HdrWizardForm::transform_indices_into_values() {
	qDebug("HdrWizardForm:: transforming indices into values");
//for precise values I cannot parse back what I put in the combobox, I have to recompute the values. In other words 4/3 != 1.3
	for (int i=0; i<numberinputfiles; i++) {
		int ni=(int)expotimes[i]-(6*4);
		float v=-100;
		switch (abs(ni%4)) {
			case 0:
				v=ni/4;
				break;
			case 1:
				v=float((ni/4)*3 + ((ni>0)? +1:-1)) / 3.0f;
				break;
			case 2:
				v=float((ni/4)*2 + ((ni>0)? +1:-1)) / 2.0f;
				break;
			case 3:
				v=float((ni/4)*3 + ((ni>0)? +2:-2)) / 3.0f;
				break;
		}
		assert(v!=-100);
		expotimes[i]=exp2f(v);
	}
}

void HdrWizardForm::EVcomboBoxactivated(int i) {
	enable_usability_jump_hack=true;
	assert(listShowFiles->count()==numberinputfiles);
	//for the time being expotimes contains the indices into the combobox (from 0 to 48)
	expotimes[listShowFiles->currentRow()]=i;
// 	qDebug("setting expotimes[%d]=%d",listShowFiles->currentRow(),i);
	bool all_ok=true;
	int files_unspecified=0;
	for (int i=0; i<numberinputfiles; i++) {
		if (expotimes[i]==-1) {
			all_ok=false;
			files_unspecified++;
		}
	}
	if (all_ok) {
		Next_Finishbutton->setEnabled(TRUE);
		confirmloadlabel->setText(tr("<center><font color=\"#008400\"><h3><b>All values have been set!</b></h3></font></center>"));
		need_to_transform_indices=true;
	} else {
		confirmloadlabel->setText(QString(tr("<center><font color=\"#FF9500\"><h3><b>To proceed you need to manually set the exposure values.<br>%1 values still required.</b></h3></font></center>")).arg(files_unspecified));
	}
}

void HdrWizardForm::fileselected(int i) {
	assert(listShowFiles->count()==numberinputfiles);
	//right now expotimes contains the indices into the combobox (from 0 to 48)
	EVcomboBox->setCurrentIndex((int)expotimes[i]); //works with -1 (value not set) as well.
}

HdrWizardForm::~HdrWizardForm() {
	// here PfsFrameHDR is not free-ed because we want to get it outside of this class via the getPfsFrameHDR() method.
	if (expotimes) delete [] expotimes;
	clearlists();
}

void HdrWizardForm::clearlists() {
	if (ImagePtrList.size() != 0) {
// 		qDebug("cleaning LDR exposures list");
		if (ldr_tiff) {
			foreach(QImage *p,ImagePtrList) {
// 				qDebug("while cleaning ldr tiffs, %ld",p->bits());
				delete [] p->bits();
// 				qDebug("cleaned");
			}
		}
		qDeleteAll(ImagePtrList);
// 		qDebug("cleaned ImagePtrList");
		ImagePtrList.clear();
	}
	if (listhdrR.size()!=0 && listhdrG.size()!=0 && listhdrB.size()!=0) {
// 		qDebug("cleaning HDR exposures list");
		Array2DList::iterator itR=listhdrR.begin(), itG=listhdrG.begin(), itB=listhdrB.begin();
		for (; itR!=listhdrR.end(); itR++,itG++,itB++ ){
			delete *itR; delete *itG; delete *itB;
		}
		listhdrR.clear(); listhdrG.clear(); listhdrB.clear();
	}
}
