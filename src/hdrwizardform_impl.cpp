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
#include <cmath>
#include "hdrwizardform_impl.h"
#include "fileformat/pfsindcraw.h"
#include "fileformat/pfstiff.h"

config_triple predef_confs[6]= {
{TRIANGULAR, GAMMA, DEBEVEC,""},
{TRIANGULAR, LINEAR,DEBEVEC,""},
{PLATEAU, LINEAR,DEBEVEC,""},
{PLATEAU, GAMMA,DEBEVEC,""},
{GAUSSIAN, LINEAR,DEBEVEC,""},
{GAUSSIAN, GAMMA,DEBEVEC,""},
};


HdrWizardForm::HdrWizardForm(QWidget *p, dcraw_opts *options) : QDialog(p), curvefilename(""), expotimes(NULL), expotime(NULL), expotime2(NULL), iso(NULL), fnum(NULL), fnum2(NULL), input_is_ldr(true), ldr_tiff(false), opts(options) {
	setupUi(this);
	connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextpressed()));
	connect(backbutton,SIGNAL(clicked()),this,SLOT(backpressed()));
	connect(pagestack,SIGNAL(currentChanged(int)),this,SLOT(currentPageChangedInto(int)));

	connect(checkbox_load_from_file,SIGNAL(toggled(bool)),this,SLOT(update_current_config_file_or_notfile(bool)));
	connect(comboBox_model,SIGNAL(activated(const QString&)),this,SLOT(update_current_config_model(const QString&)));
	connect(comboBox_gamma_lin_log,SIGNAL(activated(const QString&)),this,SLOT(update_current_config_gamma_lin_log(const QString&)));
	connect(comboBox_tri_gau_plateau,SIGNAL(activated(const QString&)),this,SLOT(update_current_config_weights(const QString&)));
	connect(comboBox_PredefConfigs,SIGNAL(activated(int)),this,SLOT(update_currentconfig(int)));
	connect(loadFileButton,SIGNAL(clicked()),this,SLOT(load_response_curve_from_file()));
	connect(loadsetButton,SIGNAL(clicked()),this,SLOT(loadfiles()));
	connect(radioButton_use_Predefined,SIGNAL(clicked()),this,SLOT(fix_gui_custom_config()));
	connect(checkBoxCallCustom,SIGNAL(toggled(bool)),this,SLOT(fix_gui_uncheck_custom(bool)));
	connect(coboxRespCurve_Antighost,SIGNAL(activated(int)),this,SLOT(update_current_antighost_curve(int)));
	connect(radio_usecalibrate_02,SIGNAL(toggled(bool)),this,SLOT(update_current_config_calibrate()));
	connect(ShowFileloadedLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(setLoadFilename(const QString&)));
	fromQStringToWeight["Triangular"]=TRIANGULAR;
	fromQStringToWeight["Gaussian"]=GAUSSIAN;
	fromQStringToWeight["Plateau"]=PLATEAU;
	fromQStringToResponse["Gamma"]=GAMMA;
	fromQStringToResponse["Linear"]=LINEAR;
	fromQStringToResponse["Log"]=LOG10;
	fromQStringToResponse["Calibration"]=FROM_ROBERTSON;
	fromQStringToModel["Robertson"]=ROBERTSON;
	fromQStringToModel["Debevec"]=DEBEVEC;

	fnum=new Exiv2::ExifKey("Exif.Photo.FNumber");
	fnum2=new Exiv2::ExifKey("Exif.Photo.ApertureValue");
	iso=new Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings");
	expotime=new Exiv2::ExifKey("Exif.Photo.ExposureTime");
	expotime2=new Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue");

	QSettings settings("Qtpfsgui", "Qtpfsgui");
	RecentDirInputLDRs=settings.value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR,QDir::currentPath()).toString();
	chosen_config=&predef_confs[0];
	backbutton->setEnabled(FALSE);
	Next_Finishbutton->setEnabled(FALSE);
}

