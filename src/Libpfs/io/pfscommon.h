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

//! \brief PFS file format common (used for compatibility with PFSTOOLS)
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_IO_PFSCOMMON_H
#define PFS_IO_PFSCOMMON_H

#define PFSEOL "\x0a"
#define PFSEOLCH '\x0a'

#define MAX_RES 65535
#define MAX_CHANNEL_NAME 32
#define MAX_TAG_STRING 1024
#define MAX_CHANNEL_COUNT 1024

#endif  // PFS_IO_PFSCOMMON_H
