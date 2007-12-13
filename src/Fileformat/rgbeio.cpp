/**
 * @brief IO operations on Radiance's RGBE file format
 * 
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003-2007 Rafal Mantiuk and Grzegorz Krawczyk
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
 * $Id: rgbeio.cpp,v 1.6 2006/11/20 11:19:21 gkrawczyk Exp $
 */

#include <iostream>

#include <math.h>
#include <assert.h>

#include "../Libpfs/pfs.h"

#include "rgbeio.h"

using namespace std;

/// constant to change between radiance and luminance
#define WHITE_EFFICACY 179.0

void readRadianceHeader( FILE *file, int &width, int &height, float &exposure );
void readRadiance( FILE *file, int width, int height, float exposure,
		   pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z );
void writeRadiance( FILE *file, pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z );

// RGBE IO classes implementation

RGBEReader::RGBEReader( FILE *fh ) : fh(fh)
{
  //TODO: read header from radiance file
  readRadianceHeader( fh, width, height, exposure );
}

void RGBEReader::readImage( pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z )
{
  readRadiance( fh, width, height, exposure, X, Y, Z );
}

RGBEReader::~RGBEReader()
{}

void RGBEWriter::writeImage( pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z )
{
  writeRadiance( fh, X, Y, Z );
}



//--------------------------------------------------------------------
// RGBE IO format support functions

typedef unsigned char Trgbe;
struct Trgbe_pixel
{
  /// @name RGB values and their exponent
  //@{
  Trgbe r;
  Trgbe g;
  Trgbe b;
  Trgbe e;
  //@}

};

void rgbe2rgb(const Trgbe_pixel& rgbe, float exposure, float &r, float &g, float &b)
{
  if( rgbe.e!=0 )		// a non-zero pixel
  {
    int e = rgbe.e - int(128+8);
    double f = ldexp( 1.0, e ) * WHITE_EFFICACY / exposure;

    r = (float)(rgbe.r * f);
    g = (float)(rgbe.g * f);
    b = (float)(rgbe.b * f);
  }
  else
    r = g = b = 0.f;
}


void rgb2rgbe( float r, float g, float b, Trgbe_pixel& rgbe)
{
  r /= WHITE_EFFICACY;
  g /= WHITE_EFFICACY;
  b /= WHITE_EFFICACY;
  
  double v = r;	// max rgb value
  if( v < g)
    v = g;
  if( v < b )
    v = b;

  if( v < 1e-32 )
  {
    rgbe.r = rgbe.g = rgbe.b = rgbe.e = 0;
  }
  else
  {
    int e;	// exponent

    v = frexp(v,&e) * 256.0/v;
    rgbe.r = Trgbe( v*r );
    rgbe.g = Trgbe( v*g );
    rgbe.b = Trgbe( v*b );
    rgbe.e = Trgbe( e+128 );
  }
}


//--------------------------------------------------------------------
// RGBE IO support functions


// Reading RGBE files
void readRadianceHeader( FILE *file, int &width, int &height, float &exposure )
{
//   DEBUG_STR << "RGBE: reading header..." << endl;

  // read header information
  char head[255];
  float fval;
  int format=0;
  exposure = 1.0f;
  while( !feof(file) )
  {
    fgets(head, 200, file);
    if( strcmp(head, "\n")==0 )
      break;
    if( strcmp(head, "#?RADIANCE\n")==0 )
    {
      // format specifier found
      format=1;
    }
    if( strcmp(head, "#?RGBE\n")==0 )
    {
      // format specifier found
      format=1;
    }
    if( strcmp(head, "#?AUTOPANO\n")==0 )
    {
      // format specifier found
      format=1;
    }
    if( head[0]=='#' ) // comment found - skip
      continue;
    if( strcmp(head, "FORMAT=32-bit_rle_rgbe\n")==0 )
    {
      // header found
      continue;
    }
    if( sscanf(head, "EXPOSURE=%f", &fval)==1 )
    {
      // exposure value
      exposure *= fval;
    }
  }

  // ignore wierd exposure adjustments
  if( exposure>1e12 || exposure<1e-12 )
    exposure=1.0f;

  if( !format )
  {
    throw pfs::Exception( "RGBE: no format specifier found" );
  }

  // image size
  char xbuf[4], ybuf[4];
  if( fgets(head,sizeof(head)/sizeof(head[0]),file) == 0
      || sscanf(head,"%s %d %s %d",ybuf,&height,xbuf,&width) != 4 )
  {
    throw pfs::Exception( "RGBE: unknown image size" );
  }

/*	if( ybuf[1]=='x' || ybuf[1]=='X' ) {
	height += width;
	width = height - width;
	height = height - width;
	}
*/
//   DEBUG_STR << "RGBE: image size " << width << "x" << height << endl;
}



