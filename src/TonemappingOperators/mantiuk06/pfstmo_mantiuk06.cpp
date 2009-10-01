/**
 * @brief Contrast mapping TMO
 *
 * From:
 * 
 * Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
 * A Perceptual Framework for Contrast Processing of High Dynamic Range Images
 * In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
 * http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 *
 * $Id: pfstmo_mantiuk06.cpp,v 1.9 2008/06/16 22:09:33 rafm Exp $
 */

#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <iostream>

#include "Libpfs/pfs.h"
#include "contrast_domain.h"

void pfstmo_mantiuk06(pfs::Frame* frame, float scaleFactor, float saturationFactor, float detailFactor, bool cont_eq, ProgressHelper *ph)
{
    //--- default tone mapping parameters;
    //float scaleFactor = 0.1f;
    //float saturationFactor = 0.8f;
    //bool cont_map = false, cont_eq = false
    bool cont_map = false;
    bool bcg = true;
    int itmax = 200;
    float tol = 1e-3;

    if (!cont_eq)
	cont_map = true;
    else 
	scaleFactor = -scaleFactor;

   std::cout << "pfstmo_mantiuk06" << std::endl;
   std::cout << "cont_map: " << cont_map << std::endl;
   std::cout << "cont_eq: " << cont_eq << std::endl;
   std::cout << "scaleFactor: " << scaleFactor << std::endl;
   std::cout << "saturationFactor: " << saturationFactor << std::endl;
   std::cout << "detailFactor: " << detailFactor << std::endl;

    pfs::DOMIO pfsio;

    pfs::Channel *inX, *inY, *inZ;
	
    frame->getXYZChannels(inX, inY, inZ);
    int cols = frame->getWidth();
    int rows = frame->getHeight();

    pfs::Array2DImpl R( cols, rows );
  
    pfs::transformColorSpace( pfs::CS_XYZ, inX, inY, inZ, pfs::CS_RGB, inX, &R, inZ );

    tmo_mantiuk06_contmap( cols, rows, inX->getRawData(), R.getRawData(), inZ->getRawData(), inY->getRawData(),
      scaleFactor, saturationFactor, bcg, itmax, tol, ph);	

    pfs::transformColorSpace( pfs::CS_RGB, inX, &R, inZ, pfs::CS_XYZ, inX, inY, inZ );
    frame->getTags()->setString("LUMINANCE", "RELATIVE");
}
