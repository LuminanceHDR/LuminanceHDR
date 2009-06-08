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
	delete allLDRs;
	// originalPfsFrame: HdrViewer takes ownership of pfsFrame
	//std::cout << "TonemappingWindow::~TonemappingWindow()" << std::endl;
	//std::cout << originalPfsFrame << std::endl;
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
	connect(actionShowNext,SIGNAL(triggered()),mdiArea,SLOT(activateNextSubWindow() ));
	connect(actionShowPrevious,SIGNAL(triggered()),mdiArea,SLOT(activatePreviousSubWindow() ));

	// list of all LDR SubWindows
	allLDRs = new QList<QMdiSubWindow *>;

	// original HDR window
	originalHDR = new HdrViewer(this, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor, false, true);
	originalHDR->setAttribute(Qt::WA_DeleteOnClose);
	originalHDR->updateHDR(originalPfsFrame);
        originalHDR->filename = QString("Original HDR");
        originalHDR->setWindowTitle(QString("Original HDR"));
        originalHDR->resize(500,400);
        mdiArea->addSubWindow(originalHDR);
	originalHdrSubWin = mdiArea->currentSubWindow();
	originalHdrSubWin->hide();
	isHdrShown = false;
	
	showMaximized();
}

bool TonemappingWindow::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::Close) {
		allLDRs->removeOne(currentLdrSubWin);		
		if (!allLDRs->isEmpty())
			currentLdrSubWin = allLDRs->first();
		else
			currentLdrSubWin = NULL;
		updateActions( currentLdrSubWin );
		//std::cout << "MDI Closed" << std::endl;
		//std::cout << "allLDRs size: " << allLDRs->size() << std::endl;
		//std::cout << "allLDRs empty: " << allLDRs->isEmpty() << std::endl;
		//std::cout << "SubWins size: " << mdiArea->subWindowList().size() << std::endl;
		return false; // propagate the event
	} 
	else {
			// standard event processing
			return QObject::eventFilter(obj, event);
	}
}

void TonemappingWindow::addMDIresult(const QImage& i,tonemapping_options* opts) {
	LdrViewer *n = new LdrViewer(this,i,opts);
	connect(n,SIGNAL(levels_closed()),this,SLOT(levels_closed()));
	mdiArea->addSubWindow(n);
	n->show();
	n->installEventFilter(this);
	currentLdrSubWin = mdiArea->currentSubWindow();
	currentLdrSubWin->setAttribute(Qt::WA_DeleteOnClose);
	allLDRs->append(currentLdrSubWin);
	updateActions(currentLdrSubWin);
	//std::cout << "allLDRs size: " << allLDRs->size() << std::endl;
	//std::cout << "SubWins size: " << mdiArea->subWindowList().size() << std::endl;
}

void TonemappingWindow::fit_current_to_win(bool checked) {
	GenericViewer* current=((GenericViewer*)(mdiArea->activeSubWindow()->widget()));
	if (current==NULL)
		return;
	current->fitToWindow(checked);
}

void TonemappingWindow::LevelsRequested(bool checked) {
	if (checked) {
		LdrViewer* currentLDR = (LdrViewer*) currentLdrSubWin->widget();
		if (currentLDR==NULL)
			return;
		actionFix_Histogram->setDisabled(true);
		currentLDR->LevelsRequested(checked);
	}
}

void TonemappingWindow::levels_closed() {
	actionFix_Histogram->setDisabled(false);
	actionFix_Histogram->setChecked(false);
}

void TonemappingWindow::on_actionSave_triggered() {
	LdrViewer* currentLDR = (LdrViewer*) currentLdrSubWin->widget();
	if (currentLDR==NULL)
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
	bool b = !allLDRs->isEmpty();
	currentLdrSubWin = w;
	actionFix_Histogram->setEnabled( b );
	actionSave->setEnabled( b );
	actionSaveAll->setEnabled( b );
	actionClose_All->setEnabled( b );
	actionAsThumbnails->setEnabled( b );
	actionFit_to_Window->setEnabled( b || isHdrShown ); 
	actionCascade->setEnabled( b || isHdrShown );
	actionShowNext->setEnabled( b );
	actionShowPrevious->setEnabled( b );
	//TODO
	if (w!=NULL) {
			GenericViewer *current = (GenericViewer*) w->widget(); 
			if (current==NULL)
				return;
			actionFit_to_Window->setChecked(current->getFittingWin());
			//actionFix_Histogram->setChecked(current->hasLevelsOpen());
		}
}

void TonemappingWindow::closeEvent ( QCloseEvent * ) {
	// originalPfsFrame: HdrViewer takes ownership of pfsFrame
	//std::cout << "TonemappingWindow::closeEvent()" << std::endl;
	//std::cout << originalPfsFrame << std::endl;
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
	//QList<QMdiSubWindow*> allLDRresults=mdiArea->subWindowList();
	foreach (QMdiSubWindow *p, *allLDRs) {
		p->close();
	}
	actionClose_All->setEnabled(false);
}

void TonemappingWindow::on_actionSaveAll_triggered() {
	QString dir = QFileDialog::getExistingDirectory(
			0,
			tr("Save files in"),
			settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString()
	);
	if (!dir.isEmpty()) {
		settings.setValue(KEY_RECENT_PATH_SAVE_LDR, dir);
		//QList<QMdiSubWindow*> allLDRresults=mdiArea->subWindowList();
		foreach (QMdiSubWindow *p, *allLDRs) {
			LdrViewer* ldr = (LdrViewer*) p->widget();
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

void TonemappingWindow::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

void TonemappingWindow::showHDR(bool toggled) {
	bool b = !allLDRs->isEmpty();
	toggled ? originalHdrSubWin->show() : originalHdrSubWin->hide();
	mdiArea->setActiveSubWindow(originalHdrSubWin);
	isHdrShown = toggled;
	actionFit_to_Window->setEnabled( b || isHdrShown ); 
	actionCascade->setEnabled( b || isHdrShown );
}
