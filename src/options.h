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

struct dcraw_opts {
	bool auto_wb; // "-a" Use automatic white balance. DEFAULT IS FALSE.
	bool camera_wb; //"-w" Use camera white balance, if possible. DEFAULT IS (FALSE, but set it) TRUE.
	int highlights; // "-H [0-9]"  Highlight mode (0=clip (default), 1=no clip, 2+=recover)
	int quality; //"-q [0-2]"  Set the interpolation quality (default is 2, or 1 if(fuji_width))0=bilin,1=vng,2=ahd
	bool four_colors; //"-f"  Interpolate RGGB as four colors. If true, dcraw will use VNG (quality=2)
	int output_color_space;//-o output color space, should we always use -m 0 i.e. raw color?
	//always 16bit
	//always write to memory, to a pfs::Frame.
};

struct qtpfsgui_opts {
	//options for RAW import functionality, thanks to dcraw
	dcraw_opts dcraw_options;
	//color used to draw the NAN/INF or the negative colors
	unsigned int naninfcolor, negcolor;
	//if true, we save a logluv tiff (if false a uncompressed 32 bit tiff)
	bool saveLogLuvTiff;
	//path to save temporary cached pfs files.
	QString tempfilespath;
	//number of threads to use for the batch feature.
	int num_batch_threads;
	//Image format used to save LDRs in batch mode.
	QString batch_ldr_format;
	//pfs::ColorSpace used for LDR output (1=RGB,2=sRGB)
	int output_cs;
};


enum tmoperator {ashikhmin,drago,durand,fattal,pattanaik,reinhard02,reinhard04,mantiuk};
struct tonemapping_options {
	int xsize;
	float pregamma;
	enum tmoperator tmoperator;
	union {
		struct {
			bool simple;
			bool eq2; //false means eq4
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
			bool newfattal;
		} fattaloptions;
		struct {
			bool autolum;
			bool local;
			float cone;
			float rod;
			float multiplier;
		} pattanaikoptions;
		struct {
			bool scales;
			float key;
			float phi;
			int range;
			int lower;
			int upper;
		} reinhard02options;
		struct {
			float brightness;
			float chromaticAdaptation;
			float lightAdaptation;
		} reinhard04options;
		struct {
			float contrastfactor;
			float saturationfactor;
			bool contrastequalization;
		} mantiukoptions;
	} operator_options;
};

class TMOptionsOperations {
public:
	TMOptionsOperations(tonemapping_options* opts);
	static tonemapping_options* parseFile(QString file);
	QString getPostfix();
	QString getCaption();
	QString getExifComment();
private:
	tonemapping_options* opts;
};

#endif