void HdrWizardForm::loadfiles() {
    QString filetypes;
    filetypes += "JPEG (*.jpeg *.jpg *.JPG *.JPEG);;";
    filetypes += "TIFF Images (*.tiff *.tif *.TIFF *.TIF);;";
    filetypes += "RAW Images (*.crw *.CRW *.cr2 *CR2 *.nef *.NEF *.dng *.DNG *.mrw *.MRW *.olf *.OLF *.kdc *.KDC *.dcr *DCR *.arw *.ARW *.raf *.RAF *.ptx *.PTX *.pef *.PEF *.x3f *.X3F)";
    QStringList files = QFileDialog::getOpenFileNames(this, "Select the input Images", RecentDirInputLDRs, filetypes );
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
		QFileInfo *qfi=new QFileInfo(*it);
		expotimes[index_expotimes]=obtain_expotime(qfi->filePath()); //fill array of exposure times
		if (expotimes[index_expotimes]==-1) {
			//if we are here it means that this image doesn't contain (enough) exif tags
			//we therefore warn the user, clear the graphical list and return.
			//we also clear the lists containing the ldr/hdr images
			listShowFiles->clear();
			clearlists();
			delete expotimes;
			QMessageBox::critical(this,"Error reading the EXIF tags in the image", QString("<font color=\"#FF0000\"><h3><b>ERROR:</b></h3></font> Qtpfsgui was not able to find the relevant <i>EXIF</i> tags\nin the input image: <font color=\"#FF9500\"><i><b>%1</b</i></font>.<br>\
Please make sure to load images that have at least\nthe following exif data: \
<ul><li>Shutter Speed (seconds)</li>\
<li>Aperture (f-number)</li></ul>\
<hr>This can happen when you preprocess your pictures.<br>\
You can <b>Copy the exif data</b> from some input images to some other output images via the <i><b>\"Tools->Copy Exif Data...\"</b></i> menu item.").arg(qfi->fileName()));
			return;
		}
		listShowFiles->addItem(qfi->fileName()); //fill graphical list
		QString extension=qfi->suffix().toUpper(); //get filename extension
		if (extension.startsWith("JP")) { //check for extension: if JPEG:
			ImagePtrList.append( new QImage(qfi->filePath()) ); // fill memory with image data
			input_is_ldr=true;
		} else if(extension.startsWith("TIF")) {
			TiffReader reader(qfi->filePath().toAscii().constData());
			if (reader.is8bitTiff()) {
				ImagePtrList.append( reader.readIntoQImage() );
				input_is_ldr=true;
				ldr_tiff=true;
			} else if (reader.is16bitTiff()) {
				pfs::Frame *framepointer=reader.readIntoPfsFrame();
				pfs::Channel *R, *G, *B;
				framepointer->getRGBChannels( R, G, B );
				listhdrR.push_back(R);
				listhdrG.push_back(G);
				listhdrB.push_back(B);
				input_is_ldr=false;
			} else {
				listShowFiles->clear();
				clearlists();
				delete expotimes;
				QMessageBox::critical(this,"Tiff error", QString("8 bit or 16 bit tiffs only").arg(qfi->fileName()));
				return;
			}
		} else { //not a jpeg of tiff_LDR file, so it's raw input (hdr)
			pfs::Frame *framepointer=readRAWfile(qfi->filePath().toAscii().constData(), opts);
			pfs::Channel *R, *G, *B;
			framepointer->getRGBChannels( R, G, B );
			listhdrR.push_back(R);
			listhdrG.push_back(G);
			listhdrB.push_back(B);
			input_is_ldr=false;
		}
		++it;
		index_expotimes++;
		delete qfi;
		progressBar->setValue(progressBar->value()+1); // increment progressbar
	}
	Next_Finishbutton->setEnabled(TRUE);
	confirmloadlabel->setText("<center><font color=\"#00FF00\"><h3><b>Done!</b></h3></font></center>");
	loadsetButton->setEnabled(FALSE);
	progressBar->setEnabled(FALSE);
    }
}

