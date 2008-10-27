/**
 * @file pfstmo_fattal02.cpp
 * @brief Tone map XYZ channels using Fattal02 model
 *
 * Gradient Domain High Dynamic Range Compression
 * R. Fattal, D. Lischinski, and M. Werman
 * In ACM Transactions on Graphics, 2002.
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
 * $Id: pfstmo_fattal02.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <QFile>
#include "../../Libpfs/pfs.h"
#include "tmo_fattal02.h"

using namespace std;


pfs::Frame* pfstmo_fattal02(pfs::Frame* inpfsframe, float _opt_alfa,float _opt_beta,float _opt_saturation, float _opt_noise, bool newfattal) {
	assert(inpfsframe!=NULL);

	pfs::DOMIO pfsio;
	//--- get tone mapping parameters;
	float opt_alfa = _opt_alfa;
	float opt_beta = _opt_beta;
	float opt_saturation=_opt_saturation;
	float opt_noise=_opt_noise;

	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );

	pfs::Frame *outframe = pfsio.createFrame( inpfsframe->getWidth(), inpfsframe->getHeight() );
	assert(outframe != NULL);
	pfs::Channel *Ro, *Go, *Bo;
	outframe->createRGBChannels( Ro, Go, Bo );
	assert( Ro!=NULL && Go!=NULL && Bo!=NULL );

	// tone mapping
	int w = Y->getCols();
	int h = Y->getRows();

	pfs::Array2DImpl* L = new pfs::Array2DImpl(w,h);
	tmo_fattal02(w, h, Y->getRawData(), L->getRawData(), opt_alfa, opt_beta, opt_noise, newfattal);

	pfs::transformColorSpace( pfs::CS_XYZ, X,Y,Z, pfs::CS_RGB, Ro,Go,Bo );
	for( int x=0 ; x<w ; x++ )
		for( int y=0 ; y<h ; y++ )
		{
			(*Ro)(x,y) = powf( (*Ro)(x,y)/(*Y)(x,y), opt_saturation ) * (*L)(x,y);
			(*Go)(x,y) = powf( (*Go)(x,y)/(*Y)(x,y), opt_saturation ) * (*L)(x,y);
			(*Bo)(x,y) = powf( (*Bo)(x,y)/(*Y)(x,y), opt_saturation ) * (*L)(x,y);
		}
	delete L;

	return outframe;
}
