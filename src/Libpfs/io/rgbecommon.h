/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003-2007 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
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

//! \brief Radiance RGBE file format reader common code
//! \author Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Giuseppe Rota  <grota@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Code adaptation for Luminance HDR and LibHDR

#ifndef PFS_IO_RGBECOMMON_H
#define PFS_IO_RGBECOMMON_H

//! \brief constant to change between radiance and luminance
#define WHITE_EFFICACY 179.0f

typedef unsigned char Trgbe;

//! \name RGB values and their exponent
struct Trgbe_pixel {
    Trgbe r;
    Trgbe g;
    Trgbe b;
    Trgbe e;
};

#endif  // PFS_IO_RGBECOMMON_H
