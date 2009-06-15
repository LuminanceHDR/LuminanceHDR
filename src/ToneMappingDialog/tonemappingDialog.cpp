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

#include <QFileDialog>
#include <QMessageBox>
#include <QWhatsThis>
#include <QTextStream>
#include "tonemappingDialog.h"
#include "ldrviewer.h"
#include "../Exif/exif_operations.h"
#include "../Common/genericViewer.h"
#include "../Common/config.h"
#include "../Common/gang.h"
#include "../generated_uic/ui_about.h"

//#include <iostream> // for debug

TonemappingWindow::~TonemappingWindow() {
}

TonemappingWindow::TonemappingWindow(QWidget *parent, pfs::Frame* pfsFrame, QString _file) : QMainWindow(parent), isLocked(false), changedImage(NULL) {
	setupUi(this);

	qtpfsgui_options=QtpfsguiOptions::getInstance();
	cachepath=QtpfsguiOptions::getInstance()->tempfilespath;

	tmToolBar->setToolButtonStyle((Qt::ToolButtonStyle)settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt());

	prefixname=QFileInfo(_file).completeBaseName();
	setWindowTitle(windowTitle() + prefixname);
	recentPathSaveLDR=settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString();
	
	//main toolbar setup
	QActionGroup *tmToolBarOptsGroup = new QActionGroup(this);
	tmToolBarOptsGroup->addAction(actionText_Under_Icons);
	tmToolBarOptsGroup->addAction(actionIcons_Only);
	tmToolBarOptsGroup->addAction(actionText_Alongside_Icons);
	tmToolBarOptsGroup->addAction(actionText_Only);
	menuToolbars->addAction(tmToolBar->toggleViewAction());

	load_options();

	mdiArea = new QMdiArea(this);
	mdiArea->setBackground(QBrush(QColor::fromRgb(192, 192, 192)) );
	mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setCentralWidget(mdiArea);

	dock = new QDockWidget(tr("Tone mapping Panel"), this);
	dock->setObjectName("Tone Mapping Panel"); // for save and restore docks state
	//dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	//menuToolbars->addAction(dock->toggleViewAction());

	//swap original frame to hd
	pfs::DOMIO pfsio;
	pfsio.writeFrame(pfsFrame, QFile::encodeName(cachepath+"/original.pfs").constData());
	
	//re-read original frame
	originalPfsFrame = pfsio.readFrame(QFile::encodeName(cachepath+"/original.pfs").constData());
	if (!originalPfsFrame) // TODO: show a warning and close
		close();

	tmwidget = new TMWidget(dock, statusbar );
	dock->setWidget(tmwidget);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	
	// original HDR window
	originalHDR = new HdrViewer(this,false,true, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor);
	originalHDR->updateHDR(originalPfsFrame);
        originalHDR->setFileName(QString(tr("Original HDR")));
        originalHDR->setWindowTitle(QString(tr("Original HDR")));
	originalHDR->normalSize();
	originalHDR->showMaximized();
	originalHDR->fitToWindow(true);
	originalHdrSubWin = new QMdiSubWindow(this);
	originalHdrSubWin->setWidget(originalHDR);
	originalHdrSubWin->hide();
	connect(originalHDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
	
	restoreState( settings.value("TonemappingWindowState").toByteArray() );
	actionViewTMdock->setChecked( settings.value("actionViewTMdockState").toBool() );
	int x = settings.value("TonemappinWindowPosX").toInt();
	int y = settings.value("TonemappinWindowPosY").toInt();
	int w = settings.value("TonemappinWindowWidth").toInt();
	int h = settings.value("TonemappinWindowHeight").toInt();

	if (w==0) w=800;
	if (h==0) h=600;

	setGeometry(x, y, w, h);

	setupConnections();
}

void TonemappingWindow::setupConnections() {
	connect(actionText_Under_Icons,SIGNAL(triggered()),this,SLOT(Text_Under_Icons()));
	connect(actionIcons_Only,SIGNAL(triggered()),this,SLOT(Icons_Only()));
	connect(actionText_Alongside_Icons,SIGNAL(triggered()),this,SLOT(Text_Alongside_Icons()));
	connect(actionText_Only,SIGNAL(triggered()),this,SLOT(Text_Only()));

	connect(mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateActions(QMdiSubWindow *)) );

	connect(actionViewTMdock,SIGNAL(toggled(bool)),dock,SLOT(setVisible(bool)));
	connect(dock->toggleViewAction(),SIGNAL(toggled(bool)),actionViewTMdock,SLOT(setChecked(bool)));
	connect(tmwidget,SIGNAL(newResult(const QImage&, tonemapping_options*)), this,SLOT(addMDIresult(const QImage&, tonemapping_options*)));

	connect(actionAsThumbnails,SIGNAL(triggered()),this,SLOT(viewAllAsThumbnails()));
	connect(actionCascade,SIGNAL(triggered()),mdiArea,SLOT(cascadeSubWindows()));
	connect(actionFix_Histogram,SIGNAL(toggled(bool)),this,SLOT(LevelsRequested(bool)));
	connect(documentationAction,SIGNAL(triggered()),parent(),SLOT(openDocumentation()));
	connect(actionWhat_s_This,SIGNAL(triggered()),this,SLOT(enterWhatsThis()));
	connect(actionShowHDR,SIGNAL(toggled(bool)),this,SLOT(showHDR(bool)));
	connect(actionShowNext,SIGNAL(triggered()),mdiArea,SLOT(activateNextSubWindow()));
	connect(actionShowPrevious,SIGNAL(triggered()),mdiArea,SLOT(activatePreviousSubWindow()));
	connect(actionZoom_In,SIGNAL(triggered()),this,SLOT(current_mdi_zoomin()));
	connect(actionZoom_Out,SIGNAL(triggered()),this,SLOT(current_mdi_zoomout()));
	connect(actionFit_to_Window,SIGNAL(toggled(bool)),this,SLOT(current_mdi_fit_to_win(bool)));
	connect(actionNormal_Size,SIGNAL(triggered()),this,SLOT(current_mdi_original_size()));
	connect(actionLockImages,SIGNAL(toggled(bool)),this,SLOT(lockImages(bool)));
	connect(actionAbout_Qt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));
	connect(actionAbout_Qtpfsgui,SIGNAL(triggered()),this,SLOT(aboutQtpfsgui()));
}

