/**
 * @file pde.cpp
 * @brief Solving Partial Differential Equations
 *
 * Full Multigrid Algorithm and Successive Overrelaxation.
 *
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * Some code from Numerical Recipes in C
 *
 * This file is a part of Qtpfsgui package, based on pfstmo.
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
 * $Id: pde.cpp,v 1.7 2005/11/29 18:36:55 aefremov Exp $
 */

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../../Libpfs/array2d.h"
#include "pde.h"

using namespace std;

//////////////////////////////////////////////////////////////////////

#define MODYF 0 /* 1 or 0 (1 is better) */
#define MINS 16	/* minimum size 4 6 or 100 */

//#define MODYF_SQRT -1.0f /* -1 or 0 */
#define SMOOTH_IT 1 /* minimum 1  */
#define BCG_STEPS 20
#define V_CYCLE 2 /* number of v-cycles  */

// precision
#define EPS 1.0e-8

void linbcg(unsigned int n, float b[], float x[], int itol, float tol,
  int itmax, int *iter, float *err);

inline float max( float a, float b )
{
  return a > b ? a : b;
}

inline float min( float a, float b )
{
  return a < b ? a : b;
}


//!!: for debugging purposes
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

//////////////////////////////////////////////////////////////////////
// Full Multigrid Algorithm for solving partial differential equations
//////////////////////////////////////////////////////////////////////

void restrict( const pfs::Array2D *in, pfs::Array2D *out )
{
  const float inRows = in->getRows();
  const float inCols = in->getCols();

  const int outRows = out->getRows();
  const int outCols = out->getCols();

  const float dx = (float)in->getCols() / (float)out->getCols();
  const float dy = (float)in->getRows() / (float)out->getRows();

  const float filterSize = 0.5f;
  
  float sx, sy;
  int x, y;
  
  for( y = 0, sy = dy/2-0.5f; y < outRows; y++, sy += dy )
    for( x = 0, sx = dx/2-0.5f; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float w = 0;
      for( float ix = max( 0, ceilf( sx-dx*filterSize ) ); ix <= min( floorf( sx+dx*filterSize ), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-dx*filterSize ) ); iy <= min( floorf( sy+dx*filterSize), inRows-1 ); iy++ ) {
          pixVal += (*in)( (int)ix, (int)iy );
          w += 1;
        }     
      (*out)(x,y) = pixVal/w;      
    }
}

// from_level>to_level, from_size>to_size
// void restrict( pfs::Array2D *F, pfs::Array2D *T )
// {
// //   DEBUG_STR << "restrict" << endl;
  
//   int sxt = T->getCols();
//   int syt = T->getRows();
//   int sxf = F->getCols();
//   int syf = F->getRows();
//   int x,y;

//   float w[] = {.25, .5, .25};
//   for( int x=0 ; x<sxt ; x++ )
//     for( int y=0 ; y<syt ; y++ )
//     {
//       float sum=0.0; float norm=0.0f;
//       for( int m=-1 ; m<=1 ; m++ )
// 	for( int n=-1 ; n<=1 ; n++ )
//         {
//           int xf = 2*x+m;
//           int yf = 2*y+n;
//           if( xf>=0 && yf>=0 && xf<sxf && yf<syf )
//           {
//             sum += w[1+m]*w[1+n] * (*F)(xf,yf);
//             norm += w[1+m]*w[1+n];
//           }
//         }
//       assert(norm!=0.0f);
//       (*T)(x,y)=sum/norm;
//     }
// }


void prolongate( const pfs::Array2D *in, pfs::Array2D *out )
{
  float dx = (float)in->getCols() / (float)out->getCols();
  float dy = (float)in->getRows() / (float)out->getRows();

  float pad;
  
  /*float filterSamplingX = */max( modff( dx, &pad ), 0.01f );
  /*float filterSamplingY = */max( modff( dy, &pad ), 0.01f );

  const int outRows = out->getRows();
  const int outCols = out->getCols();

  const float inRows = in->getRows();
  const float inCols = in->getCols();

  const float filterSize = 1;

  float sx, sy;
  int x, y;
  for( y = 0, sy = -dy/2; y < outRows; y++, sy += dy )
    for( x = 0, sx = -dx/2; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float weight = 0;
      
      for( float ix = max( 0, ceilf( sx-filterSize ) ); ix <= min( floorf(sx+filterSize), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-filterSize ) ); iy <= min( floorf( sy+filterSize), inRows-1 ); iy++ ) {
          float fx = fabsf( sx - ix );
          float fy = fabsf( sy - iy );

          const float fval = (1-fx)*(1-fy);
          
          pixVal += (*in)( (int)ix, (int)iy ) * fval;
          weight += fval;
        }
      
      assert( weight != 0 );
      (*out)(x,y) = pixVal / weight;

    } 
}

