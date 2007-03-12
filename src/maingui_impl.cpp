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

#include "maingui_impl.h"
#include "imagehdrviewer.h"
#include "tonemappingdialog_impl.h"
#include "../generated_uic/ui_help_about.h"
#include "pfsindcraw.h"
#include "config.h"
#include "transplant_impl.h"
#include "align_impl.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

pfs::Frame* readEXRfile  (const char * filename);
pfs::Frame* readRGBEfile (const char * filename);
pfs::Frame* rotateFrame( pfs::Frame* inputpfsframe, bool clock_wise );
void writeRGBEfile (pfs::Frame* inputpfsframe, const char* outfilename);
void writeEXRfile  (pfs::Frame* inputpfsframe, const char* outfilename);

static const QString helptext = 
"<p align=\"center\"><b><font size=\"+4\"><u>Q t p f s g u i</u></b></p>\
<p align=\"center\">v"QTPFSGUIVERSION " (Qt4)</p>\
<hr> <h1>Overview and Usage</h1> \
<p>This is a graphical user interface (GUI) that operates on High dynamic range (HDR) images.</p> \
<p>If you use this software to create HDRs and/or to Tonemap an HDR to a LDR, please specify somewhere that you used this program. For example use a \"qtpfsgui\" tag in flickr.</p> \
<p>The <b>Open</b>&#160;<img src=\":/new/prefix1/images/fileopen.png\">,&#160; <b>New</b>   <img src=\":/new/prefix1/images/newfile.png\"> &#160;and <b>Save as...</b><img src=\":/new/prefix1/images/filesave.png\">&#160; buttons in the main window operate on HDR images: the \"<b>New</b>\" button pops up a wizard. <br>\
Throughout the guided procedure you will be asked about the location of your source images, and the configuration you want to use to create the HDR.</p> \
<h2>The HDR configurations in the wizard</h2>\
<p>Usually one of the 6 default configurations leads to very good results, but if you want you can enable the \"<b>custom configuration</b>\" checkbox that will prompt you with all the possibilities. <br>\
Let me know if you find one combination (that is not already in the default ones) that creates good results for you.</p>\
<br><hr>\
<h1>Tonemapping the HDR into a LDR</h1>\
<p>Once you have a window with a HDR in it you can tonemap it to a Low Dynamic Range (LDR) image via the <b>Tonemap</b>  <img src=\":/new/prefix1/images/tonemap.png\"> button, that creates a new dialog where you can fiddle with the various <b><i>tone mapping operators</i></b> (<b>TMO</b>s) and related settings.\
It is usually a good idea to work on a smaller scale image while trying out the TMOs, because they can be very cpu-intensive.<br>\
Once you are satisfied with the results you can click on the Save button:\
<br><img src=\":/new/prefix1/images/filesaveas.png\"><br>\
to save the resulting <b><i>low dynamic range</i></b> (<b>LDR</b>) image.</p>\
<hr><h1>Contact</h1>\
<p>If you use this program, please, please let me know:</p>\
<ul>\
<li>if you find any bug.\
<li>if you find any usability issue.\
</ul>\
<p>Even if you just use this program and find it useful, it would be great if you would let me know.</p>\
Giuseppe Rota<br>\
grota at users.sourceforge.net";


MainGui::MainGui(QWidget *p) : QMainWindow(p), settings("Qtpfsgui", "Qtpfsgui")
{
	setupUi(this);
	connect(this->fileExitAction, SIGNAL(triggered()), this, SLOT(close()));
	workspace = new QWorkspace(this);
	workspace->setScrollBarsEnabled( TRUE );
	setCentralWidget(workspace);

	RecentDirHDRSetting=settings.value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString();
	qtpfsgui_options=new qtpfsgui_opts();
	load_options(qtpfsgui_options);

	setWindowTitle("Qtpfsgui v"QTPFSGUIVERSION);
	connect(workspace, SIGNAL(windowActivated(QWidget*)), this, SLOT(updatecurrentMDIwindow(QWidget *)) );
	connect(fileNewAction, SIGNAL(triggered()), this, SLOT(fileNewViaWizard()));
	connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
	connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
	connect(TonemapAction, SIGNAL(triggered()), this, SLOT(tonemap_requested()));
	connect(rotateccw, SIGNAL(triggered()), this, SLOT(rotateccw_requested()));
	connect(rotatecw, SIGNAL(triggered()), this, SLOT(rotatecw_requested()));
	connect(actionResizeHDR, SIGNAL(triggered()), this, SLOT(resize_requested()));
	connect(Low_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdiwindow_ldr_exposure()));
	connect(Fit_to_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdiwindow_fit_exposure()));
	connect(Shrink_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdiwindow_shrink_exposure()));
	connect(Extend_dynamic_range,SIGNAL(triggered()),this,SLOT(current_mdiwindow_extend_exposure()));
	connect(Decrease_exposure,SIGNAL(triggered()),this,SLOT(current_mdiwindow_decrease_exposure()));
	connect(Increase_exposure,SIGNAL(triggered()),this,SLOT(current_mdiwindow_increase_exposure()));
	connect(zoomInAct,SIGNAL(triggered()),this,SLOT(current_mdiwindow_zoomin()));
	connect(zoomOutAct,SIGNAL(triggered()),this,SLOT(current_mdiwindow_zoomout()));
	connect(fitToWindowAct,SIGNAL(toggled(bool)),this,SLOT(current_mdiwindow_fit_to_win(bool)));
	connect(normalSizeAct,SIGNAL(triggered()),this,SLOT(current_mdiwindow_original_size()));
