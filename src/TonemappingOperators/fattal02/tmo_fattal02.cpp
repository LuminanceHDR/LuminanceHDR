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

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <vector>

#include <assert.h>
#include <math.h>

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/msec_timer.h"
#include "TonemappingOperators/pfstmo.h"

#include "pde.h"
#include "tmo_fattal02.h"

using namespace std;
using namespace pfs;
using namespace utils;

//!! TODO: for debugging purposes
// #define PFSEOL "\x0a"
// static void dumpPFS( const char *fileName, const pfs::Array2D *data, const
// char *channelName )
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

void downSample(const pfs::Array2Df &A, pfs::Array2Df &B) {
    const int width = B.getCols();
    const int height = B.getRows();

    // Note, I've uncommented all omp directives. They are all ok but are
    // applied to too small problems and in total don't lead to noticable
    // speed improvements. The main issue is the pde solver and in case of the
    // fft solver uses optimised threaded fftw routines.
    //#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float p = A(2 * x, 2 * y);
            p += A(2 * x + 1, 2 * y);
            p += A(2 * x, 2 * y + 1);
            p += A(2 * x + 1, 2 * y + 1);
            B(x, y) = p * 0.25f;  // p / 4.0f;
        }
    }
}

void gaussianBlur(const pfs::Array2Df &I, pfs::Array2Df &L) {
    const int width = I.getCols();
    const int height = I.getRows();

    pfs::Array2Df T(width, height);

    //--- X blur
    //#pragma omp parallel for shared(I, T)
    for (int y = 0; y < height; y++) {
        for (int x = 1; x < width - 1; x++) {
            float t = 2.f * I(x, y);
            t += I(x - 1, y);
            t += I(x + 1, y);
            T(x, y) = t * 0.25f;  // t / 4.f;
        }
        T(0, y) = (3.f * I(0, y) + I(1, y)) * 0.25f;  // / 4.f;
        T(width - 1, y) =
            (3.f * I(width - 1, y) + I(width - 2, y)) * 0.25f;  // / 4.f;
    }

    //--- Y blur
    //#pragma omp parallel for shared(T, L)
    for (int x = 0; x < width; x++) {
        for (int y = 1; y < height - 1; y++) {
            float t = 2.f * T(x, y);
            t += T(x, y - 1);
            t += T(x, y + 1);
            L(x, y) = t * 0.25f;  // t/4.0f;
        }
        L(x, 0) = (3.f * T(x, 0) + T(x, 1)) * 0.25f;  // / 4.0f;
        L(x, height - 1) =
            (3.f * T(x, height - 1) + T(x, height - 2)) * 0.25f;  // / 4.0f;
    }
}

void createGaussianPyramids(pfs::Array2Df &H, pfs::Array2Df **pyramids,
                            int nlevels) {
    int width = H.getCols();
    int height = H.getRows();
    const int size = width * height;

    pyramids[0] = new pfs::Array2Df(width, height);
    //#pragma omp parallel for shared(pyramids, H)
    for (int i = 0; i < size; i++) (*pyramids[0])(i) = H(i);

    pfs::Array2Df L(width, height);
    gaussianBlur(*pyramids[0], L);

    for (int k = 1; k < nlevels; k++) {
        width /= 2;
        height /= 2;
        pyramids[k] = new pfs::Array2Df(width, height);
        downSample(L, *pyramids[k]);

        L.fill(0.f);
        gaussianBlur(*pyramids[k], L);
    }
}

//--------------------------------------------------------------------

float calculateGradients(pfs::Array2Df &H, pfs::Array2Df &G, int k) {
    const int width = H.getCols();
    const int height = H.getRows();
    const float divider = pow(2.0f, k + 1);
    float avgGrad = 0.0f;

    //#pragma omp parallel for shared(G,H) reduction(+:avgGrad)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float gx, gy;
            int w, n, e, s;
            w = (x == 0 ? 0 : x - 1);
            n = (y == 0 ? 0 : y - 1);
            s = (y + 1 == height ? y : y + 1);
            e = (x + 1 == width ? x : x + 1);

            gx = (H(w, y) - H(e, y)) / divider;

            gy = (H(x, s) - H(x, n)) / divider;
            // note this implicitely assumes that H(-1)=H(0)
            // for the fft-pde slover this would need adjustment as H(-1)=H(1)
            // is assumed, which means gx=0.0, gy=0.0 at the boundaries
            // however, the impact is not visible so we ignore this here

            G(x, y) = sqrt(gx * gx + gy * gy);
            avgGrad += G(x, y);
        }
    }

    return avgGrad / (width * height);
}

//--------------------------------------------------------------------

