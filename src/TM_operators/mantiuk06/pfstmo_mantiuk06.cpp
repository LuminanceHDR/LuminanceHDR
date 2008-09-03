/**
 * @brief Contrast mapping TMO
 *
 *
 * Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
 * A Perceptual Framework for Contrast Processing of High Dynamic Range Images
 * In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
 * http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
 *
 * This file is a part of Qtpfsgui package (reworked from the PFSTMO package).
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * @author Radoslaw Mantiuk, <radoslaw.mantiuk@gmail.com>
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 * Updated 2008/07/26 by Dejan Beric <dejan.beric@dmsgroup.co.yu>
 *  Added the detail Factor slider which offers more control over contrast in details
 *
 * $Id: pfstmo_mantiuk06.cpp,v 1.7 2008/02/29 16:46:28 rafm Exp $
 */

#include "../../Libpfs/pfs.h"
#include "contrast_domain.h"
#include <math.h>
void progress_report( int progress )
{
    fprintf( stderr, "\rcompleted %d%%", progress );
    if( progress == 100 )
      fprintf( stderr, "\n" );
}

pfs::Frame* pfstmo_mantiuk06(pfs::Frame* inpfsframe, float scalefactor, float saturationfactor,float detailfactor, bool constrast_equalization ) {

	assert(inpfsframe!=NULL);
	//get tone mapping parameters;
	float scaleFactor = scalefactor;
	float saturationFactor = saturationfactor;
	//Dejan Beric'c contribution:
	float detailFactor = detailfactor;
	if (constrast_equalization)
		scaleFactor=-scaleFactor;
	pfs::DOMIO pfsio;
	//Ed Brambley's contribution:
	const bool bcg = false; //use biconjucate gradients?
	const int itmax = 200;
	const float tol = 1e-3;

	pfs::Channel *inX, *inY, *inZ;
	inpfsframe->getXYZChannels(inX, inY, inZ);
	int cols = inpfsframe->getWidth();
	int rows = inpfsframe->getHeight();

	pfs::Frame *outframe = pfsio.createFrame( cols, rows );
	assert(outframe!=NULL);
	pfs::Channel *Ro, *Go, *Bo;
	outframe->createRGBChannels( Ro, Go, Bo );
	assert( Ro!=NULL && Go!=NULL && Bo!=NULL );

// 	pfs::Array2DImpl *R=new pfs::Array2DImpl( cols, rows );
	pfs::transformColorSpace( pfs::CS_XYZ, inX, inY, inZ, pfs::CS_RGB, Ro, Go, Bo );
	tmo_mantiuk06_contmap( cols, rows, Ro->getRawData(), Go->getRawData(), Bo->getRawData(),
	inY->getRawData(), scaleFactor, saturationFactor, detailFactor, bcg, itmax, tol, &progress_report );
	pfs::transformColorSpace( pfs::CS_RGB, Ro, Go, Bo, pfs::CS_SRGB, Ro, Go, Bo );

	outframe->getTags()->setString("LUMINANCE", "RELATIVE");

// 	delete R;
	return outframe;
}


