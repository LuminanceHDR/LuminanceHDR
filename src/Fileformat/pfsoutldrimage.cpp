/**
 * @brief Writing QImage from PFS stream (which is a tonemapped LDR)
 *
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 */

#include <QImage>
#include <QSysInfo>

#include "Libpfs/pfs.h"

static inline unsigned char clamp( const float v, const unsigned char minV, const unsigned char maxV )
{
    if( v < /*(float)*/minV ) return minV;
    if( v > /*(float)*/maxV ) return maxV;
    return (unsigned char)v;
}

QImage fromLDRPFStoQImage( pfs::Frame* inpfsframe ) {
	assert(inpfsframe!=NULL);

	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels( X, Y, Z );
	assert( X!=NULL && Y!=NULL && Z!=NULL );
	
	// Back to CS_RGB for the Viewer
	pfs::transformColorSpace( pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, X, Y, Z );	
	
	int width = X->getCols();
	int height =  X->getRows();
	uchar *data=new uchar[width*height*4]; //this will contain the image data: data must be 32-bit aligned, in Format: 0xffRRGGBB
	for( int y = 0; y < height; y++ ) { // For each row of the image
		for( int x = 0; x < width; x++ ) {
			if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
			*(data + 0 + (y*width+x)*4) = ( clamp( (*Z)( x, y )*255.f, 0, 255) );
			*(data + 1 + (y*width+x)*4) = ( clamp( (*Y)( x, y )*255.f, 0, 255) );
			*(data + 2 + (y*width+x)*4) = ( clamp( (*X)( x, y )*255.f, 0, 255) );
			*(data + 3 + (y*width+x)*4) = 0xff;
			} else {
			*(data + 3 + (y*width+x)*4) = ( clamp( (*X)( x, y )*255.f, 0, 255) );
			*(data + 2 + (y*width+x)*4) = ( clamp( (*Y)( x, y )*255.f, 0, 255) );
			*(data + 1 + (y*width+x)*4) = ( clamp( (*Z)( x, y )*255.f, 0, 255) );
			*(data + 0 + (y*width+x)*4) = 0xff;
			}
		}
	}

	return QImage (const_cast<uchar *>(data),width,height,QImage::Format_ARGB32);
}
