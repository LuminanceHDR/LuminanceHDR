/**
 * @file pfstmo_reinhard02.cpp
 * @brief Tone map XYZ channels using Reinhard02 model
 *
 * Photographic Tone Reproduction for Digital Images.
 * E. Reinhard, M. Stark, P. Shirley, and J. Ferwerda.
 * In ACM Transactions on Graphics, 2002.
 *
 * 
 * This file is a part of Luminance package, based on pfstmo.
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
 * $Id: pfstmo_reinhard02.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include <math.h>

#include "Libpfs/pfs.h"
#include "tmo_reinhard02.h"

#include <iostream>

void pfstmo_reinhard02 (pfs::Frame* frame, float key, float phi, int num, int low, int high, bool use_scales, ProgressHelper *ph )
{
    pfs::DOMIO pfsio;

    //--- default tone mapping parameters;
    //float key = 0.18;
    //float phi = 1.0;
    //int num = 8;
    //int low = 1;
    //int high = 43;
    //bool use_scales = false;
    bool temporal_coherent = false;  

	std::cout << "pfstmo_reinhard02" << std::endl;
	std::cout << "key: " << key << std::endl;
	std::cout << "phi: " << phi << std::endl;
	std::cout << "range: " << num << std::endl;
	std::cout << "lower scale: " << low << std::endl;
	std::cout << "upper scale: " << high << std::endl;
	std::cout << "use scales: " << use_scales << std::endl;


    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );
    frame->getTags()->setString("LUMINANCE", "RELATIVE");
    //---

    if( Y==NULL || X==NULL || Z==NULL)
      throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
        
    // tone mapping
    int w = Y->getCols();
    int h = Y->getRows();
    pfs::Array2DImpl* L = new pfs::Array2DImpl(w,h);

    tmo_reinhard02( w, h, Y->getRawData(), L->getRawData(), use_scales, key, phi, num, low, high, temporal_coherent, ph );

    for( int x=0 ; x<w ; x++ )
      for( int y=0 ; y<h ; y++ )
      {
        float scale = (*L)(x,y) / (*Y)(x,y);
        (*Y)(x,y) *= scale;
        (*X)(x,y) *= scale;
        (*Z)(x,y) *= scale;
      }

    delete L;
}
