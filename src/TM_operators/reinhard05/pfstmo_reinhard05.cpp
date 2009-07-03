/**
 * @file pfstmo_reinhard05.cpp
 * @brief Tone map XYZ channels using Reinhard05 model
 *
 * Dynamic Range Reduction Inspired by Photoreceptor Physiology.
 * E. Reinhard and K. Devlin.
 * In IEEE Transactions on Visualization and Computer Graphics, 2005.
 *
 * 
 * This file is a part of Qtpfsgui package, based on pfstmo.
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_reinhard05.cpp,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */


#include <math.h>

#include "../../Libpfs/pfs.h"

#include "tmo_reinhard05.h"


void pfstmo_reinhard05(pfs::Frame *frame, float brightness, float chromaticadaptation, float lightadaptation,
		ProgressHelper *ph)
{
    pfs::DOMIO pfsio;

    //--- default tone mapping parameters;
    //float brightness = 0.0f;
    //float chromaticadaptation = 0.5f;
    //float lightadaptation = 0.75f;

    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );
    frame->getTags()->setString("LUMINANCE", "RELATIVE");
    //---

    if( Y==NULL || X==NULL || Z==NULL)
      throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
        
    // tone mapping
    int w = Y->getCols();
    int h = Y->getRows();
    pfs::Array2DImpl* R = new pfs::Array2DImpl(w,h);
    pfs::Array2DImpl* G = new pfs::Array2DImpl(w,h);
    pfs::Array2DImpl* B = new pfs::Array2DImpl(w,h);

    pfs::transformColorSpace( pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, R, G, B );

    tmo_reinhard05(w, h, R->getRawData(), G->getRawData(), B->getRawData(), Y->getRawData(), brightness, chromaticadaptation, lightadaptation, ph );
    pfs::transformColorSpace( pfs::CS_SRGB, R, G, B, pfs::CS_XYZ, X, Y, Z );

	if (!ph->isTerminationRequested())
		ph->newValue( 100 );

    delete R;
    delete G;
    delete B;
}
