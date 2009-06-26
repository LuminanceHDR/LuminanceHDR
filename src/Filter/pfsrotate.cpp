/**
 * @brief Resize images in PFS stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk,
 *  Alexander Efremov
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

#include <math.h>
#include "../Libpfs/pfs.h"

void rotateArray( const pfs::Array2D *in, pfs::Array2D *out, bool clockwise );


pfs::Frame* rotateFrame(pfs::Frame* frame, bool clock_wise)
{
  pfs::DOMIO pfsio;

  int xSize = -1;
  int ySize = -1;
  
  bool firstFrame = true;
  pfs::Frame *resizedFrame = NULL;
  
    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );

    pfs::Channel *dX, *dY, *dZ;
    
    xSize = frame->getHeight();
    ySize = frame->getWidth();
    resizedFrame = pfsio.createFrame( xSize, ySize );

    pfs::ChannelIterator *it = frame->getChannels();
    while( it->hasNext() ) {
      pfs::Channel *originalCh = it->getNext();
      pfs::Channel *newCh = resizedFrame->createChannel( originalCh->getName() );

      rotateArray( originalCh, newCh, clock_wise );
    }

    pfs::copyTags( frame, resizedFrame );
    return resizedFrame;
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

