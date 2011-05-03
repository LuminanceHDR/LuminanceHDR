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

    restoreState(settings->value("MainWindowState").toByteArray());
    restoreGeometry(settings->value("MainWindowGeometry").toByteArray());

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
    statusBar()->showMessage(tr("Ready. Now open an existing HDR image or create a new one!"), 10000);

    // progress bar
    m_progressbar = new QProgressBar(this);
    m_progressbar->hide();
    this->statusBar()->addWidget(m_progressbar);

    // I/O
    initIOThread();

    setupConnections();
    cropToSelectionAction->setEnabled(false);

    // SPLASH SCREEN
    if (settings->contains("ShowSplashScreen"))
    {
        if (settings->value("ShowSplashScreen").toInt())
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

    if ( !files.isEmpty() )
    {
        // Update working folder
        // All the files are in the same folder, so I pick the first as reference to update the settings
        QFileInfo qfi(files.first());
        if ( RecentDirHDRSetting != qfi.path() )
        {
            // if the new dir (the one just chosen by the user)
            // is different from the one stored in the settings
            // update the settings
            updateRecentDirHDRSetting(qfi.path());
        }

        emit open_frames(files);
    }
}

void MainWindow::updateRecentDirHDRSetting(QString newvalue)
{
    // update internal field variable
    RecentDirHDRSetting=newvalue;
    settings->setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, RecentDirHDRSetting);
}

void MainWindow::fileSaveAs()
{
    if (currenthdr == NULL)
        return;

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

    if ( !fname.isEmpty() )
    {
        // Update working folder
        QFileInfo qfi(fname);
        if ( RecentDirHDRSetting != qfi.path() )
        {
            // if the new dir (the one just chosen by the user)
            // is different from the one stored in the settings
            // update the settings
            updateRecentDirHDRSetting(qfi.path());
        }

        emit save_frame(currenthdr, fname);
    }
}

void MainWindow::save_success(HdrViewer* saved_hdr, QString fname)
{
    QFileInfo qfi(fname);
    QString absoluteFileName = qfi.absoluteFilePath();

    setCurrentFile(absoluteFileName);
    saved_hdr->setFileName(fname);
    saved_hdr->setWindowTitle(absoluteFileName);
}

void MainWindow::save_failed()
{
    // TODO give some kind of feedback to the user!
}

void MainWindow::saveHdrPreview()
{
    if (currenthdr==NULL)
        return;
    currenthdr->saveHdrPreview();
}

