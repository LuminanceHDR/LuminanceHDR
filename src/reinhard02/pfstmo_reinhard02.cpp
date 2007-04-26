/**
 * @file pfstmo_reinhard02.cpp
 * @brief Tone map XYZ channels using Reinhard02 model
 *
 * Photographic Tone Reproduction for Digital Images.
 * E. Reinhard, M. Stark, P. Shirley, and J. Ferwerda.
 * In ACM Transactions on Graphics, 2002.
 *
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003-2007 Grzegorz Krawczyk
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <QFile>
#include <math.h>
#include "../libpfs/pfs.h"
#include "tmo_reinhard02.h"

// using namespace std;

pfs::Frame* pfstmo_reinhard02 (pfs::Frame* inpfsframe, float _key, float _phi, int _num, int _low, int _high, bool _use_scales ) {
	assert(inpfsframe!=NULL);

	pfs::DOMIO pfsio;
	//--- default tone mapping parameters;
	float key = _key;
	float phi = _phi;
	int num = _num;
	int low = _low;
	int high = _high;
	bool use_scales = _use_scales;
	bool temporal_coherent=false;

	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );

	pfs::Frame *outframe = pfsio.createFrame( inpfsframe->getWidth(), inpfsframe->getHeight() );
	assert(outframe != NULL);
	pfs::Channel *Xo, *Yo, *Zo;
	outframe->createXYZChannels( Xo, Yo, Zo );
	assert( Xo!=NULL && Yo!=NULL && Zo!=NULL );\

	// tone mapping
	int w = Y->getCols();
	int h = Y->getRows();
	pfs::Array2D* L = new pfs::Array2DImpl(w,h);
	assert(L!=NULL);

	tmo_reinhard02( Y, L, use_scales, key, phi, num, low, high, temporal_coherent );
	for( int x=0 ; x<w ; x++ ) {
		for( int y=0 ; y<h ; y++ )
		{
			float scale = (*L)(x,y) / (*Y)(x,y);
			(*Yo)(x,y) = (*Y)(x,y)*scale;
			(*Xo)(x,y) = (*X)(x,y)*scale;
			(*Zo)(x,y) = (*Z)(x,y)*scale;
		}
	}
// 	pfs::transformColorSpace( pfs::CS_XYZ, Ro, Go, Bo, pfs::CS_RGB, Ro, Go, Bo );

	delete L;

	return outframe;
}
