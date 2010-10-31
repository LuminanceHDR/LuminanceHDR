/**
 * @brief Ashikhmin Tone Mapping Operator: tone reproduction for displaying high contrast scenes
 * Michael Ashikhmin 2002
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * @author Akiko Yoshida, <yoshida@mpi-sb.mpg.de>
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_ashikhmin02.cpp,v 1.5 2004/12/15 10:11:03 krawczyk Exp $
 */

#include <stdlib.h>
//#include <math.h>
#include <cmath>
#include <iostream>
#include <QFile>
#include <assert.h>

#include "Libpfs/colorspace.h"
#include "tmo_ashikhmin02.h"

using namespace std;

void calculateLuminance( pfs::Array2D* Y, float& avLum, float& maxLum, float& minLum);

void pfstmo_ashikhmin02(pfs::Frame* inpfsframe,  bool simple_flag, float lc_value, int eq, ProgressHelper *ph)
{
	assert(inpfsframe!=NULL);

	pfs::DOMIO pfsio;
	
	//--- default tone mapping parameters;

	cout << "pfstmo_ashikhmin02" << endl;
	cout << "simple: " << simple_flag << endl;
	cout << "lc_value: " << lc_value << endl;
	cout << "eq: " << eq << endl;

	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );

  pfs::Array2DImpl* Xr = X->getChannelData();
  pfs::Array2DImpl* Yr = Y->getChannelData();
  pfs::Array2DImpl* Zr = Z->getChannelData();
  
	pfs::transformColorSpace( pfs::CS_RGB, Xr, Yr, Zr, pfs::CS_XYZ, Xr, Yr, Zr );
	float maxLum, avLum, minLum;
	calculateLuminance( Yr, avLum, maxLum, minLum);
	
	int w = Yr->getCols();
	int h = Yr->getRows();
	
	pfs::Array2D* L = new pfs::Array2DImpl(w,h);
	tmo_ashikhmin02(Yr, L, maxLum, minLum, avLum, simple_flag, lc_value, eq, ph);

  // TODO: this section can be rewritten using SSE Function
	for( int x=0 ; x<w ; x++ )
  {
		for( int y=0 ; y<h ; y++ )
		{
			float scale = (*L)(x,y) / (*Yr)(x,y);
			(*Yr)(x,y) = (*Yr)(x,y) * scale;
			(*Xr)(x,y) = (*Xr)(x,y) * scale;
			(*Zr)(x,y) = (*Zr)(x,y) * scale;
		}
  }

	if (!ph->isTerminationRequested())
		ph->newValue( 100 );

	delete L;

	pfs::transformColorSpace(pfs::CS_XYZ, Xr, Yr, Zr, pfs::CS_RGB, Xr, Yr, Zr);
}

void calculateLuminance( pfs::Array2D* Y, float& avLum, float& maxLum, float& minLum)
{
  avLum = 0.0f;
  maxLum = 0.0f;
  minLum = 0.0f;

  int size = Y->getCols() * Y->getRows();

  for( int i=0 ; i<size; i++ )
  {
    avLum += log( (*Y)(i) + 1e-4 );
    maxLum = ( (*Y)(i) > maxLum ) ? (*Y)(i) : maxLum ;
    minLum = ( (*Y)(i) < minLum ) ? (*Y)(i) : minLum ;
  }
  avLum =exp( avLum/ size);
}

