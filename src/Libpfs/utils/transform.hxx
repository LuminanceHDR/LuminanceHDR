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

#ifndef PFS_COLORSPACE_TRANSFORM_HXX
#define PFS_COLORSPACE_TRANSFORM_HXX

#include <Libpfs/utils/transform.h>
#include <algorithm>
#include <cassert>
#include <iterator>

namespace pfs {
namespace utils {

namespace detail {

// transform for generic iterators
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator, typename InputIteratorTag,
          typename OutputIteratorTag>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out1, OutputIterator out2,
               OutputIterator out3, ConversionOperator convOp, InputIteratorTag,
               OutputIteratorTag) {
    while (in1 != in1End) {
        convOp(*in1++, *in2++, *in3++, *out1++, *out2++, *out3++);
    }
}

// transform for random_access_iterator_tag, so we can use OpenMP (optimized)
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out1, OutputIterator out2,
               OutputIterator out3, ConversionOperator convOp,
               std::random_access_iterator_tag,
               std::random_access_iterator_tag) {
    typename std::iterator_traits<InputIterator>::difference_type numElem =
        (in1End - in1);
#pragma omp parallel for
    for (int idx = 0; idx < numElem; ++idx) {
        convOp(in1[idx], in2[idx], in3[idx], out1[idx], out2[idx], out3[idx]);
    }
}

}  // detail

template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out1, OutputIterator out2,
               OutputIterator out3, ConversionOperator convOp) {
    // dispatch to best implementation!
    detail::transform(
        in1, in1End, in2, in3, out1, out2, out3, convOp,
        typename std::iterator_traits<InputIterator>::iterator_category(),
        typename std::iterator_traits<OutputIterator>::iterator_category());
}

namespace detail {

// transform for generic iterators
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator, typename InputIteratorTag,
          typename OutputIteratorTag>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, InputIterator in4, OutputIterator out1,
               OutputIterator out2, OutputIterator out3,
               ConversionOperator convOp, InputIteratorTag, OutputIteratorTag) {
    while (in1 != in1End) {
        convOp(*in1++, *in2++, *in3++, *in4++, *out1++, *out2++, *out3++);
    }
}

// transform for random_access_iterator_tag, so we can use OpenMP (optimized)
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, InputIterator in4, OutputIterator out1,
               OutputIterator out2, OutputIterator out3,
               ConversionOperator convOp, std::random_access_iterator_tag,
               std::random_access_iterator_tag) {
    typename std::iterator_traits<InputIterator>::difference_type numElem =
        (in1End - in1);
#pragma omp parallel for
    for (int idx = 0; idx < numElem; ++idx) {
        convOp(in1[idx], in2[idx], in3[idx], in4[idx], out1[idx], out2[idx],
               out3[idx]);
    }
}

}  // detail

template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, InputIterator in4, OutputIterator out1,
               OutputIterator out2, OutputIterator out3,
               ConversionOperator convOp) {
    // dispatch to best implementation!
    detail::transform(
        in1, in1End, in2, in3, in4, out1, out2, out3, convOp,
        typename std::iterator_traits<InputIterator>::iterator_category(),
        typename std::iterator_traits<OutputIterator>::iterator_category());
}

namespace detail {

// transform for generic iterators
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator, typename InputIteratorTag,
          typename OutputIteratorTag>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out, ConversionOperator convOp,
               InputIteratorTag, OutputIteratorTag) {
    while (in1 != in1End) {
        convOp(*in1++, *in2++, *in3++, *out++);
    }
}

// transform for random_access_iterator_tag, so we can use OpenMP (optimized)
template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out, ConversionOperator convOp,
               std::random_access_iterator_tag,
               std::random_access_iterator_tag) {
    typename std::iterator_traits<InputIterator>::difference_type numElem =
        (in1End - in1);
#pragma omp parallel for
    for (int idx = 0; idx < numElem; ++idx) {
        convOp(in1[idx], in2[idx], in3[idx], out[idx]);
    }
}

}  // detail

template <typename InputIterator, typename OutputIterator,
          typename ConversionOperator>
void transform(InputIterator in1, InputIterator in1End, InputIterator in2,
               InputIterator in3, OutputIterator out,
               ConversionOperator convOp) {
    // dispatch to best implementation!
    detail::transform(
        in1, in1End, in2, in3, out, convOp,
        typename std::iterator_traits<InputIterator>::iterator_category(),
        typename std::iterator_traits<OutputIterator>::iterator_category());
}

}  // utils
}  // pfs

#endif  //  PFS_COLORSPACE_TRANSFORM_HXX
