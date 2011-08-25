/**
 * This file is a part of LuminanceHDR package.
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

#include "options.h"
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDir>
#include <QLocale>
#include "global.h"
#include "config.h"

LuminanceOptions *LuminanceOptions::instance = 0;

LuminanceOptions *LuminanceOptions::getInstance()
{
	if (!instance) {
		instance = new LuminanceOptions();
	}
	return instance;
}

LuminanceOptions::LuminanceOptions () {
	loadFromQSettings();
}

LuminanceOptions::~LuminanceOptions() {
}

void LuminanceOptions::deleteInstance() {
	delete instance; instance=0;
}

void LuminanceOptions::loadFromQSettings()
{
	//write system default language the first time around (discard "_country")
	if (!settings->contains(KEY_GUI_LANG))
		settings->setValue(KEY_GUI_LANG,QLocale::system().name().left(2));
	gui_lang = settings->value(KEY_GUI_LANG,QLocale::system().name().left(2)).toString();

	settings->beginGroup(GROUP_EXTERNALTOOLS);
		//bug 2001032, remove spurious default QString "-a aligned_" value set by ver 1.9.2
		if (!settings->contains(KEY_EXTERNAL_AIS_OPTIONS) || settings->value(KEY_EXTERNAL_AIS_OPTIONS).toString()=="-v -a aligned_")
			settings->setValue(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << "-v" << "-a" << "aligned_");
		align_image_stack_options=settings->value(KEY_EXTERNAL_AIS_OPTIONS).toStringList();
	settings->endGroup();

	settings->beginGroup(GROUP_HDRVISUALIZATION);
		if (!settings->contains(KEY_NANINFCOLOR))
			settings->setValue(KEY_NANINFCOLOR,0xFF000000);
		naninfcolor=settings->value(KEY_NANINFCOLOR,0xFF000000).toUInt();
	
		if (!settings->contains(KEY_NEGCOLOR))
			settings->setValue(KEY_NEGCOLOR,0xFF000000);
		negcolor=settings->value(KEY_NEGCOLOR,0xFF000000).toUInt();
	settings->endGroup();

	settings->beginGroup(GROUP_TONEMAPPING);
		if (!settings->contains(KEY_TEMP_RESULT_PATH))
			settings->setValue(KEY_TEMP_RESULT_PATH, QDir::currentPath());
		tempfilespath=settings->value(KEY_TEMP_RESULT_PATH,QDir::currentPath()).toString();
		if (!settings->contains(KEY_BATCH_LDR_FORMAT))
			settings->setValue(KEY_BATCH_LDR_FORMAT, "JPEG");
		batch_ldr_format=settings->value(KEY_BATCH_LDR_FORMAT,"JPEG").toString();
		if (!settings->contains(KEY_NUM_BATCH_THREADS))
			settings->setValue(KEY_NUM_BATCH_THREADS, 1);
		num_threads=settings->value(KEY_NUM_BATCH_THREADS,1).toInt();
	settings->endGroup();

	settings->beginGroup(GROUP_TIFF);
		if (!settings->contains(KEY_SAVE_LOGLUV))
			settings->setValue(KEY_SAVE_LOGLUV,true);
		saveLogLuvTiff=settings->value(KEY_SAVE_LOGLUV,true).toBool();
	settings->endGroup();

	settings->beginGroup(GROUP_TMOWINDOW);
        if (!settings->contains(KEY_TMOWINDOW_MAX))
			settings->setValue(KEY_TMOWINDOW_MAX,false);
		tmowindow_max=settings->value(KEY_TMOWINDOW_MAX,false).toBool();
		
		if (!settings->contains(KEY_TMOWINDOW_SHOWPROCESSED))
			settings->setValue(KEY_TMOWINDOW_SHOWPROCESSED,false);
		tmowindow_showprocessed=settings->value(KEY_TMOWINDOW_SHOWPROCESSED,false).toBool();

		if (!settings->contains(KEY_TMOWINDOW_SHOWPREVIEWPANEL))
			settings->setValue(KEY_TMOWINDOW_SHOWPREVIEWPANEL,true);
		tmowindow_showpreviewpanel=settings->value(KEY_TMOWINDOW_SHOWPREVIEWPANEL,true).toBool();
	settings->endGroup();

	settings->beginGroup(GROUP_HDR_WIZARD);
		if (!settings->contains(KEY_WIZARD_SHOWFIRSTPAGE))
			settings->setValue(KEY_WIZARD_SHOWFIRSTPAGE,true);
		wizard_show_firstpage=settings->value(KEY_WIZARD_SHOWFIRSTPAGE,true).toBool();
	settings->endGroup();

	settings->beginGroup(GROUP_TMOWARNING);
	if (!settings->contains(KEY_TMOWARNING_FATTALSMALL))
			settings->setValue(KEY_TMOWARNING_FATTALSMALL,true);
		tmowarning_fattalsmall=settings->value(KEY_TMOWARNING_FATTALSMALL,true).toBool();
	settings->endGroup();
	settings->beginGroup(GROUP_RAW_CONVERSION_OPTIONS);
	if (!settings->contains(KEY_GAMM_0)) {
		//set default values
		settings->setValue(KEY_ABER_0,1.0);
		settings->setValue(KEY_ABER_2,1.0);
		settings->setValue(KEY_GAMM_0,1/2.4);
		settings->setValue(KEY_GAMM_1,12.92);
		settings->setValue(KEY_TK,6500);
		settings->setValue(KEY_GREEN,1.0);
		settings->setValue(KEY_USER_MUL_0,1.0);
		settings->setValue(KEY_USER_MUL_1,1.0);
		settings->setValue(KEY_USER_MUL_2,1.0);
		settings->setValue(KEY_USER_MUL_3,1.0);
		settings->setValue(KEY_AUTO_BRIGHT,true);
		settings->setValue(KEY_BRIGHTNESS,1.0);
		settings->setValue(KEY_THRESHOLD,100.0);
		settings->setValue(KEY_HALF_SIZE,0);
		settings->setValue(KEY_FOUR_COLOR_RGB,false);
		settings->setValue(KEY_WB_METHOD,0);
		settings->setValue(KEY_OUTPUT_COLOR,1);
		settings->setValue(KEY_OUTPUT_PROFILE,"");
		settings->setValue(KEY_CAMERA_PROFILE,"");
		settings->setValue(KEY_USER_FLIP,0);
		settings->setValue(KEY_USER_QUAL,0);
		settings->setValue(KEY_USER_BLACK,0);
		settings->setValue(KEY_USER_SAT,20000);
		settings->setValue(KEY_MED_PASSES,0);
		settings->setValue(KEY_HIGHLIGHTS,0);
		settings->setValue(KEY_LEVEL,0);
		settings->setValue(KEY_AUTO_BRIGHT_THR,0);
		settings->setValue(KEY_ADJUST_MAXIMUM_THR,0);
		settings->setValue(KEY_DO_NOT_USE_FUJI_ROTATE,false);
		settings->setValue(KEY_USE_BLACK,false);
		settings->setValue(KEY_USE_SAT,false);
		settings->setValue(KEY_USE_NOISE,true);
		settings->setValue(KEY_USE_CHROMA,false);
	}
	aber_0 = settings->value(KEY_ABER_0).toDouble();
	aber_2 = settings->value(KEY_ABER_2).toDouble();
	gamm_0 = settings->value(KEY_GAMM_0).toDouble();
	gamm_1 = settings->value(KEY_GAMM_1).toDouble();
	TK = settings->value(KEY_TK).toInt();
	green = settings->value(KEY_GREEN).toFloat();
	user_mul_0 = settings->value(KEY_USER_MUL_0).toFloat();
	user_mul_1 = settings->value(KEY_USER_MUL_1).toFloat();
	user_mul_2 = settings->value(KEY_USER_MUL_2).toFloat();
	user_mul_3 = settings->value(KEY_USER_MUL_3).toFloat();
	auto_bright = settings->value(KEY_AUTO_BRIGHT).toBool();
	brightness = settings->value(KEY_BRIGHTNESS).toFloat();
	threshold = settings->value(KEY_THRESHOLD).toFloat();
	half_size = settings->value(KEY_HALF_SIZE).toInt();
	four_color_rgb = settings->value(KEY_FOUR_COLOR_RGB).toBool();
	wb_method = settings->value(KEY_WB_METHOD).toInt();
	output_color = settings->value(KEY_OUTPUT_COLOR).toInt();
        output_profile = QFile::encodeName(settings->value(KEY_OUTPUT_PROFILE).toString()).constData();
        camera_profile = QFile::encodeName(settings->value(KEY_CAMERA_PROFILE).toString()).constData();
	user_flip = settings->value(KEY_USER_FLIP).toInt();
	user_qual = settings->value(KEY_USER_QUAL).toInt();
	user_black = settings->value(KEY_USER_BLACK).toInt();
	user_sat = settings->value(KEY_USER_SAT).toInt();
	med_passes = settings->value(KEY_MED_PASSES).toInt();
	highlights = settings->value(KEY_HIGHLIGHTS).toInt();
	level = settings->value(KEY_LEVEL).toInt();
	auto_bright_thr = settings->value(KEY_AUTO_BRIGHT_THR).toFloat();
	adjust_maximum_thr = settings->value(KEY_ADJUST_MAXIMUM_THR).toFloat();
	do_not_use_fuji_rotate = settings->value(KEY_DO_NOT_USE_FUJI_ROTATE).toBool();
	use_black = settings->value(KEY_USE_BLACK).toBool();
	use_sat = settings->value(KEY_USE_SAT).toBool();
	use_noise = settings->value(KEY_USE_NOISE).toBool();
	use_chroma = settings->value(KEY_USE_CHROMA).toBool();
	settings->endGroup();
}



