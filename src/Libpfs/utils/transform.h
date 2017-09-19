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

//! \brief colorspace conversion base functions
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_COLORSPACE_TRANSFORM_H
#define PFS_COLORSPACE_TRANSFORM_H

namespace pfs {
namespace utils {

//! \brief 3 components to 3 components transform function
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out1, OutputIterator out2,
               OutputIterator out3, ConversionOperator convOp);

//! \brief 4 components to 3 components transform function
//! useful for CMYK to RGB conversion (or RGBA to RGB)
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, InputIterator in4, OutputIterator out1,
               OutputIterator out2, OutputIterator out3,
               ConversionOperator convOp);

//! \brief 3 components to 1 component transform function
//! useful for channel stripping or RGB to Y conversion
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out1,
               ConversionOperator convOp);

}  // utils
}  // pfs

#include <Libpfs/utils/transform.hxx>
#endif  //  PFS_COLORSPACE_TRANSFORM_H
