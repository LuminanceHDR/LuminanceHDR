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
	//path to save temporary cached pfs files.
	QString tempfilespath;
	//number of threads to use (where threaded execution is enabled).
	int num_threads;
	//Image format used to save LDRs in batch mode.
	QString batch_ldr_format;
	//2-chars ISO 639 language code for Luminance's user interface.
	QString gui_lang;
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

struct TonemappingOptions {
	int origxsize;
	int xsize;
	float pregamma;
	bool tonemapSelection;
	bool tonemapOriginal;
	enum TMOperator tmoperator;
	char *tmoperator_str;
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
