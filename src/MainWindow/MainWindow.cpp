/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2011 Davide Anastasia
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
 * Original Work
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Improvements, bugfixing
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 * Implementation of the SDI functionalities
 * New Central Widget based on QTabWidget
 * Division of the Central Widget using QSplitter
 *
 */

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QWhatsThis>
#include <QSignalMapper>
#include <QTextStream>
#include <QDesktopServices>
#include <QTimer>

#include "MainWindow/MainWindow.h"
#include "MainWindow/DnDOption.h"

#include "ui_Splash.h"
#include "ui_MainWindow.h"
#include "Common/archs.h"
#include "Common/config.h"
#include "Common/global.h"
#include "TonemappingPanel/TonemappingWarnDialog.h"
#include "BatchHDR/BatchHDRDialog.h"
#include "Batch/BatchTMDialog.h"
#include "Fileformat/pfs_file_format.h"
#include "Filter/pfscut.h"
#include "Filter/pfsrotate.h"
#include "TransplantExif/TransplantExifDialog.h"
#include "Viewers/HdrViewer.h"
#include "Viewers/LuminanceRangeWidget.h"
#include "Viewers/LdrViewer.h"
#include "Common/ImageQualityDialog.h"
#include "Libpfs/frame.h"
#include "UI/UMessageBox.h"
#include "PreviewPanel/PreviewPanel.h"
#include "HelpBrowser/helpbrowser.h"
#include "TonemappingPanel/TMOProgressIndicator.h"
#include "TonemappingPanel/TonemappingPanel.h"
#include "HdrWizard/HdrWizard.h"
#include "Resize/ResizeDialog.h"
#include "Projection/ProjectionsDialog.h"
#include "Preferences/PreferencesDialog.h"

int MainWindow::sm_NumMainWindows = 0;

MainWindow::MainWindow(QWidget *parent):
        QMainWindow(parent)
{
    init();

    // SPLASH SCREEN    ----------------------------------------------------------------------
    if (luminance_options.value("ShowSplashScreen", true).toBool())
    {
        showSplash();
        //UMessageBox::donationSplashMB();
    }

    // END SPLASH SCREEN    ------------------------------------------------------------------
}

MainWindow::MainWindow(pfs::Frame* curr_frame, QString new_file, bool needSaving, QWidget *parent) :
        QMainWindow(parent)

{
    init();

    emit load_success(curr_frame, new_file, needSaving);
}

MainWindow::~MainWindow()
{
    sm_NumMainWindows--;

#ifdef QT_DEBUG
    qDebug() << "MainWindow::~MainWindow() = " << sm_NumMainWindows;
#endif

    if ( sm_NumMainWindows == 0 )
    {
        // Last MainWindow is dead...
        luminance_options.setValue("MainWindowState", saveState());
        luminance_options.setValue("MainWindowGeometry", saveGeometry());
    }

    clearRecentFileActions();
}

void MainWindow::init()
{
    sm_NumMainWindows++;

    helpBrowser = NULL;
    num_ldr_generated = 0;
    curr_num_ldr_open = 0;

    if ( sm_NumMainWindows == 1)
    {
        // Register symbols on the first activation!

        qRegisterMetaType<QImage>("QImage");
        qRegisterMetaType<TonemappingOptions>("TonemappingOptions");
    }

    createUI();
    loadOptions();
    createMenus();
    createToolBar();
    createCentralWidget();
    createStatusBar();
    setupIO();
    setupTM();
    createConnections();
}

void MainWindow::createUI()
{
    m_Ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QDir dir(QDir::homePath());

#ifdef WIN32
    if (!dir.exists("LuminanceHDR"))
        dir.mkdir("LuminanceHDR");
#else
    if (!dir.exists(".LuminanceHDR"))
        dir.mkdir(".LuminanceHDR");
#endif

    restoreState(luminance_options.value("MainWindowState").toByteArray());
    restoreGeometry(luminance_options.value("MainWindowGeometry").toByteArray());

    setAcceptDrops(true);
    setWindowModified(false);
    setWindowTitle("Luminance HDR "LUMINANCEVERSION);
}

void MainWindow::createCentralWidget()
{
    // Central Widget Area
    m_centralwidget_splitter = new QSplitter; //(this);
    setCentralWidget(m_centralwidget_splitter);

    // create tonemapping panel
    tmPanel = new TonemappingPanel; //(m_centralwidget_splitter);

    m_tabwidget = new QTabWidget; //(m_centralwidget_splitter);

    m_tabwidget->setDocumentMode(true);
    m_tabwidget->setTabsClosable(true);

    m_centralwidget_splitter->addWidget(tmPanel);
    m_centralwidget_splitter->addWidget(m_tabwidget);

    m_centralwidget_splitter->setStretchFactor(0, 1);
    m_centralwidget_splitter->setStretchFactor(1, 5);

    // create preview panel
    previewPanel = new PreviewPanel(m_centralwidget_splitter);

    // add panel to central widget
    m_centralwidget_splitter->addWidget(previewPanel);
    m_centralwidget_splitter->setStretchFactor(2, 0);

    tmPanel->hide();
    previewPanel->hide();

    connect(m_tabwidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));
    connect(m_tabwidget, SIGNAL(currentChanged(int)), this, SLOT(updateActions(int)));
    connect(tmPanel, SIGNAL(startTonemapping(TonemappingOptions*)), this, SLOT(tonemapImage(TonemappingOptions*)));
    connect(this, SIGNAL(updatedHDR(pfs::Frame*)), tmPanel, SLOT(updatedHDR(pfs::Frame*)));
    connect(this, SIGNAL(destroyed()), previewPanel, SLOT(deleteLater()));
}

void MainWindow::createToolBar()
{
    //main toolbars setup
    QActionGroup *toolBarOptsGroup = new QActionGroup(this);
    toolBarOptsGroup->addAction(m_Ui->actionText_Under_Icons);
    toolBarOptsGroup->addAction(m_Ui->actionIcons_Only);
    toolBarOptsGroup->addAction(m_Ui->actionText_Alongside_Icons);
    toolBarOptsGroup->addAction(m_Ui->actionText_Only);
    m_Ui->menuToolbars->addAction(m_Ui->toolBar->toggleViewAction());

    connect(m_Ui->actionLock, SIGNAL(toggled(bool)), this, SLOT(lockViewers(bool)));
    connect(m_Ui->actionText_Under_Icons,SIGNAL(triggered()),this,SLOT(Text_Under_Icons()));
    connect(m_Ui->actionIcons_Only,SIGNAL(triggered()),this,SLOT(Icons_Only()));
    connect(m_Ui->actionText_Alongside_Icons,SIGNAL(triggered()),this,SLOT(Text_Alongside_Icons()));
    connect(m_Ui->actionText_Only,SIGNAL(triggered()),this,SLOT(Text_Only()));
}