// 	connect(menuView,SIGNAL(aboutToShow()),this,SLOT(viewMenuAboutToShow()));
	connect(helpAboutAction,SIGNAL(triggered()),this,SLOT(helpAbout()));
	connect(OptionsAction,SIGNAL(triggered()),this,SLOT(options_called()));
	connect(Transplant_Exif_Data_action,SIGNAL(triggered()),this,SLOT(transplant_called()));
	connect(actionAlign_Images,SIGNAL(triggered()),this,SLOT(align_called()));

        for (int i = 0; i < MaxRecentFiles; ++i) {
            recentFileActs[i] = new QAction(this);
            recentFileActs[i]->setVisible(false);
            connect(recentFileActs[i], SIGNAL(triggered()),
                    this, SLOT(openRecentFile()));
        }
        separatorRecentFiles = menuFile->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
		menuFile->addAction(recentFileActs[i]);
	updateRecentFileActions();

	this->showMaximized();
	statusBar()->showMessage("Ready.... Now open an HDR or create one!",17000);
}

void MainGui::fileNewViaWizard() {
	wizard=new HdrWizardForm (this,&(qtpfsgui_options->dcraw_options));
	if (wizard->exec() == QDialog::Accepted) {
		ImageMDIwindow *newmdi=new ImageMDIwindow( this,qtpfsgui_options->negcolor,qtpfsgui_options->naninfcolor );
		newmdi->updateHDR(wizard->getPfsFrameHDR());
		workspace->addWindow(newmdi);
// 		newmdi->toolBar->addAction(TonemapAction);
		newmdi->setWindowTitle(wizard->getCaptionTEXT());
		newmdi->show();
	}
	if (wizard != NULL)
		delete wizard;
}

