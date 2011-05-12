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

#include "MainWindow/MainWindow.h"
#include "MainWindow/DnDOption.h"

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
#include "Viewers/LdrViewer.h"
#include "Common/ImageQualityDialog.h"


MainWindow::MainWindow(QWidget *p) : QMainWindow(p), active_frame(NULL), helpBrowser(NULL), current_state(IO_STATE)
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
    mdiArea->setBackground(QBrush( QColor::fromRgb(192, 192, 192)) );

    // Mac OS X only has tabbed images
#ifdef Q_WS_MAC
    mdiArea->setViewMode(QMdiArea::TabbedView);
    mdiArea->setDocumentMode(true);
#endif

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

    // progress bar
    m_progressbar = new QProgressBar(this);
    m_progressbar->hide();
    statusBar()->addWidget(m_progressbar);

    // I/O
    setup_io();

    setup_tm();
    setup_tm_slots();

    setupConnections();
    cropToSelectionAction->setEnabled(false);

    // SPLASH SCREEN    ----------------------------------------------------------------------
    if (settings->contains("ShowSplashScreen"))
    {
        if (settings->value("ShowSplashScreen").toInt())
            showSplash();
    }
    else
        showSplash();
    // END SPLASH SCREEN    ------------------------------------------------------------------

    testTempDir(luminance_options->tempfilespath);
    statusBar()->showMessage(tr("Ready. Now open an existing HDR image or create a new one!"), 10000);

    // align on the right the tonemap action
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // toolBar is a pointer to an existing toolbar
    toolBar->insertWidget(TonemapAction, spacer);

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

    QStringList files = QFileDialog::getOpenFileNames(this,
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
    RecentDirHDRSetting = newvalue;                                 // update class member
    settings->setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, newvalue);    // update settings
}

void MainWindow::updateRecentDirLDRSetting(QString newvalue)
{
    RecentDirLDRSetting = newvalue;                         // update class member
    settings->setValue(KEY_RECENT_PATH_SAVE_LDR, newvalue); // update settings
}

void MainWindow::fileSaveAs()
{
    QMdiSubWindow* c_f = mdiArea->activeSubWindow();
    if (c_f == NULL) return; // could happen if the mdi area is empty. Ideally MainWindow should have this disabled if no file is open

    GenericViewer* g_v = (GenericViewer*)c_f->widget();
    if ( g_v == NULL ) return; // should never happen

    if ( g_v->isHDR() )
    {
        /*
         * In this case I'm saving an HDR
         */
        QString filetypes = tr("All HDR formats ");
        filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS);;" ;
        filetypes += "OpenEXR (*.exr *.EXR);;" ;
        filetypes += "Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;";
        filetypes += "HDR TIFF (*.tiff *.tif *.TIFF *.TIF);;";
        filetypes += "PFS Stream (*.pfs *.PFS)";

        QString fname = QFileDialog::getSaveFileName(this,
                                                     tr("Save the HDR image as..."),
                                                     RecentDirHDRSetting,
                                                     filetypes);

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

            emit save_hdr_frame(dynamic_cast<HdrViewer*>(g_v), fname);
        }
    }
    else
    {
        /*
         * In this case I'm saving an LDR
         */
        LdrViewer* l_v = dynamic_cast<LdrViewer*>(g_v);

        QString filetypes = QObject::tr("All LDR formats") + " (*.jpg *.jpeg *.png *.ppm *.pbm *.bmp *.JPG *.JPEG *.PNG *.PPM *.PBM *.BMP);;";
        filetypes += "JPEG (*.jpg *.jpeg *.JPG *.JPEG);;" ;
        filetypes += "PNG (*.png *.PNG);;" ;
        filetypes += "PPM PBM (*.ppm *.pbm *.PPM *.PBM);;";
        filetypes += "BMP (*.bmp *.BMP)";

        QString outfname = QFileDialog::getSaveFileName(this,
                                                QObject::tr("Save the LDR image as..."),
                                                RecentDirLDRSetting,
                                                filetypes);

        if ( !outfname.isEmpty() )
        {
            QFileInfo qfi(outfname);
            QString format = qfi.suffix();

            if ( RecentDirLDRSetting != qfi.path() )
            {
                updateRecentDirLDRSetting( qfi.path() );
            }

            if ( format.isEmpty() )
            {
                // default as JPG
                format    =   "jpg";
                outfname  +=  ".jpg";
            }

            int quality = 100; // default value is 100%
            if ( format == "png" || format == "jpg" )
            {
                ImageQualityDialog savedFileQuality(l_v->getQImage(), format, this);
                QString winTitle(QObject::tr("Save as..."));
                winTitle += format.toUpper();
                savedFileQuality.setWindowTitle( winTitle );
                if ( savedFileQuality.exec() == QDialog::Rejected )
                    return;
                else
                    quality = savedFileQuality.getQuality();
            }
            emit save_ldr_frame(l_v, outfname, quality);
        }
    }
}

