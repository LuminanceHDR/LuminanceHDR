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

	//commandline options for RAW import functionality, thanks to dcraw
	QStringList dcraw_options;
	//commandline options for align_image_stack
	QStringList align_image_stack_options;
	//colors used to draw the NAN/INF or the negative colors
	unsigned int naninfcolor, negcolor;
	//if true, we save a logluv tiff (if false a uncompressed 32 bit tiff)
	bool saveLogLuvTiff;
	// if true, tmo result windows should come up maximized
	bool tmowindow_max;
	//if true also show processed hdr
	bool tmowindow_showprocessed;
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
	float user_mul_0;
	float user_mul_1;
	float user_mul_2;
	float user_mul_3;
	int auto_bright;
	float brightness;
	float threshold;
	int half_size;
	int four_color_rgb;
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
	//float auto_bright_thr;
	//float adjust_maximum_thr;
	int use_fuji_rotate;
	int use_black;
	int use_sat;
	int use_noise;
	int use_chroma;
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


//----------------- DO NOT CHANGE ENUMERATION ORDER -----------------------
enum TMOperator {mantiuk06,mantiuk08,fattal,drago,durand,reinhard02,reinhard05,ashikhmin,pattanaik};

struct TonemappingOptions
{
	int origxsize;
	int xsize;
	float pregamma;
	bool tonemapSelection;  
	bool tonemapOriginal;
	enum TMOperator tmoperator;
	const char *tmoperator_str;
	union {
		struct {
			bool  simple;
			bool  eq2; //false means eq4
			float lct;
		} ashikhminoptions;
		struct{
			float bias;
		} dragooptions;
		struct {
			float spatial;
			float range;
			float base;
		} durandoptions;
		struct {
			float alpha;
			float beta;
			float color;
			float noiseredux;
			bool  newfattal;
		} fattaloptions;
		struct {
			bool  autolum;
			bool  local;
			float cone;
			float rod;
			float multiplier;
		} pattanaikoptions;
		struct {
			bool  scales;
			float key;
			float phi;
			int   range;
			int   lower;
			int   upper;
		} reinhard02options;
		struct {
			float brightness;
			float chromaticAdaptation;
			float lightAdaptation;
		} reinhard05options;
		struct {
			float contrastfactor;
			float saturationfactor;
			float detailfactor;
			bool  contrastequalization;
		} mantiuk06options;
		struct {
			float colorsaturation;
			float contrastenhancement;
			float luminancelevel;
			bool  setluminance;
		} mantiuk08options;
	} operator_options;
  
  // Davide Anastasia <davide.anastasia@gmail.com>
  // Adding the coordinates of the crop inside this structure will allow TMOThread
  // to crop itself the region of interest, keeping the code tight and simple
  // and avoiding useless copy in memory of the frame to be processed.
  int selection_x_up_left;
  int selection_y_up_left;
  int selection_x_bottom_right;
  int selection_y_bottom_right;
};

class TMOptionsOperations {
public:
	TMOptionsOperations(const TonemappingOptions* opts);
	static TonemappingOptions* parseFile(QString file);
	static TonemappingOptions* getDefaultTMOptions();
	QString getPostfix();
	QString getCaption();
	QString getExifComment();
private:
	const TonemappingOptions* opts;
};

#endif
