/*
* This file is a part of Luminance HDR package.
* ----------------------------------------------------------------------
* Copyright (C) 2012 Davide Anastasia
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
*/

#ifndef VEX_MINMAX_H
#define VEX_MINMAX_H

#include <cstddef>

// NOTE: the name vex has to change in the future...
namespace vex {

template <typename _Type>
_Type minElement(const _Type* data, size_t size);

template <typename _Type>
_Type maxElement(const _Type* data, size_t size);

//! \brief computes the maximum and the minumum between 3 samples using the
//! least amount of compares
template <typename Type>
void minmax(Type i1, Type i2, Type i3, Type& min, Type& max);

}   // vex

#include "minmax.hxx"
#endif // VEX_MINMAX_H