void MainWindow::save_hdr_success(HdrViewer* saved_hdr, QString fname)
{
    QFileInfo qfi(fname);
    QString absoluteFileName = qfi.absoluteFilePath();

    setCurrentFile(absoluteFileName);
    saved_hdr->setFileName(fname);
    saved_hdr->setWindowTitle(absoluteFileName);
}

void MainWindow::save_hdr_failed()
{
    // TODO give some kind of feedback to the user!
    // TODO pass the name of the file, so the user know which file didn't save correctly
}

void MainWindow::save_ldr_success(LdrViewer* saved_ldr, QString fname)
{
    saved_ldr->setFileName(fname);
    saved_ldr->setWindowTitle(QFileInfo(fname).absoluteFilePath());
}

void MainWindow::save_ldr_failed()
{
    // TODO give some kind of feedback to the user!
    // TODO pass the name of the file, so the user know which file didn't save correctly
    QMessageBox::warning(0,"",QObject::tr("Failed to save"), QMessageBox::Ok, QMessageBox::NoButton);
    //QMessageBox::warning(0,"",QObject::tr("Failed to save <b>") + outfname + "</b>", QMessageBox::Ok, QMessageBox::NoButton);
}

void MainWindow::saveHdrPreview()
{
    // TODO : rewrite this function!
    //if (active_frame==NULL)
    //    return;
    //active_frame->saveHdrPreview();
}