void MainWindow::updateActions( QMdiSubWindow * w )
{
    bool state = (w != NULL);

    action_Projective_Transformation->setEnabled(state);
    actionSave_Hdr_Preview->setEnabled(state);
    TonemapAction->setEnabled(state);
    fileSaveAsAction->setEnabled(state);
    rotateccw->setEnabled(state);
    rotatecw->setEnabled(state);
    menuHDR_Histogram->setEnabled(state);
    Low_dynamic_range->setEnabled(state);
    Fit_to_dynamic_range->setEnabled(state);
    Shrink_dynamic_range->setEnabled(state);
    Extend_dynamic_range->setEnabled(state);
    Decrease_exposure->setEnabled(state);
    Increase_exposure->setEnabled(state);
    actionResizeHDR->setEnabled(state);

    if (state)
    {
        currenthdr = (HdrViewer*)(mdiArea->activeSubWindow()->widget());
        if (currenthdr->isFittedToWindow())
        {
            normalSizeAct->setEnabled(false);
            zoomInAct->setEnabled(false);
            zoomOutAct->setEnabled(false);
            fitToWindowAct->setEnabled(true);
        }
        else
        {
            zoomOutAct->setEnabled(currenthdr->getScaleFactor() > 0.222);
            zoomInAct->setEnabled(currenthdr->getScaleFactor() < 3.0);
            fitToWindowAct->setEnabled(true);
            normalSizeAct->setEnabled(true);
        }
        if (currenthdr->hasSelection())
        {
            cropToSelectionAction->setEnabled(true);
            removeSelectionAction->setEnabled(true);
        }
        else
        {
            cropToSelectionAction->setEnabled(false);
            removeSelectionAction->setEnabled(false);
        }
    }
    else
    {
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

void MainWindow::setActiveSubWindow(QWidget* w)
{
    QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
    foreach(QMdiSubWindow *p,allhdrs)
        if (p->widget() == w)
            mdiArea->setActiveSubWindow(p);
}

void MainWindow::tonemap_requested()
{
    if (currenthdr==NULL)
        return;
    this->setDisabled(true);
    try {
        TonemappingWindow *tmodialog=new TonemappingWindow(this, currenthdr->getHDRPfsFrame(), currenthdr->getFileName());
        tmodialog->setAttribute(Qt::WA_DeleteOnClose);
        //tmodialog->setAttribute(Qt::WA_Window);
        connect(tmodialog,SIGNAL(closing()),this,SLOT(reEnableMainWin()));
        tmodialog->show();
#ifndef WIN32
        // Why only windows?
        hide();
#endif
        if (helpBrowser)
            helpBrowser->hide();
    }
    catch (pfs::Exception e) {
        QMessageBox::warning(this,tr("Luminance HDR"),tr("Error: %1 ").arg(e.getMessage()));
        reEnableMainWin();
    }
    catch (...) {
        QMessageBox::warning(this,tr("Luminance HDR"),tr("Error: Failed to Tonemap Image"));
        reEnableMainWin();
    }
}

bool MainWindow::testTempDir(QString dirname)
{
    QFileInfo test(dirname);
    if ( test.isWritable() && test.exists() && test.isDir() )
    {
        return true;
    }
    else
    {
        // TODO: Universal Message Box!

        QMessageBox::critical(this,tr("Error..."),tr("Luminance HDR needs to cache its results using temporary files, but the currently selected directory is not valid.<br>Please choose a valid path in Tools -> Preferences... -> Tonemapping."),
                              QMessageBox::Ok,QMessageBox::NoButton);
        return false;
    }
}

void MainWindow::reEnableMainWin()
{
    setEnabled(true);
    show();
    if (helpBrowser)
        helpBrowser->show();
}

void MainWindow::rotateccw_requested()
{
    dispatchrotate(false);
}

void MainWindow::rotatecw_requested()
{
    dispatchrotate(true);
}

void MainWindow::dispatchrotate(bool clockwise)
{
    if (currenthdr == NULL)
        return;

    rotateccw->setEnabled(false);
    rotatecw->setEnabled(false);
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    pfs::Frame *rotated = pfs::rotateFrame(currenthdr->getHDRPfsFrame(), clockwise);
    //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
    currenthdr->updateHDR(rotated);
    if ( !currenthdr->needsSaving() )
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
    if (currenthdr == NULL)
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

void MainWindow::projectiveTransf_requested()
{
    if (currenthdr==NULL)
        return;

    ProjectionsDialog *projTranfsDialog = new ProjectionsDialog(this,currenthdr->getHDRPfsFrame());
    if (projTranfsDialog->exec() == QDialog::Accepted)
    {
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

void MainWindow::current_mdi_decrease_exp()
{
	currenthdr->lumRange()->decreaseExposure();
}

void MainWindow::current_mdi_extend_exp()
{
	currenthdr->lumRange()->extendRange();
}

void MainWindow::current_mdi_fit_exp()
{
	currenthdr->lumRange()->fitToDynamicRange();
}

void MainWindow::current_mdi_increase_exp()
{
	currenthdr->lumRange()->increaseExposure();
}

void MainWindow::current_mdi_shrink_exp()
{
	currenthdr->lumRange()->shrinkRange();
}

void MainWindow::current_mdi_ldr_exp()
{
	currenthdr->lumRange()->lowDynamicRange();
}

void MainWindow::current_mdi_zoomin()
{
	currenthdr->zoomIn();
	zoomOutAct->setEnabled(true);
	zoomInAct->setEnabled(currenthdr->getScaleFactor() < 3.0);
}

void MainWindow::current_mdi_zoomout()
{
	currenthdr->zoomOut();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(currenthdr->getScaleFactor() > 0.222);
}

void MainWindow::current_mdi_fit_to_win(bool checked)
{
	currenthdr->fitToWindow(checked);
	zoomInAct->setEnabled(!checked);
	zoomOutAct->setEnabled(!checked);
	normalSizeAct->setEnabled(!checked);
}

void MainWindow::current_mdi_original_size()
{
	currenthdr->normalSize();
	zoomInAct->setEnabled(true);
	zoomOutAct->setEnabled(true);
}

void MainWindow::openDocumentation()
{
	helpBrowser = new HelpBrowser(this,"Luminance HDR Help");
	helpBrowser->setAttribute(Qt::WA_DeleteOnClose);
	connect(helpBrowser, SIGNAL(closed()), this, SLOT(helpBrowserClosed()));
	helpBrowser->show();
}

void MainWindow::helpBrowserClosed()
{
	helpBrowser = NULL;
}

void MainWindow::enterWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    QStringList files = settings->value(KEY_RECENT_FILES).toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
    {
        files.removeLast();
    }

    settings->setValue(KEY_RECENT_FILES, files);
    updateRecentFileActions();
}

void MainWindow::updateRecentFileActions()
{
    QStringList files = settings->value(KEY_RECENT_FILES).toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);
    separatorRecentFiles->setVisible(numRecentFiles > 0);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
    {
        recentFileActs[j]->setVisible(false);
    }
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        emit open_frame(action->data().toString());
    }
}

void MainWindow::initIOThread()
{
    // Init Object/Thread
    IO_thread = new QThread;
    IO_Worker = new IOWorker;

    IO_Worker->moveToThread(IO_thread);

    // Open
    connect(this, SIGNAL(open_frame(QString)), IO_Worker, SLOT(read_frame(QString)));
    connect(this, SIGNAL(open_frames(QStringList)), IO_Worker, SLOT(read_frames(QStringList)));
    connect(IO_Worker, SIGNAL(read_success(pfs::Frame*, QString)), this, SLOT(load_success(pfs::Frame*, QString)));
    connect(IO_Worker, SIGNAL(read_failed(QString)), this, SLOT(load_failed(QString)));

    // Save
    connect(this, SIGNAL(save_frame(HdrViewer*, QString)), IO_Worker, SLOT(write_frame(HdrViewer*, QString)));
    connect(IO_Worker, SIGNAL(write_success(HdrViewer*, QString)), this, SLOT(save_success(HdrViewer*, QString)));
    connect(IO_Worker, SIGNAL(write_failed()), this, SLOT(save_failed()));

    // progress bar handling
    connect(IO_Worker, SIGNAL(setValue(int)), this, SLOT(ProgressBarSetValue(int)));
    connect(IO_Worker, SIGNAL(setMaximum(int)), this, SLOT(ProgressBarSetMaximum(int)));
    connect(IO_Worker, SIGNAL(IO_init()), this, SLOT(IO_start()));
    connect(IO_Worker, SIGNAL(IO_finish()), this, SLOT(IO_done()));

    // start thread waiting for signals (I/O requests)
    IO_thread->start();
}

void MainWindow::load_failed(QString error_message)
{
    // TODO: use unified style?
    QMessageBox::critical(this,tr("Aborting..."), error_message, QMessageBox::Ok, QMessageBox::NoButton);
}

void MainWindow::load_success(pfs::Frame* new_hdr_frame, QString new_fname)
{
    // Create new MdiSubWindow and HdrViewer
    QMdiSubWindow* subWindow = new QMdiSubWindow(this);
    HdrViewer * newhdr = new HdrViewer(subWindow, false, false, luminance_options->negcolor, luminance_options->naninfcolor);

    newhdr->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);

    subWindow->setWidget(newhdr);

    connect(newhdr, SIGNAL(selectionReady(bool)), this, SLOT(enableCrop(bool)));
    newhdr->setSelectionTool(true);

    newhdr->updateHDR(new_hdr_frame);
    newhdr->setFileName(new_fname);
    newhdr->setWindowTitle(new_fname);

    newhdr->normalSize();
    newhdr->fitToWindow(true);
    subWindow->resize((int) (0.66 * this->mdiArea->width()),(int)(0.66 * this->mdiArea->height()));
    this->mdiArea->addSubWindow(subWindow);
    this->setCurrentFile(new_fname);

    subWindow->showMaximized();
    newhdr->show();
}

void MainWindow::IO_start()
{
    //QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // Init Progress Bar
    ProgressBarInit();
}

void MainWindow::IO_done()
{
    // Stop Progress Bar
    ProgressBarFinish();

    //QApplication::restoreOverrideCursor();
}

void MainWindow::preferences_called()
{
    unsigned int negcol=luminance_options->negcolor;
    unsigned int naninfcol=luminance_options->naninfcolor;
    PreferencesDialog *opts=new PreferencesDialog(this);
    opts->setAttribute(Qt::WA_DeleteOnClose);
    if (opts->exec() == QDialog::Accepted && (negcol!=luminance_options->negcolor || naninfcol!=luminance_options->naninfcolor) )
    {
        QList<QMdiSubWindow*> allhdrs = mdiArea->subWindowList();
        foreach (QMdiSubWindow *p, allhdrs)
        {
            ((HdrViewer*)p->widget())->update_colors(luminance_options->negcolor,luminance_options->naninfcolor);
        }
    }
}

void MainWindow::transplant_called()
{
	TransplantExifDialog *transplant=new TransplantExifDialog(this);
	transplant->setAttribute(Qt::WA_DeleteOnClose);
	transplant->exec();
}

void MainWindow::load_options() {
	//load from settings the path where hdrs have been previously opened/loaded
	RecentDirHDRSetting=settings->value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString();

	//load from settings the main toolbar visualization mode
	if (!settings->contains(KEY_TOOLBAR_MODE))
		settings->setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
	switch (settings->value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt()) {
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

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        QStringList files = convertUrlListToFilenameList(event->mimeData()->urls());
        if (files.size() > 0)
        {
            DnDOptionDialog dndOption(this, files);
            dndOption.exec();

            switch (dndOption.result) {
            case 1: // create new using LDRS
                fileNewViaWizard(files);
                break;
            case 2: // openHDRs
                emit open_frames(files);
                break;
            }
        }
    }
    event->acceptProposedAction();
}

MainWindow::~MainWindow()
{
    // let them delete themself
    IO_Worker->deleteLater();
    IO_thread->deleteLater();

    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        delete recentFileActs[i];
    }
    settings->setValue("MainWindowState", saveState());
}

