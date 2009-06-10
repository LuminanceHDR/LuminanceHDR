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

#include <QDockWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QWhatsThis>
#include "tonemappingDialog.h"
#include "ldrviewer.h"
#include "../Exif/exif_operations.h"
#include "../Common/genericViewer.h"
#include "../Common/config.h"
#include "../Common/gang.h"

//#include <iostream> // for debug

TonemappingWindow::~TonemappingWindow() {
}

TonemappingWindow::TonemappingWindow(QWidget *parent, pfs::Frame* pfsFrame, QString _file) : QMainWindow(parent) {
	setupUi(this);
	
	qtpfsgui_options=QtpfsguiOptions::getInstance();
	cachepath=QtpfsguiOptions::getInstance()->tempfilespath;

	toolBar->setToolButtonStyle((Qt::ToolButtonStyle)settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt());

	prefixname=QFileInfo(_file).completeBaseName();
	setWindowTitle(windowTitle() + prefixname);
	recentPathSaveLDR=settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString();

	mdiArea = new QMdiArea(this);
	mdiArea->setBackground(QBrush(QColor::fromRgb(192, 192, 192)) );
	mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	connect(mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateActions(QMdiSubWindow *)) );
	setCentralWidget(mdiArea);

	QDockWidget *dock = new QDockWidget(tr("Tone mapping Panel"), this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	connect(actionViewTMdock,SIGNAL(toggled(bool)),dock,SLOT(setVisible(bool)));
	//swap original frame to hd
	pfs::DOMIO pfsio;
	pfsio.writeFrame(pfsFrame, QFile::encodeName(cachepath+"/original.pfs").constData());
	
	//re-read original frame
	originalPfsFrame = pfsio.readFrame(QFile::encodeName(cachepath+"/original.pfs").constData());
	if (!originalPfsFrame) // TODO: show a warning and close
		close();

	TMWidget *tmwidget=new TMWidget(dock, statusBar());
	dock->setWidget(tmwidget);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	connect(tmwidget,SIGNAL(newResult(const QImage&, tonemapping_options*)), this,SLOT(addMDIresult(const QImage&,tonemapping_options*)));

	connect(actionAsThumbnails,SIGNAL(triggered()),this,SLOT(viewAllAsThumbnails()));
	connect(actionCascade,SIGNAL(triggered()),mdiArea,SLOT(cascadeSubWindows()));
	connect(actionFit_to_Window,SIGNAL(toggled(bool)),this,SLOT(fit_current_to_win(bool)));
	connect(actionFix_Histogram,SIGNAL(toggled(bool)),this,SLOT(LevelsRequested(bool)));
	connect(documentationAction,SIGNAL(triggered()),parent,SLOT(openDocumentation()));
	connect(actionWhat_s_This,SIGNAL(triggered()),this,SLOT(enterWhatsThis()));
	connect(actionShowHDR,SIGNAL(toggled(bool)),this,SLOT(showHDR(bool)));
	connect(actionShowNext,SIGNAL(triggered()),mdiArea,SLOT(activateNextSubWindow()));
	connect(actionShowPrevious,SIGNAL(triggered()),mdiArea,SLOT(activatePreviousSubWindow()));

	// original HDR window
	originalHDR = new HdrViewer(this,false,true, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor);
	originalHDR->updateHDR(originalPfsFrame);
        originalHDR->setFileName(QString("Original HDR"));
        originalHDR->setWindowTitle(QString("Original HDR"));
	originalHDR->normalSize();
	originalHDR->showMaximized();
	originalHDR->fitToWindow(true);
        mdiArea->addSubWindow(originalHDR);
	originalHdrSubWin = mdiArea->currentSubWindow();
	originalHdrSubWin->showMaximized();
	originalHdrSubWin->hide();
	
	showMaximized();
}

bool TonemappingWindow::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::Close) {
		int n = mdiArea->subWindowList().size();
		if (n == 2) { // no LDRs - mdi list size not yet updated !!!
			actionFix_Histogram->setEnabled( false );
			actionSave->setEnabled( false );
			actionSaveAll->setEnabled( false );
			actionClose_All->setEnabled( false );
			actionShowNext->setEnabled( false );
			actionShowPrevious->setEnabled( false );
		}
		else if ( (n == 3) && !originalHdrSubWin->isVisible() ) { // must be done here
			actionShowNext->setEnabled( false );
                        actionShowPrevious->setEnabled( false );
		}
		mdiArea->activateNextSubWindow();
		return false; // propagate the event
	} 
	else {
			// standard event processing
			return QObject::eventFilter(obj, event);
	}
}


void TonemappingWindow::addMDIresult(const QImage& i,tonemapping_options *opts) {
	LdrViewer *n = new LdrViewer( i, this, false, false, opts);
	connect(n,SIGNAL(levels_closed()),this,SLOT(levels_closed()));
	n->setAttribute(Qt::WA_DeleteOnClose);
	n->normalSize();	
	n->showMaximized();
	n->fitToWindow(true);
	n->installEventFilter(this);
	mdiArea->addSubWindow(n);
	n->show();
	currentLdrSubWin = mdiArea->currentSubWindow();
	currentLdrSubWin->setAttribute(Qt::WA_DeleteOnClose);
	currentLdrSubWin->showMaximized();
	updateActions(currentLdrSubWin);
}