// to_level<from_level, from_size<to_size
void prolongate_old( pfs::Array2D *F, pfs::Array2D *T )
{
//   DEBUG_STR << "prolongate" << endl;

  int sxt = T->getCols();
  int syt = T->getRows();
  int sxf = F->getCols();
  int syf = F->getRows();
  int x,y;

  // elements that are copies
  for( x=0 ; x<sxf ; x++ )
    for( y=0 ; y<syf ; y++ )
    {
      int x2 = x*2;            
      int y2 = y*2;
      if(x2<sxt && y2<syt)
        (*T)(x2,y2) = (*F)(x,y);
    }

  // For odd number of dest cols/rows copy the last source col/row
  if( sxt & 1 ) {
    for( y=0 ; y<syf ; y++ ) {
      int y2 = y*2;
      if(y2<syt)
        (*T)(sxt-1,y2) = (*F)(sxf-1,y);
    }
  }
  if( syt & 1 ) {
    for( x=0 ; x<sxf ; x++ ) {
      int x2 = x*2;            
      if(x2<sxt)
        (*T)(x2,syt-1) = (*F)(x,syf-1);
    }
  }
  if( (sxt & 1) && (syt & 1) )
    (*T)(sxt-1,syt-1) = (*F)(sxf-1,syf-1);

  // even columns interpolated horizontally (every second row)
  for( x=1 ; x<sxt ; x+=2 )
    for( y=0 ; y<syt ; y+=2 )
    {
      int xp1 = x+1;
      int xm1 = x-1;
      if( xp1>=sxt )
        xp1 = xm1;
      (*T)(x,y) = ( (*T)(xm1,y) + (*T)(xp1,y) ) * 0.5f;
    }

  // even rows interpolated vertically (every column)
  for( x=0 ; x<sxt ; x++ )
    for( y=1 ; y<syt ; y+=2 )
    {
      int yp1 = y+1;
      int ym1 = y-1;
      if( yp1>=syt )
        yp1 = ym1;
      (*T)(x,y) = ( (*T)(x,ym1) + (*T)(x,yp1) ) * 0.5f;
    }
}

void exact_sollution( pfs::Array2D *F, pfs::Array2D *U )
{
//   DEBUG_STR << "exact sollution" << endl;

  int sx = F->getCols();
  int sy = F->getRows();
//   int x,y;

  float h = 1.0/sqrt(sx*sy*1.0f);
//   float h2 = h*h;

    setArray( U, 0.0f); return;   /* also works well?? */
  
//   if( sx==3 && sy==3 )
//   {
//     (*U)(1,1) = -h2* (*F)(1,1) / 4.0f;
    
//     // boundary points
//     for( x=0 ; x<sx ; x++ )
//       (*U)(x,0) = (*U)(x,sy-1) = 0.0f;
//     for( y=0 ; y<sy ; y++ )
//       (*U)(0,y) = (*U)(sx-1,y) = 0.0f;
//   }
//   else
//   {
//     setArray( U, 0.0f); return;   /* also works well?? */
  
//     // TODO: this produces incorrect results
// //     solve_pde_sor(F,U);
// //     for( y=0 ; y<sy ; y++ )
// //       for( x=0 ; x<sx ; x++ )
// //         (*U)(x,y) *= h2;
//   }
}

static int rows, cols;

inline int idx( int r, int c )
{
  return r*cols+c+1;
}

