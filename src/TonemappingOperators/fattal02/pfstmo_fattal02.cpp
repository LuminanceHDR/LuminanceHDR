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

#include "tmo_fattal02.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "Libpfs/frame.h"
#include "Common/ProgressHelper.h"

void pfstmo_fattal02(pfs::Frame* frame,
                     float opt_alpha,
                     float opt_beta,
                     float opt_saturation,
                     float opt_noise,
                     bool newfattal,
                     bool fftsolver,
                     int detail_level,
                     ProgressHelper *ph)
{
  if (fftsolver)
  {
      // opt_alpha = 1.f;
      newfattal = true; // let's make sure, prudence is never enough!
  }

  if ( opt_noise <= 0.0f )
  {
      opt_noise = opt_alpha * 0.01f;
  }
  
  std::stringstream ss;
  ss << "pfstmo_fattal02 (";
  ss << "alpha: " << opt_alpha;
  ss << ", beta: " << opt_beta;
  ss << ". saturation: " <<  opt_saturation;
  ss << ", noise: " <<  opt_noise;
  ss << ", fftsolver: " << fftsolver << ")";
  std::cout << ss.str();
  
  //Store RGB data temporarily in XYZ channels
  pfs::Channel *X, *Y, *Z;
  frame->getXYZChannels( X, Y, Z );
  frame->getTags()->setString("LUMINANCE", "RELATIVE");
  //---
  
  if ( Y==NULL || X==NULL || Z==NULL )
  {
      throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
  }
  
  pfs::Array2D& Xr = *X->getChannelData();
  pfs::Array2D& Yr = *Y->getChannelData();
  pfs::Array2D& Zr = *Z->getChannelData();
  
  // tone mapping
  int w = Y->getWidth();
  int h = Y->getHeight();
  
  pfs::Array2D L(w,h);

  tmo_fattal02(w, h, Yr, L,
               opt_alpha, opt_beta, opt_noise, newfattal,
               fftsolver, detail_level,
               ph);

  if ( !ph->isTerminationRequested() )
  {
      for ( int idx = 0; idx < w*h; ++idx )
      {
          Xr(idx) = powf( Xr(idx)/Yr(idx), opt_saturation ) * L(idx);
          Zr(idx) = powf( Zr(idx)/Yr(idx), opt_saturation ) * L(idx);
          Yr(idx) = L(idx);
      }

      ph->newValue( 100 );
  }
}
