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
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QWhatsThis>
#include "tonemappingdialog_impl.h"
#include "ldrviewer.h"
#include "../options.h"
#include "../Exif/exif_operations.h"
#include "../config.h"

TonemappingWindow::~TonemappingWindow() {}

TonemappingWindow::TonemappingWindow(QWidget *parent, pfs::Frame* &OriginalPfsFrame, QString cachepath, QString _file) : QMainWindow(parent), settings("Qtpfsgui", "Qtpfsgui") {
	setupUi(this);
	switch (settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt()) {
	case Qt::ToolButtonIconOnly:
		toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
		settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonIconOnly);
		break;
	case Qt::ToolButtonTextOnly:
		toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
		settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextOnly);
		break;
	case Qt::ToolButtonTextBesideIcon:
		toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextBesideIcon);
		break;
	case Qt::ToolButtonTextUnderIcon:
		toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
		settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
		break;
	}

	prefixname=QFileInfo(_file).completeBaseName();
	setWindowTitle(tr("Tone mapping Window: ")+ prefixname);
	recentPathSaveLDR=settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString();

	workspace = new QWorkspace(this);
	workspace->setScrollBarsEnabled( TRUE );
	connect(workspace,SIGNAL(windowActivated(QWidget*)), this, SLOT(updateActions(QWidget *)) );
	setCentralWidget(workspace);

	QDockWidget *dock = new QDockWidget(tr("Tone mapping Panel"), this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	connect(actionViewTMdock,SIGNAL(toggled(bool)),dock,SLOT(setShown(bool)));

	TMWidget *tmwidget=new TMWidget(dock, OriginalPfsFrame, cachepath, statusBar());
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
	((LdrViewer*)(workspace->activeWindow()))->fitToWindow(checked);
}

void TonemappingWindow::LevelsRequested(bool checked) {
	if (checked) {
		((LdrViewer*)(workspace->activeWindow()))->LevelsRequested(checked);
		actionFix_Histogram->setDisabled(true);
	}
}

void TonemappingWindow::levels_closed() {
// 	qDebug("levels_closed");
	actionFix_Histogram->setDisabled(false);
	actionFix_Histogram->setChecked(false);
}


void TonemappingWindow::saveLDR() {
	LdrViewer* currentLDR=((LdrViewer*)(workspace->activeWindow()));
	QStringList filetypes;
	filetypes += tr("All LDR formats (*.jpg *.jpeg *.png *.ppm *.pbm *.bmp)");
	filetypes += "JPEG (*.jpg *.jpeg)";
	filetypes += "PNG (*.png)";
	filetypes += "PPM PBM (*.ppm *.pbm)";
	filetypes += "BMP (*.bmp)";
	QFileDialog *fd = new QFileDialog(this);
	fd->setWindowTitle(tr("Save the LDR to..."));
	fd->setDirectory( recentPathSaveLDR );
	fd->selectFile(prefixname + "_" + currentLDR->getFilenamePostFix()+ ".jpg");
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilters(filetypes);
	fd->setAcceptMode(QFileDialog::AcceptSave);
	fd->setConfirmOverwrite(true);
	if (fd->exec()) {
		QString outfname=(fd->selectedFiles()).at(0);
		if(!outfname.isEmpty()) {
			QFileInfo qfi(outfname);
			// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
			if (recentPathSaveLDR != qfi.path()) {
				// update internal field variable
				recentPathSaveLDR=qfi.path();
				//save settings
				settings.setValue(KEY_RECENT_PATH_SAVE_LDR, recentPathSaveLDR);
			}
			QString format=qfi.suffix();
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
			if(!((currentLDR->getQImage())->save(outfname,format.toAscii().constData(),100))) {
				QMessageBox::warning(this,"",tr("Failed to save <b>") + outfname + "</b>",
						QMessageBox::Ok, QMessageBox::NoButton);
			} else { //save is succesful
				if (format=="jpeg" || format=="jpg") {
					//time to write the exif data...
					LdrViewer* currentLDR = ((LdrViewer*)(workspace->activeWindow()));
					ExifOperations::writeExifData( outfname.toStdString(), currentLDR->getExifComment().toStdString() );
				}
			}
		}
	}
	delete fd;
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

