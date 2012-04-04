/**
 * @file tmo_fattal02.cpp
 * @brief TMO: Gradient Domain High Dynamic Range Compression
 *
 * Implementation of Gradient Domain High Dynamic Range Compression
 * by Raanan Fattal, Dani Lischinski, Michael Werman.
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
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
 * $Id: tmo_fattal02.cpp,v 1.3 2008/11/04 23:43:08 rafm Exp $
 */


#include <iostream>
#include <vector>
#include <algorithm>

#include <math.h>
#include <assert.h>

#include "pde.h"
#include "tmo_fattal02.h"

using namespace std;


//!! TODO: for debugging purposes
// #define PFSEOL "\x0a"
// static void dumpPFS( const char *fileName, const pfs::Array2D *data, const char *channelName )
// {
//   FILE *fh = fopen( fileName, "wb" );
//   assert( fh != NULL );

//   int width = data->getCols();
//   int height = data->getRows();

//   fprintf( fh, "PFS1" PFSEOL "%d %d" PFSEOL "1" PFSEOL "0" PFSEOL
//     "%s" PFSEOL "0" PFSEOL "ENDH", width, height, channelName );

//   for( int y = 0; y < height; y++ )
//     for( int x = 0; x < width; x++ ) {
//       fwrite( &((*data)(x,y)), sizeof( float ), 1, fh );
//     }
  
//   fclose( fh );
// }

//--------------------------------------------------------------------

void downSample(pfs::Array2D* A, pfs::Array2D* B)
{
  const int width = B->getCols();
  const int height = B->getRows();

#pragma omp parallel for
  for( int y=0 ; y<height ; y++ )
    for( int x=0 ; x<width ; x++ )
    {
      float p = 0.0f;
      p += (*A)(2*x,2*y);
      p += (*A)(2*x+1,2*y);
      p += (*A)(2*x,2*y+1);
      p += (*A)(2*x+1,2*y+1);
      (*B)(x,y) = p / 4.0f;
    }	
}
	
void gaussianBlur(pfs::Array2D* I, pfs::Array2D* L)
{
  const int width = I->getCols();
  const int height = I->getRows();

  pfs::Array2D* T = new pfs::Array2D(width,height);

  //--- X blur
#pragma omp parallel for shared(I, T)
  for( int y=0 ; y<height ; y++ )
  {
    for( int x=1 ; x<width-1 ; x++ )
    {
      float t = 2*(*I)(x,y);
      t += (*I)(x-1,y);
      t += (*I)(x+1,y);
      (*T)(x,y) = t/4.0f;
    }
    (*T)(0,y) = ( 3*(*I)(0,y)+(*I)(1,y) ) / 4.0f;
    (*T)(width-1,y) = ( 3*(*I)(width-1,y)+(*I)(width-2,y) ) / 4.0f;
  }

  //--- Y blur
#pragma omp parallel for shared(T, L)
  for( int x=0 ; x<width ; x++ )
  {
    for( int y=1 ; y<height-1 ; y++ )
    {
      float t = 2*(*T)(x,y);
      t += (*T)(x,y-1);
      t += (*T)(x,y+1);
      (*L)(x,y) = t/4.0f;
    }
    (*L)(x,0) = ( 3*(*T)(x,0)+(*T)(x,1) ) / 4.0f;
    (*L)(x,height-1) = ( 3*(*T)(x,height-1)+(*T)(x,height-2) ) / 4.0f;
  }

  delete T;
}

