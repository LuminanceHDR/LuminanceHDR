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

#ifndef TONEMAPPINGWIDGET_H
#define TONEMAPPINGWIDGET_H

#include <QProgressBar>
#include "../generated_uic/ui_tonemappingoptions.h"
#include "../Common/gang.h"
#include "../Common/global.h"
#include "../Common/options.h"
#include "../Libpfs/pfs.h"

class QStatusBar;
class MyProgressBar : public QProgressBar {
Q_OBJECT
public:
	MyProgressBar(QWidget * parent = 0);
	~MyProgressBar();
public slots:
	void advanceCurrentProgress();
protected:
	void mousePressEvent(QMouseEvent *event);
signals:
	void leftMouseButtonClicked();
private:
	bool removed;
};

class TMWidget : public QWidget, public Ui::ToneMappingOptions
{
Q_OBJECT
public:
	TMWidget(QWidget *parent, pfs::Frame* &_OriginalPfsFrame, QStatusBar* sb);
	~TMWidget();
signals:
	void newResult(const QImage&,tonemapping_options*);
private:
	QVector<int> sizes;

	Gang *contrastfactorGang, *saturationfactorGang, *detailfactorGang, *contrastGang, *biasGang, *spatialGang, *rangeGang, *baseGang, *alphaGang, *betaGang, *saturation2Gang, *noiseGang, *multiplierGang, *coneGang, *rodGang, *keyGang, *phiGang, *range2Gang, *lowerGang, *upperGang, *brightnessGang, *chromaticGang, *lightGang, *pregammagang;

	tonemapping_options ToneMappingOptions;
	pfs::Frame* &OriginalPfsFrame;
	void FillToneMappingOptions();
	void fromGui2Txt(QString destination); //i.e. WRITE tmo settings to text file
	QString RecentPathLoadSaveTmoSettings, TMOSettingsFilename, cachepath;
	int out_ldr_cs;
	QStatusBar *sb;
	bool adding_custom_size;
	float HeightWidthRatio;
protected:
	void keyPressEvent(QKeyEvent* event);
private slots:
	void preGammaReset();
	void ashikhminReset();
	void dragoReset();
	void durandReset();
	void fattalReset();
	void pattanaikReset();
	void reinhard02Reset();
	void reinhard05Reset();
	void mantiukReset();
	void apply_clicked();
	void savesettings();
	void loadsettings();
	//APPLY tmo settings from text file
	void fromTxt2Gui();
	//user wants a custom size.
	void addCustomSize();
};

#endif
