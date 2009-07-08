/**
 * @brief PFS library - additional utilities
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 * 
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * $Id: pfsutils.cpp,v 1.4 2006/09/21 21:42:54 rafm Exp $
 */

#include <config.h>

#include "pfs.h"

#include <stdlib.h>
#include <list>

#include <getopt.h>

#define MAX_FRAME 99999

using namespace std;

namespace pfs
{

static void removeCommandLineArg( int &argc, char* argv[],
  int firstArgToRemove, int numArgsToRemove = 1 );


static void parseFrameRange( const char *rangeString,
  int &firstFrame, int &lastFrame, int &everyNthFrame );

struct FilePattern
{
  const char *pattern;
  int firstFrame;
  int lastFrame;
  bool skipMissingFrames;
  int everyNthFrame;
  int currentFrame;
  bool isPattern;
  FILE *stdinout;               // Use this fh if "-" found istead of file name

  FilePattern( const char *pattern, FILE *n_stdinout ): pattern( pattern ),
                                      firstFrame( 0 ),
                                      lastFrame( MAX_FRAME ),
                                      skipMissingFrames( false ),
                                      currentFrame( 0 ),
                                      everyNthFrame( 1 )
  {
    isPattern = strstr( pattern, "%" ) != NULL;
    if( !strcmp( pattern, "-" ) ) stdinout = n_stdinout;
    else stdinout = NULL;
  }
};


class FrameFileIteratorImpl
{
  char *pattern;
  const char *fopenMode;

  char fileName[1024];
  FILE *stdinout;

  typedef list<FilePattern> PatternList;

  PatternList patternList;
  PatternList::iterator currentPattern;

public:
  FrameFileIteratorImpl( int &argc, char* argv[], const char *fopenMode,
    const char *fileNamePrefix, FILE *stdinout,
    const char *optstring, const struct option *getopt_long )
    : fopenMode( fopenMode ), stdinout( stdinout )
  {
    PatternList::pointer lastPattern = NULL;

    for( int i=1 ; i<argc; )
    {
      if( !strcmp( argv[i], "--frames" ) ) {
        if( i+1 >= argc )
          throw CommandLineException( "Missing frame range after '--frame' switch" );
        if( lastPattern == NULL )
          throw CommandLineException( "File pattern must be specified before '--frame' switch" );
        parseFrameRange( argv[i+1], lastPattern->firstFrame, lastPattern->lastFrame,
          lastPattern->everyNthFrame );
        lastPattern->currentFrame = lastPattern->firstFrame;
        removeCommandLineArg( argc, argv, i, 2 );
      }
      else if( fileNamePrefix != NULL && !strcmp( argv[i], fileNamePrefix ) ) {
        if( i+1 >= argc )
          throw CommandLineException( "Missing file name" );
        patternList.push_back( FilePattern( argv[i+1], stdinout ) );
        lastPattern = &patternList.back();
        removeCommandLineArg( argc, argv, i, 2 );
      }
      else if( !strcmp( argv[i], "--skip-missing" ) ) {
        if( lastPattern == NULL )
          throw CommandLineException( "File pattern must be specified before '--skip-missing' switch" );
        lastPattern->skipMissingFrames = true;
        removeCommandLineArg( argc, argv, i, 1 );
      }
      else if( fileNamePrefix == NULL &&
        ( argv[i][0] != '-' || !strcmp( argv[i], "-" )) ) {
        patternList.push_back( FilePattern( argv[i], stdinout ) );
        lastPattern = &patternList.back();
        removeCommandLineArg( argc, argv, i, 1 );
      } else {
        bool optionProcessed = false;

        if( getopt_long != NULL && !strncmp( argv[i], "--", 2 ) ) { 
          // Make sure that option parameters are processed properly
          const struct option *opt = getopt_long;
          while( opt->name != NULL ) {
            if( !strncmp( argv[i]+2, opt->name, strlen(opt->name) ) ) {
              if( opt->has_arg == required_argument ) {
                if( argv[i][strlen(opt->name)+2] == '=' ) { // --long=arg
                  i++;
                  optionProcessed = true;
                  break;
                } else {          // --long arg
                  i += 2;   
                  optionProcessed = true;
                  break;
                } 
              } else if( opt->has_arg == no_argument ) {                
                i += 1;
                optionProcessed = true;
                break;                
              } else
                throw CommandLineException( "Internal error: FrameFileIterator can handle only required_argument and no_argument options" );

            }
            opt++;
          }
        }
        if( !optionProcessed && optstring != NULL && argv[i][0] == '-' ) {
          for( const char *opt = optstring; *opt != 0; opt++ ) {            
            if( argv[i][1] == *opt ) {
              if( *(opt+1) == ':' ) {
                i += 2;
              } else {
                i += 1;
              }
              optionProcessed = true;
              break;
            }
          }
        }
        if( !optionProcessed )
          i++;
      }
    }

    currentPattern = patternList.begin();
  }

