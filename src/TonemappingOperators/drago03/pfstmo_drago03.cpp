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

#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "tmo_drago03.h"

namespace
{
template <typename T>
inline
T decode(const T& value)
{
    if ( value <= 0.0031308f )
    {
        return (value * 12.92f);
    }
    return (1.055f * std::pow( value, 1.f/2.4f ) - 0.055f);
}
}

void pfstmo_drago03(pfs::Frame& frame, float biasValue, pfs::Progress &ph)
{
    std::cout << "pfstmo_drago03 (";
    std::cout << "bias: " << biasValue << ")" << std::endl;

    pfs::Channel *X, *Y, *Z;
    frame.getXYZChannels( X, Y, Z );

    frame.getTags().setString("LUMINANCE", "RELATIVE");
    //---

    if ( Y == NULL )
    {
        throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );
    }

    pfs::Array2D& Xr = *X->getChannelData();
    pfs::Array2D& Yr = *Y->getChannelData();
    pfs::Array2D& Zr = *Z->getChannelData();

    int w = Yr.getCols();
    int h = Yr.getRows();

    float maxLum;
    float avLum;
    calculateLuminance(w, h, Yr.getRawData(), avLum, maxLum);

    pfs::Array2D L(w, h);
    tmo_drago03(Yr, L, maxLum, avLum, biasValue, ph);

    for (int x=0 ; x<w ; x++)
    {
        for (int y=0 ; y<h ; y++)
        {
            float scale = L(x,y) / Yr(x,y);
            Yr(x,y) = decode( Yr(x,y) * scale );
            Xr(x,y) = decode( Xr(x,y) * scale );
            Zr(x,y) = decode( Zr(x,y) * scale );
        }
    }

    if (!ph.canceled())
    {
        ph.setValue( 100 );
    }
}

