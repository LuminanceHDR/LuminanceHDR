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

pfs::Frame* rotateFrame( pfs::Frame* inputpfsframe, bool clock_wise );
void writeRGBEfile (pfs::Frame* inputpfsframe, const char* outfilename);
void writeEXRfile  (pfs::Frame* inputpfsframe, const char* outfilename);

MainGui::MainGui(QWidget *p) : QMainWindow(p), currenthdr(NULL) {
	setupUi(this);
	setAcceptDrops(true);

	//main toolbar setup
	QActionGroup *toolBarOptsGroup = new QActionGroup(this);
	toolBarOptsGroup->addAction(actionText_Under_Icons);
	toolBarOptsGroup->addAction(actionIcons_Only);
	toolBarOptsGroup->addAction(actionText_Alongside_Icons);
	toolBarOptsGroup->addAction(actionText_Only);
	menuToolbars->addAction(toolBar->toggleViewAction());

	workspace = new QWorkspace(this);
	workspace->setScrollBarsEnabled( TRUE );
	workspace->setBackground(QBrush(QColor::fromRgb(192, 192, 192)) );
	setCentralWidget(workspace);

	qtpfsgui_options=QtpfsguiOptions::getInstance();
	load_options();

	setWindowTitle("Qtpfsgui "QTPFSGUIVERSION);

	connect(workspace,SIGNAL(windowActivated(QWidget*)),this,SLOT(updateActions(QWidget*)));
	connect(fileNewAction, SIGNAL(triggered()), this, SLOT(fileNewViaWizard()));
	connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
	connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(TonemapAction, SIGNAL(triggered()), this, SLOT(tonemap_requested()));
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
	connect(actionTile,SIGNAL(triggered()),workspace,SLOT(tile()));
	connect(actionCascade,SIGNAL(triggered()),workspace,SLOT(cascade()));
	connect(fileExitAction, SIGNAL(triggered()), this, SLOT(fileExit()));
	connect(menuWindows, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
	connect(actionSave_Hdr_Preview, SIGNAL(triggered()), this, SLOT(saveHdrPreview()));

	//QSignalMapper?
	connect(actionText_Under_Icons,SIGNAL(triggered()),this,SLOT(Text_Under_Icons()));
	connect(actionIcons_Only,SIGNAL(triggered()),this,SLOT(Icons_Only()));
	connect(actionText_Alongside_Icons,SIGNAL(triggered()),this,SLOT(Text_Alongside_Icons()));
	connect(actionText_Only,SIGNAL(triggered()),this,SLOT(Text_Only()));

	windowMapper = new QSignalMapper(this);
	connect(windowMapper,SIGNAL(mapped(QWidget*)),workspace,SLOT(setActiveWindow(QWidget*)));

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
}

void MainGui::fileNewViaWizard(QStringList files) {
	HdrWizardForm *wizard;
	if (testTempDir(qtpfsgui_options->tempfilespath)) {
		wizard=new HdrWizardForm (this, files);
		if (wizard->exec() == QDialog::Accepted) {
			HdrViewer *newmdi=new HdrViewer( this, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor, true); //true means needs saving
			newmdi->updateHDR(wizard->getPfsFrameHDR());
			workspace->addWindow(newmdi);
			newmdi->setWindowTitle(wizard->getCaptionTEXT());
			newmdi->show();
		}
		delete wizard;
	}
}


void MainGui::fileOpen() {
	QString filetypes = tr("All Hdr formats ");
	filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2);;" ;
	filetypes += "OpenEXR (*.exr);;" ;
	filetypes += "Radiance RGBE (*.hdr *.pic);;";
	filetypes += "TIFF Images (*.tiff *.tif);;";
	filetypes += "RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2);;";
	filetypes += "PFS Stream (*.pfs)";
	QString filename = QFileDialog::getOpenFileName(
			this,
			tr("Load an Hdr file..."),
			RecentDirHDRSetting,
			filetypes );
	setupLoadThread(filename);
}

void MainGui::addHdrViewer(pfs::Frame* hdr_pfs_frame, QString fname) {
	HdrViewer *newhdr=new HdrViewer(this, qtpfsgui_options->negcolor, qtpfsgui_options->naninfcolor, false);
	newhdr->updateHDR(hdr_pfs_frame);
	newhdr->filename=fname;
	newhdr->setWindowTitle(fname);
	workspace->addWindow(newhdr);
	newhdr->show();
	setCurrentFile(fname);
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
	QStringList filetypes;
	filetypes += tr("All Hdr formats (*.exr *.hdr *.pic *.tiff *.tif *.pfs)");
	filetypes += "OpenEXR (*.exr)";
	filetypes += "Radiance RGBE (*.hdr *.pic)";
	filetypes += "HDR TIFF (*.tiff *.tif)";
	filetypes += "PFS Stream (*.pfs)";

	QFileDialog *fd = new QFileDialog(this);
	fd->setWindowTitle(tr("Save the HDR..."));
	fd->setDirectory(RecentDirHDRSetting);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilters(filetypes);
	fd->setAcceptMode(QFileDialog::AcceptSave);
	fd->setConfirmOverwrite(true);
	fd->setDefaultSuffix("exr");
	if (fd->exec()) {
		QString fname=(fd->selectedFiles()).at(0);
		if(!fname.isEmpty()) {
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
				if (qtpfsgui_options->saveLogLuvTiff)
					tiffwriter.writeLogLuvTiff();
				else
					tiffwriter.writeFloatTiff();
			} else if (qfi.suffix().toUpper()=="PFS") {
				pfs::DOMIO pfsio;
				(currenthdr->getHDRPfsFrame())->convertRGBChannelsToXYZ();
				pfsio.writeFrame(currenthdr->getHDRPfsFrame(),encodedName);
				(currenthdr->getHDRPfsFrame())->convertXYZChannelsToRGB();
			} else {
				QMessageBox::warning(this,tr("Aborting..."), tr("Qtpfsgui supports only the following formats: <br>Radiance RGBE (hdr), PFS, tiff-hdr and OpenEXR."),
				QMessageBox::Ok,QMessageBox::NoButton);
				delete fd;
				return;
			}
			free(encodedName);
			setCurrentFile(absoluteFileName);
			currenthdr->NeedsSaving=false;
			currenthdr->filename=absoluteFileName;
			currenthdr->setWindowTitle(absoluteFileName);
		}
	} //if (fd->exec())
	delete fd;
}

