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

#ifndef PFS_COLORSPACE_NORMALIZER_H
#define PFS_COLORSPACE_NORMALIZER_H

#include <cassert>

#include <boost/math/special_functions/fpclassify.hpp>

namespace pfs {
namespace colorspace {

struct Normalizer {
    Normalizer(float min, float max) : m_min(min), m_range(max - min) {
        assert(m_range != 0.f);
    }

    float operator()(float sample) const {
        assert(!boost::math::isnan(sample));

        return (sample - m_min) / m_range;
    }

    void operator()(float i1, float i2, float i3, float &o1, float &o2,
                    float &o3) const {
        o1 = (*this)(i1);
        o2 = (*this)(i2);
        o3 = (*this)(i3);
    }

   private:
    float m_min;
    float m_range;
};

}  // colorspace
}  // pfs

#endif
