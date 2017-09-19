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

//! \brief LCMS2 wrappers for utils::transfor
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_COLORSPACE_LCMS_H
#define PFS_COLORSPACE_LCMS_H

#include <Libpfs/colorspace/convert.h>
#include <lcms2.h>

namespace pfs {
namespace colorspace {

//! \brief Functor 4 -> 3 conversion based on LCMS
struct Convert4LCMS3 {
    Convert4LCMS3(cmsHTRANSFORM transform) : transform_(transform) {}

    //! \brief
    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn i1, TypeIn i2, TypeIn i3, TypeIn i4, TypeOut &o1,
                    TypeOut &o2, TypeOut &o3) const {
        TypeIn inBuffer[4] = {i1, i2, i3, i4};
        TypeOut outBuffer[3];

        cmsDoTransform(transform_, &inBuffer[0], &outBuffer[0], 1);

        o1 = outBuffer[0];
        o2 = outBuffer[1];
        o3 = outBuffer[2];
    }

   private:
    cmsHTRANSFORM transform_;
};

//! \brief Functor 4 -> 3 conversion based on LCMS
struct Convert3LCMS3 {
    Convert3LCMS3(cmsHTRANSFORM transform) : transform_(transform) {}

    //! \brief
    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn i1, TypeIn i2, TypeIn i3, TypeOut &o1, TypeOut &o2,
                    TypeOut &o3) const {
        TypeIn inBuffer[3] = {i1, i2, i3};
        TypeOut outBuffer[3];

        cmsDoTransform(transform_, &inBuffer[0], &outBuffer[0], 1);

        o1 = outBuffer[0];
        o2 = outBuffer[1];
        o3 = outBuffer[2];
    }

   private:
    cmsHTRANSFORM transform_;
};

}  // colorspace
}  // pfs

#endif  // PFS_COLORSPACE_CMYK_H