void RLERead( FILE* file, Trgbe* scanline, int size )
{
  int peek=0;
  while( peek<size )
  {
    Trgbe p[2];
    fread(p, sizeof(p), 1, file);
    if( p[0]>128 )
    {
      // a run
      int run_len = p[0]-128;

      while( run_len>0 )
      {
	scanline[peek++] = p[1];
	run_len--;
      }
    }
    else
    {
      // a non-run
      scanline[peek++] = p[1];

      int nonrun_len = p[0]-1;
      if( nonrun_len>0 )
      {
	fread(scanline+peek, sizeof(*scanline), nonrun_len, file);
	peek += nonrun_len;
      }
    }
  }
  if( peek!=size )
  {
    throw pfs::Exception( "RGBE: difference in size while reading RLE scanline");
  }

}


void readRadiance( FILE *file, int width, int height, float exposure,
		   pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z )
{
  // read image
  // depending on format read either rle or normal (note: only rle supported)
  Trgbe* scanline = new Trgbe[width*4];

  for( int y=0 ; y<height ; y++ )
  {
    // read rle header
    Trgbe header[4];
    fread(header, sizeof(header), 1, file);
    if( header[0] != 2 || header[1] != 2 || (header[2]<<8) + header[3] != width )
    {
      //--- simple scanline (not rle)
      size_t rez = fread(scanline+4, sizeof(Trgbe), 4*width-4, file);
      if( rez!=4*width-4 )
      {
// 	DEBUG_STR << "RGBE: scanline " << y
// 		  << "(" << (int)rez << "/" << width << ")" <<endl;
        throw pfs::Exception( "RGBE: not enough data to read "
			      "in the simple format." );
      }
      //--- yes, we've read one pixel as a header
      scanline[0]=header[0];
      scanline[1]=header[1];
      scanline[2]=header[2];
      scanline[3]=header[3];

      //--- write scanline to the image
      for( int x=0 ; x<width ; x++ )
      {
	Trgbe_pixel rgbe;
	rgbe.r = scanline[4*x+0];
	rgbe.g = scanline[4*x+1];
	rgbe.b = scanline[4*x+2];
	rgbe.e = scanline[4*x+3];

	rgbe2rgb(rgbe, exposure, (*X)(x,y), (*Y)(x,y), (*Z)(x,y));
      }
    }
    else
    {
      //--- rle scanline

      //--- each channel is encoded separately
      for( int ch=0 ; ch<4 ; ch++ )
	RLERead(file, scanline+width*ch, width);

      //--- write scanline to the image
      for( int x=0 ; x<width ; x++ )
      {
	Trgbe_pixel rgbe;
	rgbe.r = scanline[x+width*0];
	rgbe.g = scanline[x+width*1];
	rgbe.b = scanline[x+width*2];
	rgbe.e = scanline[x+width*3];

	rgbe2rgb(rgbe, exposure, (*X)(x,y), (*Y)(x,y), (*Z)(x,y));
      }
    }
  }
  delete[] scanline;
}