void MainWindow::updateActions( QMdiSubWindow * w )
{
    if ( w == NULL )
    {
        //no file open currently! (is it true?!)
        if (mdiArea->subWindowList().empty())
        {
            active_frame = NULL;
            if ( current_state == TM_STATE ) tonemap_requested();

            fileSaveAsAction->setEnabled(false);
            actionSave_Hdr_Preview->setEnabled(false);

            normalSizeAct->setEnabled(false);
            zoomInAct->setEnabled(false);
            zoomOutAct->setEnabled(false);
            fitToWindowAct->setEnabled(false);

            TonemapAction->setEnabled(false);
            action_Projective_Transformation->setEnabled(false);
            cropToSelectionAction->setEnabled(false);

            rotateccw->setEnabled(false);
            rotatecw->setEnabled(false);

            menuHDR_Histogram->setEnabled(false);
            Low_dynamic_range->setEnabled(false);
            Fit_to_dynamic_range->setEnabled(false);
            Shrink_dynamic_range->setEnabled(false);
            Extend_dynamic_range->setEnabled(false);
            Decrease_exposure->setEnabled(false);
            Increase_exposure->setEnabled(false);
            actionResizeHDR->setEnabled(false);
        }
        // else I consider the previous state, because there is still a file open but it is not "on focus"
    }
    else
    {
        if ( current_state == IO_STATE )
        {
            fileSaveAsAction->setEnabled(true);
            rotateccw->setEnabled(true);
            rotatecw->setEnabled(true);

            normalSizeAct->setEnabled(true);
            zoomInAct->setEnabled(true);
            zoomOutAct->setEnabled(true);
            fitToWindowAct->setEnabled(true);

            // IO_STATE
            active_frame = (GenericViewer*)(mdiArea->activeSubWindow()->widget());
            if ( active_frame->isHDR() )
            {   // current selected file is an HDR

                TonemapAction->setEnabled(true);
                // disable levels

                action_Projective_Transformation->setEnabled(true);
                actionSave_Hdr_Preview->setEnabled(true);
                cropToSelectionAction->setEnabled(true);

                rotateccw->setEnabled(true);
                rotatecw->setEnabled(true);

                menuHDR_Histogram->setEnabled(true);
                Low_dynamic_range->setEnabled(true);
                Fit_to_dynamic_range->setEnabled(true);
                Shrink_dynamic_range->setEnabled(true);
                Extend_dynamic_range->setEnabled(true);
                Decrease_exposure->setEnabled(true);
                Increase_exposure->setEnabled(true);
                actionResizeHDR->setEnabled(true);
            }
            else
            {   // current selected file is an LDR

                TonemapAction->setEnabled(false);
                // enable levels

                // projective transformation
                action_Projective_Transformation->setEnabled(false);
                // hdr preview save
                actionSave_Hdr_Preview->setEnabled(false);
                cropToSelectionAction->setEnabled(false);

                rotateccw->setEnabled(false);
                rotatecw->setEnabled(false);

                menuHDR_Histogram->setEnabled(false);
                Low_dynamic_range->setEnabled(false);
                Fit_to_dynamic_range->setEnabled(false);
                Shrink_dynamic_range->setEnabled(false);
                Extend_dynamic_range->setEnabled(false);
                Decrease_exposure->setEnabled(false);
                Increase_exposure->setEnabled(false);
                actionResizeHDR->setEnabled(false);
            }
        }
        else
        {
            // If I'm here, some features have been already disable by tonemap_request()

            // TM_STATE
            GenericViewer* curr_file = (GenericViewer*)(mdiArea->activeSubWindow()->widget());
            if (curr_file->isHDR())
            {
                // Do nothing: in TM_STATE, there is only one HDR frame open, the one currently under Tonemapping
            }
            else
            {
                // update the preview frame, so that at the next tonemapping this one will be refreshed!
                // preview_frame = curr_frame;
            }

        }
    }

//    bool state = (w != NULL);

//    action_Projective_Transformation->setEnabled(state);
//    actionSave_Hdr_Preview->setEnabled(state);
//
//
//
//
//    menuHDR_Histogram->setEnabled(state);
//    Low_dynamic_range->setEnabled(state);
//    Fit_to_dynamic_range->setEnabled(state);
//    Shrink_dynamic_range->setEnabled(state);
//    Extend_dynamic_range->setEnabled(state);
//    Decrease_exposure->setEnabled(state);
//    Increase_exposure->setEnabled(state);
//    actionResizeHDR->setEnabled(state);

//    if (state)
//    {
//        currenthdr = (HdrViewer*)(mdiArea->activeSubWindow()->widget());
//        if (currenthdr->isFittedToWindow())
//        {
//            normalSizeAct->setEnabled(false);
//            zoomInAct->setEnabled(false);
//            zoomOutAct->setEnabled(false);
//            fitToWindowAct->setEnabled(true);
//        }
//        else
//        {
//            zoomOutAct->setEnabled(currenthdr->getScaleFactor() > 0.222);
//            zoomInAct->setEnabled(currenthdr->getScaleFactor() < 3.0);
//            fitToWindowAct->setEnabled(true);
//            normalSizeAct->setEnabled(true);
//        }
//        if (currenthdr->hasSelection())
//        {
//            cropToSelectionAction->setEnabled(true);
//            removeSelectionAction->setEnabled(true);
//        }
//        else
//        {
//            cropToSelectionAction->setEnabled(false);
//            removeSelectionAction->setEnabled(false);
//        }
//    }
//    else
//    {

//    }
}

