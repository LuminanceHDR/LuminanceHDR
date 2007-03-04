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

#ifndef TONEMAPPINGDIALOG_IMPL_H
#define TONEMAPPINGDIALOG_IMPL_H

#include <QSettings>

#include "../generated_uic/ui_tonemappingdialog.h"
#include "gang.h"
#include "tmo_ashikhmin02.h"
#include "tmo_drago03.h"
#include "tmo_durand02.h"
#include "tmo_fattal02.h"
#include "tmo_pattanaik00.h"
#include "tmo_reinhard02.h"

extern "C" pfs::Frame* pfstmo_reinhard04(pfs::Frame* inpfsframe, float br, float sat );

class TMODialog : public QDialog, private Ui::TMODialog 
{
Q_OBJECT

public:
	TMODialog(QWidget *parent, bool keepsize);
	~TMODialog();
	void setOrigBuffer(pfs::Frame*);
private:
	QVector<int> sizes;
	Gang  *contrastGang,*biasGang,*spatialGang,*rangeGang,*baseGang,*alphaGang,*betaGang,*saturation2Gang,*multiplierGang,*coneGang,*rodGang,*keyGang,*phiGang,*range2Gang,*lowerGang,*upperGang,*brightnessGang,*saturationGang,*pregammagang,*postgammagang;
	pfs::Frame *origbuffer;
	pfs::Frame *resizedHdrbuffer, *afterPreGammabuffer, *afterToneMappedBuffer, *afterPostgammaBuffer;
	void disable_Applybuttons();
	void enable_Applybuttons();
	bool filter_first_tabpage_change;
	QLabel *imageLabel;
	void executeToneMap();
	void getCaptionAndFileName();
	void prepareLDR();
	bool setting_graphic_widgets;
	QImage *QImagecurrentLDR; uchar *imagedata;
	QString caption,fname,exif_comment;
	QSettings settings;
	QString RecentDirTMOSetting,inputSettingsFilename;
	QString RecentDirLDRSetting;
	bool keep_size;

	void setWorkSize( int index );
	void pregammasliderchanged();
	void runToneMap();
	void postgammasliderchanged();
	void writeExifData(const QString);

private slots:
	void startChain();
	void preGammaReset();
	void postGammaReset();
	void ashikhminReset();
	void dragoReset();
	void durandReset();
	void fattalReset();
	void pattanaikReset();
	void reinhard02Reset();
	void reinhard04Reset();
	void saveLDR();
// 	void tabPageChanged();

	void savesettings();
	void loadsettings();
	void fromTxt2Gui(); //i.e. APPLY tmo settings from text file
	void fromGui2Txt(QString destination); //i.e. WRITE tmo settings to   text file
	void decide_size();
};
#endif
