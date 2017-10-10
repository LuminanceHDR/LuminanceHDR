/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
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

//! \brief Saturation enhancement function
//! \author Franco Comida <francocomida@users.sourceforge.net>

#ifndef PFS_COLORSPACE_SATURATION_HXX
#define PFS_COLORSPACE_SATURATION_HXX

#include <Common/CommonFunctions.h>
#include <Libpfs/colorspace/saturation.h>

namespace pfs {
namespace colorspace {

inline void ChangeSaturation::operator()(float i1, float i2, float i3, float &o1,
                                        float &o2, float &o3) const {
    float h, s, l;
    rgb2hsl(i1, i2, i3, h, s, l);
    hsl2rgb(h, multiplier*s, l, o1, o2, o3);
}

}
}

#endif  // PFS_COLORSPACE_SATURATION_HXX
