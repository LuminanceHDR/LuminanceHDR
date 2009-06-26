/**
 * @brief Adaptive logarithmic tone mapping
 * 
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes. 
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-inf.mpg.de>
 *
 * $Id: pfstmo_drago03.cpp,v 1.3 2008/09/04 12:46:48 julians37 Exp $
 */


#include <math.h>
#include "../../Libpfs/pfs.h"

#include "tmo_drago03.h"

#include <iostream>

pfs::Frame* pfstmo_drago03(pfs::Frame *frame, float biasValue) {
    std::cout << "pfstmo_drago03" << std::endl;
    std::cout << "bias: " << biasValue << std::endl;

    pfs::DOMIO pfsio;

    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );
    frame->getTags()->setString("LUMINANCE", "RELATIVE");
    //---

    if( Y == NULL )
      throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );

    int w = Y->getCols();
    int h = Y->getRows();
        
    float maxLum,avLum;
    calculateLuminance( w, h, Y->getRawData(), avLum, maxLum );

    pfs::Array2DImpl* L = new pfs::Array2DImpl(w,h);
    tmo_drago03(w, h, Y->getRawData(), L->getRawData(), maxLum, avLum, biasValue);
		
    for( int x=0 ; x<w ; x++ )
      for( int y=0 ; y<h ; y++ )
      {
        float scale = (*L)(x,y) / (*Y)(x,y);
        (*Y)(x,y) *= scale;
        (*X)(x,y) *= scale;
        (*Z)(x,y) *= scale;
      }

    delete L;

    //---
    return frame;
}

