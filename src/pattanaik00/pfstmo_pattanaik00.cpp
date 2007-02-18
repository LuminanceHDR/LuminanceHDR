/**
 * @file pfstmo_pattanaik00.cpp
 * @brief Tone map XYZ channels using Pattanaik00 model
 *
 * Time-Dependent Visual Adaptation for Realistic Image Display
 * S.N. Pattanaik, J. Tumblin, H. Yee, and D.P. Greenberg
 * In Proceedings of ACM SIGGRAPH 2000
 *
 * 
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003-2007 Grzegorz Krawczyk
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * $Id: pfstmo_pattanaik00.cpp,v 1.3 2004/12/14 15:12:16 krawczyk Exp $
 */


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pfs.h>
#include <tmo_pattanaik00.h>
#include <QFile>

using namespace std;

void multiplyChannels( pfs::Array2D* X, pfs::Array2D* Y, pfs::Array2D* Z, float mult );

pfs::Frame* pfstmo_pattanaik00 (pfs::Frame* inpfsframe, bool _local,float _multiplier, float _Acone, float _Arod, bool autoY ) {
	assert(inpfsframe!=NULL);

	pfs::DOMIO pfsio;
	//--- default tone mapping parameters;
	bool local = _local;
	float multiplier = _multiplier;
	float Acone = _Acone;
	float Arod  = _Arod;
	
	VisualAdaptationModel* am = new VisualAdaptationModel();
	
	pfs::Channel *X, *Y, *Z;
	inpfsframe->getXYZChannels(X,Y,Z);
	assert( X!=NULL && Y!=NULL && Z!=NULL );

	pfs::Frame *outframe = pfsio.createFrame( inpfsframe->getWidth(), inpfsframe->getHeight() );
	assert(outframe!=NULL);
	pfs::Channel *Xo, *Yo, *Zo;
	outframe->createXYZChannels( Xo, Yo, Zo );
	assert( Xo!=NULL && Yo!=NULL && Zo!=NULL );
	pfs::copyArray(X,Xo);
	pfs::copyArray(Y,Yo);
	pfs::copyArray(Z,Zo);

	
	// adaptation model
	if( multiplier!=1.0f )
		multiplyChannels( Xo, Yo, Zo, multiplier );
	
	if( !local ) {
		if( !autoY )
			am->setAdaptation(Acone,Arod);
		else
			am->setAdaptation(Yo);
	}
	
// 	// tone mapping
// 	int w = Y->getCols();
// 	int h = Y->getRows();
	
	pfs::transformColorSpace( pfs::CS_XYZ, Xo, Yo, Zo, pfs::CS_RGB, Xo, Yo, Zo ); //in:xyz ---> out:rgb
	tmo_pattanaik00( Xo, Yo, Zo, Y, am, local );
	pfs::transformColorSpace( pfs::CS_RGB, Xo, Yo, Zo, pfs::CS_XYZ, Xo, Yo, Zo ); //out:rgb ---> out:xyz
	
	delete am; // delete visual adaptation model
	
	return outframe;
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