void MainWindow::setActiveSubWindow(QWidget* w)
{
    // This function sets the active window in the mdi area when the file name is selected inside the "Window" menu
    QList<QMdiSubWindow*> allhdrs=mdiArea->subWindowList();
    foreach(QMdiSubWindow *p,allhdrs)
        if (p->widget() == w)
            mdiArea->setActiveSubWindow(p);
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
    if (active_frame == NULL)
        return;

    rotateccw->setEnabled(false);
    rotatecw->setEnabled(false);
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    pfs::Frame *rotated = pfs::rotateFrame(active_frame->getHDRPfsFrame(), clockwise);
    //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
    active_frame->updateHDR(rotated);
    if ( !active_frame->needsSaving() )
    {
        active_frame->setNeedsSaving(true);
        active_frame->setWindowTitle(active_frame->windowTitle().prepend("(*) "));
    }
    QApplication::restoreOverrideCursor();
    rotateccw->setEnabled(true);
    rotatecw->setEnabled(true);
}

void MainWindow::resize_requested()
{
    if (active_frame == NULL)
        return;

    ResizeDialog *resizedialog = new ResizeDialog(this, active_frame->getHDRPfsFrame());
    if (resizedialog->exec() == QDialog::Accepted)
    {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
        active_frame->updateHDR(resizedialog->getResizedFrame());
        if (! active_frame->needsSaving())
        {
            active_frame->setNeedsSaving(true);
            active_frame->setWindowTitle(active_frame->windowTitle().prepend("(*) "));
        }
        QApplication::restoreOverrideCursor();
    }
    delete resizedialog;
}

void MainWindow::projectiveTransf_requested()
{
    if (active_frame==NULL)
        return;

    ProjectionsDialog *projTranfsDialog = new ProjectionsDialog(this,active_frame->getHDRPfsFrame());
    if (projTranfsDialog->exec() == QDialog::Accepted)
    {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
        active_frame->updateHDR(projTranfsDialog->getTranformedFrame());
        if (! active_frame->needsSaving()) {
            active_frame->setNeedsSaving(true);
            active_frame->setWindowTitle(active_frame->windowTitle().prepend("(*) "));
        }
        QApplication::restoreOverrideCursor();
    }
    delete projTranfsDialog;
}

void MainWindow::current_mdi_decrease_exp()
{
    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(active_frame);
    if ( hdr_viewer != NULL )
        hdr_viewer->lumRange()->decreaseExposure();
}

void MainWindow::current_mdi_extend_exp()
{
    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(active_frame);
    if ( hdr_viewer != NULL )
        hdr_viewer->lumRange()->extendRange();
}

void MainWindow::current_mdi_fit_exp()
{
    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(active_frame);
    if ( hdr_viewer != NULL )
        hdr_viewer->lumRange()->fitToDynamicRange();
}

void MainWindow::current_mdi_increase_exp()
{
    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(active_frame);
    if ( hdr_viewer != NULL )
        hdr_viewer->lumRange()->increaseExposure();
}

void MainWindow::current_mdi_shrink_exp()
{
    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(active_frame);
    if ( hdr_viewer != NULL )
        hdr_viewer->lumRange()->shrinkRange();
}

void MainWindow::current_mdi_ldr_exp()
{
    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(active_frame);
    if ( hdr_viewer != NULL )
        hdr_viewer->lumRange()->lowDynamicRange();
}

// This bunch of functions need revision because
// when the focus changes to a new function, we need to update the scale factor actions
void MainWindow::current_mdi_zoomin()
{
    QMdiSubWindow* c_f = mdiArea->activeSubWindow();
    if ( c_f != NULL )
    {
        GenericViewer* g_v = (GenericViewer*)c_f->widget();

        g_v->zoomIn();
        zoomOutAct->setEnabled(true);
        zoomInAct->setEnabled(g_v->getScaleFactor() < 3.0);
    }
}