void createGaussianPyramids( pfs::Array2D* H, pfs::Array2D** pyramids, int nlevels, ProgressHelper *ph )
{
  int width = H->getCols();
  int height = H->getRows();
  const int size = width*height;

  pyramids[0] = new pfs::Array2D(width,height);
  for( int i=0 ; i<size ; i++ ) {
	ph->newValue(100*i/size);
    (*pyramids[0])(i) = (*H)(i);
  }

  pfs::Array2D* L = new pfs::Array2D(width,height);
  gaussianBlur( pyramids[0], L );
	
  for( int k=1 ; k<nlevels ; k++ )
  {
	ph->newValue(100*k/nlevels);
	
    width /= 2;
    height /= 2;		
    pyramids[k] = new pfs::Array2D(width,height);
    downSample(L, pyramids[k]);
    
    delete L;
    L = new pfs::Array2D(width,height);
    gaussianBlur( pyramids[k], L );
  }

  delete L;
}

//--------------------------------------------------------------------

float calculateGradients(pfs::Array2D* H, pfs::Array2D* G, int k)
{
  const int width = H->getCols();
  const int height = H->getRows();
  const float divider = pow( 2.0f, k+1 );
  float avgGrad = 0.0f;

#pragma omp parallel for shared(G,H) reduction(+:avgGrad)
  for( int y=0 ; y<height ; y++ )
  {
    for( int x=0 ; x<width ; x++ )
    {
      float gx, gy;
      int w, n, e, s;
      w = (x == 0 ? 0 : x-1);
      n = (y == 0 ? 0 : y-1);
      s = (y+1 == height ? y : y+1);
      e = (x+1 == width ? x : x+1);

      gx = ((*H)(w,y)-(*H)(e,y)) / divider;
        
      gy = ((*H)(x,s)-(*H)(x,n)) / divider;
      
      (*G)(x,y) = sqrt(gx*gx+gy*gy);
      avgGrad += (*G)(x,y);
    }
  }

  return avgGrad / (width*height);
}

//--------------------------------------------------------------------

void upSample(pfs::Array2D* A, pfs::Array2D* B)
{
  const int width = B->getCols();
  const int height = B->getRows();
  const int awidth = A->getCols();
  const int aheight = A->getRows();

#pragma omp parallel for shared(A, B)
  for( int y=0 ; y<height ; y++ )
    for( int x=0 ; x<width ; x++ )
    {
      int ax = x/2;
      int ay = y/2;
      ax = (ax<awidth) ? ax : awidth-1;
      ay = (ay<aheight) ? ay : aheight-1;
      
      (*B)(x,y) = (*A)(ax,ay);
    }
//--- this code below produces 'use of uninitialized value error'
//   int width = A->getCols();
//   int height = A->getRows();
//   int x,y;

//   for( y=0 ; y<height ; y++ )
//     for( x=0 ; x<width ; x++ )
//     {
//       (*B)(2*x,2*y) = (*A)(x,y);
//       (*B)(2*x+1,2*y) = (*A)(x,y);
//       (*B)(2*x,2*y+1) = (*A)(x,y);
//       (*B)(2*x+1,2*y+1) = (*A)(x,y);
//     }	
}

void calculateFiMatrix(pfs::Array2D* FI, pfs::Array2D* gradients[],
  float avgGrad[], int nlevels,
  float alfa, float beta, float noise, bool newfattal)
{
  int width = gradients[nlevels-1]->getCols();
  int height = gradients[nlevels-1]->getRows();
  pfs::Array2D** fi = new pfs::Array2D*[nlevels];

  fi[nlevels-1] = new pfs::Array2D(width,height);
  if (newfattal)
#pragma omp parallel for shared(fi)
	for( int k=0 ; k<width*height ; k++ )
    	(*fi[nlevels-1])(k) = 1.0f;
  
  for( int k=nlevels-1 ; k>=0 ; k-- )
  {
    width = gradients[k]->getCols();
    height = gradients[k]->getRows();

#pragma omp parallel for shared(fi,avgGrad)
    for( int y=0 ; y<height ; y++ )
      for( int x=0 ; x<width ; x++ )
      {
        float grad = (*gradients[k])(x,y);
        float a = alfa * avgGrad[k];

        float value=1.0;
        if( grad>1e-4 )
          value = a/(grad+noise) * pow((grad+noise)/a, beta);
		if (newfattal)
        	(*fi[k])(x,y) *= value;
		else
			(*fi[k])(x,y) = value;
      }
		
    // create next level
    if( k>1 )
    {
      width = gradients[k-1]->getCols();
      height = gradients[k-1]->getRows();
      fi[k-1] = new pfs::Array2D(width,height);
    }
    else
      fi[0] = FI;               // highest level -> result

    if( k>0  && newfattal )
    {
      upSample(fi[k], fi[k-1]);		// upsample to next level
      gaussianBlur(fi[k-1],fi[k-1]);
    }
  }
	
  for( int k=1 ; k<nlevels ; k++ )
    delete fi[k];
  delete[] fi;
}

