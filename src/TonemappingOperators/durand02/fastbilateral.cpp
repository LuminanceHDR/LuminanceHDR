/**
 * @file fastbilateral.cpp
 * @brief Fast bilateral filtering
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: fastbilateral.cpp,v 1.5 2008/09/09 18:10:49 rafm Exp $
 */

#include <cmath>

#include <Libpfs/array2d.h>
#include <Libpfs/progress.h>
#include "fastbilateral.h"

#ifdef BRANCH_PREDICTION
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
#define BENCHMARK
#include "../../StopWatch.h"
#include "../../sleef.c"
#include "../../opthelper.h"
#include "../../gauss.h"

using namespace std;

// TODO: use spatial convolution rather than FFT, should be much
// faster except for a very large kernels
// DONE

// According to the original paper, downsampling can be used to speed
// up computation. However, downsampling cannot be mathematically
// justified and introduces large errors (mostly excessive bluring) in
// the result of the bilateral filter. Therefore, in this
// implementation, downsampling is disabled.

#if 0
/**
 * @brief upsampling and downsampling
 *
 * original code from pfssize
 */
void upsampleArray( const pfs::Array2D *in, pfs::Array2D *out )
{
  float dx = (float)in->getCols() / (float)out->getCols();
  float dy = (float)in->getRows() / (float)out->getRows();

  float pad;

  float filterSamplingX = max( modff( dx, &pad ), 0.01f );
  float filterSamplingY = max( modff( dy, &pad ), 0.01f );

  const int outRows = out->getRows();
  const int outCols = out->getCols();

  const float inRows = in->getRows();
  const float inCols = in->getCols();

  const float filterSize = 1;

  float sx, sy;
  int x, y;
  for( y = 0, sy = -0.5 + dy/2; y < outRows; y++, sy += dy )
    for( x = 0, sx = -0.5 + dx/2; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float weight = 0;

      for( float ix = max( 0, ceilf( sx-filterSize ) ); ix <= min( floorf(sx+filterSize), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-filterSize ) ); iy <= min( floorf( sy+filterSize), inRows-1 ); iy++ ) {
          float fx = fabs( sx - ix );
          float fy = fabs( sy - iy );

          const float fval = (1.0f-fx)*(1.0f-fy);

          pixVal += (*in)( (int)ix, (int)iy ) * fval;
          weight += fval;
        }

      if( weight == 0 ) {
        fprintf( stderr, "%g %g %g %g\n", sx, sy, dx, dy );
      }
//      assert( weight != 0 );
      (*out)(x,y) = pixVal / weight;

    }
}

