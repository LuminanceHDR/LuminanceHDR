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

#include "tonemappingdialog_impl.h"
#include <QMessageBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QTextStream>
#include <math.h>
#include "config.h"

pfs::Frame* resizeFrame(pfs::Frame* inpfsframe, int _xSize);
pfs::Frame* applyGammaFrame( pfs::Frame* inpfsframe, float _gamma, bool inputXYZ);
QImage* fromLDRPFStoPPM_QImage( pfs::Frame* inpfsframe, uchar * data);


TMODialog::~TMODialog() {
delete contrastGang; delete biasGang;delete spatialGang;delete rangeGang;delete baseGang;delete alphaGang;delete betaGang;delete saturation2Gang;delete multiplierGang;delete coneGang;delete rodGang;delete keyGang;delete phiGang;delete range2Gang;delete lowerGang;delete upperGang;delete brightnessGang;delete saturationGang;delete pregammagang;delete postgammagang;

if (resizedHdrbuffer && (sizeComboBox->currentIndex() != (sizeComboBox->count()-1))) delete resizedHdrbuffer;
if (afterPreGammabuffer)   delete afterPreGammabuffer;
if (afterToneMappedBuffer) delete afterToneMappedBuffer;
if (afterPostgammaBuffer)  delete afterPostgammaBuffer;
if (QImagecurrentLDR)      delete QImagecurrentLDR;
if (imagedata)             delete [] imagedata;
}

