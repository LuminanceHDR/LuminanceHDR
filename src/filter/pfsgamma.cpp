/**
 * @brief Apply gamma correction the the pfs stream
 * 
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003-2005 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
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
 *
 * $Id: pfsgamma.cpp,v 1.1 2005/06/15 13:36:54 rafm Exp $
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pfs.h>
#include <QFile>

void applyGamma( pfs::Array2D *arrayto, pfs::Array2D *arrayfrom, const float exponent/*, const float multiplier*/ );

pfs::Frame* applyGammaFrame( pfs::Frame* inpfsframe, float _gamma, bool inputXYZ) {

	assert(inpfsframe!=NULL);
	pfs::DOMIO pfsio;
	float gamma = _gamma;

	pfs::Channel *R, *G, *B;
	if (inputXYZ)
		inpfsframe->getXYZChannels( R, G, B );
	else
		inpfsframe->getRGBChannels( R, G, B );
	assert( R!=NULL && G!=NULL && B!=NULL );

	pfs::Frame *outframe = pfsio.createFrame( inpfsframe->getWidth(), inpfsframe->getHeight() );
	assert(outframe != NULL);
	pfs::Channel  *X, *Y, *Z;
	outframe->createXYZChannels( X,Y,Z );
	assert( X!=NULL && Y!=NULL && Z!=NULL );

	if (gamma==1) {
		if (inputXYZ) {
			copyArray(R,X);
			copyArray(G,Y);
			copyArray(B,Z);
		} else {
			pfs::transformColorSpace( pfs::CS_RGB, R, G, B, pfs::CS_XYZ, X,Y,Z );
		}
	} else {
		if (inputXYZ)
			pfs::transformColorSpace( pfs::CS_XYZ, R, G, B, pfs::CS_RGB, R, G, B );
		applyGamma( R,X, 1/gamma/*, multiplier*/ );
		applyGamma( G,Y, 1/gamma/*, multiplier*/ );
		applyGamma( B,Z, 1/gamma/*, multiplier*/ );
		pfs::transformColorSpace( pfs::CS_RGB, X,Y,Z, pfs::CS_XYZ, X,Y,Z );
	}
	inpfsframe->getTags()->setString("LUMINANCE", "DISPLAY");
	return outframe;
}

void applyGamma( pfs::Array2D *arrayfrom, pfs::Array2D *to, const float exponent/*, const float multiplier*/ )
{
	int imgSize = arrayfrom->getRows()*arrayfrom->getCols();
	for( int index = 0; index < imgSize ; index++ ) {
		float &v_in = (*arrayfrom)(index);
		float &v_out = (*to)(index);
		if( v_in < 0 ) v_out = 0;
		v_out = powf( v_in/**multiplier*/, exponent );
	}
}
