/**
 * @brief PFS library - color space transformations
 *
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2010-2013 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net> (2010 10 13)
 *  Reimplementation of most of the functions (in particular the ones involving
 * RGB and XYZ)
 *
 * $Id: colorspace.h,v 1.6 2007/07/18 08:49:25 rafm Exp $
 */

#ifndef COLORSPACE_H
#define COLORSPACE_H

#include <Libpfs/array2d_fwd.h>
#include <Libpfs/exception.h>

namespace pfs {

//! This enum is used to specify color spaces for transformColorSpace function
enum ColorSpace {
    CS_XYZ = 0,  //!< Absolute XYZ space, reference white - D65, Y is calibrated
                 //! luminance in cd/m^2
    CS_RGB = 1,  //!< Absolute RGB space, reference white - D65
    CS_SRGB = 2,  //!< sRGB color space for LDR images (see
                  //!< www.srgb.com). The possible pixel values
                  //!< for R, G and B channel should be within
                  //!< range 0-1 (the values above or below this
                  //!< range will be clamped). Peak luminance
                  //!< level of the display is 80cd/m^2.
    CS_YUV = 3,   //!< Perceptually uniform u and v color coordinates, Y is
                  //! calibrated luminance in cd/m^2
    CS_Yxy =
        4  //!< Luminance and normalized chromacities (x=X/(X+Y+Z), y=Y/(X+Y+Z))
};

// XYZ -> *
void transformXYZ2SRGB(const Array2Df *inC1, const Array2Df *inC2,
                       const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                       Array2Df *outC3);
void transformXYZ2RGB(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);
void transformXYZ2Yxy(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);
void transformXYZ2Yuv(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);

// SRGB -> *
void transformSRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                       const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                       Array2Df *outC3);
void transformSRGB2Y(const Array2Df *inC1, const Array2Df *inC2,
                     const Array2Df *inC3, Array2Df *outY);

// RGB -> *
void transformRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);
void transformRGB2Y(const Array2Df *inC1, const Array2Df *inC2,
                    const Array2Df *inC3, Array2Df *outC1);
void transformRGB2Yuv(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);

// Yuv -> *
void transformYuv2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);
void transformYuv2RGB(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);

// Yxy -> *
void transformYxy2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3);

//! \brief Transform color channels from one color space into
//! another. Input and output channels may point to the same data
//! for in-memory transform.
//!
//! \param inCS input color space
//! \param inC1 first color channel of the input image
//! \param inC2 second color channel of the input image
//! \param inC3 third color channel of the input image
//! \param outCS output color space
//! \param outC1 first color channel of the output image
//! \param outC2 second color channel of the output image
//! \param outC3 third color channel of the output image
//!
void transformColorSpace(ColorSpace inCS, const Array2Df *inC1,
                         const Array2Df *inC2, const Array2Df *inC3,
                         ColorSpace outCS, Array2Df *outC1, Array2Df *outC2,
                         Array2Df *outC3);
}

#endif  // COLORSPACE_H
