/**
 * @brief Apply gamma correction the the pfs stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * $Id: pfsgamma.h,v 1.3 2008/07/29 16:14:29 rafm Exp $
 */

#ifndef PFSGAMMA_H
#define PFSGAMMA_H

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"

namespace pfs
{
    // Note: passing basic types by "const" copy is completely useless,
    // and shows a wrong design either [Davide: 2012.04.28]
    void applyGamma(pfs::Array2D *array,
                    const float exponent,
                    const float multiplier);

    pfs::Frame* applyGammaOnFrame(pfs::Frame* frame,
                                  const float gamma);
}

#endif // PFSGAMMA_H
