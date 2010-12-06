/**
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

//#include <iostream>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QWhatsThis>
#include <QSignalMapper>
#include <QTextStream>
#include <QDesktopServices>

#include "ui_about.h"
#include "ui_Splash.h"
#include "Common/archs.h"
#include "Common/config.h"
#include "Common/global.h"
#include "Batch/BatchTMDialog.h"
#include "Fileformat/pfs_file_format.h"
#include "Filter/pfscut.h"
#include "Filter/pfsrotate.h"
#include "Threads/LoadHdrThread.h"
#include "TonemappingWindow/TonemappingWindow.h"
#include "TransplantExif/TransplantExifDialog.h"
#include "Viewers/HdrViewer.h"
#include "MainWindow.h"
#include "DnDOption.h"

MainWindow::MainWindow(QWidget *p) : QMainWindow(p), currenthdr(NULL), helpBrowser(NULL)
{
	setupUi(this);

	QDir dir(QDir::homePath());

#ifdef WIN32
	if (!dir.exists("LuminanceHDR"))
		dir.mkdir("LuminanceHDR");
#else
	if (!dir.exists(".LuminanceHDR"))
		dir.mkdir(".LuminanceHDR");
#endif

	restoreState(settings.value("MainWindowState").toByteArray());
	restoreGeometry(settings.value("MainWindowGeometry").toByteArray());

	setAcceptDrops(true);
	windowMapper = new QSignalMapper(this);

	//main toolbar setup
	QActionGroup *toolBarOptsGroup = new QActionGroup(this);
	toolBarOptsGroup->addAction(actionText_Under_Icons);
	toolBarOptsGroup->addAction(actionIcons_Only);
	toolBarOptsGroup->addAction(actionText_Alongside_Icons);
	toolBarOptsGroup->addAction(actionText_Only);
	menuToolbars->addAction(toolBar->toggleViewAction());

  setUnifiedTitleAndToolBarOnMac(true);
  
	mdiArea = new QMdiArea(this);
	mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setBackground(QBrush(QColor::fromRgb(192, 192, 192)) );
	setCentralWidget(mdiArea);

	luminance_options = LuminanceOptions::getInstance();
	load_options();

	setWindowTitle("Luminance HDR "LUMINANCEVERSION);

	//recent files
	for (int i = 0; i < MaxRecentFiles; ++i)
  {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
	separatorRecentFiles = menuFile->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
		menuFile->addAction(recentFileActs[i]);
	updateRecentFileActions();

	testTempDir(luminance_options->tempfilespath);
	statusBar()->showMessage(tr("Ready. Now open an existing HDR image or create a new one!"),17000);
	saveProgress = new QProgressDialog(0, 0, 0, 0, this);
	saveProgress->setWindowTitle(tr("Saving file..."));
	saveProgress->setWindowModality(Qt::WindowModal);
	saveProgress->setMinimumDuration(0);
	cropToSelectionAction->setEnabled(false);

	setupConnections();

	// SPLASH SCREEN
	if (settings.contains("ShowSplashScreen")) {
		if (settings.value("ShowSplashScreen").toInt())
			showSplash();
	}
	else 
		showSplash();
}

void MainWindow::setupConnections()
{
	connect(mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(updateActions(QMdiSubWindow*)));
	connect(fileNewAction, SIGNAL(triggered()), this, SLOT(fileNewViaWizard()));
	connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
	connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(TonemapAction, SIGNAL(triggered()), this, SLOT(tonemap_requested()));
	connect(cropToSelectionAction, SIGNAL(triggered()), this, SLOT(cropToSelection()));
	connect(removeSelectionAction, SIGNAL(triggered()), this, SLOT(disableCrop()));
	connect(rotateccw, SIGNAL(triggered()), this, SLOT(rotateccw_requested()));
	connect(rotatecw, SIGNAL(triggered()), this, SLOT(rotatecw_requested()));
	connect(actionResizeHDR, SIGNAL(triggered()), this, SLOT(resize_requested()));
	connect(action_Projective_Transformation, SIGNAL(triggered()), this, SLOT(projectiveTransf_requested()));
	connect(actionBatch_Tone_Mapping, SIGNAL(triggered()), this, SLOT(batch_requested()));
	connect(Low_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdi_ldr_exp()));
	connect(Fit_to_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdi_fit_exp()));
	connect(Shrink_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdi_shrink_exp()));
	connect(Extend_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdi_extend_exp()));
	connect(Decrease_exposure,SIGNAL(triggered()),this,SLOT(current_mdi_decrease_exp()));
	connect(Increase_exposure,SIGNAL(triggered()),this,SLOT(current_mdi_increase_exp()));
	connect(zoomInAct,SIGNAL(triggered()),this,SLOT(current_mdi_zoomin()));
	connect(zoomOutAct,SIGNAL(triggered()),this,SLOT(current_mdi_zoomout()));
	connect(fitToWindowAct,SIGNAL(toggled(bool)),this,SLOT(current_mdi_fit_to_win(bool)));
	connect(normalSizeAct,SIGNAL(triggered()),this,SLOT(current_mdi_original_size()));
	connect(documentationAction,SIGNAL(triggered()),this,SLOT(openDocumentation()));
	connect(actionWhat_s_This,SIGNAL(triggered()),this,SLOT(enterWhatsThis()));
	connect(actionAbout_Qt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));
	connect(actionAbout_Luminance,SIGNAL(triggered()),this,SLOT(aboutLuminance()));
	connect(OptionsAction,SIGNAL(triggered()),this,SLOT(preferences_called()));
	connect(Transplant_Exif_Data_action,SIGNAL(triggered()),this,SLOT(transplant_called()));
	connect(actionTile,SIGNAL(triggered()),mdiArea,SLOT(tileSubWindows()));
	connect(actionCascade,SIGNAL(triggered()),mdiArea,SLOT(cascadeSubWindows()));
	connect(fileExitAction, SIGNAL(triggered()), this, SLOT(fileExit()));
	connect(menuWindows, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
	connect(actionSave_Hdr_Preview, SIGNAL(triggered()), this, SLOT(saveHdrPreview()));

	//QSignalMapper?
	connect(actionText_Under_Icons,SIGNAL(triggered()),this,SLOT(Text_Under_Icons()));
	connect(actionIcons_Only,SIGNAL(triggered()),this,SLOT(Icons_Only()));
	connect(actionText_Alongside_Icons,SIGNAL(triggered()),this,SLOT(Text_Alongside_Icons()));
	connect(actionText_Only,SIGNAL(triggered()),this,SLOT(Text_Only()));
	connect(actionDonate, SIGNAL(activated()), this, SLOT(showDonationsPage()));

	connect(windowMapper,SIGNAL(mapped(QWidget*)),this,SLOT(setActiveSubWindow(QWidget*)));
}

void MainWindow::showDonationsPage()
{
  QDesktopServices::openUrl(QUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=77BSTWEH7447C")); //davideanastasia
}


void MainWindow::fileNewViaWizard(QStringList files)
{
	HdrWizard *wizard;
	if (testTempDir(luminance_options->tempfilespath))
  {
		wizard=new HdrWizard (this, files);
		if (wizard->exec() == QDialog::Accepted)
    {
			HdrViewer *newmdi=new HdrViewer(this, true, false, luminance_options->negcolor, luminance_options->naninfcolor); //true means needs saving
			connect(newmdi, SIGNAL(selectionReady(bool)), this, SLOT(enableCrop(bool)));
			newmdi->updateHDR(wizard->getPfsFrameHDR());
			mdiArea->addSubWindow(newmdi);
			newmdi->setWindowTitle(wizard->getCaptionTEXT());
			newmdi->fitToWindow(true);
			newmdi->showMaximized();
			newmdi->setSelectionTool(true);
		}
		delete wizard;
	}
}

void MainWindow::fileOpen()
{
	QString filetypes = tr("All HDR formats ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.rw2 *.sr2 *.3fr *.mef *.mos *.erf *.nrw";
	filetypes +=  "*.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.RW2 *.SR2 *.3FR *.MEF *.MOS *.ERF *.NRW);;" ;
	filetypes += "OpenEXR (*.exr *.EXR);;" ;
	filetypes += "Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;";
	filetypes += "TIFF images (*.TIFF *.TIF *.tiff *.tif);;";
	filetypes += "RAW images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.rw2 *.sr2 *.3fr *.mef *.mos *.erf *.nrw *.mef *.mos *.erf *.nrw";
	filetypes +=             "*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.RW2 *.SR2 *.3FR *.MEF *.MOS *.ERF *.NRW);;";
	filetypes += "PFS stream (*.pfs *.PFS)";
	QStringList files = QFileDialog::getOpenFileNames(
       	this,
		tr("Load one or more HDR images..."),
		RecentDirHDRSetting,
		filetypes );

	foreach (QString file, files) {
		setupLoadThread(file);
	}
}

void MainWindow::updateRecentDirHDRSetting(QString newvalue)
{
	// update internal field variable
	RecentDirHDRSetting=newvalue;
	settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, RecentDirHDRSetting);
}

void MainWindow::fileSaveAs()
{
	if (currenthdr==NULL)
		return;
	
	// That's has already been fixed, plz confirm
	//HdrViewer* workaround = currenthdr; //odd workaround from Sławomir Szczyrba

	QString filetypes = tr("All HDR formats ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS);;" ;
	filetypes += "OpenEXR (*.exr *.EXR);;" ;
	filetypes += "Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;";
	filetypes += "HDR TIFF (*.tiff *.tif *.TIFF *.TIF);;";
	filetypes += "PFS Stream (*.pfs *.PFS)";

	QString fname = QFileDialog::getSaveFileName(
		this,
		tr("Save the HDR image as..."),
		RecentDirHDRSetting,
		filetypes
	);

	if(!fname.isEmpty()) {
		//currenthdr = workaround; //odd workaround from Sławomir Szczyrba
		showSaveDialog();
		QFileInfo qfi(fname);
		QString absoluteFileName=qfi.absoluteFilePath();
		char* encodedName=strdup(QFile::encodeName(absoluteFileName).constData());
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentDirHDRSetting != qfi.path() )
			// update internal field variable
			updateRecentDirHDRSetting(qfi.path());

		if (qfi.suffix().toUpper()=="EXR") {
			writeEXRfile  (currenthdr->getHDRPfsFrame(),encodedName);
		} else if (qfi.suffix().toUpper()=="HDR") {
			writeRGBEfile (currenthdr->getHDRPfsFrame(), encodedName);
		} else if (qfi.suffix().toUpper().startsWith("TIF")) {
			TiffWriter tiffwriter(encodedName, currenthdr->getHDRPfsFrame());
			connect(&tiffwriter, SIGNAL(maximumValue(int)), this, SLOT(setMaximum(int)));
			connect(&tiffwriter, SIGNAL(nextstep(int)), this, SLOT(setValue(int)));
			if (luminance_options->saveLogLuvTiff)
				tiffwriter.writeLogLuvTiff();
			else
				tiffwriter.writeFloatTiff();
		} else if (qfi.suffix().toUpper()=="PFS") {
			FILE *fd = fopen(encodedName, "w");
			pfs::DOMIO pfsio;
			//currenthdr->getHDRPfsFrame()->convertRGBChannelsToXYZ();
			pfsio.writeFrame(currenthdr->getHDRPfsFrame(), fd);
			//currenthdr->getHDRPfsFrame()->convertXYZChannelsToRGB();
			fclose(fd);
		} else {
			// TODO: [QT 4.5] This is not needed for windows (bug will be fixed in QT 4.5..?)

			// Default as EXR
			free(encodedName);
			absoluteFileName = absoluteFileName + ".exr";
			encodedName = strdup(QFile::encodeName(absoluteFileName).constData());
			writeEXRfile  (currenthdr->getHDRPfsFrame(),encodedName);
//			QMessageBox::warning(this,tr("Aborting..."), tr("Luminance supports only the following formats: <br>Radiance RGBE (hdr), PFS, tiff-hdr and OpenEXR."),
//			QMessageBox::Ok,QMessageBox::NoButton);
//			return;
		}
		hideSaveDialog();
		free(encodedName);
		setCurrentFile(absoluteFileName);
		//currenthdr->setNeedsSaving(false);
		currenthdr->setFileName(fname);
		currenthdr->setWindowTitle(absoluteFileName);
	}
}

void MainWindow::saveHdrPreview() {
	if(currenthdr==NULL)
		return;
	currenthdr->saveHdrPreview();
}

void MainWindow::updateActions( QMdiSubWindow * w ) {
	action_Projective_Transformation->setEnabled(w!=NULL);
	actionSave_Hdr_Preview->setEnabled(w!=NULL);
	TonemapAction->setEnabled(w!=NULL);
	fileSaveAsAction->setEnabled(w!=NULL);
	rotateccw->setEnabled(w!=NULL);
	rotatecw->setEnabled(w!=NULL);
	menuHDR_Histogram->setEnabled(w!=NULL);
	Low_dynamic_range->setEnabled(w!=NULL);
	Fit_to_dynamic_range->setEnabled(w!=NULL);
	Shrink_dynamic_range->setEnabled(w!=NULL);
	Extend_dynamic_range->setEnabled(w!=NULL);
	Decrease_exposure->setEnabled(w!=NULL);
	Increase_exposure->setEnabled(w!=NULL);
	actionResizeHDR->setEnabled(w!=NULL);
	if (w!=NULL) {
		currenthdr=(HdrViewer*)(mdiArea->activeSubWindow()->widget());
		if (currenthdr->isFittedToWindow()) {
			normalSizeAct->setEnabled(false);
			zoomInAct->setEnabled(false);
			zoomOutAct->setEnabled(false);
			fitToWindowAct->setEnabled(true);
		} else {
			zoomOutAct->setEnabled(currenthdr->getScaleFactor() > 0.222);
			zoomInAct->setEnabled(currenthdr->getScaleFactor() < 3.0);
			fitToWindowAct->setEnabled(true);
			normalSizeAct->setEnabled(true);
		}
		if (currenthdr->hasSelection()) {
			cropToSelectionAction->setEnabled(true);
			removeSelectionAction->setEnabled(true);
		}
		else {
			cropToSelectionAction->setEnabled(false);
			removeSelectionAction->setEnabled(false);
		}
	}
	else {
		if (mdiArea->subWindowList().empty()) {
			currenthdr=NULL;
			normalSizeAct->setEnabled(false);
			zoomInAct->setEnabled(false);
			zoomOutAct->setEnabled(false);
			fitToWindowAct->setEnabled(false);
			cropToSelectionAction->setEnabled(false);
		}
	}
}

void MainWindow::setActiveSubWindow(QWidget* w) {
	QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
	foreach(QMdiSubWindow *p,allhdrs)
		if (p->widget() == w)
			mdiArea->setActiveSubWindow(p);
}

void MainWindow::tonemap_requested() {
	if(currenthdr==NULL)
		return;
	this->setDisabled(true);
	try {
		TonemappingWindow *tmodialog=new TonemappingWindow(this, currenthdr->getHDRPfsFrame(), currenthdr->getFileName());
		tmodialog->setAttribute(Qt::WA_DeleteOnClose);
		//tmodialog->setAttribute(Qt::WA_Window);
		connect(tmodialog,SIGNAL(closing()),this,SLOT(reEnableMainWin()));
		tmodialog->show();
#ifdef WIN32
		;
#else
		hide();
#endif
		if (helpBrowser) 
			helpBrowser->hide();
	}
	catch(pfs::Exception e) {
		QMessageBox::warning(this,tr("Luminance HDR"),tr("Error: %1 ").arg(e.getMessage()));
		reEnableMainWin();	
	}
	catch(...) {
		QMessageBox::warning(this,tr("Luminance HDR"),tr("Error: Failed to Tonemap Image"));
		reEnableMainWin();	
	}
}

bool MainWindow::testTempDir(QString dirname) {
	QFileInfo test(dirname);
	if (test.isWritable() && test.exists() && test.isDir()) {
		return true;
	} else {
		QMessageBox::critical(this,tr("Error..."),tr("Luminance HDR needs to cache its results using temporary files, but the currently selected directory is not valid.<br>Please choose a valid path in Tools -> Preferences... -> Tonemapping."),
		QMessageBox::Ok,QMessageBox::NoButton);
		return false;
	}
}

void MainWindow::reEnableMainWin() {
	setEnabled(true);
	show();
	if (helpBrowser) 
		helpBrowser->show();
}

void MainWindow::rotateccw_requested() {
	dispatchrotate(false);
}

void MainWindow::rotatecw_requested() {
	dispatchrotate(true);
}

void MainWindow::dispatchrotate(bool clockwise)
{
	if (currenthdr==NULL)
		return;
	rotateccw->setEnabled(false);
	rotatecw->setEnabled(false);
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	pfs::Frame *rotated = pfs::rotateFrame(currenthdr->getHDRPfsFrame(),clockwise);
	//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
	currenthdr->updateHDR(rotated);
	if (! currenthdr->needsSaving())
  {
		currenthdr->setNeedsSaving(true);
		currenthdr->setWindowTitle(currenthdr->windowTitle().prepend("(*) "));
	}
	QApplication::restoreOverrideCursor();
	rotateccw->setEnabled(true);
	rotatecw->setEnabled(true);
}

void MainWindow::resize_requested()
{
	if (currenthdr==NULL)
		return;
	ResizeDialog *resizedialog = new ResizeDialog(this,currenthdr->getHDRPfsFrame());
	if (resizedialog->exec() == QDialog::Accepted)
  {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
		currenthdr->updateHDR(resizedialog->getResizedFrame());
		if (! currenthdr->needsSaving())
    {
			currenthdr->setNeedsSaving(true);
			currenthdr->setWindowTitle(currenthdr->windowTitle().prepend("(*) "));
		}
		QApplication::restoreOverrideCursor();
	}
	delete resizedialog;
}

void MainWindow::projectiveTransf_requested() {
	if (currenthdr==NULL)
		return;
	ProjectionsDialog *projTranfsDialog=new ProjectionsDialog(this,currenthdr->getHDRPfsFrame());
	if (projTranfsDialog->exec() == QDialog::Accepted) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
		currenthdr->updateHDR(projTranfsDialog->getTranformedFrame());
		if (! currenthdr->needsSaving()) {
			currenthdr->setNeedsSaving(true);
			currenthdr->setWindowTitle(currenthdr->windowTitle().prepend("(*) "));
		}
		QApplication::restoreOverrideCursor();
	}
	delete projTranfsDialog;
}

void MainWindow::current_mdi_decrease_exp() {
	currenthdr->lumRange()->decreaseExposure();
}

void MainWindow::current_mdi_extend_exp() {
	currenthdr->lumRange()->extendRange();
}

void MainWindow::current_mdi_fit_exp() {
	currenthdr->lumRange()->fitToDynamicRange();
}

void MainWindow::current_mdi_increase_exp() {
	currenthdr->lumRange()->increaseExposure();
}

void MainWindow::current_mdi_shrink_exp() {
	currenthdr->lumRange()->shrinkRange();
}

void MainWindow::current_mdi_ldr_exp() {
	currenthdr->lumRange()->lowDynamicRange();
}

void MainWindow::current_mdi_zoomin() {
	currenthdr->zoomIn();
	zoomOutAct->setEnabled(true);
	zoomInAct->setEnabled(currenthdr->getScaleFactor() < 3.0);
}

void MainWindow::current_mdi_zoomout() {
	currenthdr->zoomOut();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(currenthdr->getScaleFactor() > 0.222);
}

void MainWindow::current_mdi_fit_to_win(bool checked) {
	currenthdr->fitToWindow(checked);
	zoomInAct->setEnabled(!checked);
	zoomOutAct->setEnabled(!checked);
	normalSizeAct->setEnabled(!checked);
}

void MainWindow::current_mdi_original_size() {
	currenthdr->normalSize();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(true);
}

void MainWindow::openDocumentation() {
	helpBrowser = new HelpBrowser(this,"Luminance HDR Help");
	helpBrowser->setAttribute(Qt::WA_DeleteOnClose);
	connect(helpBrowser, SIGNAL(closed()), this, SLOT(helpBrowserClosed()));
	helpBrowser->show();
}

void MainWindow::helpBrowserClosed() {
	helpBrowser = NULL;
}

void MainWindow::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

void MainWindow::setCurrentFile(const QString &fileName) {
        QStringList files = settings.value(KEY_RECENT_FILES).toStringList();
        files.removeAll(fileName);
        files.prepend(fileName);
        while (files.size() > MaxRecentFiles)
                files.removeLast();

        settings.setValue(KEY_RECENT_FILES, files);
        updateRecentFileActions();
}

void MainWindow::updateRecentFileActions() {
	QStringList files = settings.value(KEY_RECENT_FILES).toStringList();

	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);
	separatorRecentFiles->setVisible(numRecentFiles > 0);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);
}

void MainWindow::openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		setupLoadThread(action->data().toString());
}

void MainWindow::setupLoadThread(QString fname)
{
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	MySubWindow *subWindow = new MySubWindow(this,this);
	newhdr=new HdrViewer(this, false, false, luminance_options->negcolor, luminance_options->naninfcolor);
	newhdr->setAttribute(Qt::WA_DeleteOnClose);
	connect(newhdr, SIGNAL(selectionReady(bool)), this, SLOT(enableCrop(bool)));
	newhdr->showLoadDialog();
	newhdr->setSelectionTool(true);
        
	subWindow->setWidget(newhdr);
	subWindow->setAttribute(Qt::WA_DeleteOnClose);

	LoadHdrThread *loadthread = new LoadHdrThread(fname, RecentDirHDRSetting);
	connect(loadthread, SIGNAL(maximumValue(int)), newhdr, SLOT(setMaximum(int)));
	connect(loadthread, SIGNAL(nextstep(int)), newhdr, SLOT(setValue(int)));
	connect(loadthread, SIGNAL(updateRecentDirHDRSetting(QString)), this, SLOT(updateRecentDirHDRSetting(QString)));
	connect(loadthread, SIGNAL(hdr_ready(pfs::Frame*,QString)), subWindow, SLOT(addHdrFrame(pfs::Frame*,QString)));
	connect(loadthread, SIGNAL(load_failed(QString)), this, SLOT(load_failed(QString)));
	connect(loadthread, SIGNAL(load_failed(QString)), subWindow, SLOT(load_failed(QString)));

  // add progress bar in the status bar and launch it
  
	loadthread->start();
  
  // remove status bar
}

void MainWindow::load_failed(QString error_message)
{
	QMessageBox::critical(this,tr("Aborting..."), error_message, QMessageBox::Ok,QMessageBox::NoButton);
	QStringList files = settings.value(KEY_RECENT_FILES).toStringList();
	LoadHdrThread *lht=(LoadHdrThread *)(sender());
	QString fname=lht->getHdrFileName();
	delete lht;
	files.removeAll(fname);
	settings.setValue(KEY_RECENT_FILES, files);
	updateRecentFileActions();
}

void MainWindow::preferences_called()
{
	unsigned int negcol=luminance_options->negcolor;
	unsigned int naninfcol=luminance_options->naninfcolor;
	PreferencesDialog *opts=new PreferencesDialog(this);
	opts->setAttribute(Qt::WA_DeleteOnClose);
	if (opts->exec() == QDialog::Accepted && (negcol!=luminance_options->negcolor || naninfcol!=luminance_options->naninfcolor) )
  {
		QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
		foreach (QMdiSubWindow *p, allhdrs)
    {
			((HdrViewer*)p->widget())->update_colors(luminance_options->negcolor,luminance_options->naninfcolor);
		}
	}
}

void MainWindow::transplant_called() {
	TransplantExifDialog *transplant=new TransplantExifDialog(this);
	transplant->setAttribute(Qt::WA_DeleteOnClose);
	transplant->exec();
}

void MainWindow::load_options() {
	//load from settings the path where hdrs have been previously opened/loaded
	RecentDirHDRSetting=settings.value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString();

	//load from settings the main toolbar visualization mode
	if (!settings.contains(KEY_TOOLBAR_MODE))
		settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
	switch (settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt()) {
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

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
	event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {

	if (event->mimeData()->hasUrls()) {
		QStringList files = convertUrlListToFilenameList(event->mimeData()->urls());
		if (files.size() > 0) {
			DnDOptionDialog dndOption(this, files);
			dndOption.exec();

			switch (dndOption.result) {
			case 1: // create new using LDRS
				fileNewViaWizard(files);
				break;
			case 2: // openHDRs
				foreach (QString file, files) {
					setupLoadThread(file);
				}
				break;
			}
		}
	}
	event->acceptProposedAction();
}

MainWindow::~MainWindow() {
	for (int i = 0; i < MaxRecentFiles; ++i) {
		delete recentFileActs[i];
	}
	settings.setValue("MainWindowState", saveState());
}

void MainWindow::fileExit() {
	QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
	bool closeok=true;
	foreach(QMdiSubWindow *p,allhdrs) {
		if (((HdrViewer*)p->widget())->needsSaving())
			closeok=false;
	}
	if (closeok || (QMessageBox::warning(this,tr("Unsaved changes..."),tr("There is at least one HDR image with unsaved changes.<br>Do you still want to quit?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
		== QMessageBox::Yes))
		emit close();
}

void MainWindow::Text_Under_Icons() {
	toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
}

void MainWindow::Icons_Only() {
	toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonIconOnly);
}

void MainWindow::Text_Alongside_Icons() {
	toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextBesideIcon);
}

void MainWindow::Text_Only() {
	toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextOnly);
}


void MainWindow::showSplash() {
	splash=new QDialog(this);
	splash->setAttribute(Qt::WA_DeleteOnClose);
	Ui::SplashLuminance ui;
	ui.setupUi(splash);
	connect(ui.yesButton, SIGNAL(clicked()), this, SLOT(splashShowDonationsPage()));
	connect(ui.noButton, SIGNAL(clicked()), this, SLOT(splashClose()));
	connect(ui.askMeLaterButton, SIGNAL(clicked()), splash, SLOT(close()));
	
	splash->show();

}

void MainWindow::splashShowDonationsPage() {
	showDonationsPage();
	splash->close();
}

void MainWindow::splashClose() {
	settings.setValue("ShowSplashScreen", 0);
	splash->close();
}

void MainWindow::aboutLuminance()
{
	QDialog *about = new QDialog(this);
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
  QStringList paths = QStringList( BASEDIR "/share/doc/luminance") << BASEDIR "/share/luminance" << docDir << "/Applications/luminance.app/Contents/Resources" << "./";
	foreach (QString path,paths)
  {
		QString fname(path+QString("/LICENSE"));
#ifdef WIN32
		fname+=".txt";
#endif
		if (QFile::exists(fname))
    {
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
	if (license_file_not_found)
  {
		ui.GPLbox->setOpenExternalLinks(true);
		ui.GPLbox->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui.GPLbox->setHtml(tr("%1 License document not found, you can find it online: %2here%3","%2 and %3 are html tags").arg("<html>").arg("<a href=\"http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\">").arg("</a></html>"));
	}
	about->show();
}

void MainWindow::updateWindowMenu() {
	menuWindows->clear();
	QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
	for (int i = 0; i < windows.size(); ++i) {
		HdrViewer *child = qobject_cast<HdrViewer *>(windows.at(i)->widget());
		QString text=QString((i < 9)?"&":"") + QString("%1 %2").arg(i + 1).arg(QFileInfo((child->getFileName().isEmpty())? tr("Untitled"):child->getFileName()).fileName());
		QAction *action  = menuWindows->addAction(text);
		action->setCheckable(true);
		action->setChecked(child==currenthdr);
		connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
		windowMapper->setMapping(action, child);
	}
	menuWindows->addSeparator();
	menuWindows->addAction(actionTile);
	menuWindows->addAction(actionCascade);
}

void MainWindow::batch_requested() {
	BatchTMDialog *batchdialog=new BatchTMDialog(this);
	batchdialog->exec();
	delete batchdialog;
}

void MainWindow::setMaximum(int max) {
	saveProgress->setMaximum( max - 1 );
}

void MainWindow::setValue(int value) {
	saveProgress->setValue( value );
}

void MainWindow::showSaveDialog(void) {
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	saveProgress->setValue( 1 );
}

void MainWindow::hideSaveDialog(void) {
	QApplication::restoreOverrideCursor();
	saveProgress->cancel();
}

void MainWindow::cropToSelection(void) {
	QRect cropRect = currenthdr->getSelectionRect();
	int x_ul, y_ul, x_br, y_br;
	cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
	disableCrop();
	pfs::Frame *original_frame = currenthdr->getHDRPfsFrame();
	HdrViewer *newHdrViewer = new HdrViewer(this, true, false, luminance_options->negcolor, luminance_options->naninfcolor);
	pfs::Frame *cropped_frame = pfs::pfscut(original_frame, x_ul, y_ul, x_br, y_br);

	newHdrViewer->updateHDR(cropped_frame);
	newHdrViewer->setFileName(QString(tr("Cropped Image")));
	newHdrViewer->setWindowTitle(QString(tr("Cropped Image")));
	newHdrViewer->setSelectionTool(true);

    newHdrViewer->setFlagUpdateImage(false); // disable updating image for performance

    float min = currenthdr->lumRange()->getRangeWindowMin();
	float max = currenthdr->lumRange()->getRangeWindowMax();
	int lumMappingMode = currenthdr->getLumMappingMethod();
	newHdrViewer->lumRange()->setRangeWindowMinMax(min, max);
	newHdrViewer->setLumMappingMethod(lumMappingMode);

    newHdrViewer->setFlagUpdateImage(true); // reenabling updating image

    newHdrViewer->updateRangeWindow();

    connect(newHdrViewer, SIGNAL(selectionReady(bool)), this, SLOT(enableCrop(bool)));
	mdiArea->addSubWindow(newHdrViewer);
	newHdrViewer->fitToWindow(true);
	newHdrViewer->showMaximized();
	newHdrViewer->show();
}

void MainWindow::enableCrop(bool isReady) {
	if (isReady) {
		cropToSelectionAction->setEnabled(true);
		removeSelectionAction->setEnabled(true);
	}
	else {
		cropToSelectionAction->setEnabled(false);
		removeSelectionAction->setEnabled(false);
	}
}

void MainWindow::disableCrop() {
	currenthdr->removeSelection();
	cropToSelectionAction->setEnabled(false);
	removeSelectionAction->setEnabled(false);
}

void MainWindow::closeEvent ( QCloseEvent *event ) {
	settings.setValue("MainWindowGeometry", saveGeometry());
	QWidget::closeEvent(event);
}

//
//===================  MySubWindow Implementation ===========================================================
//
MySubWindow::MySubWindow(MainWindow *ptr, QWidget * parent, Qt::WindowFlags flags) : QMdiSubWindow(parent, flags), mainGuiPtr(ptr) {
}

MySubWindow::~MySubWindow() {
}

void MySubWindow::addHdrFrame(pfs::Frame* hdr_pfs_frame, QString fname) {
	HdrViewer *ptr = (HdrViewer *) widget();
	ptr->hideLoadDialog();
	ptr->updateHDR(hdr_pfs_frame);
	ptr->setFileName(fname);
	ptr->setWindowTitle(fname);

	ptr->normalSize();
	ptr->fitToWindow(true);
	resize((int) (0.66 * mainGuiPtr->mdiArea->width()),(int) (0.66 * mainGuiPtr->mdiArea->height()));
	mainGuiPtr->mdiArea->addSubWindow(this);
	ptr->show();
	showMaximized();
	mainGuiPtr->setCurrentFile(fname);
	QApplication::restoreOverrideCursor();
}

void MySubWindow::load_failed(QString) {
	HdrViewer *ptr = (HdrViewer *) widget();
	ptr->hideLoadDialog();
	delete ptr;
	QApplication::restoreOverrideCursor();
}
