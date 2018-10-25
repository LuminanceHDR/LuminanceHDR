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
//!  Added trivial downsample and upsample functions when both dimension are
//!  even
//!
//! \note This implementation of Mantiuk06, while originally based on the source
//! code available in PFSTMO, is different in many ways. For this reason, while
//! the file mentions the original authors (and history of the file as well,
//! as above), license applied to this file is uniquely GPL2. If you are looking
//! for an implementation with a less stringent license, please refer to the
//! original implementation of this algorithm in PFSTMO.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "../../sleef.c"
#include "../../opthelper.h"

#include "TonemappingOperators/pfstmo.h"
#include "arch/malloc.h"
#include "arch/math.h"
#include "contrast_domain.h"

#include "pyramid.h"

#include "Libpfs/progress.h"
#include "Libpfs/utils/dotproduct.h"
#include "Libpfs/utils/minmax.h"
#include "Libpfs/utils/msec_timer.h"
#include "Libpfs/utils/numeric.h"
#include "Libpfs/utils/sse.h"
#include "Libpfs/rt_algo.h"

using namespace pfs;

// divG_sum = A * x = sum(divG(x))
void multiplyA(PyramidT &px, const PyramidT &pC, const Array2Df &x,
               Array2Df &sumOfDivG) {
    px.computeGradients(x);

    // scale gradients by Cx,Cy from main pyramid
    px.multiply(pC);

    // calculate the sum of divergences
    px.computeSumOfDivergence(sumOfDivG);
}

// conjugate linear equation solver overwrites pyramid!
//
// This version is a slightly modified version by
// Davide Anastasia <davideanastasia@users.sourceforge.net>
// March 25, 2011
//
namespace {
const int NUM_BACKWARDS_CEILING = 3;
}

void lincg(PyramidT &pyramid, PyramidT &pC, const Array2Df &b, Array2Df &x,
           const int itmax, const float tol, Progress &ph) {

    float rdotr_curr;
    float rdotr_prev;
    float rdotr_best;
    float alpha;
    float beta;

    const size_t rows = pyramid.getRows();
    const size_t cols = pyramid.getCols();
    const size_t n = rows * cols;
    const float tol2 = tol * tol;

    Array2Df x_best(cols, rows);
    Array2Df r(cols, rows);
    Array2Df p(cols, rows);
    Array2Df Ap(cols, rows);

    // bnrm2 = ||b||
    const float bnrm2 = utils::dotProduct(b.data(), n);

    // r = b - Ax
    multiplyA(pyramid, pC, x, r);                  // r = A x
    utils::vsub(b.data(), r.data(), r.data(), n);  // r = b - r

    // rdotr = r.r
    rdotr_best = rdotr_curr = utils::dotProduct(r.data(), n);

    // Setup initial vector
    std::copy(r.begin(), r.end(), p.begin());       // p = r
    std::copy(x.begin(), x.end(), x_best.begin());  // x_best = x

    const float irdotr = rdotr_curr;
    int phvalue = ph.value() + 8;
    const float percent_sf = (100.0f - phvalue) / std::log(tol2 * bnrm2 / irdotr);

    int iter = 0;
    int num_backwards = 0;
    for (; iter < itmax; ++iter) {
        // TEST
        ph.setValue(
            static_cast<int>(phvalue + std::max(std::log(rdotr_curr / irdotr) * percent_sf, 0.f)));
        // User requested abort
        if (ph.canceled() && iter > 0) {
            break;
        }

        // Ap = A p
        multiplyA(pyramid, pC, p, Ap);

        // alpha = r.r / (p . Ap)
        alpha = rdotr_curr / utils::dotProduct(p.data(), Ap.data(), n);

        // r = r - alpha Ap
        utils::vsubs(r.data(), alpha, Ap.data(), r.data(), n);

        // rdotr = r.r
        rdotr_prev = rdotr_curr;
        rdotr_curr = utils::dotProduct(r.data(), n);

        // Have we gone unstable?
        if (rdotr_curr > rdotr_prev) {
            // Save where we've got to
            if (num_backwards == 0 && rdotr_prev < rdotr_best) {
                rdotr_best = rdotr_prev;
                std::copy(x.begin(), x.end(), x_best.begin());
            }

            num_backwards++;
        } else {
            num_backwards = 0;
        }

        // x = x + alpha * p
        utils::vadds(x.data(), alpha, p.data(), x.data(), n);

        // Exit if we're done
        // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(rdotr/bnrm2));
        if (rdotr_curr / bnrm2 < tol2) break;

        if (num_backwards > NUM_BACKWARDS_CEILING) {
            // Reset
            num_backwards = 0;
            std::copy(x_best.begin(), x_best.end(), x.begin());

            // r = Ax
            multiplyA(pyramid, pC, x, r);

            // r = b - r
            utils::vsub(b.data(), r.data(), r.data(), n);

            // rdotr = r.r
            rdotr_best = rdotr_curr = utils::dotProduct(r.data(), r.size());

            // p = r
            std::copy(r.begin(), r.end(), p.begin());
        } else {
            // p = r + beta * p
            beta = rdotr_curr / rdotr_prev;
            utils::vadds(r.data(), beta, p.data(), p.data(), n);
        }
    }

    // Use the best version we found
    if (rdotr_curr > rdotr_best) {
        rdotr_curr = rdotr_best;
        std::copy(x_best.begin(), x_best.end(), x.begin());
    }

    if (rdotr_curr / bnrm2 > tol2) {
        // Not converged
        ph.setValue(
            static_cast<int>(std::log(rdotr_curr / irdotr) * percent_sf));
        if (iter == itmax) {
            std::cerr << std::endl
                      << "pfstmo_mantiuk06: Warning: Not "
                         "converged (hit maximum iterations), error = "
                      << std::sqrt(rdotr_curr / bnrm2) << " (should be below "
                      << tol << ")" << std::endl;
        } else {
            std::cerr << std::endl
                      << "pfstmo_mantiuk06: Warning: Not converged "
                         "(going unstable), error = "
                      << std::sqrt(rdotr_curr / bnrm2) << " (should be below "
                      << tol << ")" << std::endl;
        }
    }
}

