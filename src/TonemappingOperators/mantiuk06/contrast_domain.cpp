/*
 * This file is a part of Luminance HDR package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Grzegorz Krawczyk
 * Copyright (C) 2010-2012 Davide Anastasia
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
 */

//! \brief Contrast mapping TMO [Mantiuk06]
//!
//! From:
//!
//! Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
//! A Perceptual Framework for Contrast Processing of High Dynamic Range Images
//! In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
//! \ref http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
//! \author Radoslaw Mantiuk, <radoslaw.mantiuk@gmail.com>
//! \author Rafal Mantiuk, <mantiuk@gmail.com>
//! Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
//!  (more information on the changes:
//!  http://www.damtp.cam.ac.uk/user/ejb48/hdr/index.html)
//! Updated 2008/06/25 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
//!  bug fixes and OpenMP patches
//!  more on this:
//!  http://tinyurl.com/9plnn8c
//!  Optimization improvements by Lebed Dmytry
//! Updated 2008/07/26 by Dejan Beric <dejan.beric@live.com>
//!  Added the detail factor slider which offers more control over contrast in
//!  details
//! Update 2010/10/06 by Axel Voitier <axel.voitier@gmail.com>
//!  detail_factor patch in order to remove potential issues in a multithreading
//!  environment
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//!  Improvement & Clean up (August 2011)
//!  Refactoring code structure to improve modularity (September 2012)
//! \author Bruce Guenter <bruce@untroubled.org>
//!  Added trivial downsample and upsample functions when both dimension are even
//!
//! \note This implementation of Mantiuk06, while originally based on the source
//! code available in PFSTMO, is different in many ways. For this reason, while
//! the file mentions the original authors (and history of the file as well,
//! as above), license applied to this file is uniquely GPL2. If you are looking
//! for an implementation with a less stringent license, please refer to the
//! original implementation of this algorithm in PFSTMO.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "contrast_domain.h"
#include "arch/malloc.h"
#include "arch/math.h"
#include "TonemappingOperators/pfstmo.h"

#include "pyramid.h"

#include "Libpfs/vex.h"
#include "Libpfs/vex/vex.h"
#include "Libpfs/vex/minmax.h"
#include "Libpfs/vex/dotproduct.h"
#include "Common/msec_timer.h"

// divG_sum = A * x = sum(divG(x))
void multiplyA(PyramidT& px, const PyramidT& pC,
               const float* x, float* divG_sum)
{
    px.computeGradients( x );

    // scale gradients by Cx,Cy from main pyramid
    px.multiply( pC );

    // calculate the sum of divergences
    px.computeSumOfDivergence(divG_sum);
}

// conjugate linear equation solver overwrites pyramid!
//
// This version is a slightly modified version by
// Davide Anastasia <davideanastasia@users.sourceforge.net>
// March 25, 2011
//
namespace
{
const int NUM_BACKWARDS_CEILING = 3;
}

