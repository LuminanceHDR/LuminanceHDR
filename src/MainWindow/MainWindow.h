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
 */

#ifndef MAINGUI_IMPL_H
#define MAINGUI_IMPL_H

#include <QMainWindow>
#include <QStringList>
#include <QSignalMapper>
#include <QSplitter>
#include <QTabWidget>
#include <QThread>

#include "ui_MainWindow.h"
#include "HdrWizard/HdrWizard.h"
#include "Preferences/PreferencesDialog.h"
#include "Resize/ResizeDialog.h"
#include "Projection/ProjectionsDialog.h"
#include "HelpBrowser/helpbrowser.h"
#include "Viewers/HdrViewer.h"
#include "Viewers/LdrViewer.h"
#include "UI/UMessageBox.h"
#include "Core/IOWorker.h"
#include "TonemappingWindow/TonemappingPanel.h"
#include "TonemappingWindow/TMOProgressIndicator.h"
#include "Threads/TMOThread.h"
#include "Libpfs/frame.h"
#include "PreviewPanel/PreviewPanel.h"

#define MAX_RECENT_FILES (5)

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    // Constructor loading file inside
    MainWindow(pfs::Frame* curr_frame, QString new_fname, bool needSaving = false, QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    // For ProgressBar
    void ProgressBarSetMaximum(int max);
    void ProgressBarSetValue(int value);

    // I/O
    void save_hdr_success(HdrViewer* saved_hdr, QString fname);
    void save_hdr_failed();
    void save_ldr_success(LdrViewer* saved_ldr, QString fname);
    void save_ldr_failed();

    void load_failed(QString);
    void load_success(pfs::Frame* new_hdr_frame, QString new_fname, bool needSaving = false);

    void IO_done();
    void IO_start();

    void tonemapImage(TonemappingOptions *opts);
    void setMainWindowModified(bool b);

protected Q_SLOTS:
    bool eventFilter(QObject *obj, QEvent *event);
    void fileNewViaWizard(QStringList files = QStringList());
    void fileOpen();    //for File->Open, it then calls loadFile()
    void fileSaveAs();
    void saveHdrPreview();

    void rotateccw_requested();
    void rotatecw_requested();

    void resize_requested();
    void projectiveTransf_requested();

    void batch_requested();

    void hdr_increase_exp();
    void hdr_decrease_exp();
    void hdr_extend_exp();
    void hdr_shrink_exp();
    void hdr_fit_exp();
    void hdr_ldr_exp();

    void viewerZoomIn();
    void viewerZoomOut();
    void viewerFitToWin(bool checked);
    void viewerOriginalSize();

    void openDocumentation();
    void enterWhatsThis();

    void preferences_called();
    void transplant_called();

    // Tool Bar Handling
    void Text_Under_Icons();
    void Icons_Only();
    void Text_Alongside_Icons();
    void Text_Only();

    // Window Menu Display and Functionalities
    void updateWindowMenu();
    void minimizeMW();
    void maximizeMW();
    void bringAllMWToFront();

    void openRecentFile();
    void setCurrentFile(const QString &fileName);

    void updateRecentDirHDRSetting(QString);
    void updateRecentDirLDRSetting(QString);

    void aboutLuminance();
    void showSplash();

    void updateActions(int w);
    void setActiveMainWindow(QWidget* w);

    void cropToSelection();
    void enableCrop(bool);
    void disableCrop();

    void helpBrowserClosed();
    void showDonationsPage();
    void splashShowDonationsPage();
    void splashClose();

    // TM
    void addProcessedFrame(pfs::Frame *);
    void addLDRResult(QImage*);
    void tonemappingFinished();
    void deleteTMOThread(TMOThread *th);
    void showErrorMessage(const char *e);

    void lockImages(bool);
    void updateImage(GenericViewer *viewer);
    void dispatch(GenericViewer *);

    void showHDR();

    // QTabWidget
    void removeTab(int);
    void activateNextViewer();
    void activatePreviousViewer();

    // Preview Panel
    void tonemapPreview(int n);
    void generatePreviews();
    void addSmallPreviewResult(QImage *img);
    void addPreviewResult(QImage *img);

Q_SIGNALS:
    // I/O
    void save_hdr_frame(HdrViewer*, QString);
    void save_ldr_frame(LdrViewer*, QString, int);  // viewer, filename, quality level
    void open_frames(QStringList);
    void open_frame(QString);

protected:
    QSplitter *m_centralwidget_splitter;
    QTabWidget *m_tabwidget;

    QSignalMapper *windowMapper;
    LuminanceOptions *luminance_options;
    QDialog *splash;
    QProgressBar* m_progressbar;

    QString RecentDirHDRSetting;
    QString RecentDirLDRSetting;

    // Recent Files Management
    QAction* recentFileActs[MAX_RECENT_FILES];
    QAction *separatorRecentFiles;

    // Open MainWindows Handling
    QList<QAction*> openMainWindows;

    // I/O
    QThread* IO_thread;
    IOWorker* IO_Worker;

    HelpBrowser *helpBrowser;

    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);
    void closeEvent(QCloseEvent *);

    void ProgressBarInit();
    void ProgressBarFinish(void);

    void dispatchrotate(bool clockwise);

    void updateRecentFileActions();
    void initRecentFileActions();
    void clearRecentFileActions();

    bool testTempDir(QString);

    // Tone Mapping Panel
    TonemappingPanel *tmPanel;
    TMOProgressIndicator *progInd;

    struct {
        bool is_hdr_ready;
        GenericViewer* curr_tm_frame;
        TonemappingOptions* curr_tm_options;
    } tm_status;
    int num_ldr_generated;

    pfs::Frame * getSelectedFrame(HdrViewer *hdr);
    void getCropCoords(HdrViewer *hdr, int& x_ul, int& y_ul, int& x_br, int& y_br);
    
    bool m_isLocked;
    GenericViewer *m_changedImage;
    float m_scaleFactor;
    int m_VSB_Value;
    int m_HSB_Value;

    void init();
    void createUI();
    void loadOptions();
    void createMenus();
    void createToolBar();
    void createCentralWidget();
    void createStatusBar();
    void setupIO();
    void setupTM();
    void createConnections();

    void updateActionsNoImage();
    void updateActionsLdrImage();
    void updateActionsHdrImage();
    void updateMagnificationButtons(GenericViewer*);
    void updatePreviousNextActions();

    QString getCurrentHDRName();

    bool maybeSave();

    // Preview Panel
    PreviewPanel *previewPanel;
    TonemappingOptions *opts;
    int m_previewImgNum;

    // Preview Viewer
    LdrViewer *previewViewer;
};


#endif