void transformToLuminance(PyramidT &pp, Array2Df &Y, const int itmax,
                          const float tol, Progress &ph) {
    PyramidT pC = pp;  // copy ctor

    pp.computeScaleFactors(pC);

    // pyramidScaleGradient(pp, pC);
    pp.multiply(pC);

    // size of the first level of the pyramid
    Array2Df b(pp.getCols(), pp.getRows());

    // calculate the sum of divergences (equal to b)
    pp.computeSumOfDivergence(b);

    // calculate luminances from gradients
    lincg(pp, pC, b, Y, itmax, tol, ph);
}

struct HistData {
    float data;
    float cdf;
    size_t index;
};

struct HistDataCompareData {
    bool operator()(const HistData &v1, const HistData &v2) const {
        return (v1.data < v2.data);
    }
};

struct HistDataCompareIndex {
    bool operator()(const HistData &v1, const HistData &v2) const {
        return (v1.index < v2.index);
    }
};

void contrastEqualization(PyramidT &pp, const float contrastFactor) {
    // Count size
    size_t totalPixels = 0;
    for (PyramidT::const_iterator itCurr = pp.begin(), itEnd = pp.end();
         itCurr != itEnd; ++itCurr) {
        totalPixels += itCurr->size();
    }

    // Allocate memory
    std::vector<HistData> hist(totalPixels);

    // Build histogram info
    size_t offset = 0;
    for (PyramidT::const_iterator itCurr = pp.begin(), itEnd = pp.end();
         itCurr != itEnd; ++itCurr) {
        PyramidS::const_iterator xyGradIter = itCurr->begin();
        PyramidS::const_iterator xyGradEnd = itCurr->end();

        for (; xyGradIter != xyGradEnd; ++xyGradIter) {
            hist[offset].data = std::sqrt(std::pow(xyGradIter->gX(), 2) +
                                          std::pow(xyGradIter->gY(), 2));
            hist[offset].index = offset;

            offset++;
        }
    }

    std::sort(hist.begin(), hist.end(), HistDataCompareData());
    assert(hist[0].data < hist[totalPixels - 1].data);

    // Calculate cdf
    const float normalizationFactor = 1.0f / totalPixels;
    for (size_t idx = 0; idx < totalPixels; ++idx) {
        hist[idx].cdf = idx * normalizationFactor;
    }

    // Recalculate in terms of indexes
    std::sort(hist.begin(), hist.end(), HistDataCompareIndex());
    assert(hist[0].index < hist[totalPixels - 1].index);
    assert(hist[0].index == 0);

    // Remap gradient magnitudes
    offset = 0;
    for (PyramidT::iterator itCurr = pp.begin(), itEnd = pp.end();
         itCurr != itEnd; ++itCurr) {
        PyramidS::iterator xyGradIter = itCurr->begin();
        PyramidS::iterator xyGradEnd = itCurr->end();

        for (; xyGradIter != xyGradEnd; ++xyGradIter) {
            float scaleFactor =
                contrastFactor * hist[offset].cdf / hist[offset].data;

            *xyGradIter *= scaleFactor;

            offset++;
        }
    }
}

