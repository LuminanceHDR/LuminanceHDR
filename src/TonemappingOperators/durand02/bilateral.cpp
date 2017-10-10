/**
 * @file bilateral.cpp
 * @brief Bilateral filtering
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: bilateral.cpp,v 1.3 2008/09/09 00:56:49 rafm Exp $
 */

#include <boost/math/special_functions/fpclassify.hpp>

#include "arch/math.h"

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"

#ifdef BRANCH_PREDICTION
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

inline int max(int a, int b) { return (a > b) ? a : b; }

inline int min(int a, int b) { return (a < b) ? a : b; }

// support functions

void gaussianKernel(pfs::Array2Df *kern, float sigma) {
    float sigma2Sqr = 2.0f * sigma * sigma;
    int col_2 = kern->getCols() / 2;
    int row_2 = kern->getRows() / 2;
    for (unsigned int y = 0; y < kern->getRows(); y++) {
        for (unsigned int x = 0; x < kern->getCols(); x++) {
            float rx = (float)(x - col_2);
            float ry = (float)(y - row_2);
            float d2 = rx * rx + ry * ry;
            (*kern)(x, y) = exp(-d2 / sigma2Sqr);
        }
    }
}

class GaussLookup {
    float *gauss;
    float maxVal;
    float scaleFactor;

   public:
    GaussLookup(float sigma, int N) {
        float sigma2Sqr = 2.0f * sigma * sigma;
        maxVal = sqrt(-logf(0.01f) * sigma2Sqr);
        gauss = new float[N];
        for (int i = 0; i < N; i++) {
            float x = (float)i / (float)(N - 1) * maxVal;
            gauss[i] = exp(-x * x / (sigma2Sqr));
        }
        scaleFactor = (float)(N - 1) / maxVal;
    }

    ~GaussLookup() { delete[] gauss; }

    float getValue(float x) {
        x = fabs(x);
        if (unlikely(x > maxVal)) return 0;
        return gauss[(int)(x * scaleFactor)];
    }
};

void bilateralFilter(const pfs::Array2Df *I, pfs::Array2Df *J, float sigma_s,
                     float sigma_r, pfs::Progress &ph) {
    const pfs::Array2Df *X1 = I;  // intensity data     // DAVIDE : CHECK THIS!

    // x +- sigma_s*2 should contain 95% of the Gaussian distrib
    int sKernelSize = (int)(sigma_s * 4 + 0.5) + 1;
    int sKernelSize_2 = sKernelSize / 2;

    pfs::Array2Df sKernel(sKernelSize, sKernelSize);
    gaussianKernel(&sKernel, sigma_s);
    GaussLookup gauss(sigma_r, 256);

    for (unsigned int y = 0; y < I->getRows(); y++) {
        ph.setValue(y * 100 / I->getRows());

        for (unsigned int x = 0; x < I->getCols(); x++) {
            float val = 0;
            float k = 0;
            float I_s = (*X1)(x, y);  //!! previously 'I' not 'X1'

            if (unlikely(!boost::math::isfinite(I_s))) I_s = 0.0f;

            for (int py = max(0, y - sKernelSize_2),
                     pymax = min(I->getRows(), y + sKernelSize_2);
                 py < pymax; py++) {
                for (int px = max(0, x - sKernelSize_2),
                         pxmax = min(I->getCols(), x + sKernelSize_2);
                     px < pxmax; px++) {
                    float I_p = (*X1)(px, py);  //!! previously 'I' not 'X1'
                    if (unlikely(!boost::math::isfinite(I_p))) I_p = 0.0f;

                    float mult = sKernel(px - x + sKernelSize_2,
                                         py - y + sKernelSize_2) *
                                 gauss.getValue(I_p - I_s);

                    float Ixy = (*I)(px, py);
                    if (unlikely(!boost::math::isfinite(Ixy))) Ixy = 0.0f;

                    val += Ixy * mult;  //!! but here we want 'I'
                    k += mult;
                }
            }
            // avoid division by 0 when k is close to 0
            //         (*J)(x,y) = fabs(k) > 0.00000001 ? val/k : 0.;
            (*J)(x, y) = val / k;
        }
    }
}
