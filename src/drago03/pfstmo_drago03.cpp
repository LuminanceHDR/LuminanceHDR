/**
 * @brief Adaptive logarithmic tone mapping
 * 
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes. 
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
 *
 * This file is a part of Qtpfsgui package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_drago03.cpp,v 1.1 2004/09/22 10:00:23 krawczyk Exp $
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <pfs.h>
#include <QFile>
#include <tmo_drago03.h>

void calculateLuminance( pfs::Array2D* Y, float& avLum, float& maxLum );

pfs::Frame* pfstmo_drago03(pfs::Frame *inputpfsframe, float _biasValue) {
	assert(inputpfsframe!=NULL);
	pfs::DOMIO pfsio;

	//--- default tone mapping parameters;
	float biasValue = _biasValue;

// 	pfs::Channel *R, *G, *B;
// 	inputpfsframe->getRGBChannels( R, G, B );
// 	assert( R!=NULL && G!=NULL && B!=NULL );
	pfs::Channel *X, *Y, *Z;
	inputpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );

	pfs::Frame *outframe = pfsio.createFrame( inputpfsframe->getWidth(), inputpfsframe->getHeight() );
	assert(outframe != NULL);
	pfs::Channel *Xo, *Yo, *Zo;
	outframe->createXYZChannels( Xo, Yo, Zo );
	assert( Xo!=NULL && Yo!=NULL && Zo!=NULL );
// 	pfs::Channel *Ro, *Go, *Bo;
// 	outframe->createRGBChannels( Ro, Go, Bo );
// 	assert( Ro!=NULL && Go!=NULL && Bo!=NULL );

// 	pfs::transformColorSpace( pfs::CS_RGB, R, G, B, pfs::CS_XYZ, Ro, Go, Bo );

	float maxLum,avLum;
	calculateLuminance( Y, avLum, maxLum );
	
	int w = Y->getCols();
	int h = Y->getRows();

	pfs::Array2D* L = new pfs::Array2DImpl(w,h);
	tmo_drago03(Y, L, maxLum, avLum, biasValue);

	for( int x=0 ; x<w ; x++ )
		for( int y=0 ; y<h ; y++ ) 
		{
			float scale = (*L)(x,y) / (*Y)(x,y);
			(*Yo)(x,y) = (*Y)(x,y)*scale;
			(*Xo)(x,y) = (*X)(x,y)*scale;
			(*Zo)(x,y) = (*Z)(x,y)*scale;
		}

	delete L;
// 	pfs::transformColorSpace( pfs::CS_XYZ, Ro, Go, Bo, pfs::CS_RGB, Ro, Go, Bo );

	return outframe;
}

void calculateLuminance( pfs::Array2D* Y, float& avLum, float& maxLum )
{
  avLum = 0.0f;
  maxLum = 0.0f;

  int size = Y->getCols() * Y->getRows();

  for( int i=0 ; i<size; i++ )
  {
    avLum += log( (*Y)(i) + 1e-4 );
    maxLum = ( (*Y)(i) > maxLum ) ? (*Y)(i) : maxLum ;
  }
  avLum =exp( avLum/ size);
}
