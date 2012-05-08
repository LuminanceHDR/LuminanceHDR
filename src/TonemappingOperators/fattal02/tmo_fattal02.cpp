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


#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>

#include <math.h>
#include <assert.h>

#include "TonemappingOperators/pfstmo.h"
#include "Common/ProgressHelper.h"
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

// Note, I've uncommented all omp directives. They are all ok but are
// applied to too small problems and in total don't lead to noticable
// speed improvements. The main issue is the pde solver and in case of the
// fft solver uses optimised threaded fftw routines.
//#pragma omp parallel for
  for( int y=0 ; y<height ; y++ ) {
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
}
	
void gaussianBlur(pfs::Array2D* I, pfs::Array2D* L)
{
  const int width = I->getCols();
  const int height = I->getRows();

  pfs::Array2D* T = new pfs::Array2D(width,height);

  //--- X blur
//#pragma omp parallel for shared(I, T)
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
//#pragma omp parallel for shared(T, L)
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

void createGaussianPyramids( pfs::Array2D* H, pfs::Array2D** pyramids, int nlevels)
{
  int width = H->getCols();
  int height = H->getRows();
  const int size = width*height;

  pyramids[0] = new pfs::Array2D(width,height);
//#pragma omp parallel for shared(pyramids, H)
  for( int i=0 ; i<size ; i++ )
    (*pyramids[0])(i) = (*H)(i);

  pfs::Array2D* L = new pfs::Array2D(width,height);
  gaussianBlur( pyramids[0], L);
	
  for( int k=1 ; k<nlevels ; k++ )
  {
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

//#pragma omp parallel for shared(G,H) reduction(+:avgGrad)
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
      // note this implicitely assumes that H(-1)=H(0)
      // for the fft-pde slover this would need adjustment as H(-1)=H(1)
      // is assumed, which means gx=0.0, gy=0.0 at the boundaries
      // however, the impact is not visible so we ignore this here
      
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

//#pragma omp parallel for shared(A, B)
  for( int y=0 ; y<height ; y++ ) {
    for( int x=0 ; x<width ; x++ )
    {
      int ax = x/2;
      int ay = y/2;
      ax = (ax<awidth) ? ax : awidth-1;
      ay = (ay<aheight) ? ay : aheight-1;
      
      (*B)(x,y) = (*A)(ax,ay);
    }
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
  float avgGrad[], int nlevels, int detail_level,
  float alfa, float beta, float noise, bool newfattal)
{
  int width = gradients[nlevels-1]->getCols();
  int height = gradients[nlevels-1]->getRows();
  pfs::Array2D** fi = new pfs::Array2D*[nlevels];

  fi[nlevels-1] = new pfs::Array2D(width,height);
  if (newfattal){
//#pragma omp parallel for shared(fi)
    for( int k=0 ; k<width*height ; k++ )
      (*fi[nlevels-1])(k) = 1.0f;
  }
  
  for( int k=nlevels-1 ; k>=0 ; k-- )
  {
    width = gradients[k]->getCols();
    height = gradients[k]->getRows();

    // only apply gradients to levels>=detail_level but at least to the coarsest
    if(k>=detail_level || k==nlevels-1 || newfattal==false)
    {    
      //DEBUG_STR << "calculateFiMatrix: apply gradient to level " << k << endl;
//#pragma omp parallel for shared(fi,avgGrad)
      for( int y=0 ; y<height ; y++ )
        for( int x=0 ; x<width ; x++ )
        {
          float grad = (*gradients[k])(x,y);
          float a = alfa * avgGrad[k];

          float value=1.0;
          if( grad<1e-4 )
            grad=1e-4;

          value = pow((grad+noise)/a, beta-1.0f);
          
          if (newfattal)
            (*fi[k])(x,y) *= value;
          else
	    (*fi[k])(x,y) = value;
        }
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

  for( int i=0 ; i<size ; i++ ) {
    vI.push_back((*I)(i));
  }
      
  std::sort(vI.begin(), vI.end());

  minLum = vI.at( int(minPrct*vI.size()) );
  maxLum = vI.at( int(maxPrct*vI.size()) );
}

//--------------------------------------------------------------------

void tmo_fattal02(unsigned int width, unsigned int height,
                  const float* nY, float* nL, 
                  float alfa, float beta, float noise, bool newfattal, 
                  ProgressHelper *ph)
{

  bool fftsolver=true;
  float black_point=0.1;
  float white_point=0.5;
  float gamma=0.8;
  int detail_level=3;

  ph->newValue(2);
  if (ph->isTerminationRequested()) return;

  const pfs::Array2D* Y = new pfs::Array2D(width, height, const_cast<float*>(nY));

  int MSIZE = 32;         // minimum size of gaussian pyramid
  // I believe a smaller value than 32 results in slightly better overall
  // quality but I'm only applying this if the newly implemented fft solver
  // is used in order not to change behaviour of the old version
  // TODO: best let the user decide this value
  if(fftsolver)
     MSIZE=8;
	
  int size = width*height;
  unsigned int x,y;
  int i, k;

  // find max & min values, normalize to range 0..100 and take logarithm
  float minLum = (*Y)(0,0);
  float maxLum = (*Y)(0,0);
  for( i=0 ; i<size ; i++ )
  {
    minLum = ( (*Y)(i)<minLum ) ? (*Y)(i) : minLum;
    maxLum = ( (*Y)(i)>maxLum ) ? (*Y)(i) : maxLum;
  }
  pfs::Array2D* H = new pfs::Array2D(width, height);
//#pragma omp parallel for private(i) shared(H, Y, maxLum)
  for( i=0 ; i<size ; i++ )
    (*H)(i) = log( 100.0f*(*Y)(i)/maxLum + 1e-4 );
  delete Y;
  ph->newValue(4); 

  // create gaussian pyramids
  int mins = (width<height) ? width : height;	// smaller dimension
  int nlevels = 0;
  while( mins>=MSIZE )
  {
    nlevels++;
    mins /= 2;
  }
  // std::cout << "DEBUG: nlevels = " << nlevels << ", mins = " << mins << std::endl;
  // The following lines solves a bug with images particularly small
  if (nlevels == 0) nlevels = 1;

  pfs::Array2D** pyramids = new pfs::Array2D*[nlevels];
  createGaussianPyramids(H, pyramids, nlevels);
  ph->newValue(8);

  // calculate gradients and its average values on pyramid levels
  pfs::Array2D** gradients = new pfs::Array2D*[nlevels];
  float* avgGrad = new float[nlevels];
  for( k=0 ; k<nlevels ; k++ )
  {
    gradients[k] = new pfs::Array2D(pyramids[k]->getCols(), pyramids[k]->getRows());
    avgGrad[k] = calculateGradients(pyramids[k],gradients[k], k);
  }
  ph->newValue(12);

  // calculate fi matrix
  pfs::Array2D* FI = new pfs::Array2D(width, height);
  calculateFiMatrix(FI, gradients, avgGrad, nlevels, detail_level, alfa, beta, noise, newfattal);
//  dumpPFS( "FI.pfs", FI, "Y" );
  for( i=0 ; i<nlevels ; i++ )
  {
    delete pyramids[i];
    delete gradients[i];
  }
  delete[] pyramids;
  delete[] gradients;
  delete[] avgGrad;
  ph->newValue(16);
  if (ph->isTerminationRequested()){
    delete FI;
    delete H;
    return;
  }


  // attenuate gradients
  pfs::Array2D* Gx = new pfs::Array2D(width, height);
  pfs::Array2D* Gy = new pfs::Array2D(width, height);

  // the fft solver solves the Poisson pde but with slightly different
  // boundary conditions, so we need to adjust the assembly of the right hand
  // side accordingly (basically fft solver assumes U(-1) = U(1), whereas zero
  // Neumann conditions assume U(-1)=U(0)), see also divergence calculation
  if(fftsolver)
    for( y=0 ; y<height ; y++ )
      for( x=0 ; x<width ; x++ )
      {
        // sets index+1 based on the boundary assumption H(N+1)=H(N-1)
        unsigned int yp1 = (y+1 >= height ? height-2 : y+1);
        unsigned int xp1 = (x+1 >= width ?  width-2  : x+1);
        // forward differences in H, so need to use between-points approx of FI
        (*Gx)(x,y) = ((*H)(xp1,y)-(*H)(x,y)) * 0.5*((*FI)(xp1,y)+(*FI)(x,y));
        (*Gy)(x,y) = ((*H)(x,yp1)-(*H)(x,y)) * 0.5*((*FI)(x,yp1)+(*FI)(x,y));
      }
  else
    for( y=0 ; y<height ; y++ )
      for( x=0 ; x<width ; x++ )
      {
        int s, e;
        s = (y+1 == height ? y : y+1);
        e = (x+1 == width ? x : x+1);

        (*Gx)(x,y) = ((*H)(e,y)-(*H)(x,y)) * (*FI)(x,y);        
        (*Gy)(x,y) = ((*H)(x,s)-(*H)(x,y)) * (*FI)(x,y);      
      }
  delete H;
  delete FI;
  ph->newValue(18);


//   dumpPFS( "Gx.pfs", Gx, "Y" );
//   dumpPFS( "Gy.pfs", Gy, "Y" );
  

  // calculate divergence
  pfs::Array2D* DivG = new pfs::Array2D(width, height);
  for( y=0 ; y<height ; y++ )
    for( x=0 ; x<width ; x++ )
    {
      (*DivG)(x,y) = (*Gx)(x,y) + (*Gy)(x,y);
      if( x > 0 ) (*DivG)(x,y) -= (*Gx)(x-1,y);
      if( y > 0 ) (*DivG)(x,y) -= (*Gy)(x,y-1);

      if(fftsolver)
      {
        if(x==0) (*DivG)(x,y) += (*Gx)(x,y);
        if(y==0) (*DivG)(x,y) += (*Gy)(x,y);
      }

    }
  delete Gx;
  delete Gy;
  ph->newValue(20);
  if (ph->isTerminationRequested()){
    delete DivG;
    return;
  }


//  dumpPFS( "DivG.pfs", DivG, "Y" );
  
  // solve pde and exponentiate (ie recover compressed image)
  pfs::Array2D* U = new pfs::Array2D(width, height);
  if(fftsolver){
    solve_pde_fft(DivG, U, ph);
  } else {
    solve_pde_multigrid( DivG, U, ph);
  }
  printf("pde residual error: %f\n", residual_pde(U, DivG));
  delete DivG;
  ph->newValue(90); 
  if (ph->isTerminationRequested()){
    delete U;
    return;
  }

  pfs::Array2D* L = new pfs::Array2D(width, height, nL);
  for( y=0 ; y<height ; y++ ) {
    for( x=0 ; x<width ; x++ )
      (*L)(x,y) = exp( gamma * (*U)(x,y) ); 
  }
  delete U;
  ph->newValue(95); 
	
  // remove percentile of min and max values and renormalize
  float cut_min=0.01f*black_point;
  float cut_max=1.0f-0.01f*white_point;
  assert(cut_min>=0.0f && (cut_max<=1.0f) && (cut_min<cut_max));
  findMaxMinPercentile(L, cut_min, minLum, cut_max, maxLum);
  for( y=0 ; y<height ; y++ ) {
    for( x=0 ; x<width ; x++ )
    {
      (*L)(x,y) = ((*L)(x,y)-minLum) / (maxLum-minLum);
      if( (*L)(x,y)<=0.0f )
        (*L)(x,y) = 0.0;
      // note, we intentionally do not cut off values > 1.0
    }
  }
  delete L;
  ph->newValue(96); 

}