void lincg(PyramidT& pyramid, PyramidT& pC,
           const float* b, float* x,
           const int itmax, const float tol,
           ProgressHelper *ph)
{
    float rdotr_curr;
    float rdotr_prev;
    float rdotr_best;
    float alpha;
    float beta;

    const size_t rows   = pyramid.getRows();
    const size_t cols   = pyramid.getCols();
    const size_t n      = rows*cols;
    const float tol2    = tol*tol;

    std::vector< float > x_best(n);
    std::vector< float > r(n);
    std::vector< float > p(n);
    std::vector< float > Ap(n);

    // bnrm2 = ||b||
    const float bnrm2 = vex::dotProduct(b, n);

    // r = b - Ax
    multiplyA(pyramid, pC, x, r.data());     // r = A x
    vex::vsub(b, r.data(), r.data(), n);     // r = b - r

    // rdotr = r.r
    rdotr_best = rdotr_curr = vex::dotProduct(r.data(), n);

    // Setup initial vector
    std::copy(r.begin(), r.end(), p.begin());   // p = r
    std::copy(x, x + n, x_best.begin());        // x_best = x

    const float irdotr = rdotr_curr;
    const float percent_sf = 100.0f/std::log(tol2*bnrm2/irdotr);

    int iter = 0;
    int num_backwards = 0;
    for (; iter < itmax; ++iter)
    {
        // TEST
        ph->newValue(
                    static_cast<int>(std::log(rdotr_curr/irdotr)*percent_sf)
                    );
        // User requested abort
        if ( ph->isTerminationRequested() && iter > 0 )
        {
            break;
        }

        // Ap = A p
        multiplyA(pyramid, pC, p.data(), Ap.data());

        // alpha = r.r / (p . Ap)
        alpha = rdotr_curr / vex::dotProduct(p.data(), Ap.data(), n);

        // r = r - alpha Ap
        vex::vsubs(r.data(), alpha, Ap.data(), r.data(), n);

        // rdotr = r.r
        rdotr_prev = rdotr_curr;
        rdotr_curr = vex::dotProduct(r.data(), n);

        // Have we gone unstable?
        if (rdotr_curr > rdotr_prev)
        {
            // Save where we've got to
            if (num_backwards == 0 && rdotr_prev < rdotr_best)
            {
                rdotr_best = rdotr_prev;
                std::copy(x, x + n, x_best.begin());
            }

            num_backwards++;
        }
        else
        {
            num_backwards = 0;
        }

        // x = x + alpha * p
        vex::vadds(x, alpha, p.data(), x, n);

        // Exit if we're done
        // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(rdotr/bnrm2));
        if (rdotr_curr/bnrm2 < tol2)
            break;

        if (num_backwards > NUM_BACKWARDS_CEILING)
        {
            // Reset
            num_backwards = 0;
            std::copy(x_best.begin(), x_best.end(), x);

            // r = Ax
            multiplyA(pyramid, pC, x, r.data());

            // r = b - r
            vex::vsub(b, r.data(), r.data(), n);

            // rdotr = r.r
            rdotr_best = rdotr_curr = vex::dotProduct(r.data(), n);

            // p = r
            std::copy(r.begin(), r.end(), p.begin());
        }
        else
        {
            // p = r + beta * p
            beta = rdotr_curr/rdotr_prev;
            vex::vadds(r.data(), beta, p.data(), p.data(), n);
        }
    }

    // Use the best version we found
    if (rdotr_curr > rdotr_best)
    {
        rdotr_curr = rdotr_best;
        std::copy(x_best.begin(), x_best.end(), x);
    }

    if (rdotr_curr/bnrm2 > tol2)
    {
        // Not converged
        ph->newValue(
                    static_cast<int>(std::log(rdotr_curr/irdotr)*percent_sf)
                    );
        if (iter == itmax)
        {
            std::cerr << std::endl << "pfstmo_mantiuk06: Warning: Not "\
                         "converged (hit maximum iterations), error = "
                      << std::sqrt(rdotr_curr/bnrm2)
                      << " (should be below " << tol <<")"
                      << std::endl;
        }
        else
        {
            std::cerr << std::endl
                      << "pfstmo_mantiuk06: Warning: Not converged "\
                         "(going unstable), error = "
                      << std::sqrt(rdotr_curr/bnrm2)
                      << " (should be below " << tol << ")"
                      << std::endl;
        }
    }
    else
    {
        ph->newValue( itmax );
    }
}

void transformToLuminance(PyramidT& pp, float* Y,
                          ProgressHelper *ph, const int itmax, const float tol)
{
    PyramidT pC = pp; // copy ctor

    pp.computeScaleFactors( pC );

    // pyramidScaleGradient(pp, pC);
    pp.multiply( pC );

    // size of the first level of the pyramid
    std::vector<float> b( pp.getElems() );

    // calculate the sum of divergences (equal to b)
    pp.computeSumOfDivergence( b.data() );

    // calculate luminances from gradients
    lincg(pp, pC, b.data(), Y, itmax, tol, ph);
}

struct HistData
{
    float data;
    float cdf;
    size_t index;
};

struct HistDataCompareData
{
    bool operator()(const HistData& v1, const HistData& v2) const
    {
        if (v1.data >= v2.data) return false;
        else return true;
    }
};

struct HistDataCompareIndex
{
    bool operator()(const HistData& v1, const HistData& v2) const
    {
        if (v1.index >= v2.index) return false;
        else return true;
    }
};

void contrastEqualization(PyramidT& pp, const float contrastFactor)
{
    // Count size
    size_t totalPixels = 0;
    for ( PyramidT::const_iterator itCurr = pp.begin(), itEnd = pp.end();
          itCurr != itEnd;
          ++itCurr)
    {
        totalPixels += itCurr->size();
    }

    // Allocate memory
    std::vector<HistData> hist(totalPixels);

    // Build histogram info
    size_t offset = 0;
    for ( PyramidT::const_iterator itCurr = pp.begin(), itEnd = pp.end();
          itCurr != itEnd;
          ++itCurr)
    {
        const size_t pixels = itCurr->size();

        const float* gx = itCurr->gX();
        const float* gy = itCurr->gY();

        for (size_t idx = 0; idx < pixels; idx++, gx++, gy++)
        {
            hist[offset + idx].data = std::sqrt(
                        std::pow(*gx, 2) + std::pow(*gy, 2)
                        );
            hist[offset + idx].index = offset + idx;
        }

        offset += pixels;
    }

    std::sort( hist.begin(), hist.end(), HistDataCompareData() );
    assert( hist[0].data < hist[totalPixels-1].data );

    // Calculate cdf
    const float normalizationFactor = 1.0f/totalPixels;
    for (size_t idx = 0; idx < totalPixels; ++idx)
    {
        hist[idx].cdf = static_cast<float>(idx)*normalizationFactor;
    }

    // Recalculate in terms of indexes
    std::sort( hist.begin(), hist.end(), HistDataCompareIndex() );
    assert( hist[0].index < hist[totalPixels-1].index );
    assert( hist[0].index == 0 );

    //Remap gradient magnitudes
    offset = 0;
    for ( PyramidT::iterator itCurr = pp.begin(), itEnd = pp.end();
          itCurr != itEnd;
          ++itCurr)
    {
        const size_t pixels = itCurr->size();

        float* gx = itCurr->gX();
        float* gy = itCurr->gY();

        for (size_t idx = 0; idx < pixels; idx++, gx++, gy++)
        {
            float scaleFactor =
                    contrastFactor +
                    hist[offset + idx].cdf / hist[offset + idx].data;

            *gx *= scaleFactor;
            *gy *= scaleFactor;
        }

        offset += pixels;
    }
}

