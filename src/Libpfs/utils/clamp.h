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

#ifndef PFS_UTILS_CLAMP_H
#define PFS_UTILS_CLAMP_H

namespace pfs {
namespace utils {

template <typename Type>
struct Clamp {
    Clamp(Type min, Type max) : m_min(min), m_max(max) {}

    Type operator()(Type in) const {
        if (in < m_min) return m_min;
        if (in > m_max) return m_max;
        return in;
    }

    void operator()(Type i1, Type i2, Type i3, Type &o1, Type &o2,
                    Type &o3) const {
        o1 = (*this)(i1);
        o2 = (*this)(i2);
        o3 = (*this)(i3);
    }

   private:
    Type m_min;
    Type m_max;
};

typedef Clamp<float> ClampF32;

// default float clamp between 0 and 1.f (very common)
static const ClampF32 CLAMP_F32(0.f, 1.f);

}  // utils
}  // pfs

#endif  // PFS_UTILS_CLAMP_H
