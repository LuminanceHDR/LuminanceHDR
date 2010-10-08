/*
 * This file is a part of Luminance HDR package.
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QFileDialog>
#include <QMessageBox>
#include <QWhatsThis>
#include <QTextStream>
#include <QCloseEvent>
#include <QSignalMapper>
#include <iostream>

#include "ui_about.h"
#include "Common/config.h"
#include "Common/Gang.h"
#include "Exif/ExifOperations.h"
#include "Filter/pfscut.h"
#include "HelpBrowser/helpbrowser.h"
#include "Threads/TMOFactory.h"
#include "Viewers/GenericViewer.h"
#include "Viewers/LdrViewer.h"
#include "TonemappingWindow.h"

//#include <iostream>

TonemappingWindow::~TonemappingWindow() {
	delete progressbar;
}

TonemappingWindow::TonemappingWindow(QWidget *parent, pfs::Frame* frame, QString filename) : QMainWindow(parent), isLocked(false), changedImage(NULL), threadCounter(0), frameCounter(0), ldrNum(0), hdrNum(0) {
	setupUi(this);

	setWindowTitle("Luminance HDR "LUMINANCEVERSION" - Tonemapping Window - ");

	luminance_options=LuminanceOptions::getInstance();
	//cachepath=LuminanceOptions::getInstance()->tempfilespath;

	tmToolBar->setToolButtonStyle((Qt::ToolButtonStyle)settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt());

	prefixname=QFileInfo(filename).completeBaseName();
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

	dock = new QDockWidget(tr("Tone Mapping Options"), this);
	dock->setObjectName("Tone Mapping Options"); // for save and restore docks state
	//dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	//dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

	threadManager = new ThreadManager(this);

	tmPanel = new TonemappingPanel(dock);
	dock->setWidget(tmPanel);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	tmPanel->setSizes(frame->getWidth(), frame->getHeight());
	
	//original HDR window
	originalHDR = new HdrViewer(this, false, true, luminance_options->negcolor, luminance_options->naninfcolor);
	originalHDR->setFreePfsFrameOnExit(false); // avoid another copy in memory
	originalHDR->updateHDR(frame);
	originalHDR->setFileName(QString(tr("Original HDR")));
	originalHDR->setWindowTitle(QString(tr("Original HDR")));
	originalHDR->setSelectionTool(true);
	originalHDR->normalSize();
	originalHdrSubWin = new QMdiSubWindow(this);
	originalHdrSubWin->setWidget(originalHDR);
	originalHdrSubWin->hide();
	
	restoreState( settings.value("TonemappingWindowState").toByteArray() );
	restoreGeometry(settings.value("TonemappingWindowGeometry").toByteArray());
	actionViewTMdock->setChecked( settings.value("actionViewTMdockState").toBool() );

	qRegisterMetaType<QImage>("QImage");
	qRegisterMetaType<TonemappingOptions>("TonemappingOptions");

	windowMapper = new QSignalMapper(this);

	progressbar = new QProgressBar();

	setupConnections();
}

void TonemappingWindow::setupConnections()
{
	connect(actionText_Under_Icons,SIGNAL(triggered()),this,SLOT(Text_Under_Icons()));
	connect(actionIcons_Only,SIGNAL(triggered()),this,SLOT(Icons_Only()));
	connect(actionText_Alongside_Icons,SIGNAL(triggered()),this,SLOT(Text_Alongside_Icons()));
	connect(actionText_Only,SIGNAL(triggered()),this,SLOT(Text_Only()));
	connect(actionViewTMdock,SIGNAL(toggled(bool)),dock,SLOT(setVisible(bool)));
	//connect(dock->toggleViewAction(),SIGNAL(toggled(bool)),actionViewTMdock,SLOT(setChecked(bool)));
	connect(actionAsThumbnails,SIGNAL(triggered()),this,SLOT(viewAllAsThumbnails()));
	connect(actionCascade,SIGNAL(triggered()),mdiArea,SLOT(cascadeSubWindows()));
	connect(actionFix_Histogram,SIGNAL(toggled(bool)),this,SLOT(LevelsRequested(bool)));
	connect(documentationAction,SIGNAL(triggered()),this,SLOT(openDocumentation()));
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
	connect(actionAbout_Luminance,SIGNAL(triggered()),this,SLOT(aboutLuminance()));
	connect(actionThreadManager,SIGNAL(toggled(bool)),threadManager,SLOT(setVisible(bool)));
	connect(menu_Windows, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

	connect(mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(updateActions(QMdiSubWindow *)) );

	connect(tmPanel,SIGNAL(startTonemapping(const TonemappingOptions&)), this, SLOT(tonemapImage(const TonemappingOptions&)));

	connect(threadManager,SIGNAL(closeRequested(bool)),actionThreadManager,SLOT(setChecked(bool)));

	connect(originalHDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
	connect(originalHDR,SIGNAL(closeRequested(bool)),actionShowHDR,SLOT(setChecked(bool)));

	connect(windowMapper,SIGNAL(mapped(QWidget*)),this,SLOT(setActiveSubWindow(QWidget*)));
}

bool TonemappingWindow::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Close) {
		QMdiSubWindow *w = (QMdiSubWindow *) obj;
		GenericViewer *v = (GenericViewer *) w->widget();
		if (v->isHDR()) {
			if (v != originalHDR) {
				hdrNum--;
			}
		}
		else {
			ldrNum--;
		}
		return true;
	} else {
			//Standard event processing
		return QMainWindow::eventFilter(obj, event);
	}
}

void TonemappingWindow::addMDIResult(const QImage &image)
{
	ldrNum++;
	LdrViewer *n = new LdrViewer( image, this, false, false, tmoOptions);
	connect(n,SIGNAL(levels_closed()),this,SLOT(levels_closed()));
	n->normalSize();
	n->showMaximized(); // That's to have mdi subwin size right (don't ask me why)
	if (actionFit_to_Window->isChecked())
		n->fitToWindow(true);
	QMdiSubWindow *subwin = new QMdiSubWindow(this);
	subwin->setAttribute(Qt::WA_DeleteOnClose);
	subwin->setWidget(n);
	mdiArea->addSubWindow(subwin);
	n->showMaximized();
	
	if (luminance_options->tmowindow_max)
		mdiArea->activeSubWindow()->showMaximized();
	else
		mdiArea->activeSubWindow()->showNormal();

	subwin->installEventFilter(this);
	connect(n,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
}

void TonemappingWindow::addProcessedFrame(pfs::Frame *frame)
{
	hdrNum++;
	HdrViewer *HDR = new HdrViewer(this, false, false, luminance_options->negcolor, luminance_options->naninfcolor);
	HDR->setFreePfsFrameOnExit(true); 
	HDR->updateHDR(frame);
	HDR->setFileName(QString(tr("Processed HDR")));
	HDR->setWindowTitle(QString(tr("Processed HDR")));
	HDR->setSelectionTool(true);
	HDR->normalSize();
	HDR->showMaximized();
	if (actionFit_to_Window->isChecked())
		HDR->fitToWindow(true);
	QMdiSubWindow *HdrSubWin = new QMdiSubWindow(this);
	HdrSubWin->setAttribute(Qt::WA_DeleteOnClose);
	HdrSubWin->setWidget(HDR);
	mdiArea->addSubWindow(HdrSubWin);
	HDR->showMaximized();
	
	if (luminance_options->tmowindow_max)
		mdiArea->activeSubWindow()->showMaximized();
	else
		mdiArea->activeSubWindow()->showNormal();
		
	HdrSubWin->installEventFilter(this);
	connect(HDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
}

void TonemappingWindow::LevelsRequested(bool checked)
{
	if (checked)
  {
		GenericViewer* current = (GenericViewer*) mdiArea->currentSubWindow()->widget();
		if (current==NULL)
			return;
		actionFix_Histogram->setDisabled(true);
		current->levelsRequested(checked);
	}
}

void TonemappingWindow::levels_closed()
{
	actionFix_Histogram->setDisabled(false);
	actionFix_Histogram->setChecked(false);
}

void TonemappingWindow::on_actionSave_triggered()
{
	GenericViewer* current = (GenericViewer*) mdiArea->currentSubWindow()->widget();
	if (current==NULL)
  {
		return;
  }
	if (current->isHDR())
  {
		QMessageBox::critical(this, tr("Luminance HDR"), tr("Please select an LDR image to save."),	QMessageBox::Ok);
		return;
	}

	QString outfname = saveLDRImage(this, prefixname + "_" + current->getFilenamePostFix()+ ".jpg",current->getQImage());

	if (outfname == "")
  {
    //std::cout << "outfname == \"\"" << std::endl;
    return;
  }

	//if save is succesful
	if ( outfname.endsWith("jpeg", Qt::CaseInsensitive) || outfname.endsWith("jpg", Qt::CaseInsensitive) )
  {
		//time to write the exif data...
		//ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
		ExifOperations::writeExifData( QFile::encodeName(outfname).constData(), current->getExifComment().toStdString() );
	}

	QFileInfo fi(outfname);	
	mdiArea->currentSubWindow()->setWindowTitle(fi.fileName());
	current->setFileName(fi.fileName());
	updateWindowMenu();
}

void TonemappingWindow::updateActions(QMdiSubWindow *w)
{
	//std::cout << "UpdateActions" << std::endl;
	GenericViewer *viewer;

	if (w)
		viewer = (GenericViewer *) w->widget();
	else
		viewer = NULL;

	int mdiNum = mdiArea->subWindowList().size(); // mdi number
	bool isHdrVisible = !originalHdrSubWin->isHidden();
	bool more_than_one = (mdiNum > 1); // more than 1 Image
	bool noLdr = !(ldrNum == 0); // no LDRs
	//bool noHdr = !(hdrNum == 0); // no HDRs
	//int hdr_num = (isHdrVisible ? hdrNum+1 : hdrNum); // hdr number

	bool isHDR = false;
	if (viewer) 
		isHDR = viewer->isHDR();

	//std::cout << "viewer: " << viewer << std::endl;
	//std::cout << "mdiNum: " << mdiNum << std::endl;
	//std::cout << "ldrNum: " << ldrNum << std::endl;
	//std::cout << "hdrNum: " << hdrNum << std::endl;
	//std::cout << "isHdrVisible: " << isHdrVisible << std::endl;
	//std::cout << "more_than_one: " << more_than_one << std::endl;
	//std::cout << "noLdr: " << noLdr << std::endl;
	//std::cout << "noHdr: " << noHdr << std::endl;
	//std::cout << "hdr_num: " << hdr_num << std::endl;
	//std::cout << "isHDR: " << isHDR << std::endl;
	//std::cout << std::endl;

	actionFix_Histogram->setEnabled( !isHDR && ldrNum );
	actionSave->setEnabled( !isHDR && ldrNum );
	actionSaveAll->setEnabled( !isHDR && ldrNum );
	actionClose_All->setEnabled( hdrNum || ldrNum );
	actionAsThumbnails->setEnabled( mdiNum );
	actionCascade->setEnabled( more_than_one );
	actionShowNext->setEnabled( more_than_one || (noLdr && isHdrVisible) );
	actionShowPrevious->setEnabled( more_than_one || (noLdr && isHdrVisible) );
	
	if (w != NULL) {
		current = (GenericViewer*) w->widget(); 
		if ( current->isFittedToWindow())
			actionFit_to_Window->setChecked(true); 
		else
			actionFit_to_Window->setChecked(false); 

		//actionFix_Histogram->setChecked(current->hasLevelsOpen());
	}
}

void TonemappingWindow::closeEvent ( QCloseEvent *event ) {
	settings.setValue("TonemappingWindowState", saveState());
	settings.setValue("TonemappingWindowGeometry", saveGeometry());
	settings.setValue("actionViewTMdockState",actionViewTMdock->isChecked());
	QWidget::closeEvent(event);
	emit closing();
}

void TonemappingWindow::viewAllAsThumbnails() {
	mdiArea->tileSubWindows();
	QList<QMdiSubWindow*> allViewers = mdiArea->subWindowList();
	foreach (QMdiSubWindow *p, allViewers) {
		((GenericViewer*)p->widget())->fitToWindow(true);
	}
	actionFit_to_Window->setChecked(true);
}

void TonemappingWindow::on_actionClose_All_triggered() {
	QList<QMdiSubWindow*> allViewers = mdiArea->subWindowList();
	foreach (QMdiSubWindow *p, allViewers) {
		disconnect(p->widget(),SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
		p->close();
	}
	actionClose_All->setEnabled(false);
	if (!mdiArea->subWindowList().isEmpty())
		updateActions(0);
}

void TonemappingWindow::on_actionSaveAll_triggered()
{
	QString dir = QFileDialog::getExistingDirectory(0,
                                                  tr("Save files in"),
                                                  settings.value(KEY_RECENT_PATH_SAVE_LDR,QDir::currentPath()).toString());
  
	if (!dir.isEmpty())
  {
		settings.setValue(KEY_RECENT_PATH_SAVE_LDR, dir);
		QList<QMdiSubWindow*> allLDRs = mdiArea->subWindowList();
    
		foreach (QMdiSubWindow *p, allLDRs)
    {
			if ( p != originalHdrSubWin )
      { //skips the hdr
			  GenericViewer* ldr = (GenericViewer*) p->widget();
			  QString outfname = saveLDRImage(this, prefixname + "_" + ldr->getFilenamePostFix()+ ".jpg",ldr->getQImage(), true);
			  //if save is succesful
			  if ( outfname.endsWith("jpeg", Qt::CaseInsensitive) || outfname.endsWith("jpg", Qt::CaseInsensitive) )
        {
          //time to write the exif data...
          //ExifOperations methods want a std::string, we need to use the QFile::encodeName(QString).constData() trick to cope with local 8-bit encoding determined by the user's locale.
          ExifOperations::writeExifData( QFile::encodeName(outfname).constData(), ldr->getExifComment().toStdString() );
			  }
			}
		}
	}
}

void TonemappingWindow::enterWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

void TonemappingWindow::showHDR(bool toggled) {
	int n = mdiArea->subWindowList().size();
	if (toggled) {
		if (isLocked)  
			connect(originalHDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
		actionShowNext->setEnabled( toggled && (n > 1));
		actionShowPrevious->setEnabled( toggled && (n > 1));
		originalHDR->normalSize();
		if (actionFit_to_Window->isChecked()) 
			originalHDR->fitToWindow(true);
		else
			originalHDR->fitToWindow(false);
		mdiArea->addSubWindow(originalHdrSubWin);
		originalHdrSubWin->showMaximized();
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
	if (!mdiArea->subWindowList().isEmpty()) {
		GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
		viewer->zoomIn();
		actionZoom_Out->setEnabled(true);
		actionZoom_In->setEnabled(viewer->getScaleFactor() < 3.0);
	}
}

void TonemappingWindow::current_mdi_zoomout() {
	if (!mdiArea->subWindowList().isEmpty()) {
		GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
		viewer->zoomOut();
		actionZoom_In->setEnabled(true);
		actionZoom_Out->setEnabled(viewer->getScaleFactor() > 0.222);
	}
}

void TonemappingWindow::current_mdi_fit_to_win(bool checked) {
	if (isLocked) {
		QList<QMdiSubWindow*> allViewers = mdiArea->subWindowList();
		foreach (QMdiSubWindow *p, allViewers) {
			GenericViewer *viewer = (GenericViewer*)p->widget();
			viewer->fitToWindow(checked);
		}
	}
	else {
		if (!mdiArea->subWindowList().isEmpty()) {
			GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
			viewer->fitToWindow(checked);
		}
	}
	actionZoom_In->setEnabled(!checked);
	actionZoom_Out->setEnabled(!checked);
	actionNormal_Size->setEnabled(!checked);
}

void TonemappingWindow::current_mdi_original_size() {
	if (!mdiArea->subWindowList().isEmpty()) {
		GenericViewer *viewer = (GenericViewer *) mdiArea->currentSubWindow()->widget();
		viewer->normalSize();
		actionZoom_In->setEnabled(true);
		actionZoom_Out->setEnabled(true);
	}
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
	if (!mdiArea->subWindowList().isEmpty())
		dispatch((GenericViewer *) mdiArea->currentSubWindow()->widget());
}

void TonemappingWindow::updateImage(GenericViewer *viewer) {
	assert(viewer!=NULL);
	assert(changedImage!=NULL);
	if (isLocked) {
		scaleFactor = changedImage->getImageScaleFactor();
		HSB_Value = changedImage->getHorizScrollBarValue();
		VSB_Value = changedImage->getVertScrollBarValue();
		viewer->normalSize();
		if (actionFit_to_Window->isChecked())
			viewer->fitToWindow(true);
		else
			viewer->fitToWindow(false);
		viewer->zoomToFactor(scaleFactor);	
		viewer->setHorizScrollBarValue(HSB_Value);	
		viewer->setVertScrollBarValue(VSB_Value);
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

void TonemappingWindow::openDocumentation() {
	HelpBrowser *helpBrowser = new HelpBrowser(this,"Luminance HDR Help");
	helpBrowser->setAttribute(Qt::WA_DeleteOnClose);
	helpBrowser->show();
}

void TonemappingWindow::aboutLuminance() {
	QDialog *about=new QDialog(this);
	about->setAttribute(Qt::WA_DeleteOnClose);
	Ui::AboutLuminance ui;
	ui.setupUi(about);
	ui.authorsBox->setOpenExternalLinks(true);
	ui.thanksToBox->setOpenExternalLinks(true);
	ui.GPLbox->setTextInteractionFlags(Qt::TextSelectableByMouse);
	ui.label_version->setText(ui.label_version->text().append(QString(LUMINANCEVERSION)));

	bool license_file_not_found=true;
	QString docDir = QCoreApplication::applicationDirPath();
	docDir.append("/../Resources");
	QStringList paths = QStringList("/usr/share/luminance") << "/usr/local/share/luminance" << docDir << "/Applications/luminance.app/Contents/Resources" << "./";
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


pfs::Frame * TonemappingWindow::getSelectedFrame(HdrViewer *hdr) {
	assert( hdr != NULL );
	pfs::Frame *frame = hdr->getHDRPfsFrame();
	QRect cropRect = hdr->getSelectionRect();
	int x_ul, y_ul, x_br, y_br;
	cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
	return pfscut(frame, x_ul, y_ul, x_br, y_br);
}
	
void TonemappingWindow::tonemapImage(const TonemappingOptions &opts ) {
	pfs::DOMIO pfsio;
	tmoOptions = &opts;	

	if ( (opts.tonemapOriginal) && (!opts.tonemapSelection) )  {
 		workingPfsFrame = originalHDR->getHDRPfsFrame();
	}
	else if ( (!opts.tonemapOriginal) && (!opts.tonemapSelection) ){
		if (!mdiArea->subWindowList().isEmpty()) {
			GenericViewer *viewer = (GenericViewer *) mdiArea->activeSubWindow()->widget();

			if (viewer->isHDR()) {
					HdrViewer *HDR = (HdrViewer *) mdiArea->activeSubWindow()->widget();
					workingPfsFrame = HDR->getHDRPfsFrame();
			}		
			else {
				QMessageBox::critical(this,tr("Luminance HDR"),tr("Please select an HDR image to tonemap."),
					QMessageBox::Ok);
				return;
			}
		}
		else {
			QMessageBox::critical(this,tr("Luminance HDR"),tr("Please select an HDR image to tonemap."),
				QMessageBox::Ok);
			return;
		}
	} 
	else if (opts.tonemapOriginal && opts.tonemapSelection) {
		if (originalHDR->hasSelection()) {
			workingPfsFrame = getSelectedFrame(originalHDR);
		}
		else {
			QMessageBox::critical(this,tr("Luminance HDR"),tr("Please make a selection of the HDR image to tonemap."),
				QMessageBox::Ok);
			return;
		}
	}
	else {
		if (!mdiArea->subWindowList().isEmpty()) {
			GenericViewer *viewer = (GenericViewer *) mdiArea->activeSubWindow()->widget();

			if (viewer->isHDR()) {
				HdrViewer *HDR = (HdrViewer *) mdiArea->activeSubWindow()->widget();
				if (HDR->hasSelection()) {
					workingPfsFrame = getSelectedFrame(HDR);
				}
				else {
					QMessageBox::critical(this,tr("Luminance HDR"),tr("Please make a selection of the HDR image to tonemap."), 
						QMessageBox::Ok);
					return;
				}
			}		
			else {
				QMessageBox::critical(this,tr("Luminance HDR"),tr("Please select an HDR image to tonemap."),
					QMessageBox::Ok);
				return;
			}
		}
		else {
			QMessageBox::critical(this,tr("Luminance HDR"),tr("Please select an HDR image to tonemap."),
				QMessageBox::Ok);
			return;
		}
	}

	TMOThread *thread = TMOFactory::getTMOThread(opts.tmoperator, workingPfsFrame, opts);
	TMOProgressIndicator *progInd =	new TMOProgressIndicator(this, QString(opts.tmoperator_str));
	threadManager->addProgressIndicator(progInd);

	connect(thread, SIGNAL(imageComputed(const QImage&)), this, SLOT(addMDIResult(const QImage&)));
	connect(thread, SIGNAL(processedFrame(pfs::Frame *)), this, SLOT(addProcessedFrame(pfs::Frame *)));
	connect(thread, SIGNAL(setMaximumSteps(int)), progInd, SLOT(setMaximum(int)));
	connect(thread, SIGNAL(setValue(int)), progInd, SLOT(setValue(int)));
	connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showErrorMessage(const char *)));
	connect(thread, SIGNAL(finished()), progInd, SLOT(terminated()));
	connect(thread, SIGNAL(finished()), this, SLOT(tonemappingFinished()));
	connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
	connect(progInd, SIGNAL(terminate()), thread, SLOT(terminateRequested()));

	//start thread
	thread->startTonemapping();
	threadCounter++;
	if (threadCounter == 1) { 
		statusbar->insertWidget(0,progressbar);
		progressbar->setMaximum(0);
		progressbar->setMinimum(0);
		progressbar->setValue(-1);
		progressbar->show();
	}
}

void TonemappingWindow::showErrorMessage(const char *e) {
	QMessageBox::critical(this,tr("Luminance HDR"),tr("Error: %1").arg(e),
		QMessageBox::Ok,QMessageBox::NoButton);
	threadCounter--;
	updateProgressbar();
}

void TonemappingWindow::tonemappingFinished() {
	std::cout << "TonemappingWindow::tonemappingFinished()" << std::endl;
	threadCounter--;
	updateProgressbar();
}

void TonemappingWindow::deleteTMOThread(TMOThread *th) {
	delete th;
}

void TonemappingWindow::updateProgressbar() {
	if (threadCounter == 0) {
		statusbar->removeWidget(progressbar);
	}
}

void TonemappingWindow::updateWindowMenu() {
	menu_Windows->clear();
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		GenericViewer *child = qobject_cast<GenericViewer *>(windows.at(i)->widget());
		QString text=QString((i < 9)?"&":"") + QString("%1 %2").arg(i + 1).arg(QFileInfo((child->getFileName().isEmpty())? tr("Untitled"):child->getFileName()).fileName());
		QAction *action  = menu_Windows->addAction(text);
		action->setCheckable(true);
		action->setChecked(child==current);
		connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
		windowMapper->setMapping(action, child);
	}
	menu_Windows->addSeparator();
	menu_Windows->addAction(actionAsThumbnails);
	menu_Windows->addAction(actionCascade);
}

void TonemappingWindow::setActiveSubWindow(QWidget* w) {
	QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
	foreach(QMdiSubWindow *p,allhdrs)
		if (p->widget() == w)
			mdiArea->setActiveSubWindow(p);
}
