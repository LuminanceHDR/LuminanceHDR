/**
 * @brief Contrast mapping TMO
 *
 * From:
 *
 * Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
 * A Perceptual Framework for Contrast Processing of High Dynamic Range Images
 * In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
 * http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 *
 * @author Radoslaw Mantiuk, <radoslaw.mantiuk@gmail.com>
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 *
 * $Id: contrast_domain.h,v 1.7 2008/06/16 22:17:47 rafm Exp $
 */

#ifndef CONTRAST_DOMAIN_H
#define CONTRAST_DOMAIN_H

#include <Libpfs/array2d_fwd.h>
#include "TonemappingOperators/pfstmo.h"

//! \brief: Tone mapping algorithm [Mantiuk2006]
//!
//! \param R red channel
//! \param G green channel
//! \param B blue channel
//! \param Y luminance channel
//! \param contrastFactor contrast scaling factor (in 0-1 range)
//! \param saturationFactor color desaturation (in 0-1 range)
//! \param itmax maximum number of iterations for convergence (typically 50)
//! \param tol tolerence to get within for convergence (typically 1e-3)
//! \param ph callback class that reports progress
//! \return PFSTMO_OK if tone-mapping was sucessful, PFSTMO_ABORTED if
//! it was stopped from a callback function and PFSTMO_ERROR if an
//! error was encountered.
//!
int tmo_mantiuk06_contmap(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                          pfs::Array2Df &Y, float contrastFactor,
                          float saturationFactor, float detailFactor,
                          int itmax /*= 200*/, float tol /*= 1e-3*/,
                          pfs::Progress &ph);

#endif