TMODialog::TMODialog(QWidget *parent, bool ks) : QDialog(parent), setting_graphic_widgets(false), QImagecurrentLDR(NULL), imagedata(NULL), settings("Qtpfsgui", "Qtpfsgui"), keep_size(ks) {
	setupUi(this);
	// ashikhmin02
	contrastGang = 		new Gang(contrastSlider, contrastLineEdit,	0,	1,	0.5);
	
	// drago03
	biasGang = 		new Gang(biasSlider, biasLineEdit,		0.5,	1,	0.85);
	
	// durand02
	spatialGang = 		new Gang(spatialSlider, spatialLineEdit,	0,	60,	8);
	rangeGang = 		new Gang(rangeSlider, rangeLineEdit,		0,	10,	0.4);
	baseGang = 		new Gang(baseSlider, baseLineEdit,		0,	10,	5.0);
	
	// fattal02
	alphaGang = 		new Gang(alphaSlider, alphaLineEdit,		1e-3,	2,	1e-1, true);
	betaGang = 		new Gang(betaSlider, betaLineEdit,		0.1,	1.7,	0.8);
	saturation2Gang = 	new Gang(saturation2Slider, saturation2LineEdit, 0,	3,	1);
	
	// pattanaik00
	multiplierGang = 	new Gang(multiplierSlider, multiplierLineEdit,	1e-3,	50, 1,    true);
	coneGang = 		new Gang(coneSlider, coneLineEdit,		0,	1,   0.5);
	rodGang = 		new Gang(rodSlider, rodLineEdit,		0,	1,   0.5);
	
	// reinhard02
	keyGang = 		new Gang(keySlider, keyLineEdit,		0,	1,   0.18);
	phiGang = 		new Gang(phiSlider, phiLineEdit,		0,	50,  1);
	range2Gang = 		new Gang(range2Slider, range2LineEdit,		2, 	15,  8);
	lowerGang = 		new Gang(lowerSlider, lowerLineEdit,		1,	100, 1);
	upperGang = 		new Gang(upperSlider, upperLineEdit,		1,	100, 43);
	
	// reinhard04
	brightnessGang =	new Gang(brightnessSlider, brightnessLineEdit,	-35,	10, -10);
	saturationGang =	new Gang(saturationSlider, saturationLineEdit,	 0,	1.2, 0.99);
	
	pregammagang  =		new Gang(pregammaSlider,  pregammalineedit,  0, 3, 1);
	postgammagang =		new Gang(postgammaslider, postgammalineedit, 0, 3, 1);


	connect(ashikhmin02Default, SIGNAL(clicked()),this,SLOT(ashikhminReset()));
	connect(drago03Default,     SIGNAL(clicked()),this,SLOT(dragoReset()));
	connect(durand02Default,    SIGNAL(clicked()),this,SLOT(durandReset()));
	connect(fattal02Default,    SIGNAL(clicked()),this,SLOT(fattalReset()));
	connect(pattanaik00Default, SIGNAL(clicked()),this,SLOT(pattanaikReset()));
	connect(reinhard02Default,  SIGNAL(clicked()),this,SLOT(reinhard02Reset()));
	connect(reinhard04Default,  SIGNAL(clicked()),this,SLOT(reinhard04Reset()));

	connect(pregammadefault,    SIGNAL(clicked()),this,SLOT(preGammaReset()));
	connect(postgammadefault,   SIGNAL(clicked()),this,SLOT(postGammaReset()));


	connect(sizeComboBox,SIGNAL(activated(int)),this,SLOT(startChain()));
	connect(pregammaSlider, SIGNAL(sliderReleased()),this,SLOT(startChain()));
	connect(operators_tabWidget,SIGNAL(currentChanged (int)),this,SLOT(decide_size()));

	connect(simpleCheckBox,SIGNAL(clicked()),this,                SLOT(startChain()));
	connect(eq2RadioButton,SIGNAL(clicked()),this,                SLOT(startChain()));
	connect(eq4RadioButton,SIGNAL(clicked()),this,                SLOT(startChain()));
	connect(contrastSlider,SIGNAL(sliderReleased()),this,         SLOT(startChain()));
	connect(biasSlider,SIGNAL(sliderReleased()),this,             SLOT(startChain()));
	connect(spatialSlider,SIGNAL(sliderReleased()),this,          SLOT(startChain()));
	connect(rangeSlider,SIGNAL(sliderReleased()),this,            SLOT(startChain()));
	connect(baseSlider,SIGNAL(sliderReleased()),this,             SLOT(startChain()));
	connect(alphaSlider,SIGNAL(sliderReleased()),this,            SLOT(startChain()));
	connect(betaSlider,SIGNAL(sliderReleased()),this,             SLOT(startChain()));
	connect(saturation2Slider,SIGNAL(sliderReleased()),this,      SLOT(startChain()));
	connect(multiplierSlider,SIGNAL(sliderReleased()),this,       SLOT(startChain()));
	connect(autoYcheckbox,SIGNAL(clicked()),this,                 SLOT(startChain()));
	connect(pattalocal,SIGNAL(clicked()),this,                    SLOT(startChain()));
	connect(coneSlider,SIGNAL(sliderReleased()),this,             SLOT(startChain()));
	connect(rodSlider,SIGNAL(sliderReleased()),this,              SLOT(startChain()));
	connect(usescalescheckbox,SIGNAL(clicked()),this,             SLOT(startChain()));
	connect(keySlider,SIGNAL(sliderReleased()),this,              SLOT(startChain()));
	connect(phiSlider,SIGNAL(sliderReleased()),this,              SLOT(startChain()));
	connect(range2Slider,SIGNAL(sliderReleased()),this,           SLOT(startChain()));
	connect(lowerSlider,SIGNAL(sliderReleased()),this,            SLOT(startChain()));
	connect(upperSlider,SIGNAL(sliderReleased()),this,            SLOT(startChain()));
	connect(brightnessSlider,SIGNAL(sliderReleased()),this,       SLOT(startChain()));
	connect(saturationSlider,SIGNAL(sliderReleased()),this,       SLOT(startChain()));

	connect(postgammaslider,SIGNAL(sliderReleased()),this,SLOT(startChain()));

	connect(saveLDRbutton, SIGNAL(clicked()),this, SLOT(saveLDR()));

	connect(savesettingsbutton,SIGNAL(clicked()),this, SLOT(savesettings()));
	connect(loadsettingsbutton,SIGNAL(clicked()),this, SLOT(loadsettings()));
	connect(button_fromTxt2Gui,SIGNAL(clicked()),this, SLOT(fromTxt2Gui()));

	RecentDirTMOSetting=settings.value("RecentDirTMOSetting",QDir::currentPath()).toString();
	RecentDirLDRSetting=settings.value("RecentDirLDRSetting",QDir::currentPath()).toString();
	inputSettingsFilename="";

	filter_first_tabpage_change=true;

	imageLabel = new QLabel;
	imageLabel->setScaledContents(false);
	QScrollArea *scrollArea=new QScrollArea;
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageLabel);
	scrollArea->setWidgetResizable(false); //true if you want to "fit the window" in qt4 example
	PreviewFrame->setLayout(new QVBoxLayout);
	PreviewFrame->layout()->setMargin(0);
	PreviewFrame->layout()->addWidget(scrollArea);
	this->showMaximized();
	resizedHdrbuffer=NULL;
	afterPreGammabuffer=NULL;
	afterToneMappedBuffer=NULL;
	afterPostgammaBuffer=NULL;
}