void MainWindow::createMenus()
{
    // About(s)
    connect(m_Ui->actionAbout_Qt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));
    connect(m_Ui->actionAbout_Luminance,SIGNAL(triggered()),this,SLOT(aboutLuminance()));
    connect(m_Ui->actionDonate, SIGNAL(activated()), this, SLOT(showDonationsPage()));

    connect(m_Ui->OptionsAction,SIGNAL(triggered()),this,SLOT(preferences_called()));
    connect(m_Ui->documentationAction,SIGNAL(triggered()),this,SLOT(openDocumentation()));
    connect(m_Ui->actionWhat_s_This,SIGNAL(triggered()),this,SLOT(enterWhatsThis()));

    // I/O
    connect(m_Ui->fileNewAction, SIGNAL(triggered()), this, SLOT(fileNewViaWizard()));
    connect(m_Ui->fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
    connect(m_Ui->fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    connect(m_Ui->fileSaveAllAction, SIGNAL(triggered()), this, SLOT(fileSaveAll()));
    connect(m_Ui->fileExitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(m_Ui->actionSave_Hdr_Preview, SIGNAL(triggered()), this, SLOT(saveHdrPreview()));

    // HDR Editing
    connect(m_Ui->actionResizeHDR, SIGNAL(triggered()), this, SLOT(resize_requested()));
    connect(m_Ui->action_Projective_Transformation, SIGNAL(triggered()), this, SLOT(projectiveTransf_requested()));

    // HDR Exposure Control
    connect(m_Ui->Low_dynamic_range,SIGNAL(triggered()),this,SLOT(hdr_ldr_exp()));
    connect(m_Ui->Fit_to_dynamic_range,SIGNAL(triggered()),this,SLOT(hdr_fit_exp()));
    connect(m_Ui->Shrink_dynamic_range,SIGNAL(triggered()),this,SLOT(hdr_shrink_exp()));
    connect(m_Ui->Extend_dynamic_range,SIGNAL(triggered()),this,SLOT(hdr_extend_exp()));
    connect(m_Ui->Decrease_exposure,SIGNAL(triggered()),this,SLOT(hdr_decrease_exp()));
    connect(m_Ui->Increase_exposure,SIGNAL(triggered()),this,SLOT(hdr_increase_exp()));

    // Crop & Rotation
    connect(m_Ui->cropToSelectionAction, SIGNAL(triggered()), this, SLOT(cropToSelection()));
    m_Ui->cropToSelectionAction->setEnabled(false);

    connect(m_Ui->removeSelectionAction, SIGNAL(triggered()), this, SLOT(disableCrop()));
    connect(m_Ui->rotateccw, SIGNAL(triggered()), this, SLOT(rotateccw_requested()));
    connect(m_Ui->rotatecw, SIGNAL(triggered()), this, SLOT(rotatecw_requested()));

    // Zoom
    connect(m_Ui->zoomInAct,SIGNAL(triggered()),this,SLOT(viewerZoomIn()));
    connect(m_Ui->zoomOutAct,SIGNAL(triggered()),this,SLOT(viewerZoomOut()));
    connect(m_Ui->fitToWindowAct,SIGNAL(triggered()),this,SLOT(viewerFitToWin()));
    connect(m_Ui->actionFill_to_Window,SIGNAL(triggered()),this,SLOT(viewerFillToWin()));
    connect(m_Ui->normalSizeAct,SIGNAL(triggered()),this,SLOT(viewerOriginalSize()));

    // Tools
    connect(m_Ui->Transplant_Exif_Data_action,SIGNAL(triggered()),this,SLOT(transplant_called()));
    connect(m_Ui->actionBatch_HDR, SIGNAL(triggered()), this, SLOT(batch_hdr_requested()));
    connect(m_Ui->actionBatch_Tone_Mapping, SIGNAL(triggered()), this, SLOT(batch_requested()));

    connect(m_Ui->menuWindows, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
    connect(m_Ui->actionMinimize, SIGNAL(triggered()), this, SLOT(minimizeMW()));
    connect(m_Ui->actionMaximize, SIGNAL(triggered()), this, SLOT(maximizeMW()));
    connect(m_Ui->actionBring_All_to_Front, SIGNAL(triggered()), this, SLOT(bringAllMWToFront()));
    connect(m_Ui->actionShowPreviewPanel, SIGNAL(toggled(bool)), this, SLOT(showPreviewPanel(bool)));
    connect(m_Ui->actionShowPreviewPanel, SIGNAL(toggled(bool)), &luminance_options, SLOT(setPreviewPanelActive(bool)));
    connect(m_Ui->actionFix_Histogram,SIGNAL(toggled(bool)),this,SLOT(levelsRequested(bool)));
    connect(m_Ui->actionRemove_Tab,SIGNAL(triggered()),this,SLOT(removeCurrentTab()));

    //recent files
    initRecentFileActions();
    updateRecentFileActions();
}

void MainWindow::createStatusBar()
{
    // progress bar
    m_progressbar = new QProgressBar(this);
    m_progressbar->hide();
    statusBar()->addWidget(m_progressbar);

    statusBar()->showMessage(tr("Ready. Now open an existing HDR image or create a new one!"), 10000);
}

void MainWindow::createConnections()
{
    connect(m_Ui->actionShowNext, SIGNAL(triggered()), this, SLOT(activateNextViewer()));
    connect(m_Ui->actionShowPrevious, SIGNAL(triggered()), this, SLOT(activatePreviousViewer()));

    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveMainWindow(QWidget*)));
}

void MainWindow::loadOptions()
{
    //load from settings the path where hdrs have been previously opened/loaded
    RecentDirHDRSetting = luminance_options.getDefaultPathHdrInOut();
    RecentDirLDRSetting = luminance_options.getDefaultPathLdrOut();

    //load from settings the main toolbar visualization mode
    switch ( luminance_options.getMainWindowToolBarMode() ) {
    case Qt::ToolButtonIconOnly:
        Icons_Only();
        m_Ui->actionIcons_Only->setChecked(true);
	break;
    case Qt::ToolButtonTextOnly:
        Text_Only();
        m_Ui->actionText_Only->setChecked(true);
	break;
    case Qt::ToolButtonTextBesideIcon:
        Text_Alongside_Icons();
        m_Ui->actionText_Alongside_Icons->setChecked(true);
	break;
    case Qt::ToolButtonTextUnderIcon:
        Text_Under_Icons();
        m_Ui->actionText_Under_Icons->setChecked(true);
	break;
    }
    m_Ui->actionShowPreviewPanel->setChecked(luminance_options.isPreviewPanelActive());

}

void MainWindow::showDonationsPage()
{
  QDesktopServices::openUrl(QUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=77BSTWEH7447C")); //davideanastasia
}

void MainWindow::fileNewViaWizard(QStringList files)
{    
        HdrWizard *wizard = new HdrWizard (this, files);
        if (wizard->exec() == QDialog::Accepted)
        {
            emit load_success(wizard->getPfsFrameHDR(), wizard->getCaptionTEXT(), true);
        }
        delete wizard;
}

void MainWindow::fileOpen()
{
    QString filetypes = tr("All HDR formats ");
    filetypes += "(*.exr *.hdr *.pic *.tiff *.tif *.pfs *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.rw2 *.sr2 *.3fr *.mef *.mos *.erf *.nrw *.srw";
    filetypes +=  "*.EXR *.HDR *.PIC *.TIFF *.TIF *.PFS *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.RW2 *.SR2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW);;" ;
    filetypes += "OpenEXR (*.exr *.EXR);;" ;
    filetypes += "Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;";
    filetypes += "TIFF images (*.TIFF *.TIF *.tiff *.tif);;";
    filetypes += "RAW images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.rw2 *.sr2 *.3fr *.mef *.mos *.erf *.nrw *.mef *.mos *.erf *.nrw *.srw";
    filetypes +=             "*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.RW2 *.SR2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW);;";
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
    RecentDirHDRSetting = newvalue;                         // update class member
    luminance_options.setDefaultPathHdrInOut(newvalue);     // update settings
}

void MainWindow::updateRecentDirLDRSetting(QString newvalue)
{
    RecentDirLDRSetting = newvalue;                         // update class member
    luminance_options.setDefaultPathLdrOut(newvalue);       // update settings
}

void MainWindow::fileSaveAll()
{
    if (m_tabwidget->count() <= 0) return;

    QWidget *wgt;
    GenericViewer *g_v;
    LdrViewer *l_v;

    QString dir = QFileDialog::getExistingDirectory(0, tr("Save files in"), RecentDirLDRSetting);

    if (!dir.isEmpty())
    {
        updateRecentDirLDRSetting(dir);
        for (int i = 0; i < m_tabwidget->count(); i++)
        {
            wgt = m_tabwidget->widget(i);
            g_v = (GenericViewer *)wgt;

            if ( !g_v->isHDR() )
            {
                l_v = dynamic_cast<LdrViewer*>(g_v);

                QString ldr_name = QFileInfo(getCurrentHDRName()).baseName();
                QString outfname = RecentDirLDRSetting + "/" + ldr_name + "_" + l_v->getFilenamePostFix() + ".jpg";

                emit save_ldr_frame(l_v, outfname, 100);
            }
        }
    }
}

void MainWindow::fileSaveAs()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();

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

        QString filetypes = QObject::tr("All LDR formats");
        filetypes += " (*.jpg *.jpeg *.png *.ppm *.pbm *.bmp *.JPG *.JPEG *.PNG *.PPM *.PBM *.BMP);;";
        filetypes += "JPEG (*.jpg *.jpeg *.JPG *.JPEG);;" ;
        filetypes += "PNG (*.png *.PNG);;" ;
        filetypes += "PPM PBM (*.ppm *.pbm *.PPM *.PBM);;";
        filetypes += "BMP (*.bmp *.BMP);;";
        filetypes += "16 bits TIFF (*.tif *.tiff *.TIF *.TIFF)";

        QString ldr_name = QFileInfo(getCurrentHDRName()).baseName();

        QString outfname = QFileDialog::getSaveFileName(this,
                                                        QObject::tr("Save the LDR image as..."),
                                                        RecentDirLDRSetting + "/" + ldr_name + "_" + l_v->getFilenamePostFix() + ".jpg",
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

    // update name on the tab label
    m_tabwidget->setTabText(m_tabwidget->indexOf(saved_hdr), fname);
    setWindowModified(false);
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

    m_tabwidget->setTabText(m_tabwidget->indexOf(saved_ldr), fname);
}

void MainWindow::save_ldr_failed()
{
    // TODO give some kind of feedback to the user!
    // TODO pass the name of the file, so the user know which file didn't save correctly
    QMessageBox::warning(0,"", tr("Failed to save"), QMessageBox::Ok, QMessageBox::NoButton);
    //QMessageBox::warning(0,"",QObject::tr("Failed to save <b>") + outfname + "</b>", QMessageBox::Ok, QMessageBox::NoButton);
}

void MainWindow::saveHdrPreview()
{
    // TODO : This function is just rubbish, and should be changed!
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* hdr_v = dynamic_cast<HdrViewer*>(g_v);

    if (hdr_v == NULL) return;
    else hdr_v->saveHdrPreview();
}

void MainWindow::updateActionsNoImage()
{
    updateMagnificationButtons(NULL);

    m_Ui->fileSaveAsAction->setEnabled(false);
    m_Ui->actionSave_Hdr_Preview->setEnabled(false);

    // Histogram
    m_Ui->menuHDR_Histogram->setEnabled(false);
    m_Ui->Low_dynamic_range->setEnabled(false);
    m_Ui->Fit_to_dynamic_range->setEnabled(false);
    m_Ui->Shrink_dynamic_range->setEnabled(false);
    m_Ui->Extend_dynamic_range->setEnabled(false);
    m_Ui->Decrease_exposure->setEnabled(false);
    m_Ui->Increase_exposure->setEnabled(false);

    m_Ui->actionResizeHDR->setEnabled(false);
    m_Ui->action_Projective_Transformation->setEnabled(false);
    m_Ui->cropToSelectionAction->setEnabled(false);
    m_Ui->rotateccw->setEnabled(false);
    m_Ui->rotatecw->setEnabled(false);
    m_Ui->actionFix_Histogram->setEnabled(false);
}

void MainWindow::updateActionsLdrImage()
{
    // Read/Save
    m_Ui->fileSaveAsAction->setEnabled(true);
    m_Ui->actionSave_Hdr_Preview->setEnabled(true);
    if (curr_num_ldr_open >= 2)
        m_Ui->fileSaveAllAction->setEnabled(true);

    // Histogram
    m_Ui->menuHDR_Histogram->setEnabled(false);
    m_Ui->Low_dynamic_range->setEnabled(false);
    m_Ui->Fit_to_dynamic_range->setEnabled(false);
    m_Ui->Shrink_dynamic_range->setEnabled(false);
    m_Ui->Extend_dynamic_range->setEnabled(false);
    m_Ui->Decrease_exposure->setEnabled(false);
    m_Ui->Increase_exposure->setEnabled(false);

    m_Ui->actionResizeHDR->setEnabled(false);
    m_Ui->action_Projective_Transformation->setEnabled(false);
    m_Ui->cropToSelectionAction->setEnabled(false);
    m_Ui->removeSelectionAction->setEnabled(false);
    m_Ui->rotateccw->setEnabled(false);
    m_Ui->rotatecw->setEnabled(false);
    m_Ui->actionFix_Histogram->setEnabled(true);
}

void MainWindow::updateActionsHdrImage()
{
    //qDebug() << "MainWindow::updateActionsHdrImage()";

    m_Ui->fileSaveAsAction->setEnabled(true);
    m_Ui->actionSave_Hdr_Preview->setEnabled(true);
    //actionShowHDRs->setEnabled(false);

    // Histogram
    m_Ui->menuHDR_Histogram->setEnabled(true);
    m_Ui->Low_dynamic_range->setEnabled(true);
    m_Ui->Fit_to_dynamic_range->setEnabled(true);
    m_Ui->Shrink_dynamic_range->setEnabled(true);
    m_Ui->Extend_dynamic_range->setEnabled(true);
    m_Ui->Decrease_exposure->setEnabled(true);
    m_Ui->Increase_exposure->setEnabled(true);

    m_Ui->actionResizeHDR->setEnabled(true);
    m_Ui->action_Projective_Transformation->setEnabled(true);

    if (tm_status.curr_tm_frame)
    {
        if (!tm_status.curr_tm_frame->hasSelection())
        {
            m_Ui->cropToSelectionAction->setEnabled(false);
            m_Ui->removeSelectionAction->setEnabled(false);
        }
        else
        {
            m_Ui->cropToSelectionAction->setEnabled(true);
            m_Ui->removeSelectionAction->setEnabled(true);
        }
    }
    m_Ui->rotateccw->setEnabled(true);
    m_Ui->rotatecw->setEnabled(true);
    m_Ui->actionFix_Histogram->setEnabled(false);
}

void MainWindow::updateActions( int w )
{
    //qDebug() << "MainWindow::updateActions(" << w << ")";
    updatePreviousNextActions();
    if ( w < 0 )
    {
        // something wrong happened?
        updateActionsNoImage();
    }
    else
    {
        GenericViewer* g_v = (GenericViewer*)m_tabwidget->widget(w);
        updateMagnificationButtons(g_v);
        if ( g_v->isHDR() )
        {
            // current selected frame is an HDR
            updateActionsHdrImage();
        }
        else
        {
            // current selected frame is not an HDR
            updateActionsLdrImage();
        }
    }
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
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* curr_g_v = (GenericViewer*)m_tabwidget->currentWidget();

    m_Ui->rotateccw->setEnabled(false);
    m_Ui->rotatecw->setEnabled(false);

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    pfs::Frame *rotated = pfs::rotateFrame(curr_g_v->getHDRPfsFrame(), clockwise);
    //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
    curr_g_v->updateHDR(rotated);
    if ( !curr_g_v->needsSaving() )
    {
        curr_g_v->setNeedsSaving(true);
        curr_g_v->setWindowTitle(curr_g_v->windowTitle().prepend("(*) "));

        setWindowModified(true);
    }
    emit updatedHDR(curr_g_v->getHDRPfsFrame());
    QApplication::restoreOverrideCursor();

    m_Ui->rotateccw->setEnabled(true);
    m_Ui->rotatecw->setEnabled(true);
}

void MainWindow::resize_requested()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* curr_g_v = (GenericViewer*)m_tabwidget->currentWidget();

    ResizeDialog *resizedialog = new ResizeDialog(this, curr_g_v->getHDRPfsFrame());
    if (resizedialog->exec() == QDialog::Accepted)
    {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
        curr_g_v->updateHDR(resizedialog->getResizedFrame());
        if (! curr_g_v->needsSaving())
        {
            curr_g_v->setNeedsSaving(true);
            curr_g_v->setWindowTitle(curr_g_v->windowTitle().prepend("(*) "));

            setWindowModified(true);
        }
        emit updatedHDR(curr_g_v->getHDRPfsFrame());
        QApplication::restoreOverrideCursor();
    }
    delete resizedialog;
}

void MainWindow::projectiveTransf_requested()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* curr_g_v = (GenericViewer*)m_tabwidget->currentWidget();

    ProjectionsDialog *projTranfsDialog = new ProjectionsDialog(this, curr_g_v->getHDRPfsFrame());
    if (projTranfsDialog->exec() == QDialog::Accepted)
    {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        //updateHDR() method takes care of deleting its previous pfs::Frame* buffer.
        curr_g_v->updateHDR(projTranfsDialog->getTranformedFrame());
        if ( !curr_g_v->needsSaving() )
        {
            curr_g_v->setNeedsSaving(true);
            curr_g_v->setWindowTitle(curr_g_v->windowTitle().prepend("(*) "));

            setWindowModified(true);
        }
        emit updatedHDR(curr_g_v->getHDRPfsFrame());
        QApplication::restoreOverrideCursor();
    }
    delete projTranfsDialog;
}

