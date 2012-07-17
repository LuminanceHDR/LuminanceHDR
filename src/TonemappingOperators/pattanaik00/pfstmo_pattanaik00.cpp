/**
 * @file pfstmo_pattanaik00.cpp
 * @brief Tone map XYZ channels using Pattanaik00 model
 *
 * Time-Dependent Visual Adaptation for Realistic Image Display
 * S.N. Pattanaik, J. Tumblin, H. Yee, and D.P. Greenberg
 * In Proceedings of ACM SIGGRAPH 2000
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
 * $Id: pfstmo_pattanaik00.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include "tmo_pattanaik00.h"

#include <iostream>
#include <cmath>

#include "Libpfs/frame.h"
#include "Libpfs/colorspace.h"
#include "Common/ProgressHelper.h"

void multiplyChannels( pfs::Array2D* X, pfs::Array2D* Y, pfs::Array2D* Z, float mult );

void pfstmo_pattanaik00(pfs::Frame* frame, bool local, float multiplier, float Acone, float Arod, bool autolum, ProgressHelper *ph)
{  
    //--- default tone mapping parameters;
    bool timedependence = false;
    //bool local = false;
    //float multiplier = 1.0f;
    //Acone = -1.0f;
    //Arod  = -1.0f;
    float fps = 16.0f; //not used

    std::cout << "pfstmo_pattanaik00 (";
    std::cout << "local: " << local << ", ";
    std::cout << "multiplier: " << multiplier << ", ";
    std::cout << "Acone: " << Acone << ", ";
    std::cout << "Arod: " << Arod << ", ";
    std::cout << "autolum: " << autolum << ")" << std::endl;

    VisualAdaptationModel* am = new VisualAdaptationModel();

    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );
    frame->getTags().setString("LUMINANCE", "RELATIVE");
    //---

    if( Y==NULL || X==NULL || Z==NULL)
        throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );

    pfs::Array2D* Xr = X->getChannelData();
    pfs::Array2D* Yr = Y->getChannelData();
    pfs::Array2D* Zr = Z->getChannelData();

    // adaptation model
    if( multiplier != 1.0f )
        multiplyChannels(Xr, Yr, Zr, multiplier );

    if( !local )
    {
        if( !timedependence )
        {
            if( !autolum )
                am->setAdaptation(Acone,Arod);
            else
                am->setAdaptation(Yr);
        }
        else
            am->calculateAdaptation(Yr, 1.0f/fps);
    }
    // tone mapping
    int w = Y->getWidth();
    int h = Y->getHeight();
    pfs::Array2D* R = new pfs::Array2D(w,h);
    pfs::Array2D* G = new pfs::Array2D(w,h);
    pfs::Array2D* B = new pfs::Array2D(w,h);

    pfs::transformColorSpace( pfs::CS_XYZ, Xr, Yr, Zr, pfs::CS_RGB, R, G, B );
    tmo_pattanaik00( w, h, R->getRawData(), G->getRawData(), B->getRawData(), Y->getRawData(), am, local, ph );
    pfs::transformColorSpace( pfs::CS_RGB, R, G, B, pfs::CS_XYZ, Xr, Yr, Zr );

    if (!ph->isTerminationRequested())
        ph->newValue(100);

    delete R;
    delete G;
    delete B;
    delete am; // delete visual adaptation model
}

void multiplyChannels( pfs::Array2D* X, pfs::Array2D* Y, pfs::Array2D* Z, float mult )
{
  int size = Y->getCols() * Y->getRows();
  
  for( int i=0 ; i<size; i++ )
  {
    (*X)(i) *= mult;
    (*Y)(i) *= mult;
    (*Z)(i) *= mult;
  }
}
