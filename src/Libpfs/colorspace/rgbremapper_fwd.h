/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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
 */

//! \brief Forward declaration for Remapper

#ifndef PFS_RGBREMAPPER_FWD_H
#define PFS_RGBREMAPPER_FWD_H

enum RGBMappingType {
    MAP_LINEAR = 0,
    MAP_GAMMA1_4 = 1,
    MAP_GAMMA1_8 = 2,
    MAP_GAMMA2_2 = 3,
    MAP_GAMMA2_6 = 4,
    MAP_LOGARITHMIC = 5
};

template <typename TypeOut>
class Remapper;

#endif  // PFS_RGBREMAPPER_FWD_H
