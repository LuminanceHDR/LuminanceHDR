/**
 * @brief Interface for TM Operators
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ---------------------------------------------------------------------- 
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
 * @author Davide Anastasia, <davide.anastasia@gmail.com>
 *
 */

#ifndef __TM_OPERATORS_H__
#define __TM_OPERATORS_H__

#include "Libpfs/pfs.h"
#include "Common/ProgressHelper.h"

void pfstmo_ashikhmin02(pfs::Frame* inpfsframe,  bool simple_flag, float lc_value, int eq, ProgressHelper *ph);
void pfstmo_drago03(pfs::Frame *frame, float biasValue, ProgressHelper *ph);
void pfstmo_durand02(pfs::Frame* frame, float sigma_s, float sigma_r, float baseContrast, ProgressHelper *ph);
void pfstmo_fattal02(pfs::Frame* frame, float opt_alpha, float opt_beta, float opt_saturation, float opt_noise, bool newfattal, ProgressHelper *ph);
void pfstmo_mantiuk06(pfs::Frame* frame, float scaleFactor, float saturationFactor, float detailFactor, bool cont_eq, ProgressHelper *ph);
void pfstmo_mantiuk08(pfs::Frame* frame, float saturation_factor, float contrast_enhance_factor, float white_y, bool setluminance, ProgressHelper *ph);
void pfstmo_pattanaik00(pfs::Frame* frame, bool local, float multiplier, float Acone, float Arod, bool autolum, ProgressHelper *ph);
void pfstmo_reinhard02 (pfs::Frame* frame, float key, float phi, int num, int low, int high, bool use_scales, ProgressHelper *ph);
void pfstmo_reinhard05(pfs::Frame *frame, float brightness, float chromaticadaptation, float lightadaptation, ProgressHelper *ph);

#endif