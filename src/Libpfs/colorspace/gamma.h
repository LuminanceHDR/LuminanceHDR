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

#ifndef PFS_COLORSPACE_GAMMA_H
#define PFS_COLORSPACE_GAMMA_H

#include <Libpfs/colorspace/convert.h>

#include <cmath>

namespace pfs {
namespace colorspace {

struct Gamma2_2 {
    static float gamma() { return 1 / 2.2f; }
};

struct Gamma1_8 {
    static float gamma() { return 1 / 1.8f; }
};

template <typename GammaFactor>
struct Gamma {
    template <typename TypeIn, typename TypeOut>
    TypeOut operator()(TypeIn sample) const {
        return ConvertSample<TypeOut, float>()(
            std::pow(convertSample<float>(sample), GammaFactor::gamma()));
    }

    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn v1, TypeIn v2, TypeIn v3, TypeOut &o1, TypeOut &o2,
                    TypeOut &o3) {
        o1 = operator()<TypeIn, TypeOut>(v1);
        o2 = operator()<TypeIn, TypeOut>(v2);
        o3 = operator()<TypeIn, TypeOut>(v3);
    }
};

}  // colorspace
}  // pfs

#endif  // PFS_COLORSPACE_GAMMA_H