void upSample(const pfs::Array2Df &A, pfs::Array2Df &B) {
    const int width = B.getCols();
    const int height = B.getRows();
    const int awidth = A.getCols();
    const int aheight = A.getRows();

    //#pragma omp parallel for shared(A, B)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int ax = static_cast<int>(x * 0.5f);  // x / 2.f;
            int ay = static_cast<int>(y * 0.5f);  // y / 2.f;
            ax = (ax < awidth) ? ax : awidth - 1;
            ay = (ay < aheight) ? ay : aheight - 1;

            B(x, y) = A(ax, ay);
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

void calculateFiMatrix(pfs::Array2Df &FI, pfs::Array2Df *gradients[],
                       float avgGrad[], int nlevels, int detail_level,
                       float alfa, float beta, float noise, bool newfattal) {
    int width = gradients[nlevels - 1]->getCols();
    int height = gradients[nlevels - 1]->getRows();
    pfs::Array2Df **fi = new pfs::Array2Df *[nlevels];

    fi[nlevels - 1] = new pfs::Array2Df(width, height);
    if (newfattal) {
        //#pragma omp parallel for shared(fi)
        for (int k = 0; k < width * height; k++) {
            (*fi[nlevels - 1])(k) = 1.0f;
        }
    }

    for (int k = nlevels - 1; k >= 0; k--) {
        width = gradients[k]->getCols();
        height = gradients[k]->getRows();

        // only apply gradients to levels>=detail_level but at least to the
        // coarsest
        if (k >= detail_level || k == nlevels - 1 || newfattal == false) {
            // DEBUG_STR << "calculateFiMatrix: apply gradient to level " << k
            // <<
            // endl;
            //#pragma omp parallel for shared(fi,avgGrad)
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    float grad = ((*gradients[k])(x, y) < 1e-4f)
                                     ? 1e-4
                                     : (*gradients[k])(x, y);
                    float a = alfa * avgGrad[k];

                    float value = powf((grad + noise) / a, beta - 1.0f);

                    if (newfattal)
                        (*fi[k])(x, y) *= value;
                    else
                        (*fi[k])(x, y) = value;
                }
            }
        }

        // create next level
        if (k > 1) {
            width = gradients[k - 1]->getCols();
            height = gradients[k - 1]->getRows();
            fi[k - 1] = new pfs::Array2Df(width, height);
        } else
            fi[0] = &FI;  // highest level -> result

        if (k > 0 && newfattal) {
            upSample(*fi[k], *fi[k - 1]);  // upsample to next level
            gaussianBlur(*fi[k - 1], *fi[k - 1]);
        }
    }

    for (int k = 1; k < nlevels; k++) {
        delete fi[k];
    }
    delete[] fi;
}

inline void findMaxMinPercentile(const pfs::Array2Df &I, float minPrct,
                                 float &minLum, float maxPrct, float &maxLum) {
    using namespace std;

    const int size = I.getRows() * I.getCols();
    const float *data = I.data();
    std::vector<float> vI;

    copy(data, data + size, back_inserter(vI));
    sort(vI.begin(), vI.end());

    minLum = vI.at(int(minPrct * vI.size()));
    maxLum = vI.at(int(maxPrct * vI.size()));
}

