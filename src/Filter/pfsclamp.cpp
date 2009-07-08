/**
 * @brief Clamp values of X, Y, Z channels in PFS stream
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
 * $Id: pfsclamp.cpp,v 1.2 2005/11/02 13:35:42 rafm Exp $
 */

#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>

#include "Libpfs/pfs.h"

#define PROG_NAME "pfsclamp"

class QuietException 
{
};

void percentile(const pfs::Array2D *I, float& percMin, float& percMax )
{
  int size = I->getRows() * I->getCols();
  std::vector<float> vI;

  for( int i=0 ; i<size ; i++ )
    if( (*I)(i)!=0.0f )
      vI.push_back((*I)(i));
  std::sort(vI.begin(), vI.end());

  percMin = vI[int(percMin*vI.size())];
  percMax = vI[int(percMax*vI.size())];
}

void clamp( pfs::Array2D *array, float min, float max,
  bool opt_percentile, bool opt_zeromode )
{
  int imgSize = array->getRows()*array->getCols();

  if( opt_percentile )
    percentile(array,min,max);

  float minval=min;
  float maxval=max;
  if( opt_zeromode )
    minval = maxval = 0.0f;
  
  for( int index = 0; index < imgSize ; index++ )
  {
    float &v = (*array)(index);
    if( v < min ) v = minval;
    else if( v > max ) v = maxval;
    
    if( !finite(v) )
      v = maxval;
  }    
}


void printHelp()
{
  fprintf( stderr, PROG_NAME " [--min <val>] [--max <val>] [--verbose] [--help]\n"
    "See man page for more information.\n" );
}

void clampFrames( int argc, char* argv[] )
{
  pfs::DOMIO pfsio;

  float clampMin = 0.0001;    // default: 10^-4
  float clampMax = 100000000; // default: 10^8
  bool verbose = false;
  bool opt_percentile = false;
  bool opt_zeromode = false;
  bool opt_rgbmode = false;

  static struct option cmdLineOptions[] = {
    { "help", no_argument, NULL, 'h' },
    { "verbose", no_argument, NULL, 'v' },
    { "percentile", no_argument, NULL, 'p' },
    { "zero", no_argument, NULL, 'z' },
    { "rgb", no_argument, NULL, 'r' },
    { "min", required_argument, NULL, 'n' },
    { "max", required_argument, NULL, 'x' },
    { NULL, 0, NULL, 0 }
  };

  int optionIndex = 0;
  while( 1 ) {
    int c = getopt_long (argc, argv, "hvpz", cmdLineOptions, &optionIndex);
    if( c == -1 ) break;
    switch( c ) {
    case 'h':
      printHelp();
      throw QuietException();
    case 'v':
      verbose = true;
      break;
    case 'p':
      opt_percentile = true;
      break;
    case 'z':
      opt_zeromode = true;
      break;
    case 'r':
      opt_rgbmode = true;
      break;
    case 'n':
      clampMin = (float)strtod( optarg, NULL );
      break;
    case 'x':
      clampMax = (float)strtod( optarg, NULL );
      break;
    case '?':
      throw QuietException();
    case ':':
      throw QuietException();
    }
  }

  // check if clamping parameters make sense
  if( opt_percentile )
  {
    clampMin = (clampMin>1e-3) ? clampMin : 1e-3;
    clampMax = (clampMax<1) ? clampMax : 0.999f;
    if( clampMin >= clampMax )
      throw pfs::Exception("incorrect clamping range for percentile mode");
  }
  else
  {
    clampMin = (clampMin>1e-4) ? clampMin : 1e-4;
    clampMax = (clampMax<1e8) ? clampMax : 1e8;
    if( clampMin >= clampMax )
      throw pfs::Exception("incorrect clamping range");
  }

  
  if( verbose )
  {
    if( opt_rgbmode )
      fprintf(stderr, "Clamping in RGB color space.\n");
    if( opt_zeromode )
      fprintf(stderr, "Values out of clamp range will be set to zero.\n");
    if( opt_percentile )      
      fprintf( stderr, "Clamping channels to [%g, %g] percentile.\n", clampMin, clampMax );
    else
      fprintf( stderr, "Clamping channels to [%g, %g] range.\n", clampMin, clampMax );
  }

   
  while( true ) {
    pfs::Frame *frame = pfsio.readFrame( stdin );
    if( frame == NULL ) break; // No more frames
        
    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels( X, Y, Z );

    if( X != NULL )
    {           // Color, XYZ
      if( opt_rgbmode )
        pfs::transformColorSpace( pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, X, Y, Z );
        
      clamp( X, clampMin, clampMax, opt_percentile, opt_zeromode );
      clamp( Y, clampMin, clampMax, opt_percentile, opt_zeromode );
      clamp( Z, clampMin, clampMax, opt_percentile, opt_zeromode );
      
      if( opt_rgbmode )
        pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );
    }
    else if( (Y = frame->getChannel( "Y" )) != NULL )
    {
      clamp( Y, clampMin, clampMax, opt_percentile, opt_zeromode );
    }
    else
      throw pfs::Exception( "Missing X, Y, Z channels in the PFS stream" );        

    pfsio.writeFrame( frame, stdout );
    pfsio.freeFrame( frame );        
  }
}


int main( int argc, char* argv[] )
{
    try {
        clampFrames( argc, argv );
    }
    catch( pfs::Exception ex ) {
        fprintf( stderr, PROG_NAME " error: %s\n", ex.getMessage() );
        return EXIT_FAILURE;
    }        
    catch( QuietException  ex ) {
        return EXIT_FAILURE;
    }        
    return EXIT_SUCCESS;
}
