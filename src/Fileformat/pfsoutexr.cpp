/**
 * @brief save an exr file
 * 
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota  <grota@users.sourceforge.net>
 */

#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfOutputFile.h>
#include <ImfRgbaFile.h>
#include <ImfStringAttribute.h>
#include <ImfStandardAttributes.h>

#include "Libpfs/frame.h"


#define min(x,y) ( (x)<(y) ? (x) : (y) )

using namespace Imf;
using namespace Imath;
using std::string;

void writeEXRfile (pfs::Frame* inpfsframe, const char* outfilename)
{  
  Compression exrCompression = PIZ_COMPRESSION;
  // pfs::DOMIO pfsio;
  bool firstFrame = true;
  half *halfR = NULL;
  half *halfG = NULL;
  half *halfB = NULL;
  
  pfs::Frame *frame = inpfsframe;
  pfs::Channel *R, *G, *B;
  //TODO
  // Channels are named X Y Z but contain R G B data
  frame->getXYZChannels(R, G, B);
  R = frame->getChannel( "X" );
  G = frame->getChannel( "Y" );
  B = frame->getChannel( "Z" );
  const char* luminanceTag = frame->getTags()->getString("LUMINANCE");
  
  Header header(frame->getWidth(),
                frame->getHeight(),
                1,                      // aspect ratio
                Imath::V2f (0, 0),      // screenWindowCenter
                1,                      // screenWindowWidth
                INCREASING_Y,           // lineOrder
                exrCompression );
  // Define channels in Header
  {
    // 	pfs::ChannelIteratorPtr cit( frame->getChannelIterator() );
    header.channels().insert( "R", Channel(HALF) );
    header.channels().insert( "G", Channel(HALF) );
    header.channels().insert( "B", Channel(HALF) );
  }
  
  // Copy tags to attributes
  {
    pfs::TagIteratorPtr it( frame->getTags()->getIterator() );
    
    while( it->hasNext() )
    {
      const char *tagName = it->getNext();
      header.insert( tagName, StringAttribute(frame->getTags()->getString( tagName )) );
    }
    
    //Copy all channel tags
    pfs::ChannelIteratorPtr cit( frame->getChannelIterator() );
    while( cit->hasNext() )
    {
      pfs::Channel *ch = cit->getNext();
      pfs::TagIteratorPtr tit( ch->getTags()->getIterator() );
      while( tit->hasNext() )
      {
        const char *tagName = tit->getNext();
        string channelTagName = ch->getName();
        channelTagName += ":";
        channelTagName += tagName;
        header.insert( channelTagName.c_str(), StringAttribute(ch->getTags()->getString( tagName )) );
      }
    }
  }
  FrameBuffer frameBuffer;
  // Create channels in FrameBuffer
  {
    // pfs::ChannelIterator *it = frame->getChannels();
    
    if ( firstFrame )
    {
      halfR = new half[frame->getWidth()*frame->getHeight()];
      halfG = new half[frame->getWidth()*frame->getHeight()];
      halfB = new half[frame->getWidth()*frame->getHeight()];
      firstFrame = false;
    }
    frameBuffer.insert( "R",                                      // name
                       Slice( HALF,                               // type
                             (char*)halfR,                        // base
                             sizeof(half) * 1,                    // xStride
                             sizeof(half) * frame->getWidth()) );	// yStride
    
    frameBuffer.insert( "G",                                      // name
                       Slice( HALF,                               // type
                             (char*)halfG,                        // base
                             sizeof(half) * 1,                    // xStride
                             sizeof(half) * frame->getWidth()) );	// yStride
    
    frameBuffer.insert( "B",                                      // name
                       Slice( HALF,                               // type
                             (char*)halfB,                        // base
                             sizeof(half) * 1,                    // xStride
                             sizeof(half) * frame->getWidth()) );	// yStride
    
    int pixelCount = frame->getHeight()*frame->getWidth();
    
    // Get Array2D from Channel
    pfs::Array2D* Rr = R->getChannelData();
    pfs::Array2D* Gr = G->getChannelData();
    pfs::Array2D* Br = B->getChannelData();
    
    // Check if pixel values do not exceed maximum HALF value
    float maxValue = -1;
    for( int i = 0; i < pixelCount; i++ )
    {
      if( (*Rr)(i) > maxValue ) maxValue = (*Rr)(i);
      if( (*Gr)(i) > maxValue ) maxValue = (*Gr)(i);
      if( (*Br)(i) > maxValue ) maxValue = (*Br)(i);
      // if( (*X)(i) > maxValue ) maxValue = (*X)(i);
      // if( (*Y)(i) > maxValue ) maxValue = (*Y)(i);
      // if( (*Z)(i) > maxValue ) maxValue = (*Z)(i);
    }
    
    bool maxHalfExceeded = maxValue > HALF_MAX;
    
    if ( maxHalfExceeded )
    {
      // Rescale and copy pixels to half-type buffers
      float scaleFactor = HALF_MAX/maxValue;
      for( int i = 0; i < pixelCount; i++ )
      {
        halfR[i] = (half)((*Rr)(i)*scaleFactor);
        halfG[i] = (half)((*Gr)(i)*scaleFactor);
        halfB[i] = (half)((*Br)(i)*scaleFactor);
        // halfR[i] = (half)((*X)(i)*scaleFactor);
        // halfG[i] = (half)((*Y)(i)*scaleFactor);
        // halfB[i] = (half)((*Z)(i)*scaleFactor);
      }
      // Store scale factor as WhileLuminance standard sttribute
      // in order to restore absolute values later
      addWhiteLuminance( header, 1/scaleFactor );
    }
    else
    {
      // Copy pixels to half-type buffers
      for( int i = 0; i < pixelCount; i++ )
      {
        halfR[i] = min( (*Rr)(i), HALF_MAX );
        halfG[i] = min( (*Gr)(i), HALF_MAX );
        halfB[i] = min( (*Br)(i), HALF_MAX );
        // halfR[i] = min( (*X)(i), HALF_MAX );
        // halfG[i] = min( (*Y)(i), HALF_MAX );
        // halfB[i] = min( (*Z)(i), HALF_MAX );
      }
      if( luminanceTag != NULL && !strcmp( luminanceTag, "ABSOLUTE" ) )
      {
        addWhiteLuminance( header, 1 );
      }
    }
  }
  OutputFile file(outfilename, header);
  file.setFrameBuffer (frameBuffer);
  file.writePixels( frame->getHeight() );
  
  delete[] halfR;
  delete[] halfG;
  delete[] halfB;  
}
