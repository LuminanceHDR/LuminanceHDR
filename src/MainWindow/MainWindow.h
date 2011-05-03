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
#include <QThread>

#include "ui_MainWindow.h"
#include "HdrWizard/HdrWizard.h"
#include "Preferences/PreferencesDialog.h"
#include "Resize/ResizeDialog.h"
#include "Projection/ProjectionsDialog.h"
#include "HelpBrowser/helpbrowser.h"
#include "Viewers/HdrViewer.h"
#include "UI/UMessageBox.h"
#include "Core/IOWorker.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
Q_OBJECT

public:
	MainWindow(QWidget *parent=0);
	~MainWindow();
public slots:
        // For ProgressBar
        void ProgressBarSetMaximum(int max);
        void ProgressBarSetValue(int value);

        // I/O
        void save_success(HdrViewer* saved_hdr, QString fname);
        void save_failed();

        void load_failed(QString);
        void load_success(pfs::Frame* new_hdr_frame, QString new_fname);

        void IO_done();
        void IO_start();

protected slots:
	void fileNewViaWizard(QStringList files = QStringList());
	void fileOpen();//for File->Open, it then calls loadFile()
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

signals:
        // I/O
        void save_frame(HdrViewer*, QString);
        void open_frames(QStringList);
        void open_frame(QString);

protected:
        enum { MaxRecentFiles = 5 };

        QMdiArea* mdiArea;
        QSignalMapper *windowMapper;
        QAction *recentFileActs[MaxRecentFiles];
        QAction *separatorRecentFiles;
        QString RecentDirHDRSetting;
        LuminanceOptions *luminance_options;
        QDialog *splash;
        QProgressBar* m_progressbar;

        // I/O
        QThread* IO_thread;
        IOWorker* IO_Worker;

        HdrViewer* currenthdr;
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
        void initIOThread();
};


#endif
