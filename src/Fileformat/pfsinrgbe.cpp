/**
 * @brief load an radiance rgbe file
 * 
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota  <grota@users.sourceforge.net>
 */

#include <stdlib.h>

#include "rgbeio.h"
#include "Libpfs/frame.h"

pfs::Frame* readRGBEfile (const char * filename)
{
    FILE *inputRGBEfile = fopen(filename,"rb");
	RGBEReader reader( inputRGBEfile );
    pfs::Frame *frame = new pfs::Frame( reader.getWidth(), reader.getHeight() );
	pfs::Channel  *X, *Y, *Z;
	frame->createXYZChannels( X, Y, Z);
    reader.readImage(X, Y, Z);

    frame->getTags().setTag("LUMINANCE", "RELATIVE");
    frame->getTags().setTag("FILE_NAME", filename);

    fclose(inputRGBEfile);
	return frame;
}
