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

#include <fftw3.h>

#include <Common/init_fftw.h>
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

using namespace std;

// TODO: use spatial convolution rather than FFT, should be much
// faster except for a very large kernels
class GaussianBlur {
    float *source;
    fftwf_complex *freq;
    fftwf_plan fplan_fw;
    fftwf_plan fplan_in;

    float sigma;

   public:
    GaussianBlur(int nx, int ny, float sigma) : sigma(sigma) {
        init_fftw();
        int ox = nx;
        int oy = ny / 2 + 1;  // saves half of the data
        const int osize = ox * oy;
        FFTW_MUTEX::fftw_mutex_plan.lock();
        source = (float *)fftwf_malloc(sizeof(float) * nx * 2 * (ny / 2 + 1));
        freq = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * osize);
        if (source == NULL || freq == NULL) {
            std::bad_alloc excep;
            FFTW_MUTEX::fftw_mutex_plan.unlock();
            throw excep;
        }
        fplan_fw = fftwf_plan_dft_r2c_2d(nx, ny, source, freq, FFTW_ESTIMATE);
        fplan_in = fftwf_plan_dft_c2r_2d(nx, ny, freq, source, FFTW_ESTIMATE);
        FFTW_MUTEX::fftw_mutex_plan.unlock();
    }

    void blur(const pfs::Array2Df &I, pfs::Array2Df &J) {
        int x, y;

        int nx = I.getCols();
        int ny = I.getRows();
        int nsize = nx * ny;

        int ox = nx;
        int oy = ny / 2 + 1;  // saves half of the data

        for (y = 0; y < ny; y++)
            for (x = 0; x < nx; x++) source[x * ny + y] = I(x, y);

        fftwf_execute(fplan_fw);

        // filter
        float sig = nx / (2.0f * sigma);
        float sig2 = 2.0f * sig * sig;
        for (x = 0; x < ox / 2; x++)
            for (y = 0; y < oy; y++) {
                float d2 = x * x + y * y;
                float kernel = exp(-d2 / sig2);

                freq[x * oy + y][0] *= kernel;
                freq[x * oy + y][1] *= kernel;
                freq[(ox - x - 1) * oy + y][0] *= kernel;
                freq[(ox - x - 1) * oy + y][1] *= kernel;
            }

        fftwf_execute(fplan_in);

        for (x = 0; x < nx; x++)
            for (y = 0; y < ny; y++) J(x, y) = source[x * ny + y] / nsize;
    }

    ~GaussianBlur() {
        FFTW_MUTEX::fftw_mutex_destroy_plan.lock();
        fftwf_free(source);
        fftwf_free(freq);
        fftwf_destroy_plan(fplan_fw);
        fftwf_destroy_plan(fplan_in);
        FFTW_MUTEX::fftw_mutex_destroy_plan.unlock();
    }
};

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
    for (int i = 0; i < size; i++) {
        float v = I(i);
        if (unlikely(v > maxI)) maxI = v;
        if (unlikely(v < minI)) minI = v;
        J(i) = 0.0f;  // zero output
    }

    pfs::Array2Df *JJ;
    //  if( downsample != 1 )
    //    JJ = new pfs::Array2D(w,h);

    //  w /= downsample;
    //  h /= downsample;

    int sizeZ = w * h;
    //  pfs::Array2D* Iz = new pfs::Array2D(w,h);
    //  downsampleArray(I,Iz);
    const pfs::Array2Df *Iz = &I;
    //  sigma_s /= downsample;

    pfs::Array2Df jJ(w, h);
    pfs::Array2Df jG(w, h);
    pfs::Array2Df jK(w, h);
    pfs::Array2Df jH(w, h);

    const int NB_SEGMENTS = (int)ceil((maxI - minI) / sigma_r);
    float stepI = (maxI - minI) / NB_SEGMENTS;

    GaussianBlur gaussian_blur(w, h, sigma_s);

    // piecewise bilateral
    for (int j = 0; j < NB_SEGMENTS; j++) {
        ph.setValue(j * 100 / NB_SEGMENTS);
        if (ph.canceled()) break;

        float jI = minI + j * stepI;  // current intensity value

        for (int i = 0; i < sizeZ; i++) {
            float dI = (*Iz)(i)-jI;
            jG(i) = exp(-(dI * dI) / (sigma_r * sigma_r));
            jH(i) = jG(i) * I(i);
        }

        gaussian_blur.blur(jG, jK);
        gaussian_blur.blur(jH, jH);

        //    convolveArray(jG, sigma_s, jK);
        //    convolveArray(jH, sigma_s, jH);

        for (int i = 0; i < sizeZ; i++)
            if (likely(jK(i) != 0.0f))
                jJ(i) = jH(i) / jK(i);
            else
                jJ(i) = 0.0f;

        //  if( downsample == 1 )
        JJ = &jJ;  // No upsampling is necessary
                   //    else
                   //      upsampleArray(jJ,JJ);

        if (j == 0) {
            // if the first segment - to account for the range boundary
            for (int i = 0; i < size; i++) {
                if (likely(I(i) > jI + stepI)) continue;  // wi = 0;
                if (likely(I(i) > jI)) {
                    float wi = (stepI - (I(i) - jI)) / stepI;
                    J(i) += (*JJ)(i)*wi;
                } else
                    J(i) += (*JJ)(i);
            }
        } else if (j == NB_SEGMENTS - 1) {
            // if the last segment - to account for the range boundary
            for (int i = 0; i < size; i++) {
                if (likely(I(i) < jI - stepI)) continue;  // wi = 0;
                if (likely(I(i) < jI)) {
                    float wi = (stepI - (jI - I(i))) / stepI;
                    J(i) += (*JJ)(i)*wi;
                } else
                    J(i) += (*JJ)(i);
            }
        } else {
            for (int i = 0; i < size; i++) {
                float wi = (stepI - fabs(I(i) - jI)) / stepI;
                if (unlikely(wi > 0.0f)) J(i) += (*JJ)(i)*wi;
            }
        }
    }

    //  delete Iz;
    //  if( downsample != 1 )
    //    delete JJ;
}