void TonemappingWindow::fit_current_to_win(bool checked) {
	GenericViewer* current=((GenericViewer*)(mdiArea->activeSubWindow()->widget()));
	if (current==NULL)
		return;
	current->fitToWindow(checked);
}

void TonemappingWindow::LevelsRequested(bool checked) {
	if (checked) {
		GenericViewer* currentLDR = (GenericViewer*) currentLdrSubWin->widget();
		if (currentLDR==NULL)
			return;
		actionFix_Histogram->setDisabled(true);
		currentLDR->levelsRequested(checked);
	}
}

void TonemappingWindow::levels_closed() {
	actionFix_Histogram->setDisabled(false);
	actionFix_Histogram->setChecked(false);
}

void TonemappingWindow::on_actionSave_triggered() {
	GenericViewer* currentLDR = (GenericViewer*) currentLdrSubWin->widget();
	if (currentLDR==NULL)
		return;
	if (currentLDR == originalHDR)
		return;

	QString outfname = saveLDRImage(prefixname + "_" + currentLDR->getFilenamePostFix()+ ".jpg",currentLDR->getQImage());

	//if save is succesful
	if ( outfname.endsWith("jpeg",Qt::CaseInsensitive) || outfname.endsWith("jpg",Qt::CaseInsensitive) ) {
		//time to write the exif data...
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		ExifOperations::writeExifData( QFile::encodeName(outfname).constData(), currentLDR->getExifComment().toStdString() );
	}
}

void TonemappingWindow::updateActions(QMdiSubWindow *w) {
	int n = mdiArea->subWindowList().size();
	bool a = (n>2); // more than 2 LDRs !!!
	bool b = (n!=1); // no LDRs
	bool c = originalHdrSubWin->isVisible();
	if (w == originalHdrSubWin) {
		actionFix_Histogram->setEnabled( false );
		actionSave->setEnabled( false );
		actionFit_to_Window->setChecked(originalHDR->getFittingWin());
		actionShowNext->setEnabled( (b && c) );
		actionShowPrevious->setEnabled( (b && c) );
		return;
	}
	currentLdrSubWin = w;
	actionFix_Histogram->setEnabled( b );
	actionSave->setEnabled( b );
	actionSaveAll->setEnabled( b );
	actionClose_All->setEnabled( b );
	actionAsThumbnails->setEnabled( b );
	actionFit_to_Window->setEnabled( b ); 
	actionCascade->setEnabled( b );
	actionShowNext->setEnabled( a || (b && c) );
	actionShowPrevious->setEnabled( a || (b && c) );
	//TODO
	if (w!=NULL) {
		GenericViewer *current = (GenericViewer*) w->widget(); 
		actionFit_to_Window->setChecked(current->getFittingWin());
		//actionFix_Histogram->setChecked(current->hasLevelsOpen());
	}
}

void TonemappingWindow::closeEvent ( QCloseEvent * ) {
	emit closing();
}

void TonemappingWindow::viewAllAsThumbnails() {
	mdiArea->tileSubWindows();
	QList<QMdiSubWindow*> allViewers = mdiArea->subWindowList();
	foreach (QMdiSubWindow *p, allViewers) {
		((GenericViewer*)p->widget())->fitToWindow(true);
	}
}

void TonemappingWindow::on_actionClose_All_triggered() {
	QList<QMdiSubWindow*> allLDRs = mdiArea->subWindowList();
	foreach (QMdiSubWindow *p, allLDRs) {
		p->close();
	}
	actionClose_All->setEnabled(false);
	updateActions(0);
}

void TonemappingWindow::on_actionSaveAll_triggered() {
	QString dir = QFileDialog::getExistingDirectory(
			0,
			tr("Save files in"),
			settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString()
	);
	if (!dir.isEmpty()) {
		settings.setValue(KEY_RECENT_PATH_SAVE_LDR, dir);
		QList<QMdiSubWindow*> allLDRs = mdiArea->subWindowList();
		foreach (QMdiSubWindow *p, allLDRs) {
			if (p != originalHdrSubWin) { //skips the hdr
			  GenericViewer* ldr = (GenericViewer*) p->widget();
			  QString outfname = saveLDRImage(prefixname + "_" + ldr->getFilenamePostFix()+ ".jpg",ldr->getQImage(), true);
			  //if save is succesful
			  if ( outfname.endsWith("jpeg",Qt::CaseInsensitive) || outfname.endsWith("jpg",Qt::CaseInsensitive) ) {
				//time to write the exif data...
				//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
				ExifOperations::writeExifData( QFile::encodeName(outfname).constData(), ldr->getExifComment().toStdString() );
			  }
			}
		}
	}
}

void TonemappingWindow::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

void TonemappingWindow::showHDR(bool toggled) {
	int n = mdiArea->subWindowList().size();
	if (toggled) {
		originalHdrSubWin->show();
		mdiArea->setActiveSubWindow(originalHdrSubWin);
	}
	else { 
		if (originalHdrSubWin == mdiArea->currentSubWindow()) {
			originalHdrSubWin->hide();
			mdiArea->activateNextSubWindow();
			mdiArea->currentSubWindow()->showMaximized();
		}
		originalHdrSubWin->hide();
	}
	if (n == 1) { // no LDRs 
		actionShowNext->setEnabled( false );
		actionShowPrevious->setEnabled( false );
	}
	else if (n == 2)  { // must be done here
		actionShowNext->setEnabled( toggled );
		actionShowPrevious->setEnabled( toggled );
	}
}