void MainWindow::hdr_decrease_exp()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* curr_hdr_v = dynamic_cast<HdrViewer*>(g_v);
    if ( curr_hdr_v != NULL )
        curr_hdr_v->lumRange()->decreaseExposure();
}

void MainWindow::hdr_extend_exp()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* curr_hdr_v = dynamic_cast<HdrViewer*>(g_v);
    if ( curr_hdr_v != NULL )
        curr_hdr_v->lumRange()->extendRange();
}

void MainWindow::hdr_fit_exp()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* curr_hdr_v = dynamic_cast<HdrViewer*>(g_v);
    if ( curr_hdr_v != NULL )
        curr_hdr_v->lumRange()->fitToDynamicRange();
}

void MainWindow::hdr_increase_exp()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* curr_hdr_v = dynamic_cast<HdrViewer*>(g_v);
    if ( curr_hdr_v != NULL )
        curr_hdr_v->lumRange()->increaseExposure();
}

void MainWindow::hdr_shrink_exp()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* curr_hdr_v = dynamic_cast<HdrViewer*>(g_v);
    if ( curr_hdr_v != NULL )
        curr_hdr_v->lumRange()->shrinkRange();
}

void MainWindow::hdr_ldr_exp()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();
    HdrViewer* curr_hdr_v = dynamic_cast<HdrViewer*>(g_v);
    if ( curr_hdr_v != NULL )
        curr_hdr_v->lumRange()->lowDynamicRange();
}

