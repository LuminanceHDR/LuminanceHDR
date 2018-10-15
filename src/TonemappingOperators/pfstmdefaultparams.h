/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * This file holds default parameters for all tonemapping operators
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef PFSTMDEFAULTPARAMS_H
#define PFSTMDEFAULTPARAMS_H

// Ashikhmin
#define ASHIKHMIN_SIMPLE false
#define ASHIKHMIN_EQ2 true
#define ASHIKHMIN_LCT 0.5f

// Drago
#define DRAGO03_BIAS 0.85f

// Durand 02
#define DURAND02_SPATIAL 2.0f
#define DURAND02_RANGE 2.0f
#define DURAND02_BASE 5.0f

// Fattal 02
#define FATTAL02_ALPHA 1.0f
#define FATTAL02_BETA 0.9f
#define FATTAL02_COLOR 1.0f
#define FATTAL02_NOISE_REDUX 0.0f
#define FATTAL02_NEWFATTAL true

// Ferradans 11
#define FERRADANS11_RHO -2.0f
#define FERRADANS11_INV_ALPHA 5.0f

// Mantiuk 06
#define MANTIUK06_CONTRAST_FACTOR 0.1f
#define MANTIUK06_SATURATION_FACTOR 0.8f
#define MANTIUK06_DETAIL_FACTOR 0.8f
#define MANTIUK06_CONTRAST_EQUALIZATION false

// Mantiuk 08
#define MANTIUK08_COLOR_SATURATION 1.0f
#define MANTIUK08_CONTRAST_ENHANCEMENT 1.0f
#define MANTIUK08_LUMINANCE_LEVEL 1.0f
#define MANTIUK08_SET_LUMINANCE false

// Pattanaik 00
#define PATTANAIK00_AUTOLUM true
#define PATTANAIK00_LOCAL false
#define PATTANAIK00_CONE 0.5f
#define PATTANAIK00_ROD 0.5f
#define PATTANAIK00_MULTIPLIER 1.0f

// Reinhard 02
#define REINHARD02_SCALES false
#define REINHARD02_KEY 0.18f
#define REINHARD02_PHI 1.0f
#define REINHARD02_RANGE 8
#define REINHARD02_LOWER 1
#define REINHARD02_UPPER 43

// Reinhard 05
#define REINHARD05_BRIGHTNESS -10.0f
#define REINHARD05_CHROMATIC_ADAPTATION 0.0f
#define REINHARD05_LIGHT_ADAPTATION 1.0f

// Ferwerda 96
#define FERWERDA96_MULTIPLIER 1.0f
#define FERWERDA96_ADAPTATION_LUMINANCE 0.5f

// KimKautz 08
#define KIMKAUTZ08_LDMAX 1.0f
#define KIMKAUTZ08_LDMIN 1.0f
#define KIMKAUTZ08_C1 3.0f
#define KIMKAUTZ08_C2 0.5f

// VanHateren 06
#define VANHATEREN06_PUPIL_AREA 10.0f

#endif  // PFSTMDEFAULTPARAMS_H