//--------------------------------------------------------------------


static void findMaxMinPercentile(pfs::Array2D* I,
                                 float minPrct, float& minLum,
                                 float maxPrct, float& maxLum)
{
  int size = I->getRows() * I->getCols();
  std::vector<float> vI;

  for( int i=0 ; i<size ; i++ )
    if( (*I)(i)!=0.0f )
      vI.push_back((*I)(i));
      
  std::sort(vI.begin(), vI.end());

  minLum = vI.at( int(minPrct*vI.size()) );
  maxLum = vI.at( int(maxPrct*vI.size()) );
}

//--------------------------------------------------------------------

Fattal02::Fattal02(unsigned int width, unsigned int height,
                  const float* nY, float* nL,
                  float alfa, float beta, float noise, bool newfattal,
                  ProgressHelper *ph) :
m_width(width),
m_height(height),
m_Y(new pfs::Array2D(width, height, const_cast<float*>(nY))),
m_L(new pfs::Array2D(width, height, nL)),
m_alpha(alfa),
m_beta(beta),
m_noise(noise),
m_newfattal(newfattal),
m_ph(ph)
{

}

Fattal02::~Fattal02()
{
  // clean up
  delete m_H;
  for(int i=0 ; i<m_nlevels ; i++ )
  {
    delete m_pyramids[i];
    delete m_gradients[i];
  }
  delete[] m_pyramids;
  delete[] m_gradients;
  delete[] m_avgGrad;
  delete m_FI;
  delete m_Gx;
  delete m_Gy;
  delete m_DivG;
  delete m_U;

  delete m_L;
  delete m_Y;
}