void TonemappingWindow::addMDIresult(const QImage& i,tonemapping_options *opts) {
	LdrViewer *n = new LdrViewer( i, this, false, false, opts);
	connect(n,SIGNAL(levels_closed()),this,SLOT(levels_closed()));
	n->normalSize();	
	n->showMaximized();
	n->fitToWindow(true);
	QMdiSubWindow *subwin = new QMdiSubWindow(this);
	subwin->setAttribute(Qt::WA_DeleteOnClose);
	subwin->setWidget(n);
	mdiArea->addSubWindow(subwin);
	n->show();
	mdiArea->activeSubWindow()->showMaximized();
	connect(n,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
}

void TonemappingWindow::LevelsRequested(bool checked) {
	//std::cout << "TonemappingWindow::LevelsRequested" << std::endl;
	if (checked) {
		GenericViewer* current = (GenericViewer*) mdiArea->currentSubWindow()->widget();
		if (current==NULL)
			return;
		actionFix_Histogram->setDisabled(true);
		current->levelsRequested(checked);
	}
}

void TonemappingWindow::levels_closed() {
	//std::cout << "TonemappingWindow::levels_closed" << std::endl;
	actionFix_Histogram->setDisabled(false);
	actionFix_Histogram->setChecked(false);
}

void TonemappingWindow::on_actionSave_triggered() {
	GenericViewer* current = (GenericViewer*) mdiArea->currentSubWindow()->widget();
	if (current==NULL)
		return;
	if (current == originalHDR)
		return;

	QString outfname = saveLDRImage(prefixname + "_" + current->getFilenamePostFix()+ ".jpg",current->getQImage());

	//if save is succesful
	if ( outfname.endsWith("jpeg",Qt::CaseInsensitive) || outfname.endsWith("jpg",Qt::CaseInsensitive) ) {
		//time to write the exif data...
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		ExifOperations::writeExifData( QFile::encodeName(outfname).constData(), current->getExifComment().toStdString() );
	}
}

void TonemappingWindow::updateActions(QMdiSubWindow *w) {
	int mdi_num = mdiArea->subWindowList().size(); // mdi number
	bool isHdrVisible = !originalHdrSubWin->isHidden();
	int ldr_num = (isHdrVisible ? mdi_num-1 : mdi_num); // ldr number
	bool more_than_one  = (mdi_num>1); // more than 1 Visible Image 
	bool ldr = !(ldr_num==0); // no LDRs
	bool isNotHDR = (w!=originalHdrSubWin);
	//
	//std::cout << "TonemappingWindow::updateActions" << std::endl;
	//std::cout << "mdi_num: " << mdi_num << std::endl;
	//std::cout << "isHdrVisible: " << isHdrVisible << std::endl;
	//std::cout << "ldr_num: " << ldr_num << std::endl;
	//std::cout << "more_than_one: " << more_than_one << std::endl;
	//std::cout << "ldr: " << ldr << std::endl;
	//
	actionFix_Histogram->setEnabled( isNotHDR && ldr );
	actionSave->setEnabled( isNotHDR && ldr );
	actionSaveAll->setEnabled( ldr );
	actionClose_All->setEnabled( ldr );
	actionAsThumbnails->setEnabled( ldr );
	actionLockImages->setEnabled( ldr );
	actionFit_to_Window->setEnabled( ldr || isHdrVisible ); 
	actionCascade->setEnabled( more_than_one );
	actionShowNext->setEnabled( more_than_one || (ldr && isHdrVisible) );
	actionShowPrevious->setEnabled( more_than_one || (ldr && isHdrVisible) );
	if (w!=NULL) {
		GenericViewer *current = (GenericViewer*) w->widget(); 
		actionFit_to_Window->setChecked(current->getFittingWin());
		//actionFix_Histogram->setChecked(current->hasLevelsOpen());
	}
}

void TonemappingWindow::closeEvent ( QCloseEvent * ) {
	settings.setValue("TonemappingWindowState", saveState());
	settings.setValue("actionViewTMdockState",actionViewTMdock->isChecked());
	settings.setValue("TonemappinWindowPosX",x());
	settings.setValue("TonemappinWindowPosY",y());
	settings.setValue("TonemappinWindowWidth",width());
	settings.setValue("TonemappinWindowHeight",height());
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
	//std::cout << "TonemappingWindow::on_actionClose_All_triggered" << std::endl;
	QList<QMdiSubWindow*> allLDRs = mdiArea->subWindowList();
	foreach (QMdiSubWindow *p, allLDRs) {
		p->close();
	}
	actionClose_All->setEnabled(false);
	if (!mdiArea->subWindowList().isEmpty())
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
		actionShowNext->setEnabled( toggled && (n > 1));
		actionShowPrevious->setEnabled( toggled && (n > 1));
		originalHDR->normalSize();
		originalHDR->fitToWindow(true);
		mdiArea->addSubWindow(originalHdrSubWin);
		originalHdrSubWin->showMaximized();
		if (isLocked) { 
			//std::cout << "originalHDR: " << originalHDR << std::endl;
			connect(originalHDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
			//GenericViewer *current = (GenericViewer *) mdiArea->activeSubWindow();
			//assert(current!=NULL);
			//dispatch(current);
		}
	}
	else { 
		disconnect(originalHDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
		originalHdrSubWin->hide();
		actionShowNext->setEnabled(n > 2);
		actionShowPrevious->setEnabled(n > 2);
		mdiArea->removeSubWindow(originalHdrSubWin);
		if (!mdiArea->subWindowList().isEmpty())
			mdiArea->activeSubWindow()->showMaximized();
	}
}

void TonemappingWindow::Text_Under_Icons() {
	tmToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	settings.setValue(KEY_TM_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
}

void TonemappingWindow::Icons_Only() {
	tmToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	settings.setValue(KEY_TM_TOOLBAR_MODE,Qt::ToolButtonIconOnly);
}

void TonemappingWindow::Text_Alongside_Icons() {
	tmToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	settings.setValue(KEY_TM_TOOLBAR_MODE,Qt::ToolButtonTextBesideIcon);
}

void TonemappingWindow::Text_Only() {
	tmToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
	settings.setValue(KEY_TM_TOOLBAR_MODE,Qt::ToolButtonTextOnly);
}

void TonemappingWindow::current_mdi_zoomin() {
	GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
	viewer->zoomIn();
	actionZoom_Out->setEnabled(true);
	actionZoom_In->setEnabled(viewer->getScaleFactor() < 3.0);
}

void TonemappingWindow::current_mdi_zoomout() {
	GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
	viewer->zoomOut();
	actionZoom_In->setEnabled(true);
	actionZoom_Out->setEnabled(viewer->getScaleFactor() > 0.222);
}

void TonemappingWindow::current_mdi_fit_to_win(bool checked) {
	GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
	viewer->fitToWindow(checked);
	actionZoom_In->setEnabled(!checked);
	actionZoom_Out->setEnabled(!checked);
	actionNormal_Size->setEnabled(!checked);
}

void TonemappingWindow::current_mdi_original_size() {
	GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
	viewer->normalSize();
	actionZoom_In->setEnabled(true);
	actionZoom_Out->setEnabled(true);
}

void TonemappingWindow::load_options() {
	//load from settings the toolbar visualization mode
	if (!settings.contains(KEY_TM_TOOLBAR_MODE))
		settings.setValue(KEY_TM_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
	switch (settings.value(KEY_TM_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt()) {
	case Qt::ToolButtonIconOnly:
		Icons_Only();
		actionIcons_Only->setChecked(true);
	break;
	case Qt::ToolButtonTextOnly:
		Text_Only();
		actionText_Only->setChecked(true);
	break;
	case Qt::ToolButtonTextBesideIcon:
		Text_Alongside_Icons();
		actionText_Alongside_Icons->setChecked(true);
	break;
	case Qt::ToolButtonTextUnderIcon:
		Text_Under_Icons();
		actionText_Under_Icons->setChecked(true);
	break;
	}
}

void TonemappingWindow::lockImages(bool toggled) {
	isLocked = toggled;
	dispatch((GenericViewer *) mdiArea->currentSubWindow()->widget());
}

void TonemappingWindow::updateImage(GenericViewer *viewer) {
	assert(viewer!=NULL);
	assert(changedImage!=NULL);
	if (isLocked) {
		//std::cout << "TonemappingWindow::updateImage: locked" << std::endl;
		scaleFactor = changedImage->getImageScaleFactor();
		HSB_Value = changedImage->getHorizScrollBarValue();
		VSB_Value = changedImage->getVertScrollBarValue();
		viewer->normalSize();
		viewer->fitToWindow(false);
		viewer->zoomToFactor(scaleFactor);	
		viewer->setHorizScrollBarValue(HSB_Value);	
		viewer->setVertScrollBarValue(VSB_Value);
		//TODO: disable fitTowin action
	}
}

void TonemappingWindow::dispatch(GenericViewer *sender) {
	QList<QMdiSubWindow*> allViewers = mdiArea->subWindowList();
	changedImage = sender;
	foreach (QMdiSubWindow *p, allViewers) {
		GenericViewer *viewer = (GenericViewer*)p->widget();
		if (sender != viewer) {
			disconnect(viewer,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
			updateImage(viewer);
			connect(viewer,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
		}
	}
}

void TonemappingWindow::aboutQtpfsgui() {
	QDialog *about=new QDialog();
	about->setAttribute(Qt::WA_DeleteOnClose);
	Ui::AboutQtpfsgui ui;
	ui.setupUi(about);
	ui.authorsBox->setOpenExternalLinks(true);
	ui.thanksToBox->setOpenExternalLinks(true);
	ui.GPLbox->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui.label_version->setText(ui.label_version->text().append(QString(QTPFSGUIVERSION)));

        bool license_file_not_found=true;
	QString docDir = QCoreApplication::applicationDirPath();
	docDir.append("/../Resources");
	QStringList paths = QStringList("/usr/share/qtpfsgui") << "/usr/local/share/qtpfsgui" << docDir << "/Applications/qtpfsgui.app/Contents/Resources" << "./";
	foreach (QString path,paths) {
		QString fname(path+QString("/LICENSE"));
#ifdef WIN32
		fname+=".txt";
#endif
		if (QFile::exists(fname)) {
			QFile file(fname);
			//try opening it
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				break;
			QTextStream ts(&file);
			ui.GPLbox->setAcceptRichText(false);
			ui.GPLbox->setPlainText(ts.readAll());
			license_file_not_found=false;
			break;
		}
	}
	if (license_file_not_found) {
		ui.GPLbox->setOpenExternalLinks(true);
		ui.GPLbox->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui.GPLbox->setHtml(tr("%1 License document not found, you can find it online: %2here%3","%2 and %3 are html tags").arg("<html>").arg("<a href=\"http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\">").arg("</a></html>"));
	}
	about->show();
}

