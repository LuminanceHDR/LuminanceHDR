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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QString>
#include <QStringList>

//Singleton class
class QtpfsguiOptions {
public:
	//global point of access to singleton instance
	static QtpfsguiOptions *getInstance();
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
	//2-chars ISO 639 language code for Qtpfsgui's user interface.
	QString gui_lang;
private:
	//use QSettings to load stored settings (or use hardcoded defaults on very first run)
	void loadFromQSettings ();
	//private constructor
	QtpfsguiOptions ();
	//private destructor
	~QtpfsguiOptions ();
	//the private singleton instance
	static QtpfsguiOptions *instance;

};

enum tmoperator {ashikhmin,drago,durand,fattal,pattanaik,reinhard02,reinhard05,mantiuk};
struct tonemapping_options {
	int xsize;
	float pregamma;
	enum tmoperator tmoperator;
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
			bool  contrastequalization;
		} mantiukoptions;
	} operator_options;
};

class TMOptionsOperations {
public:
	TMOptionsOperations(tonemapping_options* opts);
	static tonemapping_options* parseFile(QString file);
	static tonemapping_options* getDefaultTMOptions();
	QString getPostfix();
	QString getCaption();
	QString getExifComment();
private:
	tonemapping_options* opts;
};

#endif
