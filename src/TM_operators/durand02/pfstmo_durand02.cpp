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
 * $Id: pfstmo_durand02.cpp,v 1.4 2008/09/09 18:10:49 rafm Exp $
 */

#include "../tmo_config.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <QFile>
#include "tmo_durand02.h"

using namespace std;

pfs::Frame* pfstmo_durand02(pfs::Frame* inpfsframe, float _sigma_s, float _sigma_r, float _baseContrast) {
  pfs::DOMIO pfsio;

	float sigma_s=_sigma_s;
	float sigma_r=_sigma_r;
	float baseContrast=_baseContrast;
	int downsample=1;

	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );

        int w = Y->getCols();
        int h = Y->getRows();

	pfs::Frame *outframe = pfsio.createFrame( inpfsframe->getWidth(), inpfsframe->getHeight() );
	assert( outframe != NULL );
	pfs::Channel *Ro, *Go, *Bo;
	outframe->createRGBChannels( Ro, Go, Bo );
	assert( Ro!=NULL && Go!=NULL && Bo!=NULL );

	pfs::transformColorSpace( pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, Ro, Go, Bo );

// 	pfs::Channel *Ro, *Go, *Bo;
// 	outframe->createRGBChannels( Ro, Go, Bo );
// 	assert( Ro!=NULL && Go!=NULL && Bo!=NULL );
// 	pfs::copyArray(R,Ro);
// 	pfs::copyArray(G,Go);
// 	pfs::copyArray(B,Bo);
	
	tmo_durand02( w, h, X->getRawData(), Y->getRawData(), Z->getRawData(), sigma_s, sigma_r, baseContrast, downsample/*, progress_report*/);
	pfs::transformColorSpace( pfs::CS_RGB, Ro, Go, Bo, pfs::CS_SRGB, Ro, Go, Bo );
// 	pfs::transformColorSpace( pfs::CS_RGB, Ro, Go, Bo, pfs::CS_XYZ, Ro, Go, Bo );
	
	return outframe;
}