void MainWindow::current_mdi_zoomout()
{
    QMdiSubWindow* c_f = mdiArea->activeSubWindow();
    if ( c_f != NULL )
    {
        GenericViewer* g_v = (GenericViewer*)c_f->widget();

        g_v->zoomOut();
        zoomInAct->setEnabled(true);
        zoomOutAct->setEnabled(g_v->getScaleFactor() > 0.222);
    }
}

void MainWindow::current_mdi_fit_to_win(bool checked)
{
    QMdiSubWindow* c_f = mdiArea->activeSubWindow();
    if ( c_f != NULL )
    {
        GenericViewer* g_v = (GenericViewer*)c_f->widget();

        g_v->fitToWindow(checked);
        zoomInAct->setEnabled(!checked);
        zoomOutAct->setEnabled(!checked);
        normalSizeAct->setEnabled(!checked);
    }
}

void MainWindow::current_mdi_original_size()
{
    QMdiSubWindow* c_f = mdiArea->activeSubWindow();
    if ( c_f != NULL )
    {
        GenericViewer* g_v = (GenericViewer*)c_f->widget();

        g_v->normalSize();
        zoomInAct->setEnabled(true);
        zoomOutAct->setEnabled(true);
    }
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

void MainWindow::setup_io()
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

    // Save HDR
    connect(this, SIGNAL(save_hdr_frame(HdrViewer*, QString)), IO_Worker, SLOT(write_hdr_frame(HdrViewer*, QString)));
    connect(IO_Worker, SIGNAL(write_hdr_success(HdrViewer*, QString)), this, SLOT(save_hdr_success(HdrViewer*, QString)));
    connect(IO_Worker, SIGNAL(write_hdr_failed()), this, SLOT(save_hdr_failed()));
    // Save LDR
    connect(this, SIGNAL(save_ldr_frame(LdrViewer*, QString, int)), IO_Worker, SLOT(write_ldr_frame(LdrViewer*, QString, int)));
    connect(IO_Worker, SIGNAL(write_ldr_success(LdrViewer*, QString)), this, SLOT(save_ldr_success(LdrViewer*, QString)));
    connect(IO_Worker, SIGNAL(write_ldr_failed()), this, SLOT(save_ldr_failed()));

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

    this->mdiArea->addSubWindow(subWindow);
    this->setCurrentFile(new_fname);

    newhdr->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);

    subWindow->setWidget(newhdr);
    subWindow->resize((int) (0.66 * this->mdiArea->width()),(int)(0.66 * this->mdiArea->height()));
    subWindow->showMaximized();

    connect(newhdr, SIGNAL(selectionReady(bool)), this, SLOT(enableCrop(bool)));
    newhdr->setSelectionTool(true);

    newhdr->updateHDR(new_hdr_frame);
    newhdr->setFileName(new_fname);
    newhdr->setWindowTitle(new_fname);

    newhdr->normalSize();
    newhdr->fitToWindow(true);

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
    QList<QMdiSubWindow*> allhdrs = mdiArea->subWindowList();
    bool closeok = true;
    foreach (QMdiSubWindow *p, allhdrs)
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
        action->setChecked(child == active_frame);
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

