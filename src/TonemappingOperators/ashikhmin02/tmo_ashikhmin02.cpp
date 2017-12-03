/**
 * @brief Michael Ashikhmin tone mapping operator 2002
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
 * @author Akiko Yoshida, <yoshida@mpi-sb.mpg.de>
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_ashikhmin02.cpp,v 1.6 2004/11/16 13:40:46 yoshida Exp $
 */

#include <assert.h>
#include <math.h>
#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "pyramid.h"
#include "tmo_ashikhmin02.h"
#include "../../sleef.c"

#define SMAX 10
#define LDMAX 500.f
#define EPSILON 0.00001f

//-------------------------------------------

float calc_LAL_interpolated(GaussianPyramid *myPyramid, int x, int y, int s) {
    float ratio = myPyramid->p[s - 1].lambda;

    float newX = (float)x * ratio;
    float newY = (float)y * ratio;
    int X_int = (int)newX;
    int Y_int = (int)newY;

    float dx, dy, omdx, omdy;
    dx = newX - (float)X_int;
    omdx = 1.f - dx;
    dy = newY - (float)Y_int;
    omdy = 1.f - dy;

    int w = myPyramid->p[s - 1].GP->getCols();
    int h = myPyramid->p[s - 1].GP->getRows();

    float g;
    if (X_int < w - 1 && Y_int < h - 1) {
        g = omdx * omdy * myPyramid->p[s - 1].getPixel(X_int, Y_int) +
            dx * omdy * myPyramid->p[s - 1].getPixel(X_int + 1, Y_int) +
            omdx * dy * myPyramid->p[s - 1].getPixel(X_int, Y_int + 1) +
            dx * dy * myPyramid->p[s - 1].getPixel(X_int + 1, Y_int + 1);
    } else if (X_int < w - 1 && Y_int >= h - 1) {
        Y_int = h - 1;
        g = omdx * myPyramid->p[s - 1].getPixel(X_int, Y_int) +
            dx * myPyramid->p[s - 1].getPixel(X_int + 1, Y_int);
    } else if (X_int >= w - 1 && Y_int < h - 1) {
        X_int = w - 1;
        g = omdy * myPyramid->p[s - 1].getPixel(X_int, Y_int) +
            dy * myPyramid->p[s - 1].getPixel(X_int, Y_int + 1);
    } else
        g = myPyramid->p[s - 1].getPixel(w - 1, h - 1);

    return g;
}

float calc_LAL(GaussianPyramid *myPyramid, int x, int y, int s) {
    float ratio = myPyramid->p[s - 1].lambda;

    float newX = (float)x * ratio;
    float newY = (float)y * ratio;
    unsigned int X_int = (unsigned int)newX;
    unsigned int Y_int = (unsigned int)newY;

    if (X_int >= myPyramid->p[s - 1].GP->getCols())
        X_int = myPyramid->p[s - 1].GP->getCols() - 1;
    if (Y_int >= myPyramid->p[s - 1].GP->getRows())
        Y_int = myPyramid->p[s - 1].GP->getRows() - 1;

    return myPyramid->p[s - 1].getPixel(X_int, Y_int);
}

float LAL(GaussianPyramid *myPyramid, int x, int y, float LOCAL_CONTRAST) {
    float g, gg;
    for (int s = 1; s <= SMAX; s++) {
        // with interpolation
        g = calc_LAL_interpolated(myPyramid, x, y, s);
        gg = calc_LAL_interpolated(myPyramid, x, y, 2 * s);

        // w/o interpolation
        //    g = calc_LAL(myPyramid, x, y, s);
        //     gg = calc_LAL(myPyramid, x, y, 2*s);

        if (fabs((g - gg) / g) >= LOCAL_CONTRAST) return g;
    }
    return g;
}

////////////////////////////////////////////////////////

inline float C(float lum_val) {  // linearly approximated TVI function
    if (lum_val <= 1e-20f) return 0.f;

    if (lum_val < 0.0034f) return lum_val / 0.0014f;

    if (lum_val < 1.f) return 2.4483f + xlogf(lum_val / 0.0034f) / 0.4027f;

    if (lum_val < 7.2444f) return 16.5630f + (lum_val - 1.f) / 0.4027f;

    return 32.0693f + xlogf(lum_val / 7.2444f) / 0.0556f;
}