// Zoom = Viewers (START)
void MainWindow::viewerZoomIn()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();

    g_v->zoomIn();
    //updateMagnificationButtons(g_v);
}

void MainWindow::viewerZoomOut()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();

    g_v->zoomOut();
    //updateMagnificationButtons(g_v);
}

void MainWindow::viewerFitToWin(bool checked)
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();

    g_v->fitToWindow();
    //updateMagnificationButtons(g_v);
}

void MainWindow::viewerFillToWin()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();

    g_v->fillToWindow();
    //updateMagnificationButtons(g_v);
}

void MainWindow::viewerOriginalSize()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* g_v = (GenericViewer*)m_tabwidget->currentWidget();

    g_v->normalSize();
    //updateMagnificationButtons(g_v);
}
// Zoom = Viewers (END)


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

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        emit open_frame(action->data().toString());
    }
}

void MainWindow::setupIO()
{
    // Init Object/Thread
    m_IOThread = new QThread;
    m_IOWorker = new IOWorker;

    m_IOWorker->moveToThread(m_IOThread);

    // Memory Management
    connect(this, SIGNAL(destroyed()), m_IOWorker, SLOT(deleteLater()));
    connect(m_IOWorker, SIGNAL(destroyed()), m_IOThread, SLOT(deleteLater()));

    // Open
    connect(this, SIGNAL(open_frame(QString)), m_IOWorker, SLOT(read_frame(QString)));
    connect(this, SIGNAL(open_frames(QStringList)), m_IOWorker, SLOT(read_frames(QStringList)));
    connect(m_IOWorker, SIGNAL(read_success(pfs::Frame*, QString)), this, SLOT(load_success(pfs::Frame*, QString)));
    connect(m_IOWorker, SIGNAL(read_failed(QString)), this, SLOT(load_failed(QString)));

    // Save HDR
    connect(this, SIGNAL(save_hdr_frame(HdrViewer*, QString)), m_IOWorker, SLOT(write_hdr_frame(HdrViewer*, QString)));
    connect(m_IOWorker, SIGNAL(write_hdr_success(HdrViewer*, QString)), this, SLOT(save_hdr_success(HdrViewer*, QString)));
    connect(m_IOWorker, SIGNAL(write_hdr_failed()), this, SLOT(save_hdr_failed()));
    // Save LDR
    connect(this, SIGNAL(save_ldr_frame(LdrViewer*, QString, int)), m_IOWorker, SLOT(write_ldr_frame(LdrViewer*, QString, int)));
    connect(m_IOWorker, SIGNAL(write_ldr_success(LdrViewer*, QString)), this, SLOT(save_ldr_success(LdrViewer*, QString)));
    connect(m_IOWorker, SIGNAL(write_ldr_failed()), this, SLOT(save_ldr_failed()));

    // progress bar handling
    connect(m_IOWorker, SIGNAL(setValue(int)), this, SLOT(ProgressBarSetValue(int)));
    connect(m_IOWorker, SIGNAL(setMaximum(int)), this, SLOT(ProgressBarSetMaximum(int)));
    connect(m_IOWorker, SIGNAL(IO_init()), this, SLOT(IO_start()));
    connect(m_IOWorker, SIGNAL(IO_finish()), this, SLOT(IO_done()));

    // start thread waiting for signals (I/O requests)
    m_IOThread->start();
}