void MainWindow::cropToSelection()
{
    // if active_frame != NULL
    // if active_frame is Type (HdrViewer)

    QRect cropRect = active_frame->getSelectionRect();
    int x_ul, y_ul, x_br, y_br;
    cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
    disableCrop();
    pfs::Frame *original_frame = active_frame->getHDRPfsFrame();
    HdrViewer *newHdrViewer = new HdrViewer(this, true, false, luminance_options->negcolor, luminance_options->naninfcolor);
    pfs::Frame *cropped_frame = pfs::pfscut(original_frame, x_ul, y_ul, x_br, y_br);

    newHdrViewer->updateHDR(cropped_frame);
    newHdrViewer->setFileName(QString(tr("Cropped Image")));
    newHdrViewer->setWindowTitle(QString(tr("Cropped Image")));
    newHdrViewer->setSelectionTool(true);

    newHdrViewer->setFlagUpdateImage(false); // disable updating image for performance

    //TODO: check this!
    //float min =0.0f, max= 0.0f;
    //float min = active_frame->lumRange()->getRangeWindowMin();
    //float max = active_frame->lumRange()->getRangeWindowMax();
    //int lumMappingMode = active_frame->getLumMappingMethod();
    //newHdrViewer->lumRange()->setRangeWindowMinMax(min, max);
    //newHdrViewer->setLumMappingMethod(lumMappingMode);

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
    cropToSelectionAction->setEnabled(isReady);
    removeSelectionAction->setEnabled(isReady);
}

void MainWindow::disableCrop()
{
    active_frame->removeSelection();
    cropToSelectionAction->setEnabled(false);
    removeSelectionAction->setEnabled(false);
}

void MainWindow::closeEvent ( QCloseEvent *event )
{
    QList<QMdiSubWindow*> allhdrs = mdiArea->subWindowList();
    bool closeok = true;
    foreach (QMdiSubWindow *p, allhdrs)
    {
        // TODO: check this stuff!
        if (((HdrViewer*)p->widget())->needsSaving())
        {
            closeok = false;
        }
    }
    if (closeok)
    {
        settings->setValue("MainWindowGeometry", saveGeometry());
        QWidget::closeEvent(event);
        emit close();
    }
    else
    {
        int ret = UMessageBox::warning(tr("Unsaved changes..."),
                                       tr("There is at least one HDR image with unsaved changes.<br>Do you still want to quit?"),
                                       this);

        if ( ret == QMessageBox::Yes )
        {
            settings->setValue("MainWindowGeometry", saveGeometry());
            QWidget::closeEvent(event);
            emit close();
        }
    }
}


void MainWindow::setup_tm()
{
    // create tonemapping panel
    dock = new QDockWidget(tr("Tone Mapping Options"), this);
    dock->setObjectName("Tone Mapping Options"); // for save and restore docks state

    //dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    //dock->setFeatures(QDockWidget::DockWidgetClosable);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    tmPanel = new TonemappingPanel(dock);
    dock->setWidget(tmPanel);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    // by default, it is hidden!
    dock->hide();
}

void MainWindow::setup_tm_slots()
{
    connect(tmPanel, SIGNAL(startTonemapping(TonemappingOptions*)), this, SLOT(tonemapImage(TonemappingOptions*)));
    //connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(tmDockVisibilityChanged(bool)));
}

void MainWindow::tmDockVisibilityChanged(bool /*b*/)
{
    //printf("tmDockVisibilityChanged() \n");
    //tonemap_requested();
}