void tmo_fattal02(size_t width, size_t height, const pfs::Array2Df &Y,
                  pfs::Array2Df &L, float alfa, float beta, float noise,
                  bool newfattal, bool fftsolver, int detail_level,
                  pfs::Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif
    static const float black_point = 0.1f;
    static const float white_point = 0.5f;
    static const float gamma = 1.0f;  // 0.8f;
    // static const int   detail_level = 3;
    if (detail_level < 0) detail_level = 0;
    if (detail_level > 3) detail_level = 3;

    ph.setValue(2);
    if (ph.canceled()) return;

    int MSIZE = 32;  // minimum size of gaussian pyramid
    // I believe a smaller value than 32 results in slightly better overall
    // quality but I'm only applying this if the newly implemented fft solver
    // is used in order not to change behaviour of the old version
    // TODO: best let the user decide this value
    if (fftsolver) {
        MSIZE = 8;
    }

    int size = width * height;
    // unsigned int x,y;
    // int i, k;

    // find max & min values, normalize to range 0..100 and take logarithm
    float minLum = Y(0, 0);
    float maxLum = Y(0, 0);
    for (int i = 0; i < size; i++) {
        minLum = (Y(i) < minLum) ? Y(i) : minLum;
        maxLum = (Y(i) > maxLum) ? Y(i) : maxLum;
    }
    pfs::Array2Df H(width, height);
    //#pragma omp parallel for private(i) shared(H, Y, maxLum)
    for (int i = 0; i < size; i++) {
        H(i) = logf(100.0f * Y(i) / maxLum + 1e-4);
    }
    ph.setValue(4);

    // create gaussian pyramids
    int mins = (width < height) ? width : height;  // smaller dimension
    int nlevels = 0;
    while (mins >= MSIZE) {
        nlevels++;
        mins /= 2;
    }
    // std::cout << "DEBUG: nlevels = " << nlevels << ", mins = " << mins <<
    // std::endl;
    // The following lines solves a bug with images particularly small
    if (nlevels == 0) nlevels = 1;

    pfs::Array2Df **pyramids = new pfs::Array2Df *[nlevels];
    createGaussianPyramids(H, pyramids, nlevels);
    ph.setValue(8);

    // calculate gradients and its average values on pyramid levels
    pfs::Array2Df **gradients = new pfs::Array2Df *[nlevels];
    float *avgGrad = new float[nlevels];
    for (int k = 0; k < nlevels; k++) {
        gradients[k] =
            new pfs::Array2Df(pyramids[k]->getCols(), pyramids[k]->getRows());
        avgGrad[k] = calculateGradients(*pyramids[k], *gradients[k], k);
    }
    ph.setValue(12);

    // calculate fi matrix
    pfs::Array2Df FI(width, height);
    calculateFiMatrix(FI, gradients, avgGrad, nlevels, detail_level, alfa, beta,
                      noise, newfattal);
    //  dumpPFS( "FI.pfs", FI, "Y" );
    for (int i = 0; i < nlevels; i++) {
        delete pyramids[i];
        delete gradients[i];
    }
    delete[] pyramids;
    delete[] gradients;
    delete[] avgGrad;
    ph.setValue(16);
    if (ph.canceled()) {
        return;
    }

    // attenuate gradients
    pfs::Array2Df Gx(width, height);
    pfs::Array2Df Gy(width, height);

    // the fft solver solves the Poisson pde but with slightly different
    // boundary conditions, so we need to adjust the assembly of the right hand
    // side accordingly (basically fft solver assumes U(-1) = U(1), whereas zero
    // Neumann conditions assume U(-1)=U(0)), see also divergence calculation
    if (fftsolver)
        for (size_t y = 0; y < height; y++)
            for (size_t x = 0; x < width; x++) {
                // sets index+1 based on the boundary assumption H(N+1)=H(N-1)
                unsigned int yp1 = (y + 1 >= height ? height - 2 : y + 1);
                unsigned int xp1 = (x + 1 >= width ? width - 2 : x + 1);
                // forward differences in H, so need to use between-points
                // approx of FI
                Gx(x, y) =
                    (H(xp1, y) - H(x, y)) * 0.5 * (FI(xp1, y) + FI(x, y));
                Gy(x, y) =
                    (H(x, yp1) - H(x, y)) * 0.5 * (FI(x, yp1) + FI(x, y));
            }
    else
        for (size_t y = 0; y < height; y++)
            for (size_t x = 0; x < width; x++) {
                int s, e;
                s = (y + 1 == height ? y : y + 1);
                e = (x + 1 == width ? x : x + 1);

                Gx(x, y) = (H(e, y) - H(x, y)) * FI(x, y);
                Gy(x, y) = (H(x, s) - H(x, y)) * FI(x, y);
            }
    ph.setValue(18);

    //   dumpPFS( "Gx.pfs", Gx, "Y" );
    //   dumpPFS( "Gy.pfs", Gy, "Y" );

    // calculate divergence
    pfs::Array2Df DivG(width, height);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            DivG(x, y) = Gx(x, y) + Gy(x, y);
            if (x > 0) DivG(x, y) -= Gx(x - 1, y);
            if (y > 0) DivG(x, y) -= Gy(x, y - 1);

            if (fftsolver) {
                if (x == 0) DivG(x, y) += Gx(x, y);
                if (y == 0) DivG(x, y) += Gy(x, y);
            }
        }
    }
    ph.setValue(20);
    if (ph.canceled()) {
        return;
    }

    //  dumpPFS( "DivG.pfs", DivG, "Y" );

    // solve pde and exponentiate (ie recover compressed image)
    {
        pfs::Array2Df U(width, height);
        if (fftsolver) {
            solve_pde_fft(DivG, U, ph);
        } else {
            solve_pde_multigrid(&DivG, &U, ph);
        }
#ifndef NDEBUG
        printf("\npde residual error: %f\n", residual_pde(U, DivG));
#endif
        ph.setValue(90);
        if (ph.canceled()) {
            return;
        }

        for (size_t idx = 0; idx < height * width; ++idx) {
            L(idx) = expf(gamma * U(idx));
        }
    }
    ph.setValue(95);

    // remove percentile of min and max values and renormalize
    float cut_min = 0.01f * black_point;
    float cut_max = 1.0f - 0.01f * white_point;
    assert(cut_min >= 0.0f && (cut_max <= 1.0f) && (cut_min < cut_max));
    findMaxMinPercentile(L, cut_min, minLum, cut_max, maxLum);
    for (size_t idx = 0; idx < height * width; ++idx) {
        L(idx) = (L(idx) - minLum) / (maxLum - minLum);
        if (L(idx) <= 0.0f) {
            L(idx) = 0.0;
        }
        // note, we intentionally do not cut off values > 1.0
    }
#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_fattal02 = " << stop_watch.get_time() << " msec" << endl;
#endif

    ph.setValue(96);
}