void MainWindow::load_failed(QString error_message)
{
    // TODO: use unified style?
    QMessageBox::critical(this, tr("Aborting..."), error_message, QMessageBox::Ok, QMessageBox::NoButton);
}

void MainWindow::load_success(pfs::Frame* new_hdr_frame, QString new_fname, bool needSaving)
{
    if ( tm_status.is_hdr_ready )
    {
        MainWindow *other = new MainWindow(new_hdr_frame, new_fname, needSaving);
        other->move(x() + 40, y() + 40);
        other->show();
    }
    else
    {
        tmPanel->show();
        HdrViewer * newhdr = new HdrViewer(NULL, false, false, luminance_options.getViewerNegColor(), luminance_options.getViewerNanInfColor());

        newhdr->setAttribute(Qt::WA_DeleteOnClose);

        connect(newhdr, SIGNAL(selectionReady(bool)), this, SLOT(enableCrop(bool)));
        connect(newhdr, SIGNAL(changed(GenericViewer*)), this, SLOT(syncViewers(GenericViewer*)));
        connect(newhdr, SIGNAL(changed(GenericViewer*)), this, SLOT(updateMagnificationButtons(GenericViewer*)));

        newhdr->setSelectionTool(true);

        newhdr->updateHDR(new_hdr_frame);
        newhdr->setFileName(new_fname);
        newhdr->setWindowTitle(new_fname);

        newhdr->fitToWindow(true);

        m_tabwidget->addTab(newhdr, new_fname);

        tm_status.is_hdr_ready = true;
        tm_status.curr_tm_frame = newhdr;

        m_tabwidget->setCurrentWidget(newhdr);

        if ( needSaving )
        {
            setMainWindowModified(true);
        }
        else
        {
            setCurrentFile(new_fname);
        }
        emit updatedHDR(newhdr->getHDRPfsFrame());  // signal: I have a new HDR open

        tmPanel->setEnabled(true);
        m_Ui->actionShowPreviewPanel->setEnabled(true);

        showPreviewPanel(m_Ui->actionShowPreviewPanel->isChecked());

        // done by SIGNAL(updatedHDR( ))
        //tmPanel->setSizes(newhdr->getHDRPfsFrame()->getWidth(),
        //                  newhdr->getHDRPfsFrame()->getHeight());
    }
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
    unsigned int negcol = luminance_options.getViewerNegColor();
    unsigned int naninfcol = luminance_options.getViewerNanInfColor();
    PreferencesDialog *opts = new PreferencesDialog(this);
    opts->setAttribute(Qt::WA_DeleteOnClose);
    if ( opts->exec() == QDialog::Accepted )
    {
        if (negcol != luminance_options.getViewerNegColor() || naninfcol != luminance_options.getViewerNanInfColor())
        {
            for (int idx = 0; idx < m_tabwidget->count(); idx++)
            {
                GenericViewer *viewer = (GenericViewer*)m_tabwidget->widget(idx);
                HdrViewer* hdr_v = dynamic_cast<HdrViewer*>(viewer);
                if ( hdr_v != NULL )
                {
                    hdr_v->update_colors(luminance_options.getViewerNegColor(), luminance_options.getViewerNanInfColor());
                }
            }
        }
        m_Ui->actionShowPreviewPanel->setChecked(luminance_options.isPreviewPanelActive());
    }
}

void MainWindow::transplant_called()
{
    TransplantExifDialog *transplant=new TransplantExifDialog(this);
    transplant->setAttribute(Qt::WA_DeleteOnClose);
    transplant->exec();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        openFiles(convertUrlListToFilenameList(event->mimeData()->urls()));
    }
    event->acceptProposedAction();
}

