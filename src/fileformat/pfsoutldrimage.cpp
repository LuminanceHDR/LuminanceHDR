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

#include <iostream>
#include "../libpfs/pfs.h"
#include <QImage>

#ifndef _WIN32
#if defined(__MACH__) && defined(__APPLE__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif
#endif

static inline unsigned char clamp( const float v, const unsigned char minV, const unsigned char maxV )
{
    if( v < /*(float)*/minV ) return minV;
    if( v > /*(float)*/maxV ) return maxV;
    return (unsigned char)v;
}


QImage* fromLDRPFStoQImage( pfs::Frame* inpfsframe ) {
	assert(inpfsframe!=NULL);

// 	if(data != NULL)
// 		delete [] data;

	pfs::DOMIO pfsio;
	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels( X,Y,Z );
	assert( X!=NULL && Y!=NULL && Z!=NULL );

	//we are modifying the input buffer here!!!
	//but it should be ok since this is the endpoint
	pfs::transformColorSpace( pfs::CS_XYZ, X,Y,Z, pfs::CS_RGB, X,Y,Z );

	int width = X->getCols();
	int height =  X->getRows();
	uchar *data=new uchar[width*height*4]; //this will contain the image data: data must be 32-bit aligned, in Format: 0xffRRGGBB
	for( int y = 0; y < height; y++ ) { // For each row of the image
		for( int x = 0; x < width; x++ ) {
// 			if ((*X)(x,y)>1.f||(*X)(x,y)<-1e-6||(*Y)(x,y)>1.f||(*Y)(x,y)<-1e-6||(*Z)(x,y)>1.f||(*Z)(x,y)<-1e-6)
// 			  qDebug("out of bounds, X=%f Y=%f, Z=%f",(*X)(x,y),(*Y)(x,y),(*Z)(x,y));
#if ( (BYTE_ORDER == LITTLE_ENDIAN) || (defined(_WIN32)) )
			*(data + 0 + (y*width+x)*4) = ( clamp( (*Z)( x, y )*255.f, 0, 255) );
			*(data + 1 + (y*width+x)*4) = ( clamp( (*Y)( x, y )*255.f, 0, 255) );
			*(data + 2 + (y*width+x)*4) = ( clamp( (*X)( x, y )*255.f, 0, 255) );
			*(data + 3 + (y*width+x)*4) = 0xff;
#elif ( BYTE_ORDER == BIG_ENDIAN )
			*(data + 3 + (y*width+x)*4) = ( clamp( (*Z)( x, y )*255.f, 0, 255) );
			*(data + 2 + (y*width+x)*4) = ( clamp( (*Y)( x, y )*255.f, 0, 255) );
			*(data + 1 + (y*width+x)*4) = ( clamp( (*X)( x, y )*255.f, 0, 255) );
			*(data + 0 + (y*width+x)*4) = 0xff;
#endif
		}
	}
#if QT_VERSION <= 0x040200
	QImage *toreturn=new QImage(data,width,height,QImage::Format_RGB32);
#else
//not const uchar*! will it crash on old qt distributions?
	QImage *toreturn=new QImage(const_cast<uchar *>(data),width,height,QImage::Format_RGB32);
#endif
	return toreturn;
}