void Fattal02::tmo_fattal02()
{
  const int MSIZE = 32;         // minimum size of gaussian pyramid
	
  m_size = m_width*m_height;

  // find max & min values, normalize to range 0..100 and take logarithm
  m_minLum = (*m_Y)(0,0);
  m_maxLum = (*m_Y)(0,0);
  for(int i=0 ; i<m_size ; i++ )
  {
    m_minLum = ( (*m_Y)(i)<m_minLum ) ? (*m_Y)(i) : m_minLum;
    m_maxLum = ( (*m_Y)(i)>m_maxLum ) ? (*m_Y)(i) : m_maxLum;
  }
  m_H = new pfs::Array2D(m_width, m_height);
//#pragma omp parallel for private(i) shared(m_H, m_Y, m_maxLum)
  for(int i=0 ; i<m_size ; i++ )
    (*m_H)(i) = log( 100.0f*(*m_Y)(i)/m_maxLum + 1e-4 );

  // create gaussian pyramids
  m_mins = (m_width<m_height) ? m_width : m_height;	// smaller dimension
  m_nlevels = 0;
  while( m_mins>=MSIZE )
  {
    m_nlevels++;
    m_mins /= 2;
  }
  // std::cout << "DEBUG: nlevels = " << nlevels << ", mins = " << mins << std::endl;
  // The following lines solves a bug with images particularly small
  if (m_nlevels == 0) m_nlevels = 1;

  m_pyramids = new pfs::Array2D*[m_nlevels];
  createGaussianPyramids(m_H, m_pyramids, m_nlevels, m_ph);

  // calculate gradients and its average values on pyramid levels
  m_gradients = new pfs::Array2D*[m_nlevels];
  m_avgGrad = new float[m_nlevels];
  for( int k=0 ; k<m_nlevels ; k++ )
  {
    m_gradients[k] = new pfs::Array2D(m_pyramids[k]->getCols(), m_pyramids[k]->getRows());
    m_avgGrad[k] = calculateGradients(m_pyramids[k], m_gradients[k], k);
  }

  // calculate fi matrix
  m_FI = new pfs::Array2D(m_width, m_height);
  calculateFiMatrix(m_FI, m_gradients, m_avgGrad, m_nlevels, m_alpha, m_beta, m_noise, m_newfattal);

//  dumpPFS( "FI.pfs", FI, "Y" );

  // attenuate gradients
  m_Gx = new pfs::Array2D(m_width, m_height);
  m_Gy = new pfs::Array2D(m_width, m_height);
  //#pramga omp parallel for shared(Gx, Gy, H, FI, ph)
  //FIXME: Only update ph in thread 0
  for(unsigned int  y=0 ; y<m_height ; y++ ) {
	m_ph->newValue(100*y/m_height);
	if (m_ph->isTerminationRequested())
	  break; 
    
    for(unsigned int x=0 ; x<m_width ; x++ )
    {
      int s, e;
      s = (y+1 == m_height ? y : y+1);
      e = (x+1 == m_width ? x : x+1);

      (*m_Gx)(x,y) = ((*m_H)(e,y)-(*m_H)(x,y)) * (*m_FI)(x,y);        
      (*m_Gy)(x,y) = ((*m_H)(x,s)-(*m_H)(x,y)) * (*m_FI)(x,y);      
    }
  }
//   dumpPFS( "Gx.pfs", Gx, "Y" );
//   dumpPFS( "Gy.pfs", Gy, "Y" );
  
  // calculate divergence
  m_DivG = new pfs::Array2D(m_width, m_height);
  //#pragma omp parallel for shared(ph, DivG, Gx, Gy)
  //FIXME: Only update ph in thread 0
  for(unsigned int y=0 ; y<m_height ; y++ ) {
	m_ph->newValue(100*y/m_height);
	if (m_ph->isTerminationRequested())
	  break; 
    
    for(unsigned int x=0 ; x<m_width ; x++ )
    {
      (*m_DivG)(x,y) = (*m_Gx)(x,y) + (*m_Gy)(x,y);
      if( x > 0 ) (*m_DivG)(x,y) -= (*m_Gx)(x-1,y);
      if( y > 0 ) (*m_DivG)(x,y) -= (*m_Gy)(x,y-1);
    }
  }

//  dumpPFS( "DivG.pfs", DivG, "Y" );
  
  // solve pde and exponentiate (ie recover compressed image)
  m_U = new pfs::Array2D(m_width, m_height);
  solve_pde_multigrid( m_DivG, m_U );

  for(unsigned int y=0 ; y<m_height ; y++ ) {
	m_ph->newValue(100*y/m_height);
	if (m_ph->isTerminationRequested())
	  break; 
    for(unsigned int x=0 ; x<m_width ; x++ )
      (*m_L)(x,y) = exp( (*m_U)(x,y) ) - 1e-4;
  }
	
  // remove percentile of min and max values and renormalize
  findMaxMinPercentile(m_L, 0.001f, m_minLum, 0.995f, m_maxLum);
  m_maxLum -= m_minLum;
  for(unsigned int y=0 ; y<m_height ; y++ ) {
	m_ph->newValue(100*y/m_height);
	if (m_ph->isTerminationRequested())
	  break; 
    for(unsigned int x=0 ; x<m_width ; x++ )
    {
      (*m_L)(x,y) = ((*m_L)(x,y)-m_minLum) / m_maxLum;
      if( (*m_L)(x,y)<=0.0f )
        (*m_L)(x,y) = 1e-4f;
    }
  }

}
