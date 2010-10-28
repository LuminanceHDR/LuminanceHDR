/**
 * @brief Adaptive logarithmic tone mapping
 * 
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes. 
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-inf.mpg.de>
 *
 * $Id: pfstmo_drago03.cpp,v 1.3 2008/09/04 12:46:48 julians37 Exp $
 */


#include <cmath>
#include <iostream>

#include "Libpfs/pfs.h"
#include "tmo_drago03.h"

void pfstmo_drago03(pfs::Frame *frame, float biasValue, ProgressHelper *ph)
{
  std::cout << "pfstmo_drago03" << std::endl;
  std::cout << "bias: " << biasValue << std::endl;
  
  pfs::DOMIO pfsio;
  
  pfs::Channel *X, *Y, *Z;
  frame->getXYZChannels( X, Y, Z );
  
  frame->getTags()->setString("LUMINANCE", "RELATIVE");
  //---
  
  if( Y == NULL )
    throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
  
  pfs::Array2DImpl *Xr = X->getChannelData();
  pfs::Array2DImpl *Yr = Y->getChannelData();
  pfs::Array2DImpl *Zr = Z->getChannelData();
  
  int w = Yr->getCols();
  int h = Yr->getRows();
  
  float maxLum,avLum;
  calculateLuminance( w, h, Yr->getRawData(), avLum, maxLum );
  
  pfs::Array2DImpl* L = new pfs::Array2DImpl(w, h);
  tmo_drago03(w, h, Yr->getRawData(), L->getRawData(), maxLum, avLum, biasValue, ph);
  
  for( int x=0 ; x<w ; x++ )
  {
    for( int y=0 ; y<h ; y++ )
    {
      float scale = (*L)(x,y) / (*Yr)(x,y);
      (*Yr)(x,y) *= scale;
      (*Xr)(x,y) *= scale;
      (*Zr)(x,y) *= scale;
    }
  }
  
	if (!ph->isTerminationRequested())
    ph->newValue( 100 );
  
  delete L;
}