  FrameFile getNextFrameFile( )
  {
    while( currentPattern != patternList.end() ) {

      int skippedFrames = 0;
      do {

        if( currentPattern->currentFrame > currentPattern->lastFrame ) break;

        if( currentPattern->isPattern )
          sprintf( fileName, currentPattern->pattern, currentPattern->currentFrame );
        else {          // Single file, not a pattern
          strcpy( fileName, currentPattern->pattern );
          if( currentPattern->stdinout == NULL ) {
            //Force to step to next pattern in the list, but not for stdin/out
            currentPattern->currentFrame = currentPattern->lastFrame;
          }
        }

        FILE *fh;
        if( currentPattern->stdinout != NULL )
          fh = currentPattern->stdinout;
        else
          fh = fopen( fileName, fopenMode );

        currentPattern->currentFrame += currentPattern->everyNthFrame;

        if( fh != NULL ) return FrameFile( fh, fileName );

        if( !currentPattern->isPattern || (currentPattern->currentFrame-currentPattern->everyNthFrame) == currentPattern->firstFrame ) {
          char msg[1024];
          sprintf( msg, "Can not open file '%s'", fileName );
          throw pfs::Exception( msg );
        }
        skippedFrames++;
      } while( currentPattern->skipMissingFrames && skippedFrames < 10 );

      currentPattern++;
    }

    return FrameFile( NULL, NULL );
  }

  void closeFrameFile( FrameFile &frameFile )
  {
    if( frameFile.fh != NULL && frameFile.fh != stdinout ) fclose( frameFile.fh );
    frameFile.fh = NULL;
  }


};

FrameFileIterator::FrameFileIterator( int &argc, char* argv[],
  const char *fopenMode, const char *fileNamePrefix, FILE *stdinout,
  const char *optstring, const struct option *getopt_long )
{
  impl = new FrameFileIteratorImpl( argc, argv, fopenMode,
    fileNamePrefix, stdinout, optstring, getopt_long );
}

FrameFileIterator::~FrameFileIterator()
{
  delete impl;
}

FrameFile FrameFileIterator::getNextFrameFile( )
{
  return impl->getNextFrameFile();
}

void FrameFileIterator::closeFrameFile( FrameFile &frameFile )
{
  return impl->closeFrameFile( frameFile );
}

void FrameFileIterator::printUsage( FILE *out, const char *progName )
{
  fprintf( out,
    "Usage: %s [switches] <frame_pattern> [--frames <range>] [--skip-missing] [<frame_pattern>]...\n\n"
    "<frame_pattern> can contain '%%d' to process a sequence of frames. To insert leading zeros use '%%0Nd', where n is a number of zeros. Any number of <frame_pattern>s can be given in a command line. They are processed one after another. Switches --frames and --skip-missing always refer to the last <frame_pattern>\n"
    "\nSwitches:\n"
    "  --frames <range>  : range of frame numbers to process. Range is given Octave range format, e.g. 10:2:100, to process every second frame, starting from 10 and stopping at 100\n"
    "  --skip-missing    : skip up to 10 consequtive frames if there are missing\n",
    progName
    );
}

static void parseFrameRange( const char *rangeString, int &firstFrame, int &lastFrame, int &everyNthFrame )
{
  firstFrame = 0;
  lastFrame = MAX_FRAME;
  everyNthFrame = 1;

  char *nextToken;
  int l1 = strtol( rangeString, &nextToken, 10 );
  if( nextToken != rangeString ) firstFrame = l1;

  if( nextToken[0] != ':' )
    throw CommandLineException( "Missing ':' in the frame range specification" );
  nextToken++;

  char *currentToken = nextToken;
  int l2 = strtol( currentToken, &nextToken, 10 );
  if( currentToken != nextToken ) lastFrame = l2;

  if( nextToken[0] == ':' ) {
    everyNthFrame = lastFrame;
    lastFrame = MAX_FRAME;

    nextToken++;
    currentToken = nextToken;
    int l3 = strtol( currentToken, &nextToken, 10 );
    if( currentToken != nextToken ) lastFrame = l3;
  }

}


static void removeCommandLineArg( int &argc, char* argv[],
  int firstArgToRemove, int numArgsToRemove )
{
  assert( firstArgToRemove+numArgsToRemove <= argc );
  if( argc-firstArgToRemove-numArgsToRemove > 0 ) {
    for( int i = firstArgToRemove; i < argc-numArgsToRemove; i++ )
      argv[i] = argv[i+numArgsToRemove];
  }

  argc -= numArgsToRemove;
}


}
