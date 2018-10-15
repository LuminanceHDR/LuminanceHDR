/**
 * @brief Common intrefaces shared by several operators
 *        Interfaces for TM Operators
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * @author Davide Anastasia <davide.anastasia@gmail.com>
 *  Merge of different replicated files
 *
 * $Id: pfstmo.h,v 1.3 2008/09/21 10:42:47 julians37 Exp $
 */

#ifndef PFSTMO_H
#define PFSTMO_H

namespace pfs {
class Frame;
class Progress;
}

#ifdef BRANCH_PREDICTION
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/* Common return codes for operators */
#define PFSTMO_OK 1       /* Successful */
#define PFSTMO_ABORTED -1 /* User aborted (from callback) */
#define PFSTMO_ERROR -2   /* Failed, encountered error */

void pfstmo_ashikhmin02(pfs::Frame &frame, bool simple_flag, float lc_value,
                        int eq, pfs::Progress &ph);
void pfstmo_drago03(pfs::Frame &frame, float biasValue, pfs::Progress &ph);
void pfstmo_durand02(pfs::Frame &frame, float sigma_s, float sigma_r,
                     float baseContrast, pfs::Progress &ph);
void pfstmo_fattal02(pfs::Frame &frame, float opt_alpha, float opt_beta,
                     float opt_saturation, float opt_noise, bool newfattal,
                     bool fftsolver, int detail_level, pfs::Progress &ph);
void pfstmo_ferradans11(pfs::Frame &frame, float opt_rho, float opt_inv_alpha,
                        pfs::Progress &ph);
void pfstmo_ferwerda96(pfs::Frame &frame, float Ld_Max, float L_da,
                        pfs::Progress &ph);
void pfstmo_kimkautz08(pfs::Frame &frame, float KK_c1, float KK_c2,
                        pfs::Progress &ph);
void pfstmo_mai11(pfs::Frame &frame, pfs::Progress &ph);
void pfstmo_mantiuk06(pfs::Frame &frame, float scaleFactor,
                      float saturationFactor, float detailFactor, bool cont_eq,
                      pfs::Progress &ph);
void pfstmo_mantiuk08(pfs::Frame &frame, float saturation_factor,
                      float contrast_enhance_factor, float white_y,
                      bool setluminance, pfs::Progress &ph);
void pfstmo_pattanaik00(pfs::Frame &frame, bool local, float multiplier,
                        float Acone, float Arod, bool autolum,
                        pfs::Progress &ph);
void pfstmo_reinhard02(pfs::Frame &frame, float key, float phi, int num,
                       int low, int high, bool use_scales, pfs::Progress &ph);
void pfstmo_reinhard05(pfs::Frame &frame, float brightness,
                       float chromaticadaptation, float lightadaptation,
                       pfs::Progress &ph);
void pfstmo_vanhateren06(pfs::Frame &frame, float pupil_area,
                        pfs::Progress &ph);

#endif