inline float TM(float lum_val, float minLum, float div) {
    return (LDMAX * (C(lum_val) - C(minLum)) / div);
}

////////////////////////////////////////////////////////

void getMaxMin(pfs::Array2Df *lum_map, float &maxLum, float &minLum) {
    maxLum = minLum = 0.0;

#ifdef _OPENMP
    #pragma omp parallel for reduction(min:minLum) reduction(max:maxLum)
#endif
    for (unsigned int i = 0; i < lum_map->getCols() * lum_map->getRows(); i++) {
        maxLum = ((*lum_map)(i) > maxLum) ? (*lum_map)(i) : maxLum;
        minLum = ((*lum_map)(i) < minLum) ? (*lum_map)(i) : minLum;
    }
}

void Normalize(pfs::Array2Df *lum_map, int nrows, int ncols) {
    float maxLum, minLum;
    getMaxMin(lum_map, maxLum, minLum);
    float range = maxLum - minLum;
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < nrows; y++)
        for (int x = 0; x < ncols; x++)
            (*lum_map)(x, y) = ((*lum_map)(x, y) - minLum) / range;
}

////////////////////////////////////////////////////////

int tmo_ashikhmin02(pfs::Array2Df *Y, pfs::Array2Df *L, float maxLum,
                    float minLum, float /*avLum*/, bool simple_flag,
                    float lc_value, int eq, pfs::Progress &ph) {
    assert(Y != NULL);
    assert(L != NULL);

    unsigned int nrows = Y->getRows();  // image size
    unsigned int ncols = Y->getCols();
    assert(nrows == L->getRows() && ncols == L->getCols());

    //   int im_size = nrows * ncols;

    //  maxLum /= avLum;                            // normalize maximum
    //  luminance
    //  by average luminance

    // apply ToneMapping function only
    if (simple_flag) {
        float div = C(maxLum) - C(minLum);
        div = div != 0 ? div : EPSILON;

#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for (unsigned int y = 0; y < nrows; y++)
            for (unsigned int x = 0; x < ncols; x++) {
                (*L)(x, y) = TM((*Y)(x, y), minLum, div);

                //!! FIX:
                // to keep output values in range 0.01 - 1
                //        (*L)(x,y) /= 100.0f;
            }
        Normalize(L, nrows, ncols);

        return 0;
    }

    // applying the full functions....
    GaussianPyramid *myPyramid = new GaussianPyramid(Y, nrows, ncols);

    // LAL calculation
    pfs::Array2Df la(ncols, nrows);
    int progress = 0;
    int progressSteps = std::max(nrows / 80, 1u);
    int phVal = 0;
    ph.setValue(phVal);

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,16)
#endif
    for (unsigned int y = 0; y < nrows; y++) {
        for (unsigned int x = 0; x < ncols; x++) {
            float lal = LAL(myPyramid, x, y, lc_value);
            la(x, y) = lal == 0 ? EPSILON : lal;
        }
#ifdef _OPENMP
        #pragma omp critical
#endif
        {
            progress += 1;
            if((progress % progressSteps) == 0) {
                phVal ++;
                ph.setValue(std::min(phVal,80));
            }
        }
    }

    delete myPyramid;

    // TM function
    float div = C(maxLum) - C(minLum);
    div = div != 0 ? div : EPSILON;
    progressSteps = std::max(nrows / 20, 1u);
    // final computation for each pixel
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,16)
#endif
    for (unsigned int y = 0; y < nrows; y++) {
        for (unsigned int x = 0; x < ncols; x++) {
            switch (eq) {
                case 2:
                    (*L)(x, y) = (*Y)(x, y) * TM(la(x, y), minLum, div) / la(x, y);
                    break;
                case 4:
                    (*L)(x, y) =
                        TM(la(x, y), minLum, div) +
                        C(TM(la(x, y), minLum, div)) / C(la(x, y)) * ((*Y)(x, y) - la(x, y));
                    break;
            }

            //!! FIX:
            // to keep output values in range 0.01 - 1
            //(*L)(x,y) /= 100.0f;
        }
#ifdef _OPENMP
        #pragma omp critical
#endif
        {
            progress += 1;
            if((progress % progressSteps) == 0) {
                phVal ++;
                ph.setValue(std::min(phVal,100));
            }
        }
    }

    Normalize(L, nrows, ncols);

    return 0;
}