void MainWindow::tonemap_requested()
{
    //printf("tonemap_requested() \n");
    if ( dock->isHidden() )
    {
        // set state to TM_STATE
        current_state = TM_STATE;

        // update TM struct
        tm_status.curr_tm_frame = active_frame->getHDRPfsFrame();

        // go through the list of open files and remove them from the MDI, except the current one
        QList<QMdiSubWindow*> allhdrs = mdiArea->subWindowList();
        foreach(QMdiSubWindow *p, allhdrs)
        {
            GenericViewer* c_v = (GenericViewer*)p->widget();
            if ( c_v != active_frame )
            {
                // it's not the current HDR frame, so I remove it from the MDI area...
                mdiArea->removeSubWindow(p);
                // ...and I add the current pointer into my temp QList<GenericViewer*>
                tm_status.hidden_windows.push_back(p);
            }
        }

        // disable Input
        fileNewAction->setEnabled(false);
        fileOpenAction->setEnabled(false);

        // enable TM
        tmPanel->applyButton->setEnabled(true);

        // set sizes into tonemapping panel
        tmPanel->setSizes((tm_status.curr_tm_frame)->getWidth(), (tm_status.curr_tm_frame)->getHeight());

        dock->show(); // it must be the last line of this branch!
    }
    else if ( !dock->isHidden() )
    {
        // set state to IO
        current_state = IO_STATE;

        // reset TM struct
        tm_status.curr_tm_frame = NULL;

        // go through the list of SubMdi that I currently hold into the temporary QList
        // and I insert them into the MDI area
        foreach(QMdiSubWindow *p, tm_status.hidden_windows)
        {
            mdiArea->addSubWindow(p);
            p->show();
        }
        // I clear the QList, to it will be empty next time I use it
        tm_status.hidden_windows.clear();

        // enable Input
        fileNewAction->setEnabled(true);
        fileOpenAction->setEnabled(true);

        // disable TM
        tmPanel->applyButton->setEnabled(false);

        dock->hide(); // It must be the last line of this branch!
    }

//    if (currenthdr==NULL)
//        return;
//    this->setDisabled(true);
//    try {
//        TonemappingWindow *tmodialog=new TonemappingWindow(this, currenthdr->getHDRPfsFrame(), currenthdr->getFileName());
//        tmodialog->setAttribute(Qt::WA_DeleteOnClose);
//        //tmodialog->setAttribute(Qt::WA_Window);
//        connect(tmodialog,SIGNAL(closing()),this,SLOT(reEnableMainWin()));
//        tmodialog->show();
//#ifndef WIN32
//        // Why only windows?
//        hide();
//#endif
//        if (helpBrowser)
//            helpBrowser->hide();
//    }
//    catch (pfs::Exception e) {
//        QMessageBox::warning(this,tr("Luminance HDR"),tr("Error: %1 ").arg(e.getMessage()));
//        reEnableMainWin();
//    }
//    catch (...) {
//        QMessageBox::warning(this,tr("Luminance HDR"),tr("Error: Failed to Tonemap Image"));
//        reEnableMainWin();
//    }
}

void MainWindow::tonemapImage(TonemappingOptions *opts)
{
    //printf("ToneMapping!\n");
    // This function needs an heavy clean up!

    tm_status.curr_tm_options = opts;

//    if ( opts->tonemapOriginal )
//    {
//        tm_status.curr_tm_frame = active_frame->getHDRPfsFrame();

//        if ( opts->tonemapSelection )
//        {
//            if ( originalHDR->hasSelection() )
//            {
//                getCropCoords(originalHDR,
//                              opts->selection_x_up_left,
//                              opts->selection_y_up_left,
//                              opts->selection_x_bottom_right,
//                              opts->selection_y_bottom_right);
//            }
//            else
//            {
//                QMessageBox::critical(this,tr("Luminance HDR"),tr("Please make a selection of the HDR image to tonemap."), QMessageBox::Ok);
//                return;
//            }
//        }
//    }
//    else // if ( !opts.tonemapOriginal )
//    {
//        if (!mdiArea->subWindowList().isEmpty())
//        {
//            GenericViewer *viewer = (GenericViewer *) mdiArea->activeSubWindow()->widget();

//            if ( viewer->isHDR() )
//            {
//                HdrViewer *HDR = (HdrViewer *) mdiArea->activeSubWindow()->widget();
//                workingPfsFrame = HDR->getHDRPfsFrame();
//            }
//            else
//            {
//                QMessageBox::critical(this,tr("Luminance HDR"),tr("Please select an HDR image to tonemap."), QMessageBox::Ok);
//                return;
//            }

//            if ( opts->tonemapSelection )
//            {
//                if ( originalHDR->hasSelection() )
//                {
//                    getCropCoords(originalHDR,
//                                  opts->selection_x_up_left,
//                                  opts->selection_y_up_left,
//                                  opts->selection_x_bottom_right,
//                                  opts->selection_y_bottom_right);
//                }
//                else
//                {
//                    QMessageBox::critical(this,tr("Luminance HDR"),tr("Please make a selection of the HDR image to tonemap."), QMessageBox::Ok);
//                    return;
//                }
//            }
//        }
//        else
//        {
//            QMessageBox::critical(this,tr("Luminance HDR"),tr("Please select an HDR image to tonemap."), QMessageBox::Ok);
//            return;
//        }
//    }

    TMOThread *thread = TMOFactory::getTMOThread(opts->tmoperator, tm_status.curr_tm_frame, opts);
    progInd = new TMOProgressIndicator(this);

    connect(thread, SIGNAL(imageComputed(QImage*)), this, SLOT(addMDIResult(QImage*)));
    connect(thread, SIGNAL(processedFrame(pfs::Frame *)), this, SLOT(addProcessedFrame(pfs::Frame *)));
    connect(thread, SIGNAL(setMaximumSteps(int)), progInd, SLOT(setMaximum(int)));
    connect(thread, SIGNAL(setValue(int)), progInd, SLOT(setValue(int)));
    connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showErrorMessage(const char *)));
    connect(thread, SIGNAL(finished()), progInd, SLOT(terminated()));
    connect(thread, SIGNAL(finished()), this, SLOT(tonemappingFinished()));
    connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
    connect(progInd, SIGNAL(terminate()), thread, SLOT(terminateRequested()));

    //start thread
    tmPanel->applyButton->setEnabled(false);
    thread->startTonemapping();
    statusBar()->addWidget(progInd);
}

