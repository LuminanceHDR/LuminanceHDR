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
#include <QDir>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QWhatsThis>
#include <QSignalMapper>
#include <QTextStream>

#include "mainWindow.h"
#include "../Fileformat/pfstiff.h"
#include "../ToneMappingDialog/tonemappingDialog.h"
#include "../generated_uic/ui_documentation.h"
#include "../generated_uic/ui_about.h"
#include "../TransplantExif/transplant.h"
#include "../Batch/batch_dialog.h"
#include "../Threads/loadHdrThread.h"
#include "DnDOption.h"
#include "../Common/config.h"
#include "hdrviewer.h"
#include "../Common/global.h"
#include "../Filter/pfscut.h"

#include <iostream>

pfs::Frame* rotateFrame( pfs::Frame* inputpfsframe, bool clock_wise );
void writeRGBEfile (pfs::Frame* inputpfsframe, const char* outfilename);
void writeEXRfile  (pfs::Frame* inputpfsframe, const char* outfilename);

MainGui::MainGui(QWidget *p) : QMainWindow(p), currenthdr(NULL) {
	setupUi(this);

	restoreState( settings.value("MainGuiState").toByteArray());
	int x = settings.value("MainGuiPosX").toInt();
	int y = settings.value("MainGuiPosY").toInt();
	int w = settings.value("MainGuiWidth").toInt();
	int h = settings.value("MainGuiHeight").toInt();

	if (x<0) x=0;	
	if (y<0) y=0;	
	if (w==0) w=800;
	if (h==0) h=600;

	setGeometry(x, y, w, h);

	setAcceptDrops(true);
	windowMapper = new QSignalMapper(this);

	//main toolbar setup
	QActionGroup *toolBarOptsGroup = new QActionGroup(this);
	toolBarOptsGroup->addAction(actionText_Under_Icons);
	toolBarOptsGroup->addAction(actionIcons_Only);
	toolBarOptsGroup->addAction(actionText_Alongside_Icons);
	toolBarOptsGroup->addAction(actionText_Only);
	menuToolbars->addAction(toolBar->toggleViewAction());

	mdiArea = new QMdiArea(this);
	mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setBackground(QBrush(QColor::fromRgb(192, 192, 192)) );
	setCentralWidget(mdiArea);

	qtpfsgui_options=QtpfsguiOptions::getInstance();
	load_options();

	setWindowTitle("Qtpfsgui "QTPFSGUIVERSION);


	//recent files
	for (int i = 0; i < MaxRecentFiles; ++i) {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
	separatorRecentFiles = menuFile->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
		menuFile->addAction(recentFileActs[i]);
	updateRecentFileActions();

	//this->showMaximized();

	testTempDir(qtpfsgui_options->tempfilespath);
	statusBar()->showMessage(tr("Ready.... Now open an Hdr or create one!"),17000);
	saveProgress = new QProgressDialog(0, 0, 0, 0, this);
        saveProgress->setWindowTitle(tr("Saving file..."));
        saveProgress->setWindowModality(Qt::WindowModal);
        saveProgress->setMinimumDuration(0);
	cropToSelectionAction->setEnabled(false);

	setupConnections();
}

void MainGui::setupConnections() {
	connect(mdiArea,SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(updateActions(QMdiSubWindow*)));
	connect(fileNewAction, SIGNAL(triggered()), this, SLOT(fileNewViaWizard()));
	connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
	connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(TonemapAction, SIGNAL(triggered()), this, SLOT(tonemap_requested()));
	connect(cropToSelectionAction, SIGNAL(triggered()), this, SLOT(cropToSelection()));

	connect(removeSelectionAction, SIGNAL(triggered()), this, SLOT(disableCrop()));
	windowMapper->setMapping(removeSelectionAction, QString("MainGui"));

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
	connect(actionAbout_Qtpfsgui,SIGNAL(triggered()),this,SLOT(aboutQtpfsgui()));
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

	connect(windowMapper,SIGNAL(mapped(QWidget*)),this,SLOT(setActiveSubWindow(QWidget*)));

}

void MainGui::fileNewViaWizard(QStringList files) {
	HdrWizardForm *wizard;
	if (testTempDir(qtpfsgui_options->tempfilespath)) {
		wizard=new HdrWizardForm (this, files);
		if (wizard->exec() == QDialog::Accepted) {
			HdrViewer *newmdi=new HdrViewer(this, true, false, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor); //true means needs saving
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


void MainGui::fileOpen() {
	QString filetypes = tr("All Hdr formats ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 ";
	filetypes +=  "*.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2);;" ;
	filetypes += "OpenEXR (*.exr *.EXR);;" ;
	filetypes += "Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;";
	filetypes += "TIFF Images (*.TIFF *.TIF *.tiff *.tif);;";
	filetypes += "RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 ";
	filetypes +=             "*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2);;";
	filetypes += "PFS Stream (*.pfs *.PFS)";
	QStringList files = QFileDialog::getOpenFileNames(
        	this,
		tr("Load one or more Hdr files..."),
		RecentDirHDRSetting,
		filetypes );

	foreach (QString file, files) {
		setupLoadThread(file);
	}
}

void MainGui::updateRecentDirHDRSetting(QString newvalue) {
	// update internal field variable
	RecentDirHDRSetting=newvalue;
	settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, RecentDirHDRSetting);
}

void MainGui::fileSaveAs()
{
	if (currenthdr==NULL)
		return;
	
	// That's has already been fixed, plz confirm
	//HdrViewer* workaround = currenthdr; //odd workaround from Sławomir Szczyrba

	QString filetypes = tr("All Hdr formats ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS);;" ;
	filetypes += "OpenEXR (*.exr *.EXR);;" ;
	filetypes += "Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;";
	filetypes += "HDR TIFF (*.tiff *.tif *.TIFF *.TIF);;";
	filetypes += "PFS Stream (*.pfs *.PFS)";

	QString fname = QFileDialog::getSaveFileName(
		this,
		tr("Save the HDR..."),
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
			if (qtpfsgui_options->saveLogLuvTiff)
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
//			QMessageBox::warning(this,tr("Aborting..."), tr("Qtpfsgui supports only the following formats: <br>Radiance RGBE (hdr), PFS, tiff-hdr and OpenEXR."),
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

void MainGui::saveHdrPreview() {
	if(currenthdr==NULL)
		return;
	currenthdr->saveHdrPreview();
}

void MainGui::updateActions( QMdiSubWindow * w ) {
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
		if (currenthdr->getFittingWin()) {
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

void MainGui::setActiveSubWindow(QWidget* w) {
	QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
	foreach(QMdiSubWindow *p,allhdrs)
		if (p->widget() == w)
			mdiArea->setActiveSubWindow(p);
}

void MainGui::tonemap_requested() {
	if(currenthdr==NULL)
		return;
	if (testTempDir(qtpfsgui_options->tempfilespath)) {
		this->setDisabled(true);
		try {
			TonemappingWindow *tmodialog=new TonemappingWindow(this, currenthdr->getHDRPfsFrame(), currenthdr->getFileName());
			connect(tmodialog,SIGNAL(closing()),this,SLOT(reEnableMainWin()));
			tmodialog->show();
			tmodialog->setAttribute(Qt::WA_DeleteOnClose);
		}
		catch(pfs::Exception e) {
			QMessageBox::warning(this,tr("Qtpfsgui"),tr("Error: %1 ").arg(e.getMessage()));
			reEnableMainWin();	
		}
		catch(...) {
			QMessageBox::warning(this,tr("Qtpfsgui"),tr("Error: Filed to Tonemap Image"));
			reEnableMainWin();	
		}
	}
}

bool MainGui::testTempDir(QString dirname) {
	QFileInfo test(dirname);
	if (test.isWritable() && test.exists() && test.isDir()) {
		return true;
	} else {
		QMessageBox::critical(this,tr("Error..."),tr("Qtpfsgui needs to cache its results using temporary files, but the currently selected directory is not valid.<br>Please choose a valid path in Tools -> Preferences... -> Tone Mapping."),
		QMessageBox::Ok,QMessageBox::NoButton);
		return false;
	}
}

void MainGui::reEnableMainWin() {
	this->setEnabled(true);
}

void MainGui::rotateccw_requested() {
	dispatchrotate(false);
}

void MainGui::rotatecw_requested() {
	dispatchrotate(true);
}

void MainGui::dispatchrotate(bool clockwise) {
	if(currenthdr==NULL)
		return;
	rotateccw->setEnabled(false);
	rotatecw->setEnabled(false);
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	pfs::Frame *rotated=rotateFrame(currenthdr->getHDRPfsFrame(),clockwise);
	//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
	currenthdr->updateHDR(rotated);
	if (! currenthdr->needsSaving()) {
		currenthdr->setNeedsSaving(true);
		currenthdr->setWindowTitle(currenthdr->windowTitle().prepend("(*) "));
	}
	QApplication::restoreOverrideCursor();
	rotateccw->setEnabled(true);
	rotatecw->setEnabled(true);
}

void MainGui::resize_requested() {
	if (currenthdr==NULL)
		return;
	ResizeDialog *resizedialog=new ResizeDialog(this,currenthdr->getHDRPfsFrame());
	if (resizedialog->exec() == QDialog::Accepted) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
		currenthdr->updateHDR(resizedialog->getResizedFrame());
		if (! currenthdr->needsSaving()) {
			currenthdr->setNeedsSaving(true);
			currenthdr->setWindowTitle(currenthdr->windowTitle().prepend("(*) "));
		}
		QApplication::restoreOverrideCursor();
	}
	delete resizedialog;
}

void MainGui::projectiveTransf_requested() {
	if (currenthdr==NULL)
		return;
	projectiveTransformDialog *projTranfsDialog=new projectiveTransformDialog(this,currenthdr->getHDRPfsFrame());
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

void MainGui::current_mdi_decrease_exp() {
	currenthdr->lumRange()->decreaseExposure();
}
void MainGui::current_mdi_extend_exp() {
	currenthdr->lumRange()->extendRange();
}
void MainGui::current_mdi_fit_exp() {
	currenthdr->lumRange()->fitToDynamicRange();
}
void MainGui::current_mdi_increase_exp() {
	currenthdr->lumRange()->increaseExposure();
}
void MainGui::current_mdi_shrink_exp() {
	currenthdr->lumRange()->shrinkRange();
}
void MainGui::current_mdi_ldr_exp() {
	currenthdr->lumRange()->lowDynamicRange();
}
void MainGui::current_mdi_zoomin() {
	currenthdr->zoomIn();
	zoomOutAct->setEnabled(true);
	zoomInAct->setEnabled(currenthdr->getScaleFactor() < 3.0);
}
void MainGui::current_mdi_zoomout() {
	currenthdr->zoomOut();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(currenthdr->getScaleFactor() > 0.222);
}
void MainGui::current_mdi_fit_to_win(bool checked) {
	currenthdr->fitToWindow(checked);
	zoomInAct->setEnabled(!checked);
	zoomOutAct->setEnabled(!checked);
	normalSizeAct->setEnabled(!checked);
}
void MainGui::current_mdi_original_size() {
	currenthdr->normalSize();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(true);
}

void MainGui::openDocumentation() {
	QDialog *help=new QDialog();
	help->setAttribute(Qt::WA_DeleteOnClose);
	Ui::HelpDialog ui;
	ui.setupUi(help);
	QString docDir = QCoreApplication::applicationDirPath();
	docDir.append("/../Resources/html");
	ui.tb->setSearchPaths(QStringList("/usr/share/qtpfsgui/html") << "/usr/local/share/qtpfsgui/html" << "./html" << docDir << "/Applications/qtpfsgui.app/Contents/Resources/html");
	ui.tb->setSource(QUrl("index.html"));
	help->show();
}

void MainGui::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

void MainGui::setCurrentFile(const QString &fileName) {
        QStringList files = settings.value(KEY_RECENT_FILES).toStringList();
        files.removeAll(fileName);
        files.prepend(fileName);
        while (files.size() > MaxRecentFiles)
                files.removeLast();

        settings.setValue(KEY_RECENT_FILES, files);
        updateRecentFileActions();
}

void MainGui::updateRecentFileActions() {
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

void MainGui::openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		setupLoadThread(action->data().toString());
}

void MainGui::setupLoadThread(QString fname) {
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	MySubWindow *subWindow = new MySubWindow(this,this);
	newhdr=new HdrViewer(this, false, false, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor);
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

	loadthread->start();
}

void MainGui::load_failed(QString error_message) {
	QMessageBox::critical(this,tr("Aborting..."), error_message, QMessageBox::Ok,QMessageBox::NoButton);
	QStringList files = settings.value(KEY_RECENT_FILES).toStringList();
	LoadHdrThread *lht=(LoadHdrThread *)(sender());
	QString fname=lht->getHdrFileName();
	delete lht;
	files.removeAll(fname);
	settings.setValue(KEY_RECENT_FILES, files);
	updateRecentFileActions();
}

void MainGui::preferences_called() {
	unsigned int negcol=qtpfsgui_options->negcolor;
	unsigned int naninfcol=qtpfsgui_options->naninfcolor;
	PreferenceDialog *opts=new PreferenceDialog(this);
	opts->setAttribute(Qt::WA_DeleteOnClose);
	if (opts->exec() == QDialog::Accepted && (negcol!=qtpfsgui_options->negcolor || naninfcol!=qtpfsgui_options->naninfcolor) ) {
		QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
		foreach(QMdiSubWindow *p,allhdrs) {
			((HdrViewer*)p->widget())->update_colors(qtpfsgui_options->negcolor,qtpfsgui_options->naninfcolor);
		}
	}
}

void MainGui::transplant_called() {
	TransplantExifDialog *transplant=new TransplantExifDialog(this);
	transplant->setAttribute(Qt::WA_DeleteOnClose);
	transplant->exec();
}

void MainGui::load_options() {
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

void MainGui::dragEnterEvent(QDragEnterEvent *event) {
	event->acceptProposedAction();
}

void MainGui::dropEvent(QDropEvent *event) {

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

MainGui::~MainGui() {
	for (int i = 0; i < MaxRecentFiles; ++i) {
		delete recentFileActs[i];
	}
	settings.setValue("MainGuiState", saveState());
}

void MainGui::fileExit() {
	QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
	bool closeok=true;
	foreach(QMdiSubWindow *p,allhdrs) {
		if (((HdrViewer*)p->widget())->needsSaving())
			closeok=false;
	}
	if (closeok || (QMessageBox::warning(this,tr("Unsaved changes..."),tr("There is at least one Hdr with unsaved changes.<br>Do you still want to quit?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
		== QMessageBox::Yes))
		emit close();
}

void MainGui::Text_Under_Icons() {
	toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
}

void MainGui::Icons_Only() {
	toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonIconOnly);
}

void MainGui::Text_Alongside_Icons() {
	toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextBesideIcon);
}

void MainGui::Text_Only() {
	toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
	settings.setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextOnly);
}

void MainGui::aboutQtpfsgui() {
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

void MainGui::updateWindowMenu() {
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

void MainGui::batch_requested() {
	BatchTMDialog *batchdialog=new BatchTMDialog(this);
	batchdialog->exec();
	delete batchdialog;
}

void MainGui::setMaximum(int max) {
        saveProgress->setMaximum( max - 1 );
}

void MainGui::setValue(int value) {
        saveProgress->setValue( value );
}

void MainGui::showSaveDialog(void) {
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	saveProgress->setValue( 1 );
}

void MainGui::hideSaveDialog(void) {
	QApplication::restoreOverrideCursor();
        saveProgress->cancel();
}

void MainGui::cropToSelection(void) {
	disableCrop();
	QRect cropRect = currenthdr->getSelectionRect();
	int x_ul, y_ul, x_br, y_br;
	cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
	pfs::Frame *original_frame = currenthdr->getHDRPfsFrame();
	HdrViewer *newHdrViewer = new HdrViewer(this, true, false, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor);
        pfs::Frame *cropped_frame = pfscut(original_frame, x_ul, y_ul, x_br, y_br);

	newHdrViewer->updateHDR(cropped_frame);
	newHdrViewer->setFileName(QString(tr("Cropped Frame")));
	newHdrViewer->setWindowTitle(QString(tr("Cropped Frame")));
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

void MainGui::enableCrop(bool isReady) {
	if (isReady) {
		cropToSelectionAction->setEnabled(true);
		removeSelectionAction->setEnabled(true);
	}
	else {
		cropToSelectionAction->setEnabled(false);
		removeSelectionAction->setEnabled(false);
	}
}

void MainGui::disableCrop() {
	currenthdr->removeSelection();
	cropToSelectionAction->setEnabled(false);
	removeSelectionAction->setEnabled(false);
}

void MainGui::closeEvent ( QCloseEvent * ) {
	settings.setValue("MainGuiPosX",x());
	settings.setValue("MainGuiPosY",y());
	settings.setValue("MainGuiWidth",width());
	settings.setValue("MainGuiHeight",height());
}

//
//===================  MySubWindow Implementation ===========================================================
//
MySubWindow::MySubWindow(MainGui *ptr, QWidget * parent, Qt::WindowFlags flags) : QMdiSubWindow(parent, flags), mainGuiPtr(ptr) {
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
