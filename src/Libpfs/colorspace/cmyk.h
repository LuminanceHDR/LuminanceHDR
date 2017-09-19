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

//! \brief CMYK conversion functions
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_COLORSPACE_CMYK_H
#define PFS_COLORSPACE_CMYK_H

#include <Libpfs/colorspace/convert.h>

namespace pfs {
namespace colorspace {

//! \brief Functor CMYK -> RGB conversion
struct ConvertInvertedCMYK2RGB {
    //! \brief
    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn c, TypeIn m, TypeIn y, TypeIn k, TypeOut &r,
                    TypeOut &g, TypeOut &b) const {
        float c_ = ConvertSample<float, TypeIn>()(c);
        float m_ = ConvertSample<float, TypeIn>()(m);
        float y_ = ConvertSample<float, TypeIn>()(y);
        float k_ = ConvertSample<float, TypeIn>()(k);

        r = ConvertSample<TypeOut, float>()(c_ * k_);
        g = ConvertSample<TypeOut, float>()(m_ * k_);
        b = ConvertSample<TypeOut, float>()(y_ * k_);
    }
};

struct ConvertCMYK2RGB {
    //! \brief
    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn c, TypeIn m, TypeIn y, TypeIn k, TypeOut &r,
                    TypeOut &g, TypeOut &b) const {
        float K = (1.f - ConvertSample<float, TypeIn>()(k));

        r = ConvertSample<TypeOut, float>()(
            (1.f - ConvertSample<float, TypeIn>()(c)) * K);
        g = ConvertSample<TypeOut, float>()(
            (1.f - ConvertSample<float, TypeIn>()(m)) * K);
        b = ConvertSample<TypeOut, float>()(
            (1.f - ConvertSample<float, TypeIn>()(y)) * K);
    }
};

}  // colorspace
}  // pfs

#endif  // PFS_COLORSPACE_CMYK_H