// smooth u using f at level
void smooth( pfs::Array2DImpl *U, pfs::Array2DImpl *F )
{
//   DEBUG_STR << "smooth" << endl;
  
  rows = U->getRows();
  cols = U->getCols();
  
  const int n = rows*cols;

  int iter;
  float err;
        
  linbcg( n, F->getRawData()-1, U->getRawData()-1, 1, 0.001, BCG_STEPS, &iter, &err);

//   fprintf( stderr, "." );

  // Gauss relaxation is too slow
  
//   int sx = F->getCols();
//   int sy = F->getRows();
//   int x,y,i;
//   int shx;	shift x
  
//   float h = 1.0f/sqrtf(sx*sy*1.0f);
//   float h2 = h*h;

//   h2 = 1;

//   for( int pass=0 ; pass<2 ; pass++ )
//   {
//     shx=pass;
//     for( y=0; y<sy ; y++ )
//     {
//       shx= (y+pass)%2;
//       for( x=shx ; x<sx ; x+=2 )
//       {
//         int w, n, e, s;
//         w = (x == 0 ? 0 : x-1);
//         n = (y == 0 ? 0 : y-1);
//         s = (y+1 == sy ? y : y+1);
//         e = (x+1 == sx ? x : x+1);
        
// 	(*U)(x,y) = .25 * ( (*U)(e,y) + (*U)(w,y) + (*U)(x,s) + (*U)(x,n)
//           - h2 * (*F)(x,y) );        
//       }
//     }
//   }
}

void calculate_defect( pfs::Array2D *D, pfs::Array2D *U, pfs::Array2D *F )
{
//   DEBUG_STR << "calculate defect" << endl;

  int sx = F->getCols();
  int sy = F->getRows();

  float h = 1.0f/sqrtf(sx*sy*1.0f);
  float h2i = 1.0/(h*h);

  h2i = 1;

  for( int y=0 ; y<sy ; y++ )
    for( int x=0 ; x<sx ; x++ ) {
        int w, n, e, s;
        w = (x == 0 ? 0 : x-1);
        n = (y == 0 ? 0 : y-1);
        s = (y+1 == sy ? y : y+1);
        e = (x+1 == sx ? x : x+1);
      
        (*D)(x,y) = (*F)(x,y) -( (*U)(e,y) + (*U)(w,y) + (*U)(x,n) + (*U)(x,s)
          - 4.0 * (*U)(x,y) );
    }
  
}

void add_correction( pfs::Array2D *U, pfs::Array2D *C )
{
//   DEBUG_STR << "add_correction" << endl;

  int sx = C->getCols();
  int sy = C->getRows();

  for( int i=0 ; i<sx*sy ; i++ )
    (*U)(i) += (*C)(i);
}