namespace
{
const float CUT_MARGIN = 0.1f;
const float DISP_DYN_RANGE = 2.3f;

void normalizeLuminanceAndRGB(float* R, float* G, float* B, float* Y, size_t size)
{
    const float Ymax = vex::maxElement(Y, size);
    const float clip_min = 1e-7f*Ymax;

    // std::cout << "clip_min = " << clip_min << std::endl;
    // std::cout << "Ymax = " << Ymax << std::endl;

    for (size_t idx = 0; idx < size; idx++)
    {
        if ( R[idx] < clip_min ) R[idx] = clip_min;
        if ( G[idx] < clip_min ) G[idx] = clip_min;
        if ( B[idx] < clip_min ) B[idx] = clip_min;
        if ( Y[idx] < clip_min ) Y[idx] = clip_min;

        float currY = 1.f/Y[idx];

        R[idx] *= currY;
        G[idx] *= currY;
        B[idx] *= currY;
        Y[idx] = std::log10( Y[idx] );
    }
}

/* Renormalize luminance */
void denormalizeLuminance(float* Y, size_t size)
{
    std::vector<float> temp(size);
    std::copy(Y, Y + size, temp.begin());

    std::sort(temp.begin(), temp.end());

    float trim = (size - 1) * CUT_MARGIN * 0.01f;
    float delta = trim - std::floor(trim);
    const float lumMin =
            temp[ static_cast<size_t>(std::floor(trim)) ]*delta +
            temp[ static_cast<size_t>(std::ceil(trim)) ]*(1.0f - delta);

    trim = (size - 1) * (100.0f - CUT_MARGIN) * 0.01f;
    delta = trim - std::floor(trim);
    const float lumMax =
            temp[ static_cast<size_t>(std::floor(trim)) ]*delta +
            temp[ static_cast<size_t>(std::ceil(trim)) ]*(1.0f - delta);

    const float lumRange = 1.f/(lumMax - lumMin)*DISP_DYN_RANGE;

#pragma omp parallel for // shared(lumRange, lumMin)
    for (int j = 0; j < static_cast<int>(size); j++)
    {
        Y[j] = (Y[j] - lumMin)*lumRange - DISP_DYN_RANGE; // x scaled
    }
}

template <typename T>
inline
T decode(const T& value)
{
    if ( value <= 0.0031308f )
    {
        return (value * 12.92f);
    }
    return (1.055f * std::pow( value, 1.f/2.4f ) - 0.055f);
}

void denormalizeRGB(float* R, float* G, float* B, const float* Y, size_t size,
                    float saturationFactor)
{
    /* Transform to sRGB */
#pragma omp parallel for
    for (int j = 0; j < static_cast<int>(size); j++)
    {
        float myY = std::pow( 10.f, Y[j] );
        R[j] = decode( std::pow( R[j], saturationFactor ) * myY );
        G[j] = decode( std::pow( G[j], saturationFactor ) * myY );
        B[j] = decode( std::pow( B[j], saturationFactor ) * myY );
    }
}
}

// tone mapping
int tmo_mantiuk06_contmap(const int c, const int r,
                          float* R, float* G, float* B,
                          float* Y,
                          const float contrastFactor,
                          const float saturationFactor,
                          float detailfactor,
                          const int itmax,
                          const float tol,
                          ProgressHelper *ph)
{
    const size_t n = c*r;

    normalizeLuminanceAndRGB(R, G, B, Y, n);

    // create pyramid
    PyramidT pp(r, c);
    // calculate gradients for pyramid (Y won't be changed)
    pp.computeGradients( Y );
    // transform gradients to R
    pp.transformToR( detailfactor );

    // Contrast map
    if ( contrastFactor > 0.0f )
    {
        // Contrast mapping
        pp.scale( contrastFactor );
    }
    else
    {
        // Contrast equalization
        contrastEqualization(pp, -contrastFactor);
    }

    // transform R to gradients
    pp.transformToG( detailfactor );
    // transform gradients to luminance Y (pp -> Y)
    transformToLuminance(pp, Y, ph, itmax, tol);

    denormalizeLuminance(Y, n);
    denormalizeRGB(R, G, B, Y, n, saturationFactor);

    return PFSTMO_OK;
}