void HdrWizardForm::fix_gui_uncheck_custom(bool checked) {
if (!checked) {
	if (!checkBoxAntighosting->isChecked()) {
	label_3->setDisabled(TRUE);
	coboxRespCurve_Antighost->setDisabled(TRUE);
	label_4->setDisabled(TRUE);
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
	chosen_config->response_curve=FROM_FILE;
	chosen_config->CurveFilename=curvefilename;
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
	chosen_config->response_curve=fromQStringToResponse[comboBox_gamma_lin_log->currentText()];
	chosen_config->CurveFilename="";
	//and ENABLE nextbutton
	Next_Finishbutton->setEnabled(true);
    }
}

void HdrWizardForm::currentPageChangedInto(int newindex) {
	if      (newindex==0) { //initial page
		//can't end up here, I guess.
	}
	else if (newindex==1) { //predefined configs page
	}
	else if (newindex==2) { //custom config
		update_currentconfig(0);
	}
	else if (newindex==3) { //ending page
		Next_Finishbutton->setText("Finish");
		Next_Finishbutton->setEnabled(false);
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		//CREATE THE HDR
		if (input_is_ldr)
			PfsFrameHDR=createHDR(expotimes,chosen_config,checkBoxAntighosting->isChecked(),spinBoxIterations->value(),input_is_ldr,&ImagePtrList);
		else
			PfsFrameHDR=createHDR(expotimes,chosen_config,checkBoxAntighosting->isChecked(),spinBoxIterations->value(),input_is_ldr,&listhdrR,&listhdrG,&listhdrB);
		QApplication::restoreOverrideCursor();
		Next_Finishbutton->setEnabled(true);
		return;
	}
	Next_Finishbutton->setText("Next >");
}

void HdrWizardForm::update_current_antighost_curve(int/* fromgui*/) {
	update_current_config_gamma_lin_log(coboxRespCurve_Antighost->currentText());
}

void HdrWizardForm::nextpressed() {
	if (pagestack->currentIndex()==3)
		accept();
	else {
		if (pagestack->currentIndex()==1 && !checkBoxCallCustom->isChecked()) {
			pagestack->setCurrentIndex(pagestack->currentIndex()+2);
		}
		else
			pagestack->setCurrentIndex(pagestack->currentIndex()+1);
		backbutton->setEnabled(TRUE);
	}
}

void HdrWizardForm::backpressed() {
	if (pagestack->currentIndex()==3 && !checkBoxCallCustom->isChecked())
		pagestack->setCurrentIndex(pagestack->currentIndex()-2);
	else
		pagestack->setCurrentIndex(pagestack->currentIndex()-1);
}

float HdrWizardForm::obtain_expotime( QString filename ) {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.toStdString());
    image->readMetadata();
    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
	return -1;
    }

    Exiv2::ExifData::const_iterator iexpo = exifData.findKey(*expotime);
    Exiv2::ExifData::const_iterator iexpo2 = exifData.findKey(*expotime2);
    Exiv2::ExifData::const_iterator iiso  = exifData.findKey(*iso);
    Exiv2::ExifData::const_iterator ifnum = exifData.findKey(*fnum);
    Exiv2::ExifData::const_iterator ifnum2 = exifData.findKey(*fnum2);
    float expo=-1; float iso=-1; float fnum=-1;

    if (iexpo != exifData.end()) {
	expo=iexpo->toFloat();
    } else if (iexpo2 != exifData.end()) {
        long num=1, div=1;
        double tmp = std::exp(std::log(2.0) * iexpo2->toFloat());
        if (tmp > 1) {
            div = static_cast<long>(tmp + 0.5);
        }
        else {
            num = static_cast<long>(1/tmp + 0.5);
        }
        expo=static_cast<float>(num)/static_cast<float>(div);
    }

    if (ifnum != exifData.end()) {
	fnum=ifnum->toFloat();
    } else if (ifnum2 != exifData.end()) {
	fnum=static_cast<float>(std::exp(std::log(2.0) * ifnum2->toFloat() / 2));
    }

    if (iiso == exifData.end()) {
	iso=100.0;
    } else {
	iso=iiso->toFloat();
    }
    if (expo!=-1 && iso!=-1 && fnum!=-1) {
//      std::cerr << "expo=" << expo << " fnum=" << fnum << " iso=" << iso << std::endl;
	return ( (expo * iso) / (fnum*fnum*12.07488f) ); //this is PFM :)
    } else {
	return -1;
    }
}