void MainWindow::addMDIResult(QImage* image)
{
        LdrViewer *n = new LdrViewer( image, this, false, false, tm_status.curr_tm_options);

        n->normalSize();
        //n->showMaximized(); // That's to have mdi subwin size right (don't ask me why)

//        if (actionFit_to_Window->isChecked())
//                n->fitToWindow(true);
        QMdiSubWindow *subwin = new QMdiSubWindow(this);
        subwin->setAttribute(Qt::WA_DeleteOnClose);
        subwin->setWidget(n);
        mdiArea->addSubWindow(subwin);

        //n->showMaximized();

        if (luminance_options->tmowindow_max)
                n->showMaximized();
        else
                n->showNormal();

        subwin->installEventFilter(this);

        //connect(n,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
        //connect(n,SIGNAL(levels_closed()),this,SLOT(levels_closed()));
}

void MainWindow::addProcessedFrame(pfs::Frame *frame)
{
        HdrViewer *HDR = new HdrViewer(this, false, false, luminance_options->negcolor, luminance_options->naninfcolor);
        HDR->setFreePfsFrameOnExit(true);
        HDR->updateHDR(frame);
        HDR->setFileName(QString(tr("Processed HDR")));
        HDR->setWindowTitle(QString(tr("Processed HDR")));
        HDR->setSelectionTool(true);
        HDR->normalSize();
        HDR->showMaximized();
//        if (actionFit_to_Window->isChecked())
//                HDR->fitToWindow(true);
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

        //connect(HDR,SIGNAL(changed(GenericViewer *)),this,SLOT(dispatch(GenericViewer *)));
}

void MainWindow::tonemappingFinished()
{
        std::cout << "TonemappingWindow::tonemappingFinished()" << std::endl;
        statusBar()->removeWidget(progInd);
        tmPanel->applyButton->setEnabled(true);

        delete progInd;
}

void MainWindow::deleteTMOThread(TMOThread *th)
{
        delete th;
}

void MainWindow::showErrorMessage(const char *e)
{
        QMessageBox::critical(this,tr("Luminance HDR"),tr("Error: %1").arg(e),
                        QMessageBox::Ok,QMessageBox::NoButton);

        statusBar()->removeWidget(progInd);
        tmPanel->applyButton->setEnabled(true);

        delete progInd;
}
