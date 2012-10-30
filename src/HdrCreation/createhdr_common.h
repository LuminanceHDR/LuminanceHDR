/*
 * This file is a part of Luminance HDR package
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

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \brief Common declarations/functions for the HDR creation routines

#ifndef CREATEHDR_COMMON_H
#define CREATEHDR_COMMON_H

#include <vector>
#include <Libpfs/array2d.h>

///! \brief Container for images taken with different exposures
//struct Exposure
//{
//    float ti;             // exposure value (eg time)
//    pfs::Array2D* yi;     // exposed pixel value (camera output)
//};

///! @brief Container for a list of exposures
// typedef std::vector<Exposure> ExposureList;
typedef std::vector<pfs::Array2Df*> Array2DfList;

//! \brief Given a weight function in the vector \c w, computes the minimum
//! and the maximum index for the trusted range
void computeTrustRange(const float* w, int M, int& minM, int& maxM);

#endif // CREATEHDR_COMMON_H
