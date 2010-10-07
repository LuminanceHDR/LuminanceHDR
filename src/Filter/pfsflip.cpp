/**
 * @brief Flip images in PFS stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk,
 *  Alexander Efremov
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
 * @author Alexander Efremov, <aefremov@mpi-sb.mpg.de>
 *
 * $Id: pfsflip.cpp,v 1.1 2005/06/15 13:36:54 rafm Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>

#include "Libpfs/pfs.h"

#include <sstream>

#define PROG_NAME "pfsflip"

class QuietException 
{
};

void flipArray( const pfs::Array2D *in, pfs::Array2D *out, bool h, bool v );

//void printHelp()
//{
//  fprintf( stderr, PROG_NAME " [-h] [-v] [--help]\n"
//    "See man page for more information.\n" );
//}

//void flipFrames( int argc, char* argv[] )
//{
//  pfs::DOMIO pfsio;
//
//  static struct option cmdLineOptions[] = {
//    { "help", no_argument, NULL, '1' },
//    { "h", no_argument, NULL, 'h' },
//    { "v", no_argument, NULL, 'v' },
//    { NULL, 0, NULL, 0 }
//  };
//
//  bool h = false;
//  bool v = false;
//    
//  int optionIndex = 0;
//  while( 1 ) {
//    int c = getopt_long (argc, argv, "hv", cmdLineOptions, &optionIndex);
//    if( c == -1 ) break;
//    switch( c ) {
//    case '1':
//      printHelp();
//      throw QuietException();
//    case 'h':
//      h = true;
//      break;
//    case 'v':
//      v = true;
//      break;
//    case '?':
//      throw QuietException();
//    case ':':
//      throw QuietException();
//    }
//  }
//  
//  if( (!h) & (!v) )
//    throw pfs::Exception( "Either --h or --v must be specified" );
//  
//  bool firstFrame = true;
//  pfs::Frame *resizedFrame = NULL;
//  
//  while( true ) {
//    pfs::Frame *frame = pfsio.readFrame( stdin );
//    if( frame == NULL ) break; // No more frames
//
//    pfs::Channel *X, *Y, *Z;
//    frame->getXYZChannels( X, Y, Z );
//
//    pfs::Channel *dX, *dY, *dZ;
//    
//    if( firstFrame ) {
//      int xSize = frame->getWidth();
//      int ySize = frame->getHeight();
//      resizedFrame = pfsio.createFrame( xSize, ySize );
//      firstFrame = false;
//    }
//
//    pfs::ChannelIterator *it = frame->getChannels();
//    while( it->hasNext() ) {
//      pfs::Channel *originalCh = it->getNext();
//      pfs::Channel *newCh = resizedFrame->createChannel( originalCh->getName() );
//
//      flipArray( originalCh, newCh, h, v );
//    }
//
//    pfs::copyTags( frame, resizedFrame );
//    pfsio.writeFrame( resizedFrame, stdout );
//    pfsio.freeFrame( frame );        
//  }
//  pfsio.freeFrame( resizedFrame );
//}

void flipArray(const pfs::Array2D *in, pfs::Array2D *out, bool h, bool v )
{
  int outRows = out->getRows();
  int outCols = out->getCols();
  
  if( h & v ) {
    for( int i=0; i<outCols; i++ )
      for( int j=0; j<outRows; j++ )
        (*out)( i, j ) = (*in)( outCols - i - 1, outRows - j - 1 );
  } else if( h ) {
    for( int i=0; i<outCols; i++ )
      for( int j=0; j<outRows; j++ )
        (*out)( i, j ) = (*in)( outCols - i - 1, j );
  } else if( v ) {
    for( int i=0; i<outCols; i++ )
      for( int j=0; j<outRows; j++ )
        (*out)( i, j ) = (*in)( i, outRows - j - 1 );
  }
}

//int main( int argc, char* argv[] )
//{
//  try {
//    flipFrames( argc, argv );
//  }
//  catch( pfs::Exception ex ) {
//    fprintf( stderr, PROG_NAME " error: %s\n", ex.getMessage() );
//    return EXIT_FAILURE;
//  }
//  catch( QuietException  ex ) {
//    return EXIT_FAILURE;
//  }
//  return EXIT_SUCCESS;
//}
