/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007,2008 Giuseppe Rota
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

#ifndef TONEMAPPINGDIALOG_IMPL_H
#define TONEMAPPINGDIALOG_IMPL_H

#include <QMdiArea>
#include <QDockWidget>
#include <QMdiSubWindow>
#include <QTimer>

#include "ui_TonemappingWindow.h"
#include "Common/global.h"
#include "Common/options.h"
#include "Viewers/HdrViewer.h"
#include "TonemappingPanel.h"
#include "Threads/TMOThread.h"
#include "ThreadManager.h"

class TonemappingWindow : public QMainWindow, public Ui::TonemappingWindow
{
Q_OBJECT

public:
	TonemappingWindow(QWidget *parent, pfs::Frame *f, QString prefixname);
	~TonemappingWindow();
	pfs::Frame *getSelectedFrame();
protected:
	void closeEvent(QCloseEvent *);
signals:
	void closing();
private:
	pfs::Frame *workingPfsFrame;
	QMdiArea* mdiArea;
	QDockWidget *dock;
	TonemappingPanel *tmPanel;
	ThreadManager *threadManager;
	HdrViewer *originalHDR;
	QMdiSubWindow *originalHdrSubWin;
	LuminanceOptions *luminance_options;
	const TonemappingOptions *tmoOptions;
	QString recentPathSaveLDR, prefixname, cachepath;
	bool isLocked;
	GenericViewer *changedImage;
	float scaleFactor;
	int VSB_Value;
	int HSB_Value;
	int threadCounter;
	int frameCounter;
	QTimer *workingLogoTimer;

	void load_options();
protected slots:
	//bool eventFilter(QObject *obj, QEvent *event);
	void setupConnections();
	void addMDIResult(const QImage&);
	void addProcessedFrame(pfs::Frame *);
	void LevelsRequested(bool);
	void levels_closed();
	void updateActions(QMdiSubWindow *);
	void viewAllAsThumbnails();
	void on_actionClose_All_triggered();
	void on_actionSaveAll_triggered();
	void on_actionSave_triggered();
	void enterWhatsThis();
	void showHDR(bool);
	void Text_Under_Icons();
	void Icons_Only();
	void Text_Alongside_Icons();
	void Text_Only();
	void current_mdi_zoomin();
	void current_mdi_zoomout();
	void current_mdi_fit_to_win(bool checked);
	void current_mdi_original_size();
	void lockImages(bool);
	void updateImage(GenericViewer *viewer);
	void dispatch(GenericViewer *);
	void openDocumentation(); 
	void aboutLuminance();
	void tonemapImage(const TonemappingOptions&);
	void showErrorMessage(const char *e);
	void tonemappingFinished();
	void updateLogo();
	void deleteTMOThread(TMOThread *);
};

#endif
