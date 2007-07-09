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

#include <cmath>
#include "../Libpfs/pfs.h"

void applyGamma( pfs::Array2D *arraydest, const float exponent);

//input is rgb, output will be xyz, because next step will be tone mapping, which
//requires a frame with channels in xyz format
void applyGammaFrame( pfs::Frame* inpfsframe, const float _gamma) {

	assert(inpfsframe!=NULL);
	pfs::DOMIO pfsio;
	float gamma = _gamma;

	pfs::Channel *R, *G, *B;
	inpfsframe->getRGBChannels( R, G, B );
	assert( R!=NULL && G!=NULL && B!=NULL );

	if (gamma!=1.0f) {
		applyGamma( R, 1.0f/gamma );
		applyGamma( G, 1.0f/gamma );
		applyGamma( B, 1.0f/gamma );
	}
	//convertRGBChannelsToXYZ renames the Channels performing color-space conversion
	inpfsframe->convertRGBChannelsToXYZ();
	inpfsframe->getTags()->setString("LUMINANCE", "DISPLAY");
}

void applyGamma( pfs::Array2D *arraydest, const float exponent ) {
	int imgSize = arraydest->getRows()*arraydest->getCols();
	for( int index = 0; index < imgSize ; index++ ) {
		float &v = (*arraydest)(index);
		if( v < 0.0f )
		  v = 0.0f;
		else
		  v = powf( v, exponent );
	}
}
