/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 *  Copyright (C) 2007 Nicholas Phillips
 *  Copyright (C) 2013 Davide Anastasia
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
 */

//! \brief Implementation of the MTB algorithm for image alignment
//! \author Nicholas Phillips <ngphillips@gmail.com>
//! \author Giuseppe Rota (small modifications for Qt4)
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Remove dependency from Qt and refactoring for new libhdr library

#include <vector>

#include <Libpfs/array2d_fwd.h>
#include <Libpfs/frame.h>

namespace libhdr {

void mtb_alignment(std::vector<pfs::FramePtr> &framePtrList);

}  // libhdr