void solve_pde_multigrid( pfs::Array2D *F, pfs::Array2D *U )
{
  int xmax = F->getCols();
  int ymax = F->getRows();
  
  int i;	// index for simple loops
  int k;	// index for iterating through levels
  int k2;	// index for iterating through levels in V-cycles
//   int x,y;

  // 1. restrict f to coarse-grid (by the way count the number of levels)
  //	  k=0: fine-grid = f
  //	  k=levels: coarsest-grid
  int levels = 0;
  int mins = (xmax<ymax) ? xmax : ymax;
  while( mins>=MINS )
  {
    levels++;
    mins = mins/2+MODYF;
  }

  // given function f restricted on levels
  pfs::Array2D** RHS = new pfs::Array2D*[levels+1];

  // approximate initial sollutions on levels
  pfs::Array2DImpl** IU = new pfs::Array2DImpl*[levels+1];
  // target functions in cycles (approximate sollution error (uh - ~uh) )
  pfs::Array2DImpl** VF = new pfs::Array2DImpl*[levels+1];

  VF[0] = new pfs::Array2DImpl(xmax,ymax);
  RHS[0] = F;
  IU[0] = new pfs::Array2DImpl(xmax,ymax);
  pfs::copyArray( U, IU[0] );

  int sx=xmax;
  int sy=ymax;
//   DEBUG_STR << "FMG: #0 size " << sx << "x" << sy << endl;
  for( k=0 ; k<levels ; k++ )
  {
    // calculate size of next level
    sx=sx/2+MODYF;
    sy=sy/2+MODYF;
    
    RHS[k+1] = new pfs::Array2DImpl(sx,sy);
    IU[k+1] = new pfs::Array2DImpl(sx,sy);
    VF[k+1] = new pfs::Array2DImpl(sx,sy);

    // restrict from level k to level k+1 (coarser-grid)
    restrict( RHS[k], RHS[k+1] );

//     DEBUG_STR << "FMG: #" << k+1 << " size " << sx << "x" << sy << endl;
  }

  // 2. find exact sollution at the coarsest-grid (k=levels)
  exact_sollution( RHS[levels], IU[levels] );

  // 3. nested iterations
  for( k=levels-1; k>=0 ; k-- )
  {
    // 4. interpolate sollution from last coarse-grid to finer-grid
    // interpolate from level k+1 to level k (finer-grid)
    prolongate( IU[k+1], IU[k] );

    // 4.1. first target function is the equation target function
    //      (following target functions are the defect)
    copyArray( RHS[k], VF[k] );

    // 5. V-cycle (twice repeated)
    for( int cycle=0 ; cycle<V_CYCLE ; cycle++ )
    {
      // 6. downward stroke of V
      for( k2=k ; k2<levels ; k2++ )
      {
        // 7. pre-smoothing of initial sollution using target function
        //    zero for initial guess at smoothing
        //    (except for level k when iu contains prolongated result)
	if( k2!=k )
          setArray( IU[k2], 0.0f );

//        fprintf( stderr, "Level: %d --------\n", k2 );
        
	for( i=0 ; i<SMOOTH_IT ; i++ )
          smooth( IU[k2], VF[k2] );

        // 8. calculate defect at level
        //    d[k2] = Lh * ~u[k2] - f[k2]
        pfs::Array2D* D = new pfs::Array2DImpl(IU[k2]->getCols(), IU[k2]->getRows());
	calculate_defect( D, IU[k2], VF[k2] );

        // 9. restrict deffect as target function for next coarser-grid
        //    def -> f[k2+1]
	restrict( D, VF[k2+1] );
	delete D;
      }

      // 10. solve on coarsest-grid (target function is the deffect)
      //     iu[levels] should contain sollution for
      //     the f[levels] - last deffect, iu will now be the correction
      exact_sollution(VF[levels], IU[levels] );

      // 11. upward stroke of V
      for( k2=levels-1 ; k2>=k ; k2-- )
      {
        // 12. interpolate correction from last coarser-grid to finer-grid
        //     iu[k2+1] -> cor
        pfs::Array2D* C = new pfs::Array2DImpl(IU[k2]->getCols(), IU[k2]->getRows());
	prolongate( IU[k2+1], C );

        // 13. add interpolated correction to initial sollution at level k2
	add_correction( IU[k2], C );
	delete C;

//        fprintf( stderr, "Level: %d --------\n", k2 );
        
        // 14. post-smoothing of current sollution using target function
	for( i=0 ; i<SMOOTH_IT ; i++ )
          smooth( IU[k2], VF[k2] );
      }

    } //--- end of V-cycle

  } //--- end of nested iteration

  // 15. final sollution
  //     IU[0] contains the final sollution

//   for( k=0 ; k<levels ; k++ ) {
//     char name[200];
//     sprintf( name, "u_%d.pfs", k );
//     dumpPFS( name, IU[k], "Y" );
//     sprintf( name, "rh_%d.pfs", k );
//     dumpPFS( name, RHS[k], "Y" );
//     sprintf( name, "v_%d.pfs", k );
//     dumpPFS( name, VF[k], "Y" );
//   }  

  pfs::copyArray( IU[0], U );
  
  delete VF[0];
  delete IU[0];

  for( k=1 ; k<=levels ; k++ ) {
    delete RHS[k];
    delete IU[k];
    delete VF[k];
  }

  delete RHS;
  delete IU;
  delete VF;

//   DEBUG_STR << "FMG: solved\n";
}




//////////////////////////////////////////////////////////////////////
// SOR - Succesive Overrelaxation Algorithm
//////////////////////////////////////////////////////////////////////

