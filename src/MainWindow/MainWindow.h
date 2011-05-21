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

#ifndef MAINGUI_IMPL_H
#define MAINGUI_IMPL_H

#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStringList>
#include <QProgressDialog>
#include <QSignalMapper>
#include <QSplitter>
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

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    // For ProgressBar
    void ProgressBarSetMaximum(int max);
    void ProgressBarSetValue(int value);

    // I/O
    void save_hdr_success(HdrViewer* saved_hdr, QString fname);
    void save_hdr_failed();
    void save_ldr_success(LdrViewer* saved_ldr, QString fname);
    void save_ldr_failed();

    void load_failed(QString);
    void load_success(pfs::Frame* new_hdr_frame, QString new_fname);

    void IO_done();
    void IO_start();

    void tonemapImage(TonemappingOptions *opts);

protected slots:
    bool eventFilter(QObject *obj, QEvent *event);
    void fileNewViaWizard(QStringList files = QStringList());
    void fileOpen();    //for File->Open, it then calls loadFile()
    void fileSaveAs();
    void saveHdrPreview();
    void tonemap_requested();
    void rotateccw_requested();
    void rotatecw_requested();
    void resize_requested();
    void projectiveTransf_requested();
    void batch_requested();
    void current_mdi_increase_exp();
    void current_mdi_decrease_exp();
    void current_mdi_extend_exp();
    void current_mdi_shrink_exp();
    void current_mdi_fit_exp();
    void current_mdi_ldr_exp();
    void current_mdi_zoomin();
    void current_mdi_zoomout();
    void current_mdi_fit_to_win(bool checked);
    void current_mdi_original_size();
    void openDocumentation();
    void enterWhatsThis();
    void preferences_called();
    void transplant_called();
    void reEnableMainWin();
    void fileExit();
    void Text_Under_Icons();
    void Icons_Only();
    void Text_Alongside_Icons();
    void Text_Only();
    void updateWindowMenu();

    void openRecentFile();
    void setCurrentFile(const QString &fileName);

    void updateRecentDirHDRSetting(QString);
    void updateRecentDirLDRSetting(QString);

    void aboutLuminance();
    void showSplash();

    void updateActions(QMdiSubWindow * w);
    void setActiveSubWindow(QWidget* w);
    void cropToSelection();
    void enableCrop(bool);
    void disableCrop();
    void helpBrowserClosed();
    void showDonationsPage();
    void splashShowDonationsPage();
    void splashClose();

    // TM
    void addProcessedFrame(pfs::Frame *);
    void addMDIResult(QImage*);
    void tonemappingFinished();
    void tmDockVisibilityChanged(bool);
    void deleteTMOThread(TMOThread *th);
    void showErrorMessage(const char *e);
    void lockImages(bool);
    void updateImage(GenericViewer *viewer);
    void dispatch(GenericViewer *);
    void showHDRs(bool);

signals:
    // I/O
    void save_hdr_frame(HdrViewer*, QString);
    void save_ldr_frame(LdrViewer*, QString, int);  // viewer, filename, quality level
    void open_frames(QStringList);
    void open_frame(QString);

protected:
    enum { MaxRecentFiles = 5 };
    enum MWState { IO_STATE = 0, TM_STATE = 1 };
    MWState current_state;

    QMdiArea* mdiArea;
    QSignalMapper *windowMapper;
    LuminanceOptions *luminance_options;
    QDialog *splash;
    QProgressBar* m_progressbar;

    QString RecentDirHDRSetting;
    QString RecentDirLDRSetting;
    QAction *recentFileActs[MaxRecentFiles];
    QAction *separatorRecentFiles;

    // I/O
    QThread* IO_thread;
    IOWorker* IO_Worker;

    GenericViewer* active_frame;    // ideally we should be remove this soon or later
    HelpBrowser *helpBrowser;

    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);
    void closeEvent ( QCloseEvent * );

    void ProgressBarInit();
    void ProgressBarFinish(void);
    void setupConnections();
    void dispatchrotate( bool clockwise);
    void updateRecentFileActions();
    void load_options();

    bool testTempDir(QString);

    // I/O
    void setup_io();

    // Tone Mapping Panel
    QDockWidget *dock;
    TonemappingPanel *tmPanel;
    TMOProgressIndicator *progInd;

    void setup_tm();
    void setup_tm_slots();

    struct {
        pfs::Frame* curr_tm_frame;
        TonemappingOptions* curr_tm_options;
        QList<QMdiSubWindow*> hidden_windows;
    } tm_status;

    pfs::Frame * getSelectedFrame(HdrViewer *hdr);
    void getCropCoords(HdrViewer *hdr, int& x_ul, int& y_ul, int& x_br, int& y_br);

    int m_ldrsNum,
	m_hdrsNum;
    
    bool m_isLocked;
    GenericViewer *m_changedImage;
    float m_scaleFactor;
    int m_VSB_Value;
    int m_HSB_Value;
};


#endif