void MainGui::saveHdrPreview() {
	if(currenthdr==NULL)
		return;
	currenthdr->saveHdrPreview();
}

void MainGui::updateActions( QWidget * w ) {
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
		currenthdr=(HdrViewer*)(workspace->activeWindow());
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
	} else {
		currenthdr=NULL;
		normalSizeAct->setEnabled(false);
		zoomInAct->setEnabled(false);
		zoomOutAct->setEnabled(false);
		fitToWindowAct->setEnabled(false);
	}
}

void MainGui::tonemap_requested() {
	if(currenthdr==NULL)
		return;
	if (testTempDir(qtpfsgui_options->tempfilespath)) {
		this->setDisabled(true);
		TonemappingWindow *tmodialog=new TonemappingWindow(this, currenthdr->getHDRPfsFrame(), currenthdr->filename);
		connect(tmodialog,SIGNAL(closing()),this,SLOT(reEnableMainWin()));
		tmodialog->show();
		tmodialog->setAttribute(Qt::WA_DeleteOnClose);
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
	if (! currenthdr->NeedsSaving) {
		currenthdr->NeedsSaving=true;
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
		if (! currenthdr->NeedsSaving) {
			currenthdr->NeedsSaving=true;
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
		if (! currenthdr->NeedsSaving) {
			currenthdr->NeedsSaving=true;
			currenthdr->setWindowTitle(currenthdr->windowTitle().prepend("(*) "));
		}
		QApplication::restoreOverrideCursor();
	}
	delete projTranfsDialog;
}

void MainGui::current_mdi_decrease_exp() {
	currenthdr->lumRange->decreaseExposure();
}
void MainGui::current_mdi_extend_exp() {
	currenthdr->lumRange->extendRange();
}
void MainGui::current_mdi_fit_exp() {
	currenthdr->lumRange->fitToDynamicRange();
}
void MainGui::current_mdi_increase_exp() {
	currenthdr->lumRange->increaseExposure();
}
void MainGui::current_mdi_shrink_exp() {
	currenthdr->lumRange->shrinkRange();
}
void MainGui::current_mdi_ldr_exp() {
	currenthdr->lumRange->lowDynamicRange();
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
	LoadHdrThread *loadthread = new LoadHdrThread(fname, RecentDirHDRSetting);
	connect(loadthread, SIGNAL(updateRecentDirHDRSetting(QString)), this, SLOT(updateRecentDirHDRSetting(QString)));
	connect(loadthread, SIGNAL(hdr_ready(pfs::Frame*,QString)), this, SLOT(addHdrViewer(pfs::Frame*,QString)));
	connect(loadthread, SIGNAL(load_failed(QString)), this, SLOT(load_failed(QString)));
	loadthread->start();
}

void MainGui::load_failed(QString error_message) {
	QMessageBox::critical(0,tr("Aborting..."), error_message, QMessageBox::Ok,QMessageBox::NoButton);
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
		QWidgetList allhdrs=workspace->windowList();
		foreach(QWidget *p,allhdrs) {
			((HdrViewer*)p)->update_colors(qtpfsgui_options->negcolor,qtpfsgui_options->naninfcolor);
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
}

void MainGui::fileExit() {
	QWidgetList allhdrs=workspace->windowList();
	bool closeok=true;
	foreach(QWidget *p,allhdrs) {
		if (((HdrViewer*)p)->NeedsSaving)
			closeok=false;
	}
	if (closeok || (QMessageBox::warning(this,tr("Unsaved changes..."),tr("There is at least one Hdr with unsaved changes.<br>Do you still want to quit?"),
#if QT_VERSION <= 0x040200
			QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,QMessageBox::NoButton)
		== QMessageBox::Yes))
#else
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
		== QMessageBox::Yes))
#endif
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
#if QT_VERSION >= 0x040200
	ui.authorsBox->setOpenExternalLinks(true);
	ui.thanksToBox->setOpenExternalLinks(true);
	ui.GPLbox->setTextInteractionFlags(Qt::TextSelectableByMouse);
#endif
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
#if QT_VERSION >= 0x040200
		ui.GPLbox->setOpenExternalLinks(true);
		ui.GPLbox->setTextInteractionFlags(Qt::TextBrowserInteraction);
#endif
		ui.GPLbox->setHtml(tr("%1 License document not found, you can find it online: %2here%3","%2 and %3 are html tags").arg("<html>").arg("<a href=\"http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\">").arg("</a></html>"));
	}
	about->show();
}

void MainGui::updateWindowMenu() {
	menuWindows->clear();
	QList<QWidget *> windows = workspace->windowList();
	for (int i = 0; i < windows.size(); ++i) {
		HdrViewer *child = qobject_cast<HdrViewer *>(windows.at(i));
		QString text=QString((i < 9)?"&":"") + QString("%1 %2").arg(i + 1).arg(QFileInfo((child->filename.isEmpty())? "Untitled":child->filename).fileName());
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