namespace {
const float CUT_MARGIN = 0.1f;
const float DISP_DYN_RANGE = 2.3f;

void normalizeLuminanceAndRGB(Array2Df &R, Array2Df &G, Array2Df &B,
                              Array2Df &Y) {
    const float Ymax = utils::maxElement(Y.data(), Y.size());
    const float clip_min = 1e-7f * Ymax;

// std::cout << "clip_min = " << clip_min << std::endl;
// std::cout << "Ymax = " << Ymax << std::endl;
#pragma omp parallel for
    for (int idx = 0; idx < static_cast<int>(Y.size()); idx++) {
        if (R(idx) < clip_min) R(idx) = clip_min;
        if (G(idx) < clip_min) G(idx) = clip_min;
        if (B(idx) < clip_min) B(idx) = clip_min;
        if (Y(idx) < clip_min) Y(idx) = clip_min;

        float currY = 1.f / Y(idx);

        R(idx) *= currY;
        G(idx) *= currY;
        B(idx) *= currY;
        Y(idx) = std::log10(Y(idx));
    }
}

/* Renormalize luminance */
void denormalizeLuminance(Array2Df &Y) {
    const size_t size = Y.size();

    float lumMin, lumMax;
    lhdrengine::findMinMaxPercentile(Y.data(), size, CUT_MARGIN * 0.01f, lumMin, 1.f - CUT_MARGIN * 0.01f, lumMax, true);

    const float lumRange = 1.f / (lumMax - lumMin) * DISP_DYN_RANGE;

#pragma omp parallel for  // shared(lumRange, lumMin)
    for (int j = 0; j < static_cast<int>(size); j++) {
        Y(j) = (Y(j) - lumMin) * lumRange - DISP_DYN_RANGE;  // x scaled
    }
}

template <typename T>
inline T fastDecode(const T &value) {
    if (value <= -5.766466716f) {
        return (xexpf(value) * 12.92f);
    }
    return (1.055f * xexpf(1.f / 2.4f * value) - 0.055f);
}

#ifdef __SSE2__
inline vfloat fastDecode(const vfloat &valuev, const vfloat &c0, const vfloat &c1, const vfloat &c2, const vfloat &c3, const vfloat &c4) {
    vmask selmask = vmaskf_le(valuev, c0);
    vfloat tempv = vself(selmask, valuev, valuev * c1);
    tempv = xexpf(tempv);
    return vself(selmask, tempv * c2, tempv * c3 - c4);
}
#endif

void denormalizeRGB(Array2Df &R, Array2Df &G, Array2Df &B, const Array2Df &Y,
                    float saturationFactor) {

    const float log10 = std::log(10.f);

#ifdef __SSE2__
    const vfloat log10v = F2V(log10);
    const vfloat saturationFactorv = F2V(saturationFactor);
    const vfloat c0 = F2V(-5.7664667f);
    const vfloat c1 = F2V(0.416666667f);
    const vfloat c2 = F2V(12.92f);
    const vfloat c3 = F2V(1.055f);
    const vfloat c4 = F2V(0.055f);

#endif
/* Transform to sRGB */
#pragma omp parallel for
    for (size_t i = 0; i < Y.getRows(); ++i) {
        size_t j = 0;
#ifdef __SSE2__
        for (; j < Y.getCols() - 3; j += 4) {
            vfloat myYv = LVFU(Y(j, i)) * log10v;
            STVFU(R(j, i), fastDecode(saturationFactorv * xlogf(LVFU(R(j, i))) + myYv, c0, c1, c2, c3, c4));
            STVFU(G(j, i), fastDecode(saturationFactorv * xlogf(LVFU(G(j, i))) + myYv, c0, c1, c2, c3, c4));
            STVFU(B(j, i), fastDecode(saturationFactorv * xlogf(LVFU(B(j, i))) + myYv, c0, c1, c2, c3, c4));
        }
#endif
        for (; j < Y.getCols(); ++j) {
            float myY = Y(j, i) * log10;
            R(j, i) = fastDecode(saturationFactor * xlogf(R(j, i)) + myY);
            G(j, i) = fastDecode(saturationFactor * xlogf(G(j, i)) + myY);
            B(j, i) = fastDecode(saturationFactor * xlogf(B(j, i)) + myY);
        }
    }
}
}
// tone mapping
int tmo_mantiuk06_contmap(Array2Df &R, Array2Df &G, Array2Df &B, Array2Df &Y,
                          const float contrastFactor,
                          const float saturationFactor, float detailfactor,
                          const int itmax, const float tol, Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    assert(R.getCols() == G.getCols());
    assert(G.getCols() == B.getCols());
    assert(B.getCols() == Y.getCols());
    assert(R.getRows() == G.getRows());
    assert(G.getRows() == B.getRows());
    assert(B.getRows() == Y.getRows());

    const size_t r = R.getRows();
    const size_t c = R.getCols();
    // const size_t n = r*c;

    normalizeLuminanceAndRGB(R, G, B, Y);
    ph.setValue(2);

    // create pyramid
    PyramidT pp(r, c);
    ph.setValue(6);

    // calculate gradients for pyramid (Y won't be changed)
    pp.computeGradients(Y);

    // transform gradients to R
    pp.transformToR(detailfactor);
    ph.setValue(13);

    // Contrast map
    if (contrastFactor > 0.0f) {
        // Contrast mapping
        pp.scale(contrastFactor);
    } else {
        // Contrast equalization
        contrastEqualization(pp, -contrastFactor);
    }

    // transform R to gradients
    pp.transformToG(detailfactor);
    ph.setValue(40);

    // transform gradients to luminance Y (pp -> Y)
    transformToLuminance(pp, Y, itmax, tol, ph);
    denormalizeLuminance(Y);
    denormalizeRGB(R, G, B, Y, saturationFactor);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_mantiuk06 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return PFSTMO_OK;
}
