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
#include <QString>
#include <QStringList>
#include <QSignalMapper>
#include <QSplitter>
#include <QTabWidget>
#include <QThread>
#include <QProgressBar>

#include "Common/LuminanceOptions.h"

#define MAX_RECENT_FILES (5)

// Forward declaration
namespace Ui {
    class MainWindow;
}

namespace pfs {
    class Frame;            // #include "Libpfs/frame.h"
}

class IOWorker;             // #include "Core/IOWorker.h"
class GenericViewer;
class PreviewPanel;         // #include "PreviewPanel/PreviewPanel.h"
class HelpBrowser;          // #include "HelpBrowser/helpbrowser.h"
class TMOProgressIndicator; // #include "TonemappingPanel/TMOProgressIndicator.h"
class TonemappingPanel;     // #include "TonemappingPanel/TonemappingPanel.h"
class TonemappingOptions;   // #include "Core/TonemappingOptions.h"
class TMWorker;

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    // Constructor loading file inside
    MainWindow(pfs::Frame* curr_frame, QString new_fname, bool needSaving = false, QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:

    // I/O
    void save_hdr_success(GenericViewer* saved_hdr, QString fname);
    void save_hdr_failed();
    void save_ldr_success(GenericViewer* saved_ldr, QString fname);
    void save_ldr_failed();

    void load_failed(QString);
    void load_success(pfs::Frame* new_hdr_frame, QString new_fname, bool needSaving = false);

    void ioBegin();
    void ioEnd();

    void setMainWindowModified(bool b);

    void setInputFiles(const QStringList& files);

protected Q_SLOTS:
    void fileNewViaWizard(QStringList files = QStringList());
    void fileOpen();    //for File->Open, it then calls loadFile()
    void fileSaveAs();
    void fileSaveAll();
    void saveHdrPreview();

    void rotateccw_requested();
    void rotatecw_requested();

    void resize_requested();
    void projectiveTransf_requested();

    void batch_hdr_requested();
    void batch_requested();

    void hdr_increase_exp();
    void hdr_decrease_exp();
    void hdr_extend_exp();
    void hdr_shrink_exp();
    void hdr_fit_exp();
    void hdr_ldr_exp();

    // Viewers
    void viewerZoomIn();
    void viewerZoomOut();
    void viewerFitToWin(bool checked = true);
    void viewerFillToWin();
    void viewerOriginalSize();
    void updateMagnificationButtons(GenericViewer*);

    void openDocumentation();
    void enterWhatsThis();

    void preferences_called();
    void transplant_called();

    void levelsRequested(bool checked);

    void openInputFiles();

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
    void tonemapBegin();
    void tonemapEnd();
    void tonemapImage(TonemappingOptions *opts);
    void addLdrFrame(pfs::Frame*, TonemappingOptions*);
    //void addLDRResult(QImage*, quint16*);
    void tonemapFailed(QString);

    // lock functionalities
    void lockViewers(bool);
    void syncViewers(GenericViewer*);

    void showPreviewPanel(bool b);

    // QTabWidget
    void removeTab(int);
    void removeCurrentTab();
    void activateNextViewer();
    void activatePreviousViewer();

Q_SIGNALS:
    // update HDR
    void updatedHDR(pfs::Frame*);

protected:
    QSplitter *m_centralwidget_splitter;
    QTabWidget *m_tabwidget;

    QSignalMapper *windowMapper;
    LuminanceOptions luminance_options;
    QDialog *splash;

    // Recent Files Management
    QAction* recentFileActs[MAX_RECENT_FILES];
    QAction *separatorRecentFiles;

    // Open MainWindows Handling
    QList<QAction*> openMainWindows;

    HelpBrowser* helpBrowser;
    QStringList inputFiles;

    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);
    void closeEvent(QCloseEvent *);

    void dispatchrotate(bool clockwise);

    void updateRecentFileActions();
    void initRecentFileActions();
    void clearRecentFileActions();

    struct {
        bool is_hdr_ready;
        GenericViewer* curr_tm_frame;
        TonemappingOptions* curr_tm_options;
    } tm_status;
    int num_ldr_generated;
    int curr_num_ldr_open;

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
    void updatePreviousNextActions();

    QString getCurrentHDRName();

    bool maybeSave();

    // Preview Panel
    PreviewPanel *previewPanel;

    void openFiles(const QStringList& files);

private:
    static int sm_NumMainWindows;

    // I/O
    QThread *m_IOThread;
    IOWorker *m_IOWorker;
    QProgressBar* m_ProgressBar;

    // UI declaration
    Ui::MainWindow* m_Ui;

    // TM thread
    QThread* m_TMThread;
    TMWorker* m_TMWorker;
    TMOProgressIndicator* m_TMProgressBar;
    // Tone Mapping Panel
    TonemappingPanel *tmPanel;
};


#endif