int RLEWrite( FILE* file, Trgbe* scanline, int size )
{
  Trgbe* scanend = scanline + size;
  while( scanline<scanend )
  {
    int run_start=0;
    int peek=0;
    int run_len=0;
    while( run_len<=4 && peek<128 && scanline+peek<scanend)
    {
      run_start=peek;
      run_len=0;
      while( run_len<127
	     && run_start+run_len<128
	     && scanline+peek<scanend
	     && scanline[run_start]==scanline[peek] )
      {
	peek++;
	run_len++;
      }
    }
		
    if( run_len>4 )
    {
      // write a non run: scanline[0] to scanline[run_start]
      if( run_start>0 )
      {
	Trgbe* buf = new Trgbe[run_start+1];
	buf[0] = run_start;
	for( int i=0 ; i<run_start ; i++ )
	  buf[i+1] = scanline[i];
	fwrite(buf, sizeof(Trgbe), run_start+1, file);
	delete[] buf;
      }

      // write a run: scanline[run_start], run_len
      Trgbe buf[2];
      buf[0] = 128+run_len;
      buf[1] = scanline[run_start];
      fwrite(buf, sizeof(*buf), 2, file);
    }
    else
    {
      // write a non run: scanline[0] to scanline[peek]
      Trgbe* buf = new Trgbe[peek+1];
      buf[0] = peek;
      for( int i=0 ; i<peek ; i++ )
	buf[i+1] = scanline[i];
      fwrite(buf, sizeof(Trgbe), peek+1, file);
      delete[] buf;
    }
    scanline += peek;
  }

  if( scanline!=scanend )
  {
    throw pfs::Exception( "RGBE: difference in size while writing RLE scanline");
    return -1;
  }

  return 0;
}



void writeRadiance( FILE *file, pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z )
{
  int width = X->getCols();
  int height = X->getRows();

//   DEBUG_STR << "RGBE: writing image " << width << "x" << height << endl;

  if( Y->getCols() != width || Y->getRows() != height ||
      Z->getCols() != width || Z->getRows() != height )
  {
    throw pfs::Exception( "RGBE: RGB layers have different size");
  }

  // header information
  fprintf(file, "#?RADIANCE\n");	// file format specifier
  fprintf(file, "# PFStools writer to Radiance RGBE format\n");

//    if( exposure_isset )
//      fprintf(file, "EXPOSURE=%f\n", exposure);
//    if( gamma_isset )
//      fprintf(file, "GAMMA=%f\n", gamma);

  fprintf(file, "FORMAT=32-bit_rle_rgbe\n");
  fprintf(file, "\n");

  // image size
  fprintf(file, "-Y %d +X %d\n", height, width);

  // image run length encoded
  Trgbe* scanlineR = new Trgbe[width];
  Trgbe* scanlineG = new Trgbe[width];
  Trgbe* scanlineB = new Trgbe[width];
  Trgbe* scanlineE = new Trgbe[width];

  for( int y=0 ; y<height ; y++ )
  {
    // write rle header
    unsigned char header[4];
    header[0] = 2;
    header[1] = 2;
    header[2] = width >> 8;;
    header[3] = width & 0xFF;
    fwrite(header, sizeof(header), 1, file);

    // each channel is encoded separately
    for( int x=0 ; x<width ; x++ )
    {
      Trgbe_pixel p;
      rgb2rgbe( (*X)(x,y), (*Y)(x,y), (*Z)(x,y), p );
      scanlineR[x] = p.r;
      scanlineG[x] = p.g;
      scanlineB[x] = p.b;
      scanlineE[x] = p.e;
    }
    RLEWrite(file, scanlineR, width);
    RLEWrite(file, scanlineG, width);
    RLEWrite(file, scanlineB, width);
    RLEWrite(file, scanlineE, width);
  }
  delete[] scanlineR;
  delete[] scanlineG;
  delete[] scanlineB;
  delete[] scanlineE;
}
