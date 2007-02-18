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

#define GROUP_DCRAW "DCRAW_Options"
#define GROUP_HDRVISUALIZATION "HDR_visualization"
#define GROUP_TONEMAPPING "Tonemapping_Options"

#define KEY_AUTOWB "auto_white_balance"
#define KEY_CAMERAWB "camera_white_balance"
#define KEY_HIGHLIGHTS "highlights_recovery_mode"
#define KEY_QUALITY "quality_mode"
#define KEY_4COLORS "use_4colors_interpolation"
#define KEY_OUTCOLOR "output_color_space"

#define KEY_NANINFCOLOR "nan_inf_color"
#define KEY_NEGCOLOR "neg_color"

#define KEY_KEEPSIZE "keep_size_on_tabswitch"

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
	dcraw_opts dcraw_options; //options for RAW import functionality, thanks to dcraw
	unsigned int naninfcolor, negcolor; //color used to draw the NAN/INF or the negative colors
	bool keepsize; //keep size of ToneMapping result when changing operator tab
};
#endif
