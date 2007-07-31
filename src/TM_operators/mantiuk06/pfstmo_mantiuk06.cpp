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
 *
 * $Id: pfstmo_mantiuk06.cpp,v 1.5 2007/06/16 19:23:08 rafm Exp $
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

pfs::Frame* pfstmo_mantiuk06(pfs::Frame* inpfsframe, float contrastscalefactor, float saturationfactor, bool constrast_equalization ) {

	assert(inpfsframe!=NULL);
	//get tone mapping parameters;
	float scaleFactor = contrastscalefactor;
	float saturationFactor = saturationfactor;
	if (constrast_equalization)
		scaleFactor=0;
	pfs::DOMIO pfsio;

	pfs::Channel *inX, *inY, *inZ;
	inpfsframe->getXYZChannels(inX, inY, inZ);
	int cols = inpfsframe->getWidth();
	int rows = inpfsframe->getHeight();

	pfs::Frame *outframe = pfsio.createFrame( cols, rows );
	assert(outframe!=NULL);
	pfs::Channel *Xo, *Yo, *Zo;
	outframe->createXYZChannels( Xo, Yo, Zo );
	assert( Xo!=NULL && Yo!=NULL && Zo!=NULL );

// 	pfs::Array2DImpl *R=new pfs::Array2DImpl( cols, rows );
	pfs::transformColorSpace( pfs::CS_XYZ, inX, inY, inZ, pfs::CS_RGB, Xo, Yo, Zo );
	tmo_mantiuk06_contmap( cols, rows, Xo->getRawData(), Yo->getRawData(), Zo->getRawData(),
	inY->getRawData(), scaleFactor, saturationFactor, progress_report );
	pfs::transformColorSpace( pfs::CS_RGB, Xo, inY, Zo, pfs::CS_XYZ, Xo, Yo, Zo );
	//original implementation: we can avoid creating an additional data channel. We use one of the 3 that we already have to create for the output.
// 	pfs::Array2DImpl *R=new pfs::Array2DImpl( cols, rows );
// 	pfs::transformColorSpace( pfs::CS_XYZ, inX, inY, inZ, pfs::CS_RGB, inX, R, inZ );
// 	tmo_mantiuk06_contmap( cols, rows, inX->getRawData(), R->getRawData(), inZ->getRawData(),
// 	inY->getRawData(), scaleFactor, saturationFactor, progress_report );
// 	pfs::transformColorSpace( pfs::CS_RGB, inX, inY, inZ, pfs::CS_XYZ, Xo, Yo, Zo );
	
	outframe->getTags()->setString("LUMINANCE", "RELATIVE");

// 	delete R;
	return outframe;
}


