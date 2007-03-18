/**
 * @brief load an radiance rgbe file
 * 
 * This file is a part of Qtpfsgui package.
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

#include "rgbeio.h"
#include "../libpfs/pfs.h"
#include <stdlib.h>

pfs::Frame* readRGBEfile (const char * filename) {
	pfs::DOMIO pfsio;
	FILE *inputRGBEfile=fopen(filename,"rb");
	RGBEReader reader( inputRGBEfile );
	pfs::Frame *frame = pfsio.createFrame( reader.getWidth(), reader.getHeight() );
	pfs::Channel  *X, *Y, *Z;
	frame->createRGBChannels( X,Y,Z );
// 	frame->createXYZChannels( X,Y,Z );
	reader.readImage( X,Y,Z );
// 	pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );
	frame->getTags()->setString("LUMINANCE", "RELATIVE");
	frame->getTags()->setString( "FILE_NAME", filename );
	return frame;
}
