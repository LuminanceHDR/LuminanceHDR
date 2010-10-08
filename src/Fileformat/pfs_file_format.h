/**
 * @brief Header file for pfs file format IO
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006 Davide Anastasia
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
 * @author Davide Anastasia <davide.anastasia@gmail.com>
 *
 */

#ifndef __PFS_FILE_FORMAT_H__
#define __PFS_FILE_FORMAT_H__

#include <QImage>
#include <QSysInfo>

#include "Libpfs/pfs.h"

#include "rgbeio.h"
#include "pfstiff.h"
#include "pfsoutldrimage.h"

pfs::Frame* readEXRfile  (const char * filename);
void writeEXRfile (pfs::Frame* inpfsframe,const char* outfilename);

pfs::Frame* readRGBEfile (const char * filename);
void writeRGBEfile(pfs::Frame* inputpfshdr, const char* outfilename);

#endif