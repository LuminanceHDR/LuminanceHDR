/**
 * @file pfstmo_durand02.cpp
 * @brief Tone map XYZ channels using Durand02 model
 *
 * Fast Bilateral Filtering for the Display of High-Dynamic-Range Images.
 * F. Durand and J. Dorsey.
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
 * $Id: pfstmo_durand02.cpp,v 1.5 2005/12/15 15:53:37 krawczyk Exp $
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <QFile>
#include "../../Libpfs/array2d.h"
#include "tmo_durand02.h"

using namespace std;

pfs::Frame* pfstmo_durand02(pfs::Frame* inpfsframe, float _sigma_s, float _sigma_r, float _baseContrast) {
  pfs::DOMIO pfsio;

// #ifdef HAVE_FFTW
//   float sigma_s = 40.0f;
// #else
//   float sigma_s = 8.0f;
// #endif
//   float sigma_r = 0.4f;
//   float baseContrast = 5.0f;
	float sigma_s=_sigma_s;
	float sigma_r=_sigma_r;
	float baseContrast=_baseContrast;
	int downsample=1;

// #ifdef HAVE_FFTW
//  qDebug("fast bilateral filtering (fftw3)");
// #else
//  qDebug("conventional bilateral filtering");
// #endif

	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );
// 	pfs::Channel *R, *G, *B;
// 	inpfsframe->getRGBChannels( R, G, B );
// 	assert( R!=NULL && G!=NULL && B!=NULL );

	pfs::Frame *outframe = pfsio.createFrame( inpfsframe->getWidth(), inpfsframe->getHeight() );
	assert( outframe != NULL );
	pfs::Channel *Xo, *Yo, *Zo;
	outframe->createXYZChannels( Xo, Yo, Zo );
	assert( Xo!=NULL && Yo!=NULL && Zo!=NULL );
	pfs::transformColorSpace( pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, Xo, Yo, Zo );
// 	pfs::Channel *Ro, *Go, *Bo;
// 	outframe->createRGBChannels( Ro, Go, Bo );
// 	assert( Ro!=NULL && Go!=NULL && Bo!=NULL );
// 	pfs::copyArray(R,Ro);
// 	pfs::copyArray(G,Go);
// 	pfs::copyArray(B,Bo);
	
	tmo_durand02( Xo, Yo, Zo, sigma_s, sigma_r, baseContrast, downsample );
	pfs::transformColorSpace( pfs::CS_RGB, Xo, Yo, Zo, pfs::CS_XYZ, Xo, Yo, Zo );
	
	return outframe;
}