void TMODialog::setOrigBuffer(pfs::Frame* hdrpfsframe) {
// 	qDebug("begin setOrigBuffer");
	assert(hdrpfsframe!=NULL);
	//origbuffer, internal field, points to the same memory area pointed by variables outside of this class, therefore treat origbuffer as readonly.
	origbuffer=hdrpfsframe;
	int height = origbuffer->getHeight();
	int width = origbuffer->getWidth();
	sizes.resize(0);
	for(int x = 128; x <= width; x *= 2) {
		if( x >= width )
			break;
		sizes.push_back(x);
		if( 3*(x/2) >= width )
			break;
		sizes.push_back(3*(x/2));
	}
	sizes.push_back(width);
	sizeComboBox->clear();
	float r = ( (float)height )/( (float) width);
	for(int i = 0; i < sizes.size(); i++) {
		sizeComboBox->addItem( QString("%1x%2").arg(sizes[i]).arg( (int)(r*sizes[i]) ));
	}
// 	qDebug("end  setOrigBuffer");
	//choose the smallest size
	this->setWorkSize(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////  origbuffer ----> resized  ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TMODialog::startChain() {
	setWorkSize(sizeComboBox->currentIndex());
}
void TMODialog::decide_size() {
	if (keep_size)
		setWorkSize(sizeComboBox->currentIndex()); //i.e. startChain (fullsize)
	else {
		sizeComboBox->setCurrentIndex(0);
		setWorkSize(0);
	}
}

void TMODialog::setWorkSize(int index) {
	qDebug("begin setWorkSize");
	if (origbuffer==NULL) {
		QMessageBox::warning(this,"","Problem with original (source) buffer...",
				QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	//delete its RESULT, just in case.
	if (resizedHdrbuffer!=NULL) {delete resizedHdrbuffer; resizedHdrbuffer=NULL;}
	if (index == (sizeComboBox->count()-1)) {
		//special case for 100% size: point to the same memory area
// 		qDebug("pointing to ORIG!");
		resizedHdrbuffer=origbuffer;
	} else {
		resizedHdrbuffer=resizeFrame(origbuffer, sizes[index]);
		
// 		pfs::Channel *R, *G, *B;
// 		resizedHdrbuffer->getRGBChannels( R, G, B );
// 		assert(R!=NULL);
// 		int w = R->getCols();
// 		int h = R->getRows();
// 		for( int x=0 ; x<w ; x++ ) {
// 			for( int y=0 ; y<h ; y++ ) {
// 				if ( !isfinite((*R)(x,y)) || !isfinite((*G)(x,y)) || !isfinite((*B)(x,y)) )
// 					fprintf(stderr, "res!f");
// 				if ( isnan((*R)(x,y)) || isnan((*G)(x,y)) || isnan((*B)(x,y)) )
// 					fprintf(stderr, "resN");
// 			}
// 		}
		//here I cannot delete origbuffer
	}
	QApplication::restoreOverrideCursor();
	qDebug("after setWorkSize");
	pregammasliderchanged(); //continue chain
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////  resized ----> pregamma  ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TMODialog::pregammasliderchanged() {
	qDebug("begin pregammasliderchanged");
	//do nothing if this function was called when setting graphic widgets while loading from file.
	if (setting_graphic_widgets)
		return;

	float pregamma = pregammagang->v();
	if ( resizedHdrbuffer==NULL ) {
		QMessageBox::warning(this,"","Problem with input resized buffer, in PREGAMMA",
					QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	//delete its RESULT
	if (afterPreGammabuffer) { delete afterPreGammabuffer; afterPreGammabuffer=NULL; }

	//no shortcuts here for pregamma=1, maybe add this in the future.
	afterPreGammabuffer=applyGammaFrame(resizedHdrbuffer,pregamma,false);
// 	pfs::Channel *X, *Y, *Z;
// 	afterPreGammabuffer->getXYZChannels( X, Y, Z );
// 	int w = Y->getCols();
// 	int h = Y->getRows();
// 	for( int x=0 ; x<w ; x++ ) {
// 		for( int y=0 ; y<h ; y++ ) {
// 			if ( !isfinite((*X)(x,y)) || !isfinite((*Y)(x,y)) || !isfinite((*Z)(x,y)) )
// 				fprintf(stderr, "pre!f");
// 			if ( isnan((*X)(x,y)) || isnan((*Y)(x,y)) || isnan((*Z)(x,y)) )
// 				fprintf(stderr, "preN");
// 		}
// 	}
	//now delete its SOURCE (if not 100% size, in which case resizedHdrbuffer points to origbuffer)
	if (sizeComboBox->currentIndex() != (sizeComboBox->count()-1)) {
		if (resizedHdrbuffer!=NULL) { delete resizedHdrbuffer; resizedHdrbuffer=NULL; }
	}
	resizedHdrbuffer=NULL;

	QApplication::restoreOverrideCursor();
	qDebug("after pregammasliderchanged");
	runToneMap(); //continue chain
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////   pregamma  ---> TM'ed   ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TMODialog::runToneMap() {
	qDebug("begin runToneMap");
	//do nothing if this function was called when setting graphic widgets while loading from file.
	if (setting_graphic_widgets)
		return;

	if( afterPreGammabuffer==NULL ) {
		QMessageBox::warning(this,"","Problem with input pregamma buffer, in RUNTONEMAP",
					QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

	//delete its RESULT
	if (afterToneMappedBuffer!=NULL) {
		delete afterToneMappedBuffer; afterToneMappedBuffer=NULL;
	}
	executeToneMap();
	//delete its SOURCE
// 	if (/*afterPreGammabuffer!=NULL && */(sizeComboBox->currentIndex() != (sizeComboBox->count()-1))) {
// 		qDebug("in IF"); //destructor di afterPreGammabuffer crasha in seguito a modifiche di rename;
// 		if (afterPreGammabuffer!=NULL) {delete afterPreGammabuffer; afterPreGammabuffer=NULL;}
// 		if (pregammagang->v() == 1) {
// 			resizedHdrbuffer=NULL;
// 		}
// 	} else {
// 		if (pregammagang->v() == 1) {
// 			resizedHdrbuffer=NULL;
// 			afterPreGammabuffer=NULL;
// 		} else {
			if (afterPreGammabuffer!=NULL) { delete afterPreGammabuffer; afterPreGammabuffer=NULL; }
// 		}
// 	}
	QApplication::restoreOverrideCursor();
	qDebug("after runToneMap");
	postgammasliderchanged(); //continue chain
}

void TMODialog::executeToneMap() {
switch (operators_tabWidget->currentIndex ()) {
case 0:
	afterToneMappedBuffer=pfstmo_ashikhmin02(afterPreGammabuffer,simpleCheckBox->isChecked(),contrastGang->v(),eq2RadioButton->isChecked() ? 2 : 4);
	break;
case 1:
	afterToneMappedBuffer=pfstmo_drago03(afterPreGammabuffer,biasGang->v());
	break;
case 2:
	afterToneMappedBuffer=pfstmo_durand02(afterPreGammabuffer, spatialGang->v(), rangeGang->v(), baseGang->v());
	break;
case 3:
	afterToneMappedBuffer=pfstmo_fattal02(afterPreGammabuffer, alphaGang->v(),betaGang->v(),saturation2Gang->v());
	break;
case 4:
	afterToneMappedBuffer=pfstmo_pattanaik00(afterPreGammabuffer,pattalocal->isChecked(),multiplierGang->v(),coneGang->v(),rodGang->v(),autoYcheckbox->isChecked());
	break;
case 5:
	afterToneMappedBuffer=pfstmo_reinhard02(afterPreGammabuffer,keyGang->v(),phiGang->v(),(int)range2Gang->v(),(int)lowerGang->v(),(int)upperGang->v(), usescalescheckbox->isChecked());
	break;
case 6:
	afterToneMappedBuffer=pfstmo_reinhard04(afterPreGammabuffer,brightnessGang->v(), saturationGang->v());
	break;
}
return;
}

void TMODialog::tabPageChanged() {
	if (!filter_first_tabpage_change)
		runToneMap();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////   TM'ed ---> postgamma   ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TMODialog::postgammasliderchanged() {
	qDebug("begin postgammasliderchanged");
	//do nothing if this function was called when setting graphic widgets while loading from file.
	if (setting_graphic_widgets)
		return;

	float postgamma = postgammagang->v();

// 	qDebug("before initial if");
	if (afterToneMappedBuffer==NULL ) {
		QMessageBox::warning(this,"","Problem with input resized buffer, in POSTGAMMA",
					QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	
	//delete its RESULT
	if (afterPostgammaBuffer!=NULL) {
		delete afterPostgammaBuffer; afterPostgammaBuffer=NULL;
	}

	if (postgamma==1) {
		//special case for postgamma=1: point to same memory area
		afterPostgammaBuffer=afterToneMappedBuffer;
	} else {
		afterPostgammaBuffer=applyGammaFrame(afterToneMappedBuffer,postgamma,true);
		//now delete its SOURCE
		if (afterToneMappedBuffer!=NULL)
			delete afterToneMappedBuffer;
	}
	//in any case it's source (pointer) is NULL
	afterToneMappedBuffer=NULL;

	QApplication::restoreOverrideCursor();
	qDebug("end postgammasliderchanged");
	prepareLDR();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////   postgamma ---> LDR   ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TMODialog::prepareLDR() {
	qDebug("begin prepareLDR");
	if(  afterPostgammaBuffer==NULL ) {
		QMessageBox::warning(this,"","Problem with input postgamma buffer, in prepareLDR()",
					QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	//delete its RESULT
	if (QImagecurrentLDR!=NULL) { delete QImagecurrentLDR; QImagecurrentLDR=NULL; }
	//this one takes the 0:1 tonemapped pfs stream and transforms it into a LDR image
	QImagecurrentLDR=fromLDRPFStoPPM_QImage(afterPostgammaBuffer,imagedata);
	//now delete its SOURCE
	if (afterPostgammaBuffer!=NULL) {
		delete afterPostgammaBuffer; afterPostgammaBuffer=NULL;
	}
	//assign the mapped image to the label
	imageLabel->setPixmap(QPixmap::fromImage(*QImagecurrentLDR,Qt::ColorOnly));
	//make the label use the image dimensions
	imageLabel->adjustSize();
	imageLabel->update();
	getCaptionAndFileName();
	setWindowTitle(caption);
	filter_first_tabpage_change=false;
	this->showMaximized();
	QApplication::restoreOverrideCursor();
	qDebug("after prepareLDR");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TMODialog::ashikhminReset() {
contrastGang->setDefault();
startChain();
}
void TMODialog::dragoReset(){
biasGang->setDefault();
startChain();
}
void TMODialog::durandReset(){
spatialGang->setDefault();
rangeGang->setDefault();
baseGang->setDefault();
startChain();
}
void TMODialog::fattalReset(){
alphaGang->setDefault();
betaGang->setDefault();
saturation2Gang->setDefault();
startChain();
}
void TMODialog::pattanaikReset(){
multiplierGang->setDefault();
coneGang->setDefault();
rodGang->setDefault();
startChain();
}
void TMODialog::reinhard02Reset(){
keyGang->setDefault();
phiGang->setDefault();
range2Gang->setDefault();
lowerGang->setDefault();
upperGang->setDefault();
startChain();
}
void TMODialog::reinhard04Reset(){
brightnessGang->setDefault();
saturationGang->setDefault();
startChain();
}
void TMODialog::preGammaReset(){
pregammagang->setDefault();
startChain();
}
void TMODialog::postGammaReset(){
postgammagang->setDefault();
startChain();
}


void TMODialog::saveLDR() {
	QStringList filetypes;
	filetypes += "JPEG (*.jpg *.jpeg *.JPG *.JPEG)";
	filetypes += "PNG (*.png *.PNG)";
	filetypes += "PPM PBM  (*.ppm *.pbm *.PPM *.PBM)";
	filetypes += "BMP (*.bmp *.BMP)";
//     filetypes += "ALL (*)";
	QFileDialog *fd = new QFileDialog(this);
	fd->setWindowTitle("Choose a filename to SAVE the LDR to");
	fd->setDirectory( RecentDirLDRSetting );
	fd->selectFile( fname + ".jpg");
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilters(filetypes);
	fd->setAcceptMode(QFileDialog::AcceptSave);
	fd->setConfirmOverwrite(true);
	if (fd->exec()) {
		QString outfname=(fd->selectedFiles()).at(0);
		if(!outfname.isEmpty()) {
			QFileInfo qfi(outfname);
			// update internal field variable
			RecentDirLDRSetting=qfi.path();
			// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
			if (RecentDirLDRSetting != settings.value("RecentDirLDRSetting",QDir::currentPath()).toString()) {
				settings.setValue("RecentDirLDRSetting", RecentDirLDRSetting);
			}
			const char * format=qfi.suffix().toAscii();
			if (qfi.suffix().isEmpty()) {
				QString usedfilter=fd->selectedFilter();
				if (usedfilter.startsWith("PNG")) {
					format="png";
					outfname+=".png";
				} else if (usedfilter.startsWith("JPEG")) {
					format="jpeg";
					outfname+=".jpg";
				} else if (usedfilter.startsWith("PPM")) {
					format="ppm";
					outfname+=".ppm";
				} else if (usedfilter.startsWith("BMP")) {
					format="bmp";
					outfname+=".bmp";
				}
			}
			if( ! QImagecurrentLDR->save(outfname,format,100) ) {
				QMessageBox::warning(this,"","Failed to save to <b>" + outfname + "</b>",
						QMessageBox::Ok, QMessageBox::NoButton);
			}
		}
	}
	delete fd;
}

void TMODialog::getCaptionAndFileName() {
caption="Qtpfsgui v"QTPFSGUIVERSION"   ---   ";
fname="untitled_";
switch (operators_tabWidget->currentIndex()) {
case 0:
	caption+="Ashikhmin02:     ~ ";
	fname+="ashikhmin02_";
	if (simpleCheckBox->isChecked()) {
		fname+="-simple";
		caption+="simple";
	} else {
		if (eq2RadioButton->isChecked()) {
			fname+="-eq2_";
			caption+="Equation 2 ~ ";
		} else {
			fname+="-eq4_";
			caption+="Equation 4 ~ ";
		}
		fname+=QString("local_%1").arg(contrastGang->v());
		caption+=QString("Local=%1 ~ ").arg(contrastGang->v());
	}
	break;
case 1:
	caption+="Drago03:     ~ ";
	fname+="drago03_";
	fname+=QString("bias_%1").arg(biasGang->v());
	caption+=QString("Bias=%1 ~ ").arg(biasGang->v());
	break;
case 2:
	caption+="Durand02:     ~ ";
	fname+="durand02_";
	fname+=QString("spacial_%1_").arg(spatialGang->v());
	caption+=QString("Spacial=%1 ~ ").arg(spatialGang->v());
	fname+=QString("range_%1_").arg(rangeGang->v());
	caption+=QString("Range=%1 ~ ").arg(rangeGang->v());
	fname+=QString("base_%1").arg(baseGang->v());
	caption+=QString("Base=%1 ~ ").arg(baseGang->v());
	break;
case 3:
	caption+="Fattal02:     ~ ";
	fname+="fattal02_";
	fname+=QString("alpha_%1_").arg(alphaGang->v());
	caption+=QString("Alpha=%1 ~ ").arg(alphaGang->v());
	fname+=QString("beta_%1_").arg(betaGang->v());
	caption+=QString("Beta=%1 ~ ").arg(betaGang->v());
	fname+=QString("saturation_%1").arg(saturation2Gang->v());
	caption+=QString("Saturation=%1 ~ ").arg(saturation2Gang->v());
	break;
case 4:
	caption+="Pattanaik00:     ~ ";
	fname+="pattanaik00_";
	fname+=QString("mul_%1_").arg(multiplierGang->v());
	caption+=QString("Multiplier=%1 ~ ").arg(multiplierGang->v());
	if (pattalocal->isChecked()) {
		fname+="local";
		caption+="Local";
	} else if (autoYcheckbox->isChecked()) {
		fname+="autolum";
		caption+="AutoLuminance";
	} else {
		fname+=QString("cone_%1_").arg(coneGang->v());
		caption+=QString("Cone=%1 ~ ").arg(coneGang->v());
		fname+=QString("rod_%1").arg(rodGang->v());
		caption+=QString("Rod=%1 ~ ").arg(rodGang->v());
	}
	break;
case 5:
	caption+="Reinhard02:     ~ ";
	fname+="reinhard02_";
	fname+=QString("key_%1_").arg(keyGang->v());
	caption+=QString("Key=%1 ~ ").arg(keyGang->v());
	fname+=QString("phi_%1_").arg(phiGang->v());
	caption+=QString("Phi=%1 ~ ").arg(phiGang->v());
	if (usescalescheckbox->isChecked()) {
		fname+=QString("scales_");
		caption+=QString(" ~ Scales: ");
		fname+=QString("range_%1_").arg(range2Gang->v());
		caption+=QString("Range=%1 ~ ").arg(range2Gang->v());
		fname+=QString("lower%1_").arg(lowerGang->v());
		caption+=QString("Lower=%1 ~ ").arg(lowerGang->v());
		fname+=QString("upper%1_").arg(upperGang->v());
		caption+=QString("Upper=%1 ~ ").arg(upperGang->v());
	}
	break;
case 6:
	caption+="Reinhard04:     ~ ";
	fname+="reinhard04_";
	fname+=QString("brightness_%1_").arg(brightnessGang->v());
	caption+=QString("Brightness=%1 ~ ").arg(brightnessGang->v());
	fname+=QString("saturation_%1_").arg(saturationGang->v());
	caption+=QString("Saturation=%1 ~ ").arg(saturationGang->v());
	break;
}
return;
}

void TMODialog::loadsettings() {
	QString opened = QFileDialog::getOpenFileName(
			this,
			"Choose a tonemapping settings text file to OPEN...",
			RecentDirTMOSetting,
			"Qtpfsgui tonemapping settings text file (*.txt *.TXT)" );
	if( ! opened.isEmpty() ) {
		QFileInfo qfi(opened);
		if (!qfi.isReadable()) {
		QMessageBox::warning(this,"Aborting...","File is not readable (check existence, permissions,...)",
					QMessageBox::Ok,QMessageBox::NoButton);
		return;
		}
		// update internal field variable
		RecentDirTMOSetting=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentDirTMOSetting != settings.value("RecentDirTMOSetting",QDir::currentPath()).toString()) {
			settings.setValue("RecentDirTMOSetting", RecentDirTMOSetting);
		}
		//enable graphical elements
		lineEditTMOsetting->setText(qfi.fileName());
		lineEditTMOsetting->setEnabled(TRUE);
		label_applyTMOSetting->setEnabled(TRUE);
		button_fromTxt2Gui->setEnabled(TRUE);
		//update filename internal field, used by parsing function fromTxt2Gui()
		inputSettingsFilename=opened;
		//call parsing function
		fromTxt2Gui();
	}
}

void TMODialog::savesettings() {
	QString fname =QFileDialog::getSaveFileName(
			this,
			"SAVE tonemapping settings text file to...",
			RecentDirTMOSetting,
			"Qtpfsgui tonemapping settings text file (*.txt *.TXT)");
	if( ! fname.isEmpty() ) {
		QFileInfo qfi(fname);
		// update internal field variable
		RecentDirTMOSetting=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentDirTMOSetting != settings.value("RecentDirTMOSetting",QDir::currentPath()).toString()) {
			settings.setValue("RecentDirTMOSetting", RecentDirTMOSetting);
		}
		//enable graphical elements
		lineEditTMOsetting->setText(qfi.fileName());
		lineEditTMOsetting->setEnabled(TRUE);
		label_applyTMOSetting->setEnabled(TRUE);
		button_fromTxt2Gui->setEnabled(TRUE);
		//update filename internal field, used by parsing function fromTxt2Gui()
		inputSettingsFilename=fname;
		//write txt file
		fromGui2Txt(fname);
	}
}

void TMODialog::fromGui2Txt(QString destination) {
	QFile file(destination);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::warning(this,"Aborting...","File is not writable (check permissions, path...)",
					QMessageBox::Ok,QMessageBox::NoButton);
		return;
	}
	QTextStream out(&file);
	out << "# Qtpfsgui Tonemapping Setting file." << endl;
	out << "# Editing this file by hand is risky, worst case scenario is Qtpfsgui crashing." << endl;
	out << "# Please edit this file by hand only if you know what you're doing, in any case never change the left hand side text (i.e. the part before the ``='')." << endl;
	out << "TMOSETTINGSVERSION=" << TMOSETTINGSVERSION << endl;
	switch (operators_tabWidget->currentIndex()) {
	case 0:
		out << "TMO=" << "Ashikhmin02" << endl;
		out << "SIMPLE=" << (simpleCheckBox->isChecked() ? "YES" : "NO") << endl;
		out << "EQUATION=" << (eq2RadioButton->isChecked() ? "2" : "4") << endl;
		out << "CONTRAST=" << contrastGang->v() << endl;
		break;
	case 1:
		out << "TMO=" << "Drago03" << endl;
		out << "BIAS=" << biasGang->v() << endl;
		break;
	case 2:
		out << "TMO=" << "Durand02" << endl;
		out << "SPACIAL=" << spatialGang->v() << endl;
		out << "RANGE=" << rangeGang->v() << endl;
		out << "BASE=" << baseGang->v() << endl;
		break;
	case 3:
		out << "TMO=" << "Fattal02" << endl;
		out << "ALPHA=" << alphaGang->v() << endl;
		out << "BETA=" << betaGang->v() << endl;
		out << "COLOR=" << saturation2Gang->v() << endl;
		break;
	case 4:
		out << "TMO=" << "Pattanaik00" << endl;
		out << "MULTIPLIER=" << multiplierGang->v() << endl;
		out << "LOCAL=" << (pattalocal->isChecked() ? "YES" : "NO") << endl;
		out << "AUTOLUMINANCE=" << (autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "CONE=" << coneGang->v() << endl;
		out << "ROD=" << rodGang->v() << endl;
		break;
	case 5:
		out << "TMO=" << "Reinhard02" << endl;
		out << "KEY=" << keyGang->v() << endl;
		out << "PHI=" << phiGang->v() << endl;
		out << "SCALES=" << (usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "RANGE=" << range2Gang->v() << endl;
		out << "LOWER=" << lowerGang->v() << endl;
		out << "UPPER=" << upperGang->v() << endl;
		break;
	case 6:
		out << "TMO=" << "Reinhard04" << endl;
		out << "BRIGHTNESS=" << brightnessGang->v() << endl;
		out << "SATURATION=" << saturationGang->v() << endl;
		break;
	}
	out << "PREGAMMA=" << pregammagang->v() << endl;
	out << "POSTGAMMA=" << postgammagang->v() << endl;
	file.close();
}

void TMODialog::fromTxt2Gui() {
	QFile file(inputSettingsFilename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::warning(this,"Aborting...","File is not readable (check permissions, path...)",
					QMessageBox::Ok,QMessageBox::NoButton);
		return;
	}
	QTextStream in(&file);
	QString field,value;
	setting_graphic_widgets=true;

	while (!in.atEnd()) {
		QString line = in.readLine();
		//skip comments
		if (line.startsWith('#'))
			continue;

		field=line.section('=',0,0); //get the field
		value=line.section('=',1,1); //get the value
		if (field=="TMOSETTINGSVERSION") {
			if (value != TMOSETTINGSVERSION) {
				QMessageBox::warning(this,"Aborting...","Error, Tonemapping Setting file has a different version than the expected one.",
							QMessageBox::Ok,QMessageBox::NoButton);
				return;
			}
		} else if (field=="TMO") {
			if (value=="Ashikhmin02") {
				operators_tabWidget->setCurrentIndex(0);
			} else if (value == "Drago03") {
				operators_tabWidget->setCurrentIndex(1);
			} else if (value == "Durand02") {
				operators_tabWidget->setCurrentIndex(2);
			} else if (value == "Fattal02") {
				operators_tabWidget->setCurrentIndex(3);
			} else if (value == "Pattanaik00") {
				operators_tabWidget->setCurrentIndex(4);
			} else if (value == "Reinhard02") {
				operators_tabWidget->setCurrentIndex(5);
			} else if (value == "Reinhard04") {
				operators_tabWidget->setCurrentIndex(6);
			}
		} else if (field=="SIMPLE") {
			(value == "YES") ? simpleCheckBox->setCheckState(Qt::Checked) : simpleCheckBox->setCheckState(Qt::Unchecked);
		} else if (field=="EQUATION") {
			(value=="2") ? eq2RadioButton->setChecked(true) : eq2RadioButton->setChecked(false);
			(value=="4") ? eq4RadioButton->setChecked(true) : eq4RadioButton->setChecked(false);
		} else if (field=="CONTRAST") {
			contrastSlider->setValue(contrastGang->v2p(value.toFloat()));
		} else if (field=="BIAS") {
			biasSlider->setValue(biasGang->v2p(value.toFloat()));
		} else if (field=="SPACIAL") {
			spatialSlider->setValue(spatialGang->v2p(value.toFloat()));
		} else if (field=="RANGE") {
			rangeSlider->setValue(rangeGang->v2p(value.toFloat()));
		} else if (field=="BASE") {
			baseSlider->setValue(baseGang->v2p(value.toFloat()));
		} else if (field=="ALPHA") {
			alphaSlider->setValue(alphaGang->v2p(value.toFloat()));
		} else if (field=="BETA") {
			betaSlider->setValue(betaGang->v2p(value.toFloat()));
		} else if (field=="COLOR") {
			saturation2Slider->setValue(saturation2Gang->v2p(value.toFloat()));
		} else if (field=="MULTIPLIER") {
			multiplierSlider->setValue(multiplierGang->v2p(value.toFloat()));
		} else if (field=="LOCAL") {
			(value=="YES") ? pattalocal->setCheckState(Qt::Checked) : pattalocal->setCheckState(Qt::Unchecked);
		} else if (field=="AUTOLUMINANCE") {
			(value=="YES") ? autoYcheckbox->setCheckState(Qt::Checked) : autoYcheckbox->setCheckState(Qt::Unchecked);
		} else if (field=="CONE") {
			coneSlider->setValue(coneGang->v2p(value.toFloat()));
		} else if (field=="ROD") {
			rodSlider->setValue(rodGang->v2p(value.toFloat()));
		} else if (field=="KEY") {
			keySlider->setValue(keyGang->v2p(value.toFloat()));
		} else if (field=="PHI") {
			phiSlider->setValue(phiGang->v2p(value.toFloat()));
		} else if (field=="SCALES") {
			(value=="YES") ? usescalescheckbox->setCheckState(Qt::Checked) : usescalescheckbox->setCheckState(Qt::Unchecked);
		} else if (field=="RANGE") {
			range2Slider->setValue(range2Gang->v2p(value.toFloat()));
		} else if (field=="LOWER") {
			lowerSlider->setValue(lowerGang->v2p(value.toFloat()));
		} else if (field=="UPPER") {
			upperSlider->setValue(upperGang->v2p(value.toFloat()));
		} else if (field=="BRIGHTNESS") {
			brightnessSlider->setValue(brightnessGang->v2p(value.toFloat()));
		} else if (field=="SATURATION") {
			saturationSlider->setValue(saturationGang->v2p(value.toFloat()));
		} else if (field=="PREGAMMA") {
			pregammaSlider->setValue(pregammagang->v2p(value.toFloat()));
		} else if (field=="POSTGAMMA") {
			postgammaslider->setValue(postgammagang->v2p(value.toFloat()));
		}
	}
	//setting this to false enables computing the result when calling pregammasliderchanged
	setting_graphic_widgets=false;
	pregammasliderchanged();
}
