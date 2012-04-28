/**
 * @brief Resize images in PFS stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk,
 *  Alexander Efremov
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
 * @author Alexander Efremov, <aefremov@mpi-sb.mpg.de>
 *
 * $Id: pfsrotate.h,v 1.1 2005/06/15 13:36:54 rafm Exp $
 */

#ifndef PFSROTATE_H
#define PFSROTATE_H

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"

namespace pfs
{
  void rotateArray(const pfs::Array2D *in, pfs::Array2D *out, bool clockwise);
  pfs::Frame* rotateFrame(pfs::Frame* frame, bool clock_wise);
}

#endif // PFSROTATE_H