void MainGui::fileOpen()
{	
	QString filetypes;
#ifndef _WIN32
	filetypes += "OpenEXR (*.exr);;";
#endif
	filetypes += "Radiance RGBE (*.hdr *.HDR *.pic *.PIC);;";
	filetypes += "PFS Stream (*.pfs *.PFS);;";
	filetypes += "RAW Images (*.crw *.CRW *.cr2 *CR2 *.nef *.NEF *.dng *.DNG *.mrw *.MRW *.olf *.OLF *.kdc *.KDC *.dcr *DCR *.arw *.ARW *.raf *.RAF *.ptx *.PTX *.pef *.PEF *.x3f *.X3F)";
// 	filetypes += "CANON RAW (*.crw *.CRW *.cr2 *CR2);;";
// 	filetypes += "NIKON RAW (*.nef *.NEF);;";
// 	filetypes += "ADOBE DNG (*.dng *.DNG);;";
// 	filetypes += "MINOLTA RAW (*.mrw *.MRW);;";
// 	filetypes += "OLYMPUS RAW (*.olf *.OLF);;";
// 	filetypes += "KODAK RAW (*.kdc *.KDC *.dcr *DCR);;";
// 	filetypes += "SONY RAW (*.arw *.ARW);;";
// 	filetypes += "FUJI RAW (*.raf *.RAF);;";
// 	filetypes += "PENTAX RAW (*.ptx *.PTX *.pef *.PEF);;";
// 	filetypes += "SIGMA RAW (*.x3f *.X3F)";
	QString opened = QFileDialog::getOpenFileName(
			this,
			"Choose a HDR file to OPEN...",
			RecentDirHDRSetting,
			filetypes );
	if (loadFile(opened))
		setCurrentFile(opened);
}
bool MainGui::loadFile(QString opened) {
if( ! opened.isEmpty() ) {
	QFileInfo qfi(opened);
	// update internal field variable
	RecentDirHDRSetting=qfi.path();
	// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
	if (RecentDirHDRSetting != settings.value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString()) {
		settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, RecentDirHDRSetting);
	}
	if (!qfi.isReadable()) {
	    QMessageBox::warning(this,"Aborting...","File is not readable (check existence, permissions,...)",
				 QMessageBox::Ok,QMessageBox::NoButton);
	    return false;
	}
	ImageMDIwindow *nuova;
	pfs::Frame* hdrpfsframe = NULL;
	QString extension=qfi.suffix().toUpper();
	bool rawinput=(extension!="PFS")&&(extension!="EXR")&&(extension!="HDR");
#ifndef _WIN32
	if (extension=="EXR") {
		hdrpfsframe = readEXRfile(qfi.filePath().toAscii().constData());
	} else
#endif
	       if (extension=="HDR") {
		hdrpfsframe = readRGBEfile(qfi.filePath().toAscii().constData());
	} else if (extension=="PFS") {
		pfs::DOMIO pfsio;
		FILE * fp=fopen(qfi.filePath().toAscii().constData(),"rb");
		hdrpfsframe=pfsio.readFrame(fp);
		hdrpfsframe->convertXYZChannelsToRGB();
	} 
	else if (rawinput) {
		hdrpfsframe = readRAWfile(qfi.filePath().toAscii().constData(), &(qtpfsgui_options->dcraw_options));
		assert(hdrpfsframe!=NULL);
	}
	else {
		QMessageBox::warning(this,"Aborting...","We only support <br>Radiance rgbe (hdr), PFS, raw, and exr (linux only) <br>files up until now.",
				 QMessageBox::Ok,QMessageBox::NoButton);
		return false;
	}
	nuova=new ImageMDIwindow(this,qtpfsgui_options->negcolor,qtpfsgui_options->naninfcolor);
	nuova->updateHDR(hdrpfsframe);
	workspace->addWindow(nuova);
// 	nuova->toolBar->addAction(TonemapAction);
	nuova->setWindowTitle(opened);
	nuova->show();
	return true;
}
return false;
}

