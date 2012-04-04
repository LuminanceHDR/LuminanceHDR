/**
 * @file pfstmo_fattal02.cpp
 * @brief Tone map XYZ channels using Fattal02 model
 *
 * Gradient Domain High Dynamic Range Compression
 * R. Fattal, D. Lischinski, and M. Werman
 * In ACM Transactions on Graphics, 2002.
 *
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_fattal02.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include <cmath>
#include <iostream>

#include "Libpfs/frame.h"
#include "tmo_fattal02.h"

void pfstmo_fattal02(pfs::Frame* frame, float opt_alpha, float opt_beta, float opt_saturation, float opt_noise, bool newfattal, ProgressHelper *ph)
{  
  //--- default tone mapping parameters;
  //float opt_alpha = 1.0f;
  //float opt_beta = 0.9f;
  //float opt_saturation=0.8f;
  //float opt_noise = -1.0f; // not set!
  
  // adjust noise floor if not set by user
  if ( opt_noise <= 0.0f )
  {
    opt_noise = opt_alpha*0.01f;
  }
  
  std::cout << "pfstmo_fattal02" << std::endl;
  std::cout << "alpha: " << opt_alpha << std::endl;
  std::cout << "beta: " << opt_beta << std::endl;
  std::cout << "saturation: " <<  opt_saturation << std::endl;
  std::cout << "noise: " <<  opt_noise << std::endl;
  
  //Store RGB data temporarily in XYZ channels
  pfs::Channel *X, *Y, *Z;
  frame->getXYZChannels( X, Y, Z );
  frame->getTags()->setString("LUMINANCE", "RELATIVE");
  //---
  
  
  if ( Y==NULL || X==NULL || Z==NULL )
  {
    throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
  }
  
  pfs::Array2D* Xr = X->getChannelData();
  pfs::Array2D* Yr = Y->getChannelData();
  pfs::Array2D* Zr = Z->getChannelData();
  
  // tone mapping
  int w = Y->getWidth();
  int h = Y->getHeight();
  
  pfs::Array2D* L = new pfs::Array2D(w,h);
  Fattal02 tmoperator(w, h, Y->getRawData(), L->getRawData(), opt_alpha, opt_beta, opt_noise, newfattal, ph);
  tmoperator.tmo_fattal02();

  if ( !ph->isTerminationRequested() )
  {
    for( int x=0 ; x<w ; x++ )
    {
      for( int y=0 ; y<h ; y++ )
      {
        (*Xr)(x,y) = powf( (*Xr)(x,y)/(*Yr)(x,y), opt_saturation ) * (*L)(x,y);
        (*Zr)(x,y) = powf( (*Zr)(x,y)/(*Yr)(x,y), opt_saturation ) * (*L)(x,y);
        (*Yr)(x,y) = (*L)(x,y);
      }
    }
    
    ph->newValue( 100 );
  }
  
  delete L;
}