void HdrWizardForm::load_response_curve_from_file() {
	curvefilename = QFileDialog::getOpenFileName(
			this,
			"Choose a camera response curve file to open",
			QDir::currentPath(),
			"Camera response curve (*.m);;All Files (*)" );
	if (curvefilename !="")  {
		ShowFileloadedLineEdit->setText(curvefilename);
		update_current_config_file_or_notfile(checkbox_load_from_file->isChecked());
	}
}

void HdrWizardForm::update_currentconfig( int index_from_gui ) {
	chosen_config=&predef_confs[ index_from_gui];
	lineEdit_showWeight->setText(getQStringFromConfig(1));
	lineEdit_show_resp->setText(getQStringFromConfig(2));
	lineEdit_showmodel->setText(getQStringFromConfig(3));
}

void HdrWizardForm::update_current_config_weights( const QString & from_gui) {
	chosen_config-> weights=fromQStringToWeight[from_gui];
}

void HdrWizardForm::update_current_config_gamma_lin_log( const QString & from_gui) {
	chosen_config->response_curve=fromQStringToResponse[from_gui];
}

void HdrWizardForm::update_current_config_model( const QString & from_gui) {
	chosen_config->model=fromQStringToModel[from_gui];
}

void HdrWizardForm::update_current_config_calibrate() {
	chosen_config->response_curve=FROM_ROBERTSON;;
}

void HdrWizardForm::setLoadFilename( const QString & filename_from_gui) {
	if (filename_from_gui!="") {
		chosen_config->response_curve=FROM_FILE;
		chosen_config->CurveFilename=filename_from_gui;
	}
}

pfs::Frame* HdrWizardForm::getPfsFrameHDR() {
	return PfsFrameHDR;
}

QString HdrWizardForm::getCaptionTEXT() {
	return "Weights: "+getQStringFromConfig(1) + " - Response curve: " + getQStringFromConfig(2) + " - Model: " + getQStringFromConfig(3);
}

QString HdrWizardForm::getQStringFromConfig( int type ) {
    if (type==1) { //return String for weights
	switch (chosen_config->weights) {
	case TRIANGULAR:
	    return "Triangular";
	case PLATEAU:
	    return "Plateau";
	case GAUSSIAN:
	    return "Gaussian";
	}
    } else if (type==2) {   //return String for response curve
	switch (chosen_config->response_curve) {
	case LINEAR:
	    return "Linear";
	case GAMMA:
	    return "Gamma";
	case LOG10:
	    return "Logarithmic";
	case FROM_ROBERTSON:
	    return "From Calibration";
	case FROM_FILE:
	    return "From File";
	}
    } else if (type==3) {   //return String for model
	switch (chosen_config->model) {
	case DEBEVEC:
	    return "Debevec";
	case ROBERTSON:
	    return "Robertson";
	}
    } else return "";
return "";
}


void HdrWizardForm::clearlists() {
	if (ImagePtrList.size() != 0) {
// 		qDebug("cleaning LDR exposures list");
		if (ldr_tiff) {
			foreach(QImage *p,ImagePtrList) {
// 				qDebug("while cleaning ldr tiffs, %ld",p->bits());
				delete p->bits();
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


HdrWizardForm::~HdrWizardForm() {
	// here PfsFrameHDR is not free-ed because we want to get it outside of this class via the getPfsFrameHDR() method.
	if (fnum)      delete fnum;
	if (fnum2)     delete fnum2;
	if (iso)       delete iso;
	if (expotime)  delete expotime;
	if (expotime2) delete expotime2;
	if (expotimes) delete [] expotimes;
	clearlists();
}

