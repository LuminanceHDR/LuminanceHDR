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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QString>
#include <QStringList>

//Singleton class
class LuminanceOptions {
public:
	//global point of access to singleton instance
	static LuminanceOptions *getInstance();
	//public accessor to destroy singleton
	static void deleteInstance();

	//commandline options for align_image_stack
	QStringList align_image_stack_options;
	//colors used to draw the NAN/INF or the negative colors
	unsigned int naninfcolor, negcolor;
	//if true, we save a logluv tiff (if false a uncompressed 32 bit tiff)
	bool saveLogLuvTiff;
	// width of previews result
	int previews_width;
	// if true, tmo result windows should come up maximized
	bool tmowindow_max;
	//if true always show processed hdr
	bool tmowindow_showprocessed;
	//if true always show preview panel
	bool tmowindow_showpreviewpanel;
	//if true always show first page of hdr wizard
	bool wizard_show_firstpage;
	// if true (default) warn user about differing results with small frames
	bool tmowarning_fattalsmall;
	//path to save temporary cached pfs files.
	QString tempfilespath;
	//number of threads to use (where threaded execution is enabled).
	int num_threads;
	//Image format used to save LDRs in batch mode.
	QString batch_ldr_format;
	//2-chars ISO 639 language code for Luminance's user interface.
	QString gui_lang;
	//Raw Conversion options
	double aber_0;
	double aber_1;
	double aber_2;
	double aber_3;
	double gamm_0;
	double gamm_1;
	int TK;
	float green;
	float user_mul_0;
	float user_mul_1;
	float user_mul_2;
	float user_mul_3;
	bool auto_bright;
	float brightness;
	float threshold;
	int half_size;
	bool four_color_rgb;
	int highlights;
	int level;
	int wb_method;
	int output_color;
	const char *output_profile;
	const char *camera_profile;
	int user_flip;
	int user_qual;
	int user_black;
	int user_sat;
	int med_passes;
	float auto_bright_thr;
	float adjust_maximum_thr;
	bool do_not_use_fuji_rotate;
	bool use_black;
	bool use_sat;
	bool use_noise;
	bool use_chroma;
private:
	//use QSettings to load stored settings (or use hardcoded defaults on very first run)
	void loadFromQSettings ();
	//private constructor
	LuminanceOptions ();
	//private destructor
	~LuminanceOptions ();
	//the private singleton instance
	static LuminanceOptions *instance;

};




#endif
