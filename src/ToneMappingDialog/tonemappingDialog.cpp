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
#include <QWorkspace>
#include <QFileDialog>
#include <QMessageBox>
#include <QWhatsThis>
#include "tonemappingDialog.h"
#include "ldrviewer.h"
#include "../Exif/exif_operations.h"
#include "../Common/config.h"

TonemappingWindow::~TonemappingWindow() {}

TonemappingWindow::TonemappingWindow(QWidget *parent, pfs::Frame* &OriginalPfsFrame, QString _file) : QMainWindow(parent) {
	setupUi(this);
	toolBar->setToolButtonStyle((Qt::ToolButtonStyle)settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt());

	prefixname=QFileInfo(_file).completeBaseName();
	setWindowTitle(windowTitle() + prefixname);
	recentPathSaveLDR=settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString();

	workspace = new QWorkspace(this);
	workspace->setScrollBarsEnabled( TRUE );
	connect(workspace,SIGNAL(windowActivated(QWidget*)), this, SLOT(updateActions(QWidget *)) );
	setCentralWidget(workspace);

	QDockWidget *dock = new QDockWidget(tr("Tone mapping Panel"), this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	connect(actionViewTMdock,SIGNAL(toggled(bool)),dock,SLOT(setShown(bool)));

	TMWidget *tmwidget=new TMWidget(dock, OriginalPfsFrame, statusBar());
	dock->setWidget(tmwidget);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	connect(tmwidget,SIGNAL(newResult(const QImage&, tonemapping_options*)), this,SLOT(addMDIresult(const QImage&,tonemapping_options*)));

	connect(actionAsThumbnails,SIGNAL(triggered()),this,SLOT(viewAllAsThumbnails()));
	connect(actionCascade,SIGNAL(triggered()),workspace,SLOT(cascade()));
	connect(actionFit_to_Window,SIGNAL(toggled(bool)),this,SLOT(current_ldr_fit_to_win(bool)));
	connect(actionFix_Histogram,SIGNAL(toggled(bool)),this,SLOT(LevelsRequested(bool)));
	connect(actionClose_All,SIGNAL(triggered()),this,SLOT(close_all()));
	connect(actionSave, SIGNAL(triggered()),this, SLOT(saveLDR()));
	connect(documentationAction,SIGNAL(triggered()),parent,SLOT(openDocumentation()));
	connect(actionWhat_s_This,SIGNAL(triggered()),this,SLOT(enterWhatsThis()));

	this->showMaximized();
}

void TonemappingWindow::addMDIresult(const QImage& i,tonemapping_options* opts) {
	LdrViewer *n=new LdrViewer(this,i,opts);
	connect(n,SIGNAL(levels_closed()),this,SLOT(levels_closed()));
	workspace->addWindow(n);
	n->show();
}

void TonemappingWindow::current_ldr_fit_to_win(bool checked) {
	LdrViewer* currentLDR=((LdrViewer*)(workspace->activeWindow()));
	if (currentLDR==NULL)
		return;
	currentLDR->fitToWindow(checked);
}

void TonemappingWindow::LevelsRequested(bool checked) {
	if (checked) {
		LdrViewer* currentLDR=((LdrViewer*)(workspace->activeWindow()));
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


void TonemappingWindow::saveLDR() {
	LdrViewer* currentLDR=((LdrViewer*)(workspace->activeWindow()));
	if (currentLDR==NULL)
		return;

	QString outfname = saveLDRImage(prefixname + "_" + currentLDR->getFilenamePostFix()+ ".jpg",currentLDR->getQImage());
	
	//if save is succesful
	if ( outfname.endsWith("jpeg",Qt::CaseInsensitive) || outfname.endsWith("jpg",Qt::CaseInsensitive) ) {
		//time to write the exif data...
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with utf8 characters.
		ExifOperations::writeExifData( QFile::encodeName(outfname).constData(), currentLDR->getExifComment().toStdString() );
	}
}

void TonemappingWindow::updateActions(QWidget *w) {
	actionFix_Histogram->setEnabled(w!=NULL);
	actionSave->setEnabled(w!=NULL);
	actionClose_All->setEnabled(w!=NULL);
	actionAsThumbnails->setEnabled(w!=NULL);
	actionFit_to_Window->setEnabled(w!=NULL);
	actionCascade->setEnabled(w!=NULL);
	if (w!=NULL) {
		LdrViewer* current=(LdrViewer*)(workspace->activeWindow());
		actionFit_to_Window->setChecked(current->getFittingWin());
// 		actionFix_Histogram->setChecked(current->hasLevelsOpen());
	}
}

void TonemappingWindow::closeEvent ( QCloseEvent * ) {
	emit closing();
}

void TonemappingWindow::viewAllAsThumbnails() {
	workspace->tile();
	QWidgetList allLDRresults=workspace->windowList();
	foreach (QWidget *p,allLDRresults) {
		((LdrViewer*)p)->fitToWindow(true);
	}
}

void TonemappingWindow::close_all() {
	QWidgetList allLDRresults=workspace->windowList();
	foreach (QWidget *p,allLDRresults) {
		((LdrViewer*)p)->close();
	}
}

void TonemappingWindow::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