void MainWindow::fileExit()
{
    QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
    bool closeok = true;
    foreach (QMdiSubWindow *p,allhdrs)
    {
        if (((HdrViewer*)p->widget())->needsSaving())
        {
            closeok = false;
        }
    }
    if (closeok)
    {
        emit close();
    }
    else
    {
        int ret = UMessageBox::warning(tr("Unsaved changes..."),
                                       tr("There is at least one HDR image with unsaved changes.<br>Do you still want to quit?"),
                                       this);

        if ( ret == QMessageBox::Yes )
        {
            emit close();
        }
    }
}

void MainWindow::Text_Under_Icons()
{
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    settings->setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon);
}

void MainWindow::Icons_Only()
{
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    settings->setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonIconOnly);
}

void MainWindow::Text_Alongside_Icons()
{
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settings->setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextBesideIcon);
}

void MainWindow::Text_Only()
{
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    settings->setValue(KEY_TOOLBAR_MODE,Qt::ToolButtonTextOnly);
}

void MainWindow::showSplash()
{
    // TODO: change implementation with a static member of UMessageBox
    splash=new QDialog(this);
    splash->setAttribute(Qt::WA_DeleteOnClose);
    Ui::SplashLuminance ui;
    ui.setupUi(splash);
    connect(ui.yesButton, SIGNAL(clicked()), this, SLOT(splashShowDonationsPage()));
    connect(ui.noButton, SIGNAL(clicked()), this, SLOT(splashClose()));
    connect(ui.askMeLaterButton, SIGNAL(clicked()), splash, SLOT(close()));

    splash->show();
}

