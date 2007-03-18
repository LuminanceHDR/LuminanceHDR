/**
 * @brief Resize images in PFS stream
 * 
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003-2005 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
 *  original author: Alexander Efremov
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
 * @author Alexander Efremov, <aefremov@mpi-sb.mpg.de>
 *
 * $Id: pfsrotate.cpp,v 1.1 2005/06/15 13:36:54 rafm Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../libpfs/pfs.h"
#include <sstream>

void rotateArray( const pfs::Array2D *in, pfs::Array2D *out, bool clockwise );

pfs::Frame* rotateFrame( pfs::Frame* inpfsframe, bool clock_wise ) {

	pfs::DOMIO pfsio;
	bool clockwise = clock_wise;
	int xSize = -1;
	int ySize = -1;
	
	pfs::Frame *rotatedFrame = NULL;
// 	pfs::Channel *R, *G, *B;
// 	inpfsframe->getRGBChannels( R, G, B );
// 	assert( R!=NULL && G!=NULL && B!=NULL );

	xSize = inpfsframe->getHeight();
	ySize = inpfsframe->getWidth();
	rotatedFrame = pfsio.createFrame( xSize, ySize );
	
	pfs::ChannelIterator *it = inpfsframe->getChannels();
	while( it->hasNext() ) {
		pfs::Channel *originalCh = it->getNext();
		pfs::Channel *newCh = rotatedFrame->createChannel( originalCh->getName() );
		
		rotateArray( originalCh, newCh, clockwise );
	}
	
	pfs::copyTags( inpfsframe, rotatedFrame );
	return rotatedFrame;
}

void rotateArray(const pfs::Array2D *in, pfs::Array2D *out, bool clockwise)
{
  int outRows = out->getRows();
  int outCols = out->getCols();

  for( int i=0; i<outCols; i++ )
    for( int j=0; j<outRows; j++ )
      if( clockwise )
        (*out)( i, j ) = (*in)( j, outCols - i - 1 );
      else
        (*out)( i, j ) = (*in)( outRows - j - 1, i );
}