void solve_pde_sor( pfs::Array2D *F, pfs::Array2D *U, int maxits)
{
//   DEBUG_STR << "sor" << endl;

  int xmax = F->getCols();
  int ymax = F->getRows();
  
  float rjac = 1.0 - 6.28/((xmax>ymax) ? xmax : ymax);
  int	ipass, j, jsw, l, lsw, n;
  float anorm, anormf = 0.0, omega = 1.0, resid;

//	 Compute initial norm of residual and terminate iteration when
//		 norm has been reduced by a factor EPS.
  for (j = 0; j < xmax; j++)
    for (l = 0; l < ymax; l++) {
      anormf += fabsf( (*F)(j,l) );
      (*U)(j,l)=0.0f;
    }

//	Assumes initial u is zero.
  for (n = 1; n <= maxits; n++)
  {
    anorm = 0.0;
    jsw = 0;
    for (ipass = 1; ipass <= 2; ipass++) {
      // Odd - even ordering.
      lsw = jsw;
      for (j = 0; j < xmax; j++) {		// j<xmax-1 i l<ymax-1
	for (l = lsw+1; l < ymax; l += 2)
        {
          int jp1 = j+1;
          int jm1 = j-1;
          int lp1 = l+1;
          int lm1 = l-1;
          if( jp1>=xmax ) jp1=jm1;
          if( jm1<0 ) jm1=jp1;
          if( lp1>=ymax ) lp1=lm1;
          if( lm1<0 ) lm1=lp1;
          
	  resid = (*U)(jp1,l) + (*U)(jm1,l) + (*U)(j,lp1) + (*U)(j,lm1)
            - 4.0* (*U)(j,l) - (*F)(j,l);
	  anorm += fabsf(resid);
	  (*U)(j,l) -= omega * resid / -4.0;
	}
	lsw = 1 - lsw;
      }
      jsw = 1 - jsw;
      omega = ( n==1 && ipass==1 ? 1.0 / (1.0 - 0.5 * rjac * rjac)
        : 1.0 / (1.0 - 0.25 * rjac * rjac * omega));
    }
    if( !(n%100) || n==1)
//       DEBUG_STR << "SOR:> " << n << "\tAnorm: " << anorm << "\n";
    if (anorm < EPS * anormf ) {
//       DEBUG_STR << "SOR:> solved.\n";
      return;
    }
  }
//   DEBUG_STR << "SOR:> MAXITS exceeded\n";
}



//#define EPS 1.0e-14

void asolve(unsigned int /*n*/, float b[], float x[], int/* itrnsp*/)
{
    for( int r = 0; r < rows; r++ )
      for( int c = 0; c < cols; c++ ) {
        x[idx(r,c)] = -4 * b[idx(r,c)];
      }
}

void atimes(unsigned int /*n*/, float x[], float res[], int /*itrnsp*/)
{
  for( int r = 1; r < rows-1; r++ )
    for( int c = 1; c < cols-1; c++ ) {
      res[idx(r,c)] = x[idx(r-1,c)] + x[idx(r+1,c)] +
        x[idx(r,c-1)] + x[idx(r,c+1)] - 4*x[idx(r,c)];
    }        

  for( int r = 1; r < rows-1; r++ ) {
    res[idx(r,0)] = x[idx(r-1,0)] + x[idx(r+1,0)] +
        x[idx(r,1)] - 3*x[idx(r,0)];
    res[idx(r,cols-1)] = x[idx(r-1,cols-1)] + x[idx(r+1,cols-1)] +
        x[idx(r,cols-2)] - 3*x[idx(r,cols-1)];
  }
  
  for( int c = 1; c < cols-1; c++ ) {
    res[idx(0,c)] = x[idx(1,c)] +
        x[idx(0,c-1)] + x[idx(0,c+1)] - 3*x[idx(0,c)];
    res[idx(rows-1,c)] = x[idx(rows-2,c)] +
        x[idx(rows-1,c-1)] + x[idx(rows-1,c+1)] - 3*x[idx(rows-1,c)];
  }
  res[idx(0,0)] = x[idx(1,0)] + x[idx(0,1)] - 2*x[idx(0,0)];
  res[idx(rows-1,0)] = x[idx(rows-2,0)] + x[idx(rows-1,1)] - 2*x[idx(rows-1,0)];
  res[idx(0,cols-1)] = x[idx(1,cols-1)] + x[idx(0,cols-2)] - 2*x[idx(0,cols-1)];
  res[idx(rows-1,cols-1)] = x[idx(rows-2,cols-1)] + x[idx(rows-1,cols-2)]
    - 2*x[idx(rows-1,cols-1)];  
}