void MainWindow::splashShowDonationsPage()
{
    showDonationsPage();
    splash->close();
}

void MainWindow::splashClose()
{
    settings->setValue("ShowSplashScreen", 0);
    splash->close();
}

void MainWindow::aboutLuminance()
{
    UMessageBox::about();
}

void MainWindow::updateWindowMenu()
{
    menuWindows->clear();
    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    for (int i = 0; i < windows.size(); ++i)
    {
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

void MainWindow::batch_requested()
{
    BatchTMDialog *batchdialog = new BatchTMDialog(this);
    batchdialog->exec();
    delete batchdialog;
}

void MainWindow::ProgressBarSetMaximum(int max)
{
    m_progressbar->setMaximum( max - 1 );
}

void MainWindow::ProgressBarSetValue(int value)
{
    m_progressbar->setValue( value );
}

void MainWindow::ProgressBarInit(void)
{
    statusBar()->clearMessage();

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    //m_progressbar->setValue( -1 );
    m_progressbar->setMaximum( 0 );
    m_progressbar->show();
}

void MainWindow::ProgressBarFinish(void)
{
    m_progressbar->hide();
    m_progressbar->reset();
    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(tr("Done!"), 10000);
}

void MainWindow::cropToSelection(void)
{
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

void MainWindow::enableCrop(bool isReady)
{
    if (isReady)
    {
        cropToSelectionAction->setEnabled(true);
        removeSelectionAction->setEnabled(true);
    }
    else
    {
        cropToSelectionAction->setEnabled(false);
        removeSelectionAction->setEnabled(false);
    }
}

void MainWindow::disableCrop()
{
	currenthdr->removeSelection();
	cropToSelectionAction->setEnabled(false);
	removeSelectionAction->setEnabled(false);
}

void MainWindow::closeEvent ( QCloseEvent *event ) {
	settings->setValue("MainWindowGeometry", saveGeometry());
	QWidget::closeEvent(event);
}