void MainGui::fileSaveAs()
{
	QStringList filetypes;
#ifndef __WIN32
	filetypes += "OpenEXR (*.exr)";
#endif
	filetypes += "Radiance RGBE (*.hdr *.HDR *.pic *.PIC)";
	filetypes += "PFS Stream (*.pfs *.PFS)";

	QFileDialog *fd = new QFileDialog(this);
	fd->setWindowTitle("SAVE the HDR to...");
	fd->setDirectory(RecentDirHDRSetting);
// 	fd->selectFile(...);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilters(filetypes);
	fd->setAcceptMode(QFileDialog::AcceptSave);
	fd->setConfirmOverwrite(true);
#ifdef _WIN32
	fd->setDefaultSuffix("hdr");
#else
	fd->setDefaultSuffix("exr");
#endif
	if (fd->exec()) {
		QString fname=(fd->selectedFiles()).at(0);
		if(!fname.isEmpty()) {
			QFileInfo qfi(fname);
			// update internal field variable
			RecentDirHDRSetting=qfi.path();
			// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
			if (RecentDirHDRSetting != settings.value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString()) {
				settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, RecentDirHDRSetting);
			}
#ifndef _WIN32
			if (qfi.suffix().toUpper()=="EXR") {
				writeEXRfile  (((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame(),qfi.filePath().toAscii().constData());
			} else 
#endif
				if (qfi.suffix().toUpper()=="HDR") {
				writeRGBEfile (((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame(),qfi.filePath().toAscii().constData());
			} else if (qfi.suffix().toUpper()=="PFS") {
				pfs::DOMIO pfsio;
				FILE * fp=fopen(qfi.filePath().toAscii().constData(),"wb");
				(((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame())->convertRGBChannelsToXYZ();
				pfsio.writeFrame(((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame(),fp);
				(((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame())->convertXYZChannelsToRGB();
			} else {
				QMessageBox::warning(this,"Aborting...","We only support <br>EXR(linux only), HDR, and PFS files up until now.",
					QMessageBox::Ok,QMessageBox::NoButton);
			}
			setCurrentFile(fname);
		}
	}
	delete fd;
}

void MainGui::updatecurrentMDIwindow( QWidget * w )
{
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
		ImageMDIwindow* current=(ImageMDIwindow*)(workspace->activeWindow());
// 		current->update_colors(qtpfsgui_options->negcolor,qtpfsgui_options->naninfcolor);
		if (current->getFittingWin()) {
			normalSizeAct->setEnabled(false);
			zoomInAct->setEnabled(false);
			zoomOutAct->setEnabled(false);
			fitToWindowAct->setEnabled(true);
		} else {
			zoomOutAct->setEnabled(current->getScaleFactor() > 0.222);
			zoomInAct->setEnabled(current->getScaleFactor() < 3.0);
			fitToWindowAct->setEnabled(true);
			normalSizeAct->setEnabled(true);
		}
	} else {
		normalSizeAct->setEnabled(false);
		zoomInAct->setEnabled(false);
		zoomOutAct->setEnabled(false);
		fitToWindowAct->setEnabled(false);
	}
}

void MainGui::tonemap_requested() {
	TMODialog *tmodialog=new TMODialog(this,qtpfsgui_options->keepsize);
	tmodialog->setOrigBuffer(((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame());
	tmodialog->exec();
	delete tmodialog;
}

void MainGui::rotateccw_requested() {
	dispatchrotate(false);
}

void MainGui::rotatecw_requested() {
	dispatchrotate(true);
}

void MainGui::dispatchrotate( bool clockwise) {
	if ((((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame())==NULL) {
		QMessageBox::warning(this,"","Problem with original (source) buffer...",
					QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	rotateccw->setEnabled(false);
	rotatecw->setEnabled(false);
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	pfs::Frame *rotated=rotateFrame(((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame(),clockwise);
	//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
	((ImageMDIwindow*)(workspace->activeWindow()))->updateHDR(rotated);
	QApplication::restoreOverrideCursor();
	rotateccw->setEnabled(true);
	rotatecw->setEnabled(true);
}

void MainGui::resize_requested() {
	if ((((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame())==NULL) {
		QMessageBox::warning(this,"","Problem with original (source) buffer...",
					QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	ResizeDialog *resizedialog=new ResizeDialog(this,((ImageMDIwindow*)(workspace->activeWindow()))->getHDRPfsFrame());
	if (resizedialog->exec() == QDialog::Accepted) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		//updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
		((ImageMDIwindow*)(workspace->activeWindow()))->updateHDR(resizedialog->getResizedFrame());
		QApplication::restoreOverrideCursor();
	}
	delete resizedialog;
}

void MainGui::current_mdiwindow_decrease_exposure() {
((ImageMDIwindow*)(workspace->activeWindow()))->lumRange->decreaseExposure();
}
void MainGui::current_mdiwindow_extend_exposure() {
((ImageMDIwindow*)(workspace->activeWindow()))->lumRange->extendRange();
}
void MainGui::current_mdiwindow_fit_exposure() {
((ImageMDIwindow*)(workspace->activeWindow()))->lumRange->fitToDynamicRange();
}
void MainGui::current_mdiwindow_increase_exposure() {
((ImageMDIwindow*)(workspace->activeWindow()))->lumRange->increaseExposure();
}
void MainGui::current_mdiwindow_shrink_exposure() {
((ImageMDIwindow*)(workspace->activeWindow()))->lumRange->shrinkRange();
}
void MainGui::current_mdiwindow_ldr_exposure() {
((ImageMDIwindow*)(workspace->activeWindow()))->lumRange->lowDynamicRange();
}
void MainGui::current_mdiwindow_zoomin() {
	ImageMDIwindow* current=(ImageMDIwindow*)(workspace->activeWindow());
	current->zoomIn();
	zoomOutAct->setEnabled(true);
	zoomInAct->setEnabled(current->getScaleFactor() < 3.0);
}
void MainGui::current_mdiwindow_zoomout() {
	ImageMDIwindow* current=(ImageMDIwindow*)(workspace->activeWindow());
	current->zoomOut();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(current->getScaleFactor() > 0.222);
}
void MainGui::current_mdiwindow_fit_to_win(bool checked) {
	((ImageMDIwindow*)(workspace->activeWindow()))->fitToWindow(checked);
	zoomInAct->setEnabled(!checked);
	zoomOutAct->setEnabled(!checked);
	normalSizeAct->setEnabled(!checked);
}
void MainGui::current_mdiwindow_original_size() {
	((ImageMDIwindow*)(workspace->activeWindow()))->normalSize();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(true);
}

void MainGui::helpAbout() {
	QDialog *help=new QDialog();
	help->setAttribute(Qt::WA_DeleteOnClose);
	Ui::HelpDialog ui;
	ui.setupUi(help);
	ui.tb->setHtml(helptext);
	help->show();
}

void MainGui::updateRecentFileActions() {
	QStringList files = settings.value(KEY_RECENT_FILES).toStringList();
	
	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);
	separatorRecentFiles->setVisible(numRecentFiles > 0);
	
	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);
}

QString MainGui::strippedName(const QString &fullFileName) {
	return QFileInfo(fullFileName).fileName();
}

void MainGui::openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		loadFile(action->data().toString());
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

void MainGui::options_called() {
	unsigned int negcol=qtpfsgui_options->negcolor;
	unsigned int naninfcol=qtpfsgui_options->naninfcolor;
	QtpfsguiOptions *opts=new QtpfsguiOptions(this,qtpfsgui_options,&settings);
	opts->setAttribute(Qt::WA_DeleteOnClose);
	if (opts->exec() == QDialog::Accepted && (negcol!=qtpfsgui_options->negcolor || naninfcol!=qtpfsgui_options->naninfcolor) ) {
		QWidgetList allhdrs=workspace->windowList();
		foreach(QWidget *p,allhdrs) {
			((ImageMDIwindow*)p)->update_colors(qtpfsgui_options->negcolor,qtpfsgui_options->naninfcolor);
		}
	}
}

void MainGui::transplant_called() {
	TransplantExifDialog *transplant=new TransplantExifDialog(this);
	transplant->setAttribute(Qt::WA_DeleteOnClose);
	transplant->exec();
}

void MainGui::align_called() {
	AlignDialog *aligndialog=new AlignDialog(this);
	aligndialog->setAttribute(Qt::WA_DeleteOnClose);
	aligndialog->exec();
}

void MainGui::load_options(qtpfsgui_opts *dest) {
	settings.beginGroup(GROUP_DCRAW);
		if (!settings.contains(KEY_AUTOWB))
			settings.setValue(KEY_AUTOWB,false);
		dest->dcraw_options.auto_wb=settings.value(KEY_AUTOWB,false).toBool();

		if (!settings.contains(KEY_CAMERAWB))
			settings.setValue(KEY_CAMERAWB,true);
		dest->dcraw_options.camera_wb=settings.value(KEY_CAMERAWB,true).toBool();

		if (!settings.contains(KEY_HIGHLIGHTS))
			settings.setValue(KEY_HIGHLIGHTS,0);
		dest->dcraw_options.highlights=settings.value(KEY_HIGHLIGHTS,0).toInt();

		if (!settings.contains(KEY_QUALITY))
			settings.setValue(KEY_QUALITY,2);
		dest->dcraw_options.quality=settings.value(KEY_QUALITY,2).toInt();

		if (!settings.contains(KEY_4COLORS))
			settings.setValue(KEY_4COLORS,false);
		dest->dcraw_options.four_colors=settings.value(KEY_4COLORS,false).toBool();

		if (!settings.contains(KEY_OUTCOLOR))
			settings.setValue(KEY_OUTCOLOR,4);
		dest->dcraw_options.output_color_space=settings.value(KEY_OUTCOLOR,4).toInt();
	settings.endGroup();

	settings.beginGroup(GROUP_HDRVISUALIZATION);
		if (!settings.contains(KEY_NANINFCOLOR))
			settings.setValue(KEY_NANINFCOLOR,0xFF000000);
		dest->naninfcolor=settings.value(KEY_NANINFCOLOR,0xFF000000).toUInt();
	
		if (!settings.contains(KEY_NEGCOLOR))
			settings.setValue(KEY_NEGCOLOR,0xFF000000);
		dest->negcolor=settings.value(KEY_NEGCOLOR,0xFF000000).toUInt();
	settings.endGroup();

	settings.beginGroup(GROUP_TONEMAPPING);
		if (!settings.contains(KEY_KEEPSIZE))
			settings.setValue(KEY_KEEPSIZE,false);
		dest->keepsize=settings.value(KEY_KEEPSIZE,false).toBool();
	settings.endGroup();
}

MainGui::~MainGui() {
	delete qtpfsgui_options;
	for (int i = 0; i < MaxRecentFiles; ++i) {
		delete recentFileActs[i];
	}
	//separatorRecentFiles should not be required
}

