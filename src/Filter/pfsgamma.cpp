/**
 * @brief Apply gamma correction the the pfs stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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
 * $Id: pfsgamma.cpp,v 1.3 2008/07/29 16:14:29 rafm Exp $
 */

#include <iostream>
#include <cmath>
#include <assert.h>

#include "pfsgamma.h"
#include "Libpfs/colorspace.h"
#include "Libpfs/domio.h"
#include "Common/msec_timer.h"

namespace pfs
{
  pfs::Frame* applyGammaOnFrame(pfs::Frame* frame, const float gamma)
  {
    float multiplier = 1.0f;
    
    const char *lum_type = frame->getTags().getString("LUMINANCE");
    if( lum_type )
    {
      if( !strcmp( lum_type, "DISPLAY" ) && gamma > 1.0f )
        std::cerr << "applyGammaOnFrame() warning: applying gamma correction to a display referred image" << std::endl;
      if( !strcmp( lum_type, "RELATIVE" ) && gamma < 1.0f )
        std::cerr << "applyGammaOnFrame() warning: applying inverse gamma correction to a linear luminance or radiance image" << std::endl;
      if( !strcmp( lum_type, "ABSOLUTE" ) && multiplier == 1 )
        std::cerr << "applyGammaOnFrame() warning: an image should be normalized to 0-1 before applying gamma correction" << std::endl;
    }
    
    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );
    
    // TODO: applyGamma can be improved in order to use SSE
    if ( X != NULL ) // Color, XYZ
    {
      pfs::Array2D* Xr = X->getChannelData();
      pfs::Array2D* Yr = Y->getChannelData();
      pfs::Array2D* Zr = Z->getChannelData();
      
      //pfs::transformColorSpace(pfs::CS_XYZ, Xr, Yr, Zr,
      //                         pfs::CS_RGB, Xr, Yr, Zr);
      // At this point (X,Y,Z) = (R,G,B)
      
      applyGamma(Xr, 1.0f/gamma, multiplier);
      applyGamma(Yr, 1.0f/gamma, multiplier);
      applyGamma(Zr, 1.0f/gamma, multiplier);
      
      //pfs::transformColorSpace(pfs::CS_RGB, Xr, Yr, Zr,
      //                         pfs::CS_XYZ, Xr, Yr, Zr);
      // At this point (X,Y,Z) = (X,Y,Z)
    }
    else if ( (Y = frame->getChannel( "Y" )) != NULL )
    {
      // Luminance only
      applyGamma(Y->getChannelData(), 1.0f/gamma, multiplier);
    } 
    //TODO
    //else
    // throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
    
    //if( opt_setgamma && gamma > 1.0f )
    frame->getTags().setString("LUMINANCE", "DISPLAY");
    //else if( opt_setgamma && gamma < 1.0f )
    //  frame->getTags()->setString("LUMINANCE", "RELATIVE");
    
    return frame;        
  }
  
  
  void applyGamma(pfs::Array2D *array, const float exponent, const float multiplier)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      float* Vin  = array->getRawData();

      const unsigned int V_ELEMS = array->getRows()*array->getCols();

      for (unsigned int idx = 0; idx < V_ELEMS; idx++)
      {
          if (Vin[idx] > 0.0f)
          {
              Vin[idx] = powf(Vin[idx]*multiplier, exponent);
          }
          else
          {
              Vin[idx] = 0.0f;
          }
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "applyGamma() = " << f_timer.get_time() << " msec" << std::endl;
#endif 
  }
  
}