void MainWindow::openFiles(const QStringList& files)
{
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

void MainWindow::Text_Under_Icons()
{
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    luminance_options.setMainWindowToolBarMode(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::Icons_Only()
{
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    luminance_options.setMainWindowToolBarMode(Qt::ToolButtonIconOnly);
}

void MainWindow::Text_Alongside_Icons()
{
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    luminance_options.setMainWindowToolBarMode(Qt::ToolButtonTextBesideIcon);
}

void MainWindow::Text_Only()
{
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    luminance_options.setMainWindowToolBarMode(Qt::ToolButtonTextOnly);
}

void MainWindow::showSplash()
{
    // TODO: change implementation with a static member of UMessageBox
    splash = new QDialog(this);
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
    luminance_options.setValue("ShowSplashScreen", false);
    splash->close();
}

void MainWindow::aboutLuminance()
{
    UMessageBox::about();
}

/*
 * Window Menu Display and Functionalities
 */
void MainWindow::updateWindowMenu()
{
    // Remove current elements inside the menuWindows
    foreach (QAction* Action_MW, openMainWindows)
    {
        openMainWindows.removeAll(Action_MW);
        m_Ui->menuWindows->removeAction( Action_MW );
        delete Action_MW;
    }

    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        MainWindow *MW = qobject_cast<MainWindow *>(widget);
        if (MW != NULL)
        {
            QAction *action  = m_Ui->menuWindows->addAction( MW->getCurrentHDRName() );

            action->setCheckable(true);
            action->setChecked(MW == this);
            connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
            windowMapper->setMapping(action, MW);

            openMainWindows.push_back(action);
        }
    }
}

/*
 * This function sets the active Main Window
 * when the file name is selected inside the "Window" menu
 */
void MainWindow::setActiveMainWindow(QWidget* w)
{
    MainWindow *MW = qobject_cast<MainWindow *>(w);

    if ( MW == NULL ) return;

    MW->raise();
    MW->activateWindow();
    return;
}

void MainWindow::minimizeMW()
{
    this->showMinimized();
}

void MainWindow::maximizeMW()
{
    this->showMaximized();
}

void MainWindow::bringAllMWToFront()
{
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        MainWindow *MW = qobject_cast<MainWindow *>(widget);
        if (MW != NULL)
        {
            MW->raise();
        }
    }
}

void MainWindow::batch_hdr_requested()
{
    BatchHDRDialog *batch_hdr_dialog = new BatchHDRDialog(this);
    batch_hdr_dialog->exec();
    delete batch_hdr_dialog;
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
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* curr_g_v = (GenericViewer*)m_tabwidget->currentWidget();

    if ( !curr_g_v->isHDR() ) return;
    if ( !curr_g_v->hasSelection() ) return;

    QRect cropRect = curr_g_v->getSelectionRect();
    int x_ul, y_ul, x_br, y_br;
    cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
    disableCrop();
    pfs::Frame *original_frame = curr_g_v->getHDRPfsFrame();
    pfs::Frame *cropped_frame = pfs::pfscut(original_frame, x_ul, y_ul, x_br, y_br);

    emit load_success(cropped_frame, QString(tr("Cropped Image")), true);

    curr_g_v->removeSelection();
}

void MainWindow::enableCrop(bool isReady)
{
    m_Ui->cropToSelectionAction->setEnabled(isReady);
    m_Ui->removeSelectionAction->setEnabled(isReady);
}

void MainWindow::disableCrop()
{
    if (m_tabwidget->count() <= 0) return;

    GenericViewer* curr_g_v = (GenericViewer*)m_tabwidget->currentWidget();

    if ( !curr_g_v->isHDR() ) return;

    curr_g_v->removeSelection();
    m_Ui->cropToSelectionAction->setEnabled(false);
    m_Ui->removeSelectionAction->setEnabled(false);
}

void MainWindow::closeEvent( QCloseEvent *event )
{
    if ( maybeSave() )
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

bool MainWindow::maybeSave()
{
    // if no HDR is open, return true
    if ( !tm_status.is_hdr_ready ) return true;

    if ( tm_status.curr_tm_frame->needsSaving() )
    {
        int ret = UMessageBox::saveDialog(
                tr("Unsaved changes..."),
                tr("This HDR image has unsaved changes.<br>Do you want to save it?"),
                this);
        switch(ret)
        {
        case QMessageBox::Save:
            {
                  /* if save == success return true;
                   * else return false;
                   */
                qDebug() << "HDR need to be saved";

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

                    // TODO : can I launch a signal and wait that it gets executed fully?
                    return m_IOWorker->write_hdr_frame(dynamic_cast<HdrViewer*>(tm_status.curr_tm_frame), fname);
                }
                else
                {
                    return false;
                }
            }
            break;
        case QMessageBox::Discard:
            {
                return true;
            }
            break;
        case QMessageBox::Cancel:
        default:
            {
                return false;
            }
            break;
        }
    }
    else return true;
}

void MainWindow::setupTM()
{
    // TODO: Building TM Thread
    tm_status.is_hdr_ready = false;
    tm_status.curr_tm_frame = NULL;
    tm_status.curr_tm_options = NULL;

    tmPanel->setEnabled(false);
}

void MainWindow::tonemapImage(TonemappingOptions *opts)
{
#ifdef QT_DEBUG
    qDebug() << "Start Tone Mapping";
#endif

    if ( opts->tmoperator == fattal && luminance_options.isShowFattalWarning() )
    {
        // Warning when using size dependent TMOs with smaller sizes
        if ( opts->xsize != opts->origxsize )
        {
            TonemappingWarningDialog tonemapping_warning_dialog(this);

            tonemapping_warning_dialog.exec();

            if ( !tonemapping_warning_dialog.wasAccepted() ) return;
        }
    }

    previewPanel->setEnabled(false);

    tm_status.curr_tm_options = opts;

//    if ( opts->tonemapOriginal )
//    {
        if ( tm_status.curr_tm_frame->hasSelection() )
        {
            opts->tonemapSelection = true;
            getCropCoords((HdrViewer *)tm_status.curr_tm_frame,
                           opts->selection_x_up_left,
                           opts->selection_y_up_left,
                           opts->selection_x_bottom_right,
                           opts->selection_y_bottom_right);
        }
        else
           opts->tonemapSelection = false;
//    }



//    else // if ( !opts.tonemapOriginal )
//    {
//        if (!mdiArea->subWindowList().isEmpty())
//        {
//            GenericViewer *viewer = (GenericViewer *) mdiArea->activeSubWindow()->widget();
//
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
//
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

    HdrViewer* hdr_viewer = dynamic_cast<HdrViewer*>(tm_status.curr_tm_frame);
    if ( hdr_viewer )
    {
        // TODO : can you clean up this thing?!
        // getHDRPfsFrame() is only available in HdrViewer
        TMOThread *thread = TMOFactory::getTMOThread(opts->tmoperator, hdr_viewer->getHDRPfsFrame(), tm_status.curr_tm_options);
        progInd = new TMOProgressIndicator(this);

        connect(thread, SIGNAL(imageComputed(QImage*, quint16*)), this, SLOT(addLDRResult(QImage*, quint16*)));
        connect(thread, SIGNAL(processedFrame(pfs::Frame *)), this, SLOT(addProcessedFrame(pfs::Frame *)));
        connect(thread, SIGNAL(setMaximumSteps(int)), progInd, SLOT(setMaximum(int)));
        connect(thread, SIGNAL(setValue(int)), progInd, SLOT(setValue(int)));
        connect(thread, SIGNAL(tmo_error(const char *)), this, SLOT(showErrorMessage(const char *)));
        connect(thread, SIGNAL(finished()), progInd, SLOT(terminated()));
        connect(thread, SIGNAL(finished()), this, SLOT(tonemappingFinished()));
        connect(thread, SIGNAL(deleteMe(TMOThread *)), this, SLOT(deleteTMOThread(TMOThread *)));
        connect(progInd, SIGNAL(terminate()), thread, SLOT(terminateRequested()));

        //start thread
        tmPanel->setEnabled(false);
        thread->startTonemapping();
        statusBar()->addWidget(progInd);
        progInd->show();
    }
}

void MainWindow::addLDRResult(QImage* image, quint16 *pixmap)
{
    num_ldr_generated++;
    curr_num_ldr_open++;

    LdrViewer *n = new LdrViewer( image, pixmap, this, false, false, tm_status.curr_tm_options);

    connect(n, SIGNAL(changed(GenericViewer *)), this, SLOT(syncViewers(GenericViewer *)));
    connect(n, SIGNAL(changed(GenericViewer*)), this, SLOT(updateMagnificationButtons(GenericViewer*)));
    connect(n, SIGNAL(levels_closed()), this, SLOT(levelsClosed()));

    // TODO : progressive numbering of the open LDR tabs
    if (num_ldr_generated == 1)
        m_tabwidget->addTab(n, tr("Untitled"));
    else 
	{
		if (!tmPanel->replaceLdr())
        	m_tabwidget->addTab(n, tr("Untitled %1").arg(num_ldr_generated));
		else
		{
			GenericViewer *g_v = (GenericViewer *) m_tabwidget->currentWidget();
			if (!g_v->isHDR())
				m_tabwidget->removeTab(m_tabwidget->currentIndex());
        	m_tabwidget->addTab(n, tr("Untitled %1").arg(num_ldr_generated));
		}
	}
    m_tabwidget->setCurrentWidget(n);

    n->fitToWindow(true);

    previewPanel->setEnabled(true);
}

void MainWindow::addProcessedFrame(pfs::Frame *frame)
{
    // TODO : currently, I'm thinking to remove this function
    // Even the LDR viewer needs to be build using the pfs::Frame
    // When the LDR hold the floating-point data, it can create a 16bit/channel version for high quality TIFF save
    qDebug() << "MainWindow::addProcessedFrame(pfs::Frame *frame)";
    qDebug() << "This function needs cleaning";

//    HdrViewer *HDR = new HdrViewer(this, false, false, luminance_options->negcolor, luminance_options->naninfcolor);
//    HDR->setFreePfsFrameOnExit(true);
//    HDR->updateHDR(frame);
//    HDR->setFileName(QString(tr("Processed HDR")));
//    HDR->setWindowTitle(QString(tr("Processed HDR")));
//    HDR->setSelectionTool(true);
//    HDR->normalSize();
//    HDR->showMaximized();
//    //        if (actionFit_to_Window->isChecked())
//    //                HDR->fitToWindow(true);
//    QMdiSubWindow *HdrSubWin = new QMdiSubWindow(this);
//    HdrSubWin->setAttribute(Qt::WA_DeleteOnClose);
//    HdrSubWin->setWidget(HDR);
//    mdiArea->addSubWindow(HdrSubWin);
//    //HDR->showMaximized();

//    if (luminance_options->tmowindow_max)
//        HDR->showMaximized();
//    else
//        HDR->showNormal();

//    //connect(HDR,SIGNAL(changed(GenericViewer *)),this,SLOT(syncViewers(GenericViewer *)));
}

void MainWindow::tonemappingFinished()
{
    qDebug() << "TonemappingWindow::tonemappingFinished()";
    statusBar()->removeWidget(progInd);
    tmPanel->setEnabled(true);

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
    tmPanel->setEnabled(true);

    delete progInd;
}


pfs::Frame * MainWindow::getSelectedFrame(HdrViewer *hdr)
{
    assert( hdr != NULL );
    pfs::Frame *frame = hdr->getHDRPfsFrame();
    QRect cropRect = hdr->getSelectionRect();
    int x_ul, y_ul, x_br, y_br;
    cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
    return pfs::pfscut(frame, x_ul, y_ul, x_br, y_br);
}

void MainWindow::getCropCoords(HdrViewer *hdr, int& x_ul, int& y_ul, int& x_br, int& y_br)
{
    assert( hdr != NULL );

    QRect cropRect = hdr->getSelectionRect();
    cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
}

/*
 * Lock Handling
 */
void MainWindow::lockViewers(bool /*toggled*/)
{
    if (m_Ui->actionLock->isChecked() && m_tabwidget->count())
    {
        syncViewers((GenericViewer*)m_tabwidget->currentWidget());
    }
}

void MainWindow::syncViewers(GenericViewer *sender)
{
    if (sender == NULL) return;
    if (!m_Ui->actionLock->isChecked()) return;

    for (int idx = 0; idx < m_tabwidget->count(); idx++)
    {
        GenericViewer *viewer = (GenericViewer*)m_tabwidget->widget(idx);
        if (sender != viewer)
        {
            viewer->blockSignals(true);
            //disconnect(viewer,SIGNAL(changed(GenericViewer *)),this,SLOT(syncViewers(GenericViewer *)));
            viewer->syncViewer(sender);
            //connect(viewer,SIGNAL(changed(GenericViewer *)),this,SLOT(syncViewers(GenericViewer *)));
            viewer->blockSignals(false);
        }
    }
}

void MainWindow::showPreviewPanel(bool b)
{
    if (b)
    {
        if (tm_status.is_hdr_ready)
        {
            previewPanel->show();

            // ask panel to refresh itself
            previewPanel->updatePreviews(tm_status.curr_tm_frame->getHDRPfsFrame());

            // connect signals
            connect(this, SIGNAL(updatedHDR(pfs::Frame*)), previewPanel, SLOT(updatePreviews(pfs::Frame*)));
            connect(previewPanel, SIGNAL(startTonemapping(TonemappingOptions*)), this, SLOT(tonemapImage(TonemappingOptions*)));
            connect(previewPanel, SIGNAL(startTonemapping(TonemappingOptions*)), tmPanel, SLOT(updateTonemappingParams(TonemappingOptions*)));
        }
    }
    else
    {
        previewPanel->hide();

        // disconnect signals
        disconnect(this, SIGNAL(updatedHDR(pfs::Frame*)), previewPanel, SLOT(updatePreviews(pfs::Frame*)));
        disconnect(previewPanel, SIGNAL(startTonemapping(TonemappingOptions*)), this, SLOT(tonemapImage(TonemappingOptions*)));
        disconnect(previewPanel, SIGNAL(startTonemapping(TonemappingOptions*)), tmPanel, SLOT(updateTonemappingParams(TonemappingOptions*)));
    }
}

void MainWindow::updateMagnificationButtons(GenericViewer* c_v)
{
    if ( c_v == NULL )
    {
        m_Ui->normalSizeAct->setEnabled(false);
        m_Ui->normalSizeAct->setChecked(false);
        m_Ui->fitToWindowAct->setEnabled( false );
        m_Ui->fitToWindowAct->setChecked(false);
        m_Ui->actionFill_to_Window->setEnabled(false);
        m_Ui->actionFill_to_Window->setChecked(false);

        m_Ui->zoomInAct->setEnabled( false );
        m_Ui->zoomOutAct->setEnabled( false );
    }
    else
    {  
        if ( c_v->isNormalSize() )
        {
            m_Ui->zoomInAct->setEnabled(false);
            m_Ui->zoomOutAct->setEnabled(true);

            m_Ui->normalSizeAct->setEnabled(false);
            m_Ui->normalSizeAct->setChecked(true);
            m_Ui->fitToWindowAct->setEnabled(true);
            m_Ui->fitToWindowAct->setChecked(false);
            m_Ui->actionFill_to_Window->setEnabled(true);
            m_Ui->actionFill_to_Window->setChecked(false);

            return;
        }

        if ( c_v->isFilledToWindow() )
        {
            m_Ui->zoomInAct->setEnabled(true);
            m_Ui->zoomOutAct->setEnabled(true);

            m_Ui->normalSizeAct->setEnabled(true);
            m_Ui->normalSizeAct->setChecked(false);
            m_Ui->fitToWindowAct->setEnabled(true);
            m_Ui->fitToWindowAct->setChecked(false);
            m_Ui->actionFill_to_Window->setEnabled(false);
            m_Ui->actionFill_to_Window->setChecked(true);

            return;
        }

        if ( c_v->isFittedToWindow() )
        {
            m_Ui->zoomInAct->setEnabled(true);
            m_Ui->zoomOutAct->setEnabled(false);

            m_Ui->normalSizeAct->setEnabled(true);
            m_Ui->normalSizeAct->setChecked(false);
            m_Ui->fitToWindowAct->setEnabled(false);
            m_Ui->fitToWindowAct->setChecked(true);
            m_Ui->actionFill_to_Window->setEnabled(true);
            m_Ui->actionFill_to_Window->setChecked(false);

            return;
        }
    }
}

/*
 * Next/Previous Buttons
 */

void MainWindow::removeCurrentTab() 
{
	removeTab(m_tabwidget->currentIndex());
}

void MainWindow::removeTab(int t)
{
    qDebug() << "MainWindow::remove_image("<< t <<")";

    if (t < 0) return;

    GenericViewer* w = (GenericViewer*)m_tabwidget->widget(t);
    w->blockSignals(true);
    if (w->isHDR())
    {
        qDebug() << "Remove HDR from MainWindow";
        if ( w->needsSaving() )
        {
            if ( maybeSave() )
            {
                // if discard OR saved
                m_tabwidget->removeTab(t);
                w->deleteLater();   // delete yourself whenever you want

                setWindowModified(false);

                tm_status.is_hdr_ready = false;
                tm_status.curr_tm_frame = NULL;
                tm_status.curr_tm_options = NULL;

                tmPanel->setEnabled(false);
                m_Ui->actionShowPreviewPanel->setEnabled(false);

                tmPanel->hide();
                previewPanel->hide();
            }
            // if FALSE, it means that the user said "Cancel"
            // or the saving operation went wrong
            // and we don't need to remove any tab
            else {
                w->blockSignals(false);
            }
        }
        else
        {
            // if discard OR saved
            m_tabwidget->removeTab(t);
            w->deleteLater();   // delete yourself whenever you want

            setWindowModified(false);

            tm_status.is_hdr_ready = false;
            tm_status.curr_tm_frame = NULL;
            tm_status.curr_tm_options = NULL;

            tmPanel->setEnabled(false);
            m_Ui->actionShowPreviewPanel->setEnabled(false);

            tmPanel->hide();
            previewPanel->hide();
        }
    }
    else
    {
        curr_num_ldr_open--;
        m_tabwidget->removeTab(t);
        w->deleteLater();   // delete yourself whenever you want

        if (curr_num_ldr_open == 1)
            m_Ui->fileSaveAllAction->setEnabled(false);
    }
    updatePreviousNextActions();
}

void MainWindow::activateNextViewer()
{
    int curr_num_viewers = m_tabwidget->count();
    int curr_viewer = m_tabwidget->currentIndex();

    if ( curr_viewer < curr_num_viewers-1 )
    {
        m_tabwidget->setCurrentIndex(curr_viewer+1);
    }
}

void MainWindow::activatePreviousViewer()
{
    int curr_viewer = m_tabwidget->currentIndex();

    if ( curr_viewer > 0 )
    {
        m_tabwidget->setCurrentIndex(curr_viewer-1);
    }
}

void MainWindow::updatePreviousNextActions()
{
    int curr_num_viewers = m_tabwidget->count();
    if (curr_num_viewers <= 1)
    {
        m_Ui->actionShowNext->setEnabled(false);
        m_Ui->actionShowPrevious->setEnabled(false);
        m_Ui->actionLock->setEnabled(false);
    }
    else
    {
        m_Ui->actionLock->setEnabled(true);
        int curr_viewer = m_tabwidget->currentIndex();
        if ( curr_viewer == 0 )
        {
            m_Ui->actionShowNext->setEnabled(true);
            m_Ui->actionShowPrevious->setEnabled(false);
        }
        else if (curr_viewer == (curr_num_viewers-1))
        {
            m_Ui->actionShowNext->setEnabled(false);
            m_Ui->actionShowPrevious->setEnabled(true);
        }
        else
        {
            m_Ui->actionShowNext->setEnabled(true);
            m_Ui->actionShowPrevious->setEnabled(true);
        }
    }
}

QString MainWindow::getCurrentHDRName()
{
    if (tm_status.is_hdr_ready)
    {
        return tm_status.curr_tm_frame->getFileName();
    }
    else
    {
        return QString("Untitled HDR");
    }
}

void MainWindow::setMainWindowModified(bool b)
{
    if (b)
    {
        if ( tm_status.is_hdr_ready )
        {
            tm_status.curr_tm_frame->setNeedsSaving(true);
            setWindowModified(true);
        }
    }
    else
    {
        if ( tm_status.is_hdr_ready )
        {
            tm_status.curr_tm_frame->setNeedsSaving(false);
        }
        setWindowModified(false);
    }
}

/*
 * Recent File Handling
 */
void MainWindow::setCurrentFile(const QString &fileName)
{
    QStringList files = luminance_options.value(KEY_RECENT_FILES).toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MAX_RECENT_FILES)
    {
        files.removeLast();
    }

    luminance_options.setValue(KEY_RECENT_FILES, files);

    // Update ALL MainWindow
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin)
            mainWin->updateRecentFileActions();
    }
}