void downsampleArray( const pfs::Array2D *in, pfs::Array2D *out )
{
  const float inRows = in->getRows();
  const float inCols = in->getCols();

  const int outRows = out->getRows();
  const int outCols = out->getCols();

  const float dx = (float)in->getCols() / (float)out->getCols();
  const float dy = (float)in->getRows() / (float)out->getRows();

  const float filterSize = 0.5;

  float sx, sy;
  int x, y;

  for( y = 0, sy = dy/2-0.5; y < outRows; y++, sy += dy )
    for( x = 0, sx = dx/2-0.5; x < outCols; x++, sx += dx ) {

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

#endif

/*
Pseudocode from the paper:

PiecewiseBilateral (Image I, spatial kernel fs , intensity influence gr )
  J=0 // set the output to zero
  for j=0..NB SEGMENTS
    ij= minI+j.*(max(I)-min(I))/NB SEGMENTS
    Gj=gr (I - ij ) // evaluate gr at each pixel
    Kj=Gj x fs // normalization factor
    Hj=Gj .* I // compute H for each pixel
    Hj=Hj x fs
    Jj=Hj ./ Kj // normalize
    J=J+Jj .*  InterpolationWeight(I, ij )
*/

void fastBilateralFilter(const pfs::Array2Df &I, pfs::Array2Df &J,
                         float sigma_s, float sigma_r, int /*downsample*/,
                         pfs::Progress &ph) {
    int w = I.getCols();
    int h = I.getRows();
    int size = w * h;

    // find range of values in the input array
    float maxI = I(0);
    float minI = I(0);
#ifdef _OPENMP
    #pragma omp parallel for reduction(min:minI) reduction(max:maxI)
#endif
    for (int i = 0; i < size; i++) {
        float v = I(i);
        maxI = std::max(maxI, v);
        minI = std::min(minI, v);
        J(i) = 0.0f;  // zero output
    }

    pfs::Array2Df jJ(w, h);
    pfs::Array2Df jG(w, h);
    pfs::Array2Df jH(w, h);

    const int NB_SEGMENTS = (int)ceil((maxI - minI) / sigma_r);
    float stepI = (maxI - minI) / NB_SEGMENTS;

    // piecewise bilateral
    for (int j = 0; j < NB_SEGMENTS; j++) {
        ph.setValue(j * 100 / NB_SEGMENTS);
        if (ph.canceled()) break;

        float jI = minI + j * stepI;  // current intensity value

#ifdef _OPENMP
#pragma omp parallel
#endif
{
#ifdef __SSE2__
        vfloat sqrsigma_rv = F2V(sigma_r * sigma_r);
        vfloat jIv = F2V(jI);
#endif
#ifdef _OPENMP
        #pragma omp for
#endif
        for (int i = 0; i < h; i++) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w-3; j+=4) {
                vfloat Iv = LVFU(I(j, i));
                vfloat dIv = Iv - jIv;
                vfloat jGv = xexpf(-(dIv * dIv) / sqrsigma_rv);
                STVFU(jG(j, i), jGv);
                STVFU(jH(j, i), jGv * Iv);
            }
#endif
            for (; j < w; j++) {
                float dI = I(j, i) - jI;
                jG(j, i) = xexpf(-(dI * dI) / (sigma_r * sigma_r));
                jH(j, i) = jG(j, i) * I(j, i);
            }
        }
}
        float* gaussRows[h];

        gaussRows[0] = jG.data();
        for(int i = 1; i < h; ++i)
            gaussRows[i] = gaussRows[i - 1] + w;
#ifdef _OPENMP
        #pragma omp parallel
#endif
        gaussianBlur(gaussRows, gaussRows, w, h, sigma_s);

        gaussRows[0] = jH.data();
        for(int i = 1; i < h; ++i)
            gaussRows[i] = gaussRows[i - 1] + w;
#ifdef _OPENMP
        #pragma omp parallel
#endif
        gaussianBlur(gaussRows, gaussRows, w, h, sigma_s);

#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                float temp = jG(j, i);
                jJ(j, i) = temp != 0.f ? jH(j, i) / temp : 0.f;
            }
        }

        if (j == 0) {
            // if the first segment - to account for the range boundary
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int i = 0; i < size; i++) {
                if (likely(I(i) > jI + stepI)) continue;  // wi = 0;
                if (likely(I(i) > jI)) {
                    float wi = (stepI - (I(i) - jI)) / stepI;
                    J(i) += jJ(i)*wi;
                } else
                    J(i) += jJ(i);
            }
        } else if (j == NB_SEGMENTS - 1) {
            // if the last segment - to account for the range boundary
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int i = 0; i < size; i++) {
                if (I(i) < jI - stepI) continue;  // wi = 0;
                if (likely(I(i) < jI)) {
                    float wi = (stepI - (jI - I(i))) / stepI;
                    J(i) += jJ(i)*wi;
                } else
                    J(i) += jJ(i);
            }
        } else {
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int i = 0; i < size; i++) {
                float wi = stepI - fabs(I(i) - jI);// / stepI;
                if (unlikely(wi > 0.0f)) J(i) += jJ(i)*(wi/stepI);
            }
        }
    }
    //  delete Iz;
    //  if( downsample != 1 )
    //    delete JJ;
}
