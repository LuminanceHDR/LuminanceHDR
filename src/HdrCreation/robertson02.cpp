/**
 * @brief Robertson02 algorithm for automatic self-calibration.
 *
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2004 Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
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
 * @author Grzegorz Krawczyk, <gkrawczyk@users.sourceforge.net>
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * $Id: robertson02.cpp,v 1.7 2006/11/16 15:06:17 gkrawczyk Exp $
 */

#include <iostream>
#include <vector>

#include <math.h>

#include "responses.h"
#include "robertson02.h"

#include "generic_applyResponse.h"

#define PROG_NAME "robertson02"

// maximum iterations after algorithm accepts local minima
#define MAXIT 500

// maximum accepted error
#define MAX_DELTA 1e-5f

float normalizeI( float* I, int M );

float robertson02_sum(float weight, float, float t, float response) {
	return weight * t * response;
}

float robertson02_div(float weight, float, float t, float) {
	return weight * t * t;
}

float robertson02_out(float quotient) {
	return quotient;
}

void robertson02_applyResponse(pfs::Array2D* Rout, pfs::Array2D* Gout, pfs::Array2D* Bout, const float * arrayofexptime, const float* Ir,  const float* Ig, const float* Ib, const float* w, const int M, const bool ldrinput, ...) {
	QList<QImage*> *listldr=NULL;
	Array2DList *listhdrR=NULL;
	Array2DList *listhdrG=NULL;
	Array2DList *listhdrB=NULL;
	va_list arg_pointer;
	va_start(arg_pointer,ldrinput); /* Initialize the argument list. */

	if (ldrinput) {
		listldr=va_arg(arg_pointer,QList<QImage*>*);
	} else {
		listhdrR=va_arg(arg_pointer,Array2DList*);
		listhdrG=va_arg(arg_pointer,Array2DList*);
		listhdrB=va_arg(arg_pointer,Array2DList*);
	}
	va_end(arg_pointer); /* Clean up. */

	generic_applyResponse(
	    &robertson02_sum, &robertson02_div, &robertson02_out, 
	    Rout, Gout, Bout, arrayofexptime, Ir,  Ig, Ib, w,  M, ldrinput,
	    listldr, listhdrR, listhdrG, listhdrB
	);
}