float snrm(unsigned int n, float sx[], int itol)
{
	unsigned int i,isamax;
	float ans;

	if (itol <= 3) {
		ans = 0.0;
		for (i=1;i<=n;i++) ans += sx[i]*sx[i];
		return sqrtf(ans);
	} else {
		isamax=1;
		for (i=1;i<=n;i++) {
			if (fabsf(sx[i]) > fabsf(sx[isamax])) isamax=i;
		}
		return fabsf(sx[isamax]);
	}
}

/**
 * Biconjugate Gradient Method
 * from Numerical Recipes in C
 */
void linbcg(unsigned int n, float b[], float x[], int itol, float tol,	int itmax, int *iter, float *err)
{	
	unsigned int j;
	float ak,akden,bk,bkden,bknum,bnrm,dxnrm,xnrm,zm1nrm,znrm;
	float *p,*pp,*r,*rr,*z,*zz;

	p=new float[n+1];
	pp=new float[n+1];
	r=new float[n+1];
	rr=new float[n+1];
	z=new float[n+1];
	zz=new float[n+1];

	*iter=0;
	atimes(n,x,r,0);
	for (j=1;j<=n;j++) {
		r[j]=b[j]-r[j];
		rr[j]=r[j];
	}
	atimes(n,r,rr,0);       // minimum residual
        znrm=1.0;
	if (itol == 1) bnrm=snrm(n,b,itol);
	else if (itol == 2) {
		asolve(n,b,z,0);
		bnrm=snrm(n,z,itol);
	}
	else if (itol == 3 || itol == 4) {
		asolve(n,b,z,0);
		bnrm=snrm(n,z,itol);
		asolve(n,r,z,0);
		znrm=snrm(n,z,itol);
	} else printf("illegal itol in linbcg");
	asolve(n,r,z,0);        

	while (*iter <= itmax) {
		++(*iter);
		zm1nrm=znrm;
		asolve(n,rr,zz,1);
		for (bknum=0.0,j=1;j<=n;j++) bknum += z[j]*rr[j];
		if (*iter == 1) {
			for (j=1;j<=n;j++) {
				p[j]=z[j];
				pp[j]=zz[j];
			}
		}
		else {
			bk=bknum/bkden;
			for (j=1;j<=n;j++) {
				p[j]=bk*p[j]+z[j];
				pp[j]=bk*pp[j]+zz[j];
			}
		}                
		bkden=bknum;
		atimes(n,p,z,0);
		for (akden=0.0,j=1;j<=n;j++) akden += z[j]*pp[j];
		ak=bknum/akden;
		atimes(n,pp,zz,1);
		for (j=1;j<=n;j++) {
			x[j] += ak*p[j];
			r[j] -= ak*z[j];
			rr[j] -= ak*zz[j];
		}
		asolve(n,r,z,0);
		if (itol == 1 || itol == 2) {
			znrm=1.0;
			*err=snrm(n,r,itol)/bnrm;
		} else if (itol == 3 || itol == 4) {
			znrm=snrm(n,z,itol);
			if (fabsf(zm1nrm-znrm) > EPS*znrm) {
				dxnrm=fabsf(ak)*snrm(n,p,itol);
				*err=znrm/fabsf(zm1nrm-znrm)*dxnrm;
			} else {
				*err=znrm/bnrm;
				continue;
			}
			xnrm=snrm(n,x,itol);
			if (*err <= 0.5*xnrm) *err /= xnrm;
			else {
				*err=znrm/bnrm;
				continue;
			}
		}
//		fprintf( stderr, "iter=%4d err=%12.6f\n",*iter,*err);
	if (*err <= tol) break;
	}

	delete [] p;
	delete [] pp;
	delete [] r;
	delete [] rr;
	delete [] z;
	delete [] zz;
}
//#undef EPS