void MainWindow::updateRecentFileActions()
{
    QStringList files = luminance_options.value(KEY_RECENT_FILES).toStringList();

    int numRecentFiles = qMin(files.size(), (int)MAX_RECENT_FILES);
    separatorRecentFiles->setVisible(numRecentFiles > 0);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MAX_RECENT_FILES; ++j)
    {
        recentFileActs[j]->setVisible(false);
    }
}

void MainWindow::initRecentFileActions()
{
    separatorRecentFiles = m_Ui->menuFile->addSeparator();

    for (int i = 0; i < MAX_RECENT_FILES; ++i)
    {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        m_Ui->menuFile->addAction(recentFileActs[i]);
        connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
    }
}

void MainWindow::clearRecentFileActions()
{
    for (int i = 0; i < MAX_RECENT_FILES; ++i)
    {
        delete recentFileActs[i];
    }
}

void MainWindow::levelsRequested(bool checked)
{
    if (checked)
    {
        GenericViewer* current = (GenericViewer*) m_tabwidget->currentWidget();
        if (current==NULL)
            return;
        m_Ui->actionFix_Histogram->setDisabled(true);
        current->levelsRequested(checked);
    }
}

void MainWindow::levelsClosed()
{
    m_Ui->actionFix_Histogram->setDisabled(false);
    m_Ui->actionFix_Histogram->setChecked(false);
}

void MainWindow::setInputFiles(const QStringList& files)
{
    inputFiles = files;
    QTimer::singleShot(0, this, SLOT(openInputFiles()));
}

void MainWindow::openInputFiles()
{
    openFiles(inputFiles);
}