////////////////////////////////     GET RESPONSE    /////////////////////////////////////
void robertson02_getResponse( pfs::Array2D* Rout,pfs::Array2D* Gout,pfs::Array2D* Bout, const float * arrayofexptime, float* Ir, float* Ig, float* Ib, const float* w, const int M, const bool ldrinput, ... )
{
va_list arg_pointer;
va_start(arg_pointer,ldrinput); /* Initialize the argument list. */
QList<QImage*> *listldr=NULL;
Array2DList *listhdrR=NULL;
Array2DList *listhdrG=NULL;
Array2DList *listhdrB=NULL;
int N=-1; int width=-1; int height=-1;

if (ldrinput) {
	listldr=va_arg(arg_pointer,QList<QImage*>*);
	// number of exposures
	N = listldr->count();
	// frame size
	width = (listldr->at(0))->width();
	height = (listldr->at(0))->height();
} else {
	listhdrR=va_arg(arg_pointer,Array2DList*);
	listhdrG=va_arg(arg_pointer,Array2DList*);
	listhdrB=va_arg(arg_pointer,Array2DList*);
	// number of exposures
	N = listhdrR->size();
	// frame size
	width= ((*listhdrR)[0])->getCols();
	height=((*listhdrR)[0])->getRows();
}
va_end(arg_pointer); /* Clean up. */


  // indexes
  int i,j,mR,mG,mB;

  float* Iprev_R = new float[M]; // previous response
  float* Iprev_G = new float[M]; // previous response
  float* Iprev_B = new float[M]; // previous response
  if( Iprev_R==NULL || Iprev_G==NULL || Iprev_B==NULL ) {
    std::cerr << "robertson02: could not allocate memory for camera response" << std::endl;
    exit(1);
  }

  // 0. Initialization
  normalizeI( Ir, M );
  normalizeI( Ig, M );
  normalizeI( Ib, M );
  for( int i=0 ; i<M ; i++ ) {
    Iprev_R[i] = Ir[i];
    Iprev_G[i] = Ig[i];
    Iprev_B[i] = Ib[i];
  }

  if (ldrinput)
  robertson02_applyResponse( Rout,Gout,Bout, arrayofexptime, Ir,Ig,Ib, w, M, true, listldr );
  else
  robertson02_applyResponse( Rout,Gout,Bout, arrayofexptime, Ir,Ig,Ib, w, M, false, listhdrR,listhdrG,listhdrB );

  // Optimization process
  bool converged=false;
  long* cardEm_R = new long[M];
  long* cardEm_G = new long[M];
  long* cardEm_B = new long[M];
  float* sum_R   = new float[M];
  float* sum_G   = new float[M];
  float* sum_B   = new float[M];
  if( sum_R==NULL || cardEm_R==NULL || sum_G==NULL || cardEm_G==NULL || sum_B==NULL || cardEm_B==NULL )
  {
    std::cerr << "robertson02: could not allocate memory for optimization process" << std::endl;
    exit(1);
  }

  int cur_it = 0;
  float pdeltaR= 0.0f;
  float pdeltaG= 0.0f;
  float pdeltaB= 0.0f;
  while( !converged )
  {
    // 1. Minimize with respect to I
    for( i=0 ; i<M ; i++ ) {
      cardEm_R[i]=0;
      cardEm_G[i]=0;
      cardEm_B[i]=0;
      sum_R[i]=0.0f;
      sum_G[i]=0.0f;
      sum_B[i]=0.0f;
    }

    for( i=0 ; i<N ; i++ ) {
      float ti = arrayofexptime[i];

      if (ldrinput) {
	for( j=0 ; j<width*height ; j++ )  {
		mR = qRed  (* ( (QRgb*)( (listldr->at(i) )->bits() ) + j ) );
		mG = qGreen(* ( (QRgb*)( (listldr->at(i) )->bits() ) + j ) );
		mB = qBlue (* ( (QRgb*)( (listldr->at(i) )->bits() ) + j ) );
		sum_R[mR] += ti * (*Rout)(j);
		sum_G[mG] += ti * (*Gout)(j);
		sum_B[mB] += ti * (*Bout)(j);
		cardEm_R[mR]++;
		cardEm_G[mG]++;
		cardEm_B[mB]++;
	}
      } else {
	for( j=0 ; j<width*height ; j++ )  {
		mR = (int) ( ( *( ( (*listhdrR)[i] ) ) ) (j) );
		mG = (int) ( ( *( ( (*listhdrG)[i] ) ) ) (j) );
		mB = (int) ( ( *( ( (*listhdrB)[i] ) ) ) (j) );
		sum_R[mR] += ti * (*Rout)(j);
		sum_G[mG] += ti * (*Gout)(j);
		sum_B[mB] += ti * (*Bout)(j);
		cardEm_R[mR]++;
		cardEm_G[mG]++;
		cardEm_B[mB]++;
	}
      }
    }

    for( int i=0 ; i<M ; i++ ) {
      if( cardEm_R[i]!=0 )
	Ir[i] = sum_R[i] / cardEm_R[i];
      else
	Ir[i] = 0.0f;
      if( cardEm_G[i]!=0 )
	Ig[i] = sum_G[i] / cardEm_G[i];
      else
	Ig[i] = 0.0f;
      if( cardEm_B[i]!=0 )
	Ib[i] = sum_B[i] / cardEm_B[i];
      else
	Ib[i] = 0.0f;
    }
    // 2. Normalize I
    normalizeI( Ir, M );
    normalizeI( Ig, M );
    normalizeI( Ib, M );

    // 3. Apply new response
    if (ldrinput)
	robertson02_applyResponse( Rout,Gout,Bout, arrayofexptime, Ir,Ig,Ib, w, M, true, listldr );
    else
	robertson02_applyResponse( Rout,Gout,Bout, arrayofexptime, Ir,Ig,Ib, w, M, false, listhdrR,listhdrG,listhdrB );

    // 4. Check stopping condition
    float deltaR = 0.0f;
    float deltaG = 0.0f;
    float deltaB = 0.0f;
    int hitsR=0;
    int hitsG=0;
    int hitsB=0;
    for( int i=0 ; i<M ; i++ ) {
      if( Ir[i]!=0.0f )
      {
	float diff = Ir[i]-Iprev_R[i];
	deltaR += diff * diff;
	Iprev_R[i] = Ir[i];
	hitsR++;
      }
      if( Ig[i]!=0.0f )
      {
	float diff = Ig[i]-Iprev_G[i];
	deltaG += diff * diff;
	Iprev_G[i] = Ig[i];
	hitsG++;
      }
      if( Ib[i]!=0.0f )
      {
	float diff = Ib[i]-Iprev_B[i];
	deltaB += diff * diff;
	Iprev_B[i] = Ib[i];
	hitsB++;
      }
    }
    deltaR /= hitsR;
    deltaG /= hitsG;
    deltaB /= hitsB;

    std::cerr << " #" << cur_it
                << " deltaR=" << deltaR
                << " (coverage: " << 100*hitsR/M << "%)\n"
                << " deltaG=" << deltaG
                << " (coverage: " << 100*hitsG/M << "%)\n"
                << " deltaB=" << deltaB
                << " (coverage: " << 100*hitsB/M << "%)\n";
    
    if (( deltaR < MAX_DELTA ) && ( deltaG < MAX_DELTA ) && ( deltaB < MAX_DELTA ) )
      converged=true;
    else if( isnan(deltaR) || isnan(deltaG) || isnan(deltaB) || (cur_it>MAXIT && (pdeltaR<deltaR || pdeltaG<deltaG ||pdeltaB<deltaB) ) )
    {
	std::cerr << "algorithm failed to converge, too noisy data in range\n";
      break;
    }

    pdeltaR = deltaR;
    pdeltaG = deltaG;
    pdeltaB = deltaB;
    cur_it++;
  }

  if( converged )
    std::cerr << " #" << cur_it
                << " deltaR=" << pdeltaR
                << " deltaG=" << pdeltaG
                << " deltaB=" << pdeltaB
                << " <- converged\n";

  delete[] Iprev_R;
  delete[] cardEm_R;
  delete[] sum_R;
  delete[] Iprev_G;
  delete[] cardEm_G;
  delete[] sum_G;
  delete[] Iprev_B;
  delete[] cardEm_B;
  delete[] sum_B;
  
}


/////////////////////////////////////////////////////////////////////////////////
// private part

float normalizeI( float* I, int M )
{
  int Mmin, Mmax;
  // find min max
  for( Mmin=0 ; Mmin<M && I[Mmin]==0 ; Mmin++ );
  for( Mmax=M-1 ; Mmax>0 && I[Mmax]==0 ; Mmax-- );
  
  int Mmid = Mmin+(Mmax-Mmin)/2;
  float mid = I[Mmid];

//   std::cerr << "robertson02: middle response, mid=" << mid
//             << " [" << Mmid << "]"
//             << " " << Mmin << ".." << Mmax << std::endl;
  
  if( mid==0.0f )
  {
    // find first non-zero middle response
    while( Mmid<Mmax && I[Mmid]==0.0f )
      Mmid++;
    mid = I[Mmid];
  }

  if( mid!=0.0f )
    for( int m=0 ; m<M ; m++ )
      I[m] /= mid;
  return mid;
}

