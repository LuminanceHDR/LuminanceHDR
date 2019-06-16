/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cmath>

#include "bayerhelper.h"
#include "rt_math.h"
#include "sleef.c"
#include "librtprocess.h"
#include "LUT.h"
#include "opthelper.h"
#include "median.h"
#include "StopWatch.h"

using namespace librtprocess;


namespace {

/*
   Refinement based on EECI demosaicing algorithm by L. Chang and Y.P. Tan
   Paul Lee
   Adapted for RawTherapee - Jacques Desmis 04/2013
*/

#ifdef __SSE2__
#define CLIPV(a) LIMV(a,ZEROV,c65535v)
#endif
void refinement(int width, int height, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int PassCount)
{

BENCHFUN
    float **rgb[3];
    rgb[0] = red;
    rgb[1] = green;
    rgb[2] = blue;

    for (int b = 0; b < PassCount; b++) {
        setProgCancel((double)b / PassCount);


#ifdef _OPENMP
        #pragma omp parallel
#endif
        {

            /* Reinforce interpolated green pixels on RED/BLUE pixel locations */
#ifdef _OPENMP
            #pragma omp for
#endif

            for (int row = 2; row < height - 2; row++) {
                int col = 2 + (fc(cfarray, row, 2) & 1);
                int c = fc(cfarray, row, col);
#ifdef __SSE2__
                vfloat dLv, dRv, dUv, dDv, v0v;
                const vfloat onev = F2V(1.f);
                const vfloat zd5v = F2V(0.5f);
                const vfloat c65535v = F2V(65535.f);

                for (; col < width - 8; col += 8) {
                    const vfloat centre = LC2VFU(rgb[c][row][col]);
                    dLv = onev / (onev + vabsf(LC2VFU(rgb[c][row][col - 2]) - centre) + vabsf(LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[1][row][col - 1])));
                    dRv = onev / (onev + vabsf(LC2VFU(rgb[c][row][col + 2]) - centre) + vabsf(LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[1][row][col - 1])));
                    dUv = onev / (onev + vabsf(LC2VFU(rgb[c][row - 2][col]) - centre) + vabsf(LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[1][row - 1][col])));
                    dDv = onev / (onev + vabsf(LC2VFU(rgb[c][row + 2][col]) - centre) + vabsf(LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[1][row - 1][col])));
                    v0v = CLIPV(centre + zd5v + ((LC2VFU(rgb[1][row][col - 1]) - LC2VFU(rgb[c][row][col - 1])) * dLv + (LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[c][row][col + 1])) * dRv + (LC2VFU(rgb[1][row - 1][col]) - LC2VFU(rgb[c][row - 1][col])) * dUv + (LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[c][row + 1][col])) * dDv ) / (dLv + dRv + dUv + dDv));
                    STC2VFU(rgb[1][row][col], v0v);
                }

#endif

                for (; col < width - 2; col += 2) {
                    float dL = 1.f / (1.f + fabsf(rgb[c][row][col - 2] - rgb[c][row][col]) + fabsf(rgb[1][row][col + 1] - rgb[1][row][col - 1]));
                    float dR = 1.f / (1.f + fabsf(rgb[c][row][col + 2] - rgb[c][row][col]) + fabsf(rgb[1][row][col + 1] - rgb[1][row][col - 1]));
                    float dU = 1.f / (1.f + fabsf(rgb[c][row - 2][col] - rgb[c][row][col]) + fabsf(rgb[1][row + 1][col] - rgb[1][row - 1][col]));
                    float dD = 1.f / (1.f + fabsf(rgb[c][row + 2][col] - rgb[c][row][col]) + fabsf(rgb[1][row + 1][col] - rgb[1][row - 1][col]));
                    float v0 = (rgb[c][row][col] + 0.5f + ((rgb[1][row][col - 1] - rgb[c][row][col - 1]) * dL + (rgb[1][row][col + 1] - rgb[c][row][col + 1]) * dR + (rgb[1][row - 1][col] - rgb[c][row - 1][col]) * dU + (rgb[1][row + 1][col] - rgb[c][row + 1][col]) * dD ) / (dL + dR + dU + dD));
                    rgb[1][row][col] = CLIP(v0);
                }
            }

            /* Reinforce interpolated red/blue pixels on GREEN pixel locations */
#ifdef _OPENMP
            #pragma omp for
#endif

            for (int row = 2; row < height - 2; row++) {
                int col = 2 + (fc(cfarray, row, 3) & 1);
                int c = fc(cfarray, row, col + 1);
#ifdef __SSE2__
                vfloat dLv, dRv, dUv, dDv, v0v;
                const vfloat onev = F2V(1.f);
                const vfloat zd5v = F2V(0.5f);
                const vfloat c65535v = F2V(65535.f);

                for (; col < width - 8; col += 8) {
                    for (int i = 0; i < 2; c = 2 - c, i++) {
                        dLv = onev / (onev + vabsf(LC2VFU(rgb[1][row][col - 2]) - LC2VFU(rgb[1][row][col])) + vabsf(LC2VFU(rgb[c][row][col + 1]) - LC2VFU(rgb[c][row][col - 1])));
                        dRv = onev / (onev + vabsf(LC2VFU(rgb[1][row][col + 2]) - LC2VFU(rgb[1][row][col])) + vabsf(LC2VFU(rgb[c][row][col + 1]) - LC2VFU(rgb[c][row][col - 1])));
                        dUv = onev / (onev + vabsf(LC2VFU(rgb[1][row - 2][col]) - LC2VFU(rgb[1][row][col])) + vabsf(LC2VFU(rgb[c][row + 1][col]) - LC2VFU(rgb[c][row - 1][col])));
                        dDv = onev / (onev + vabsf(LC2VFU(rgb[1][row + 2][col]) - LC2VFU(rgb[1][row][col])) + vabsf(LC2VFU(rgb[c][row + 1][col]) - LC2VFU(rgb[c][row - 1][col])));
                        v0v = CLIPV(LC2VFU(rgb[1][row][col]) + zd5v - ((LC2VFU(rgb[1][row][col - 1]) - LC2VFU(rgb[c][row][col - 1])) * dLv + (LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[c][row][col + 1])) * dRv + (LC2VFU(rgb[1][row -1][col]) - LC2VFU(rgb[c][row -1][col])) * dUv + (LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[c][row + 1][col])) * dDv ) / (dLv + dRv + dUv + dDv));
                        STC2VFU(rgb[c][row][col], v0v);
                    }
                }

#endif

                for (; col < width - 2; col += 2) {
                    for (int i = 0; i < 2; c = 2 - c, i++) {
                        float dL = 1.f / (1.f + fabsf(rgb[1][row][col - 2] - rgb[1][row][col]) + fabsf(rgb[c][row][col + 1] - rgb[c][row][col - 1]));
                        float dR = 1.f / (1.f + fabsf(rgb[1][row][col + 2] - rgb[1][row][col]) + fabsf(rgb[c][row][col + 1] - rgb[c][row][col - 1]));
                        float dU = 1.f / (1.f + fabsf(rgb[1][row - 2][col] - rgb[1][row][col]) + fabsf(rgb[c][row + 1][col] - rgb[c][row - 1][col]));
                        float dD = 1.f / (1.f + fabsf(rgb[1][row + 2][col] - rgb[1][row][col]) + fabsf(rgb[c][row + 1][col] - rgb[c][row - 1][col]));
                        float v0 = (rgb[1][row][col] + 0.5f - ((rgb[1][row][col - 1] - rgb[c][row][col - 1]) * dL + (rgb[1][row][col + 1] - rgb[c][row][col + 1]) * dR + (rgb[1][row - 1][col] - rgb[c][row - 1][col]) * dU + (rgb[1][row + 1][col] - rgb[c][row + 1][col]) * dD ) / (dL + dR + dU + dD));
                        rgb[c][row][col] = CLIP(v0);
                    }
                }
            }

            /* Reinforce integrated red/blue pixels on BLUE/RED pixel locations */
#ifdef _OPENMP
            #pragma omp for
#endif

            for (int row = 2; row < height - 2; row++) {
                int col = 2 + (fc(cfarray, row, 2) & 1);
                int c = 2 - fc(cfarray, row, col);
#ifdef __SSE2__
                vfloat dLv, dRv, dUv, dDv, v0v;
                const vfloat onev = F2V(1.f);
                const vfloat zd5v = F2V(0.5f);
                const vfloat c65535v = F2V(65535.f);

                for (; col < width - 8; col += 8) {
                    int d = 2 - c;
                    dLv = onev / (onev + vabsf(LC2VFU(rgb[d][row][col - 2]) - LC2VFU(rgb[d][row][col])) + vabsf(LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[1][row][col - 1])));
                    dRv = onev / (onev + vabsf(LC2VFU(rgb[d][row][col + 2]) - LC2VFU(rgb[d][row][col])) + vabsf(LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[1][row][col - 1])));
                    dUv = onev / (onev + vabsf(LC2VFU(rgb[d][row - 2][col]) - LC2VFU(rgb[d][row][col])) + vabsf(LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[1][row - 1][col])));
                    dDv = onev / (onev + vabsf(LC2VFU(rgb[d][row + 2][col]) - LC2VFU(rgb[d][row][col])) + vabsf(LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[1][row - 1][col])));
                    v0v = CLIPV(LC2VFU(rgb[1][row][col]) + zd5v - ((LC2VFU(rgb[1][row][col - 1]) - LC2VFU(rgb[c][row][col - 1])) * dLv + (LC2VFU(rgb[1][row][col + 1]) - LC2VFU(rgb[c][row][col + 1])) * dRv + (LC2VFU(rgb[1][row - 1][col]) - LC2VFU(rgb[c][row - 1][col])) * dUv + (LC2VFU(rgb[1][row + 1][col]) - LC2VFU(rgb[c][row + 1][col])) * dDv ) / (dLv + dRv + dUv + dDv));
                    STC2VFU(rgb[c][row][col], v0v);
                }

#endif

                for (; col < width - 2; col += 2) {
                    int d = 2 - c;
                    float dL = 1.f / (1.f + fabsf(rgb[d][row][col - 2] - rgb[d][row][col]) + fabsf(rgb[1][row][col + 1] - rgb[1][row][col - 1]));
                    float dR = 1.f / (1.f + fabsf(rgb[d][row][col + 2] - rgb[d][row][col]) + fabsf(rgb[1][row][col + 1] - rgb[1][row][col - 1]));
                    float dU = 1.f / (1.f + fabsf(rgb[d][row - 2][col] - rgb[d][row][col]) + fabsf(rgb[1][row + 1][col] - rgb[1][row - 1][col]));
                    float dD = 1.f / (1.f + fabsf(rgb[d][row + 2][col] - rgb[d][row][col]) + fabsf(rgb[1][row + 1][col] - rgb[1][row - 1][col]));
                    float v0 = (rgb[1][row][col] + 0.5f - ((rgb[1][row][col - 1] - rgb[c][row][col - 1]) * dL + (rgb[1][row][col + 1] - rgb[c][row][col + 1]) * dR + (rgb[1][row - 1][col] - rgb[c][row - 1][col]) * dU + (rgb[1][row + 1][col] - rgb[c][row + 1][col]) * dD ) / (dL + dR + dU + dD));
                    rgb[c][row][col] = CLIP(v0);
                }
            }
        } // end parallel
    }

}
}
#ifdef __SSE2__
#undef CLIPV
#endif

// LSMME demosaicing algorithm
// L. Zhang and X. Wu,
// Color demozaicing via directional Linear Minimum Mean Square-error Estimation,
// IEEE Trans. on Image Processing, vol. 14, pp. 2167-2178,
// Dec. 2005.
// Adapted to RawTherapee by Jacques Desmis 3/2013
// Improved speed and reduced memory consumption by Ingo Weyrich 2/2015
//TODO Tiles to reduce memory consumption
rpError lmmse_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int iterations)
{
    BENCHFUN
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    constexpr int ba = 4;
    const int rr1 = height + 2 * ba;
    const int cc1 = width + 2 * ba;
    const int w1 = cc1;
    const int w2 = 2 * w1;
    const int w3 = 3 * w1;
    const int w4 = 4 * w1;
    float h0, h1, h2, h3, h4, hs;
    h0 = 1.0f;
    h1 = exp( -1.0f / 8.0f);
    h2 = exp( -4.0f / 8.0f);
    h3 = exp( -9.0f / 8.0f);
    h4 = exp(-16.0f / 8.0f);
    hs = h0 + 2.0f * (h1 + h2 + h3 + h4);
    h0 /= hs;
    h1 /= hs;
    h2 /= hs;
    h3 /= hs;
    h4 /= hs;

    int passref;
    int iter;

    if (iterations <= 4) {
        iter = std::max(iterations - 1, 0);
        passref = 0;
    } else {
        iter = 3;
        passref = iterations - 4;
    }

    const bool applyGamma = iterations > 0;

    float *rix[5];
    float *qix[5] {nullptr};
    float *buffer = (float *)calloc(rr1 * cc1 * 5 * sizeof(float), 1);

    if (!buffer) { // allocation of big block of memory failed, try to get 5 smaller ones
        printf("lmmse_interpolate_omp: allocation of big memory block failed, try to get 5 smaller ones now...\n");
        bool allocationFailed = false;

        for (int i = 0; i < 5; i++) {
            qix[i] = (float *)calloc(rr1 * cc1 * sizeof(float), 1);

            if (!qix[i]) { // allocation of at least one small block failed
                allocationFailed = true;
                break;
            }
        }

        if (allocationFailed) { // fall back to igv_interpolate
            printf("lmmse_interpolate_omp: allocation of 5 small memory blocks failed, falling back to igv_demosaic...\n");

            for (int i = 0; i < 5; i++) { // free the already allocated buffers
                if (qix[i]) {
                    free(qix[i]);
                }
            }

            return RP_MEMORY_ERROR;
        }
    } else {
        qix[0] = buffer;

        for (int i = 1; i < 5; i++) {
            qix[i] = qix[i - 1] + rr1 * cc1;
        }
    }

    setProgCancel(0.0);

    LUTf gamtab(65536, LUT_CLIP_ABOVE | LUT_CLIP_BELOW);

    if (applyGamma) {
        for (int i = 0; i < 65536; i++) {
            float x = i / 65535.f;
            gamtab[i] = x <= 0.001867f ? x * 17.f : 1.044445f * xexpf(xlogf(x) / 2.4f) - 0.044445f;
        }
    } else {
        gamtab.makeIdentity(65535.f);
    }


#ifdef _OPENMP
    #pragma omp parallel private(rix)
#endif
    {
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int rrr = ba; rrr < rr1 - ba; rrr++) {
            for (int ccc = ba, row = rrr - ba; ccc < cc1 - ba; ccc++) {
                int col = ccc - ba;
                float *rix0 = qix[4] + rrr * cc1 + ccc;
                rix0[0] = gamtab[rawData[row][col]];
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.1);
        }

        // G-R(B)
#ifdef _OPENMP
        #pragma omp for schedule(dynamic,16)
#endif

        for (int rr = 2; rr < rr1 - 2; rr++) {
            // G-R(B) at R(B) location
            for (int cc = 2 + (fc(cfarray, rr, 2) & 1); cc < cc1 - 2; cc += 2) {
                rix[4] = qix[4] + rr * cc1 + cc;
                float v0 = 0.0625f * (rix[4][-w1 - 1] + rix[4][-w1 + 1] + rix[4][w1 - 1] + rix[4][w1 + 1]) + 0.25f * rix[4][0];
                // horizontal
                rix[0] = qix[0] + rr * cc1 + cc;
                rix[0][0] = -0.25f * (rix[4][ -2] + rix[4][ 2]) + 0.5f * (rix[4][ -1] + rix[4][0] + rix[4][ 1]);
                float Y = v0 + 0.5f * rix[0][0];

                if (rix[4][0] > 1.75f * Y) {
                    rix[0][0] = median(rix[0][0], rix[4][ -1], rix[4][ 1]);
                } else {
                    rix[0][0] = LIM(rix[0][0], 0.0f, 1.0f);
                }

                rix[0][0] -= rix[4][0];
                // vertical
                rix[1] = qix[1] + rr * cc1 + cc;
                rix[1][0] = -0.25f * (rix[4][-w2] + rix[4][w2]) + 0.5f * (rix[4][-w1] + rix[4][0] + rix[4][w1]);
                Y = v0 + 0.5f * rix[1][0];

                if (rix[4][0] > 1.75f * Y) {
                    rix[1][0] = median(rix[1][0], rix[4][-w1], rix[4][w1]);
                } else {
                    rix[1][0] = LIM(rix[1][0], 0.0f, 1.0f);
                }

                rix[1][0] -= rix[4][0];
            }

            // G-R(B) at G location
            for (int ccc = 2 + (fc(cfarray, rr, 3) & 1); ccc < cc1 - 2; ccc += 2) {
                rix[0] = qix[0] + rr * cc1 + ccc;
                rix[1] = qix[1] + rr * cc1 + ccc;
                rix[4] = qix[4] + rr * cc1 + ccc;
                rix[0][0] = 0.25f * (rix[4][ -2] + rix[4][ 2]) - 0.5f * (rix[4][ -1] + rix[4][0] + rix[4][ 1]);
                rix[1][0] = 0.25f * (rix[4][-w2] + rix[4][w2]) - 0.5f * (rix[4][-w1] + rix[4][0] + rix[4][w1]);
                rix[0][0] = LIM(rix[0][0], -1.0f, 0.0f) + rix[4][0];
                rix[1][0] = LIM(rix[1][0], -1.0f, 0.0f) + rix[4][0];
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.2);
        }


        // apply low pass filter on differential colors
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int rr = 4; rr < rr1 - 4; rr++) {
            for (int cc = 4; cc < cc1 - 4; cc++) {
                rix[0] = qix[0] + rr * cc1 + cc;
                rix[2] = qix[2] + rr * cc1 + cc;
                rix[2][0] = h0 * rix[0][0] + h1 * (rix[0][ -1] + rix[0][ 1]) + h2 * (rix[0][ -2] + rix[0][ 2]) + h3 * (rix[0][ -3] + rix[0][ 3]) + h4 * (rix[0][ -4] + rix[0][ 4]);
                rix[1] = qix[1] + rr * cc1 + cc;
                rix[3] = qix[3] + rr * cc1 + cc;
                rix[3][0] = h0 * rix[1][0] + h1 * (rix[1][-w1] + rix[1][w1]) + h2 * (rix[1][-w2] + rix[1][w2]) + h3 * (rix[1][-w3] + rix[1][w3]) + h4 * (rix[1][-w4] + rix[1][w4]);
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.3);
        }

        // interpolate G-R(B) at R(B)
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int rr = 4; rr < rr1 - 4; rr++) {
            int cc = 4 + (fc(cfarray, rr, 4) & 1);
#ifdef __SSE2__
            vfloat p1v, p2v, p3v, p4v, p5v, p6v, p7v, p8v, p9v, muv, vxv, vnv, xhv, vhv, xvv, vvv;
            const vfloat epsv = F2V(1e-7f);
            const vfloat ninev = F2V(9.f);

            for (; cc < cc1 - 10; cc += 8) {
                rix[0] = qix[0] + rr * cc1 + cc;
                rix[1] = qix[1] + rr * cc1 + cc;
                rix[2] = qix[2] + rr * cc1 + cc;
                rix[3] = qix[3] + rr * cc1 + cc;
                rix[4] = qix[4] + rr * cc1 + cc;
                // horizontal
                p1v = LC2VFU(rix[2][-4]);
                p2v = LC2VFU(rix[2][-3]);
                p3v = LC2VFU(rix[2][-2]);
                p4v = LC2VFU(rix[2][-1]);
                p5v = LC2VFU(rix[2][ 0]);
                p6v = LC2VFU(rix[2][ 1]);
                p7v = LC2VFU(rix[2][ 2]);
                p8v = LC2VFU(rix[2][ 3]);
                p9v = LC2VFU(rix[2][ 4]);
                muv = (p1v + p2v + p3v + p4v + p5v + p6v + p7v + p8v + p9v) / ninev;
                vxv = epsv + SQRV(p1v - muv) + SQRV(p2v - muv) + SQRV(p3v - muv) + SQRV(p4v - muv) + SQRV(p5v - muv) + SQRV(p6v - muv) + SQRV(p7v - muv) + SQRV(p8v - muv) + SQRV(p9v - muv);
                p1v -= LC2VFU(rix[0][-4]);
                p2v -= LC2VFU(rix[0][-3]);
                p3v -= LC2VFU(rix[0][-2]);
                p4v -= LC2VFU(rix[0][-1]);
                p5v -= LC2VFU(rix[0][ 0]);
                p6v -= LC2VFU(rix[0][ 1]);
                p7v -= LC2VFU(rix[0][ 2]);
                p8v -= LC2VFU(rix[0][ 3]);
                p9v -= LC2VFU(rix[0][ 4]);
                vnv = epsv + SQRV(p1v) + SQRV(p2v) + SQRV(p3v) + SQRV(p4v) + SQRV(p5v) + SQRV(p6v) + SQRV(p7v) + SQRV(p8v) + SQRV(p9v);
                xhv = (LC2VFU(rix[0][0]) * vxv + LC2VFU(rix[2][0]) * vnv) / (vxv + vnv);
                vhv = vxv * vnv / (vxv + vnv);

                // vertical
                p1v = LC2VFU(rix[3][-w4]);
                p2v = LC2VFU(rix[3][-w3]);
                p3v = LC2VFU(rix[3][-w2]);
                p4v = LC2VFU(rix[3][-w1]);
                p5v = LC2VFU(rix[3][  0]);
                p6v = LC2VFU(rix[3][ w1]);
                p7v = LC2VFU(rix[3][ w2]);
                p8v = LC2VFU(rix[3][ w3]);
                p9v = LC2VFU(rix[3][ w4]);
                muv = (p1v + p2v + p3v + p4v + p5v + p6v + p7v + p8v + p9v) / ninev;
                vxv = epsv + SQRV(p1v - muv) + SQRV(p2v - muv) + SQRV(p3v - muv) + SQRV(p4v - muv) + SQRV(p5v - muv) + SQRV(p6v - muv) + SQRV(p7v - muv) + SQRV(p8v - muv) + SQRV(p9v - muv);
                p1v -= LC2VFU(rix[1][-w4]);
                p2v -= LC2VFU(rix[1][-w3]);
                p3v -= LC2VFU(rix[1][-w2]);
                p4v -= LC2VFU(rix[1][-w1]);
                p5v -= LC2VFU(rix[1][  0]);
                p6v -= LC2VFU(rix[1][ w1]);
                p7v -= LC2VFU(rix[1][ w2]);
                p8v -= LC2VFU(rix[1][ w3]);
                p9v -= LC2VFU(rix[1][ w4]);
                vnv = epsv + SQRV(p1v) + SQRV(p2v) + SQRV(p3v) + SQRV(p4v) + SQRV(p5v) + SQRV(p6v) + SQRV(p7v) + SQRV(p8v) + SQRV(p9v);
                xvv = (LC2VFU(rix[1][0]) * vxv + LC2VFU(rix[3][0]) * vnv) / (vxv + vnv);
                vvv = vxv * vnv / (vxv + vnv);
                // interpolated G-R(B)
                muv = (xhv * vvv + xvv * vhv) / (vhv + vvv);
                STC2VFU(rix[4][0], muv);
            }

#endif

            for (; cc < cc1 - 4; cc += 2) {
                rix[0] = qix[0] + rr * cc1 + cc;
                rix[1] = qix[1] + rr * cc1 + cc;
                rix[2] = qix[2] + rr * cc1 + cc;
                rix[3] = qix[3] + rr * cc1 + cc;
                rix[4] = qix[4] + rr * cc1 + cc;
                // horizontal
                float p1 = rix[2][-4];
                float p2 = rix[2][-3];
                float p3 = rix[2][-2];
                float p4 = rix[2][-1];
                float p5 = rix[2][ 0];
                float p6 = rix[2][ 1];
                float p7 = rix[2][ 2];
                float p8 = rix[2][ 3];
                float p9 = rix[2][ 4];
                float mu = (p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9) / 9.f;
                float vx = 1e-7f + SQR(p1 - mu) + SQR(p2 - mu) + SQR(p3 - mu) + SQR(p4 - mu) + SQR(p5 - mu) + SQR(p6 - mu) + SQR(p7 - mu) + SQR(p8 - mu) + SQR(p9 - mu);
                p1 -= rix[0][-4];
                p2 -= rix[0][-3];
                p3 -= rix[0][-2];
                p4 -= rix[0][-1];
                p5 -= rix[0][ 0];
                p6 -= rix[0][ 1];
                p7 -= rix[0][ 2];
                p8 -= rix[0][ 3];
                p9 -= rix[0][ 4];
                float vn = 1e-7f + SQR(p1) + SQR(p2) + SQR(p3) + SQR(p4) + SQR(p5) + SQR(p6) + SQR(p7) + SQR(p8) + SQR(p9);
                float xh = (rix[0][0] * vx + rix[2][0] * vn) / (vx + vn);
                float vh = vx * vn / (vx + vn);

                // vertical
                p1 = rix[3][-w4];
                p2 = rix[3][-w3];
                p3 = rix[3][-w2];
                p4 = rix[3][-w1];
                p5 = rix[3][  0];
                p6 = rix[3][ w1];
                p7 = rix[3][ w2];
                p8 = rix[3][ w3];
                p9 = rix[3][ w4];
                mu = (p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9) / 9.f;
                vx = 1e-7f + SQR(p1 - mu) + SQR(p2 - mu) + SQR(p3 - mu) + SQR(p4 - mu) + SQR(p5 - mu) + SQR(p6 - mu) + SQR(p7 - mu) + SQR(p8 - mu) + SQR(p9 - mu);
                p1 -= rix[1][-w4];
                p2 -= rix[1][-w3];
                p3 -= rix[1][-w2];
                p4 -= rix[1][-w1];
                p5 -= rix[1][  0];
                p6 -= rix[1][ w1];
                p7 -= rix[1][ w2];
                p8 -= rix[1][ w3];
                p9 -= rix[1][ w4];
                vn = 1e-7f + SQR(p1) + SQR(p2) + SQR(p3) + SQR(p4) + SQR(p5) + SQR(p6) + SQR(p7) + SQR(p8) + SQR(p9);
                float xv = (rix[1][0] * vx + rix[3][0] * vn) / (vx + vn);
                float vv = vx * vn / (vx + vn);
                // interpolated G-R(B)
                rix[4][0] = (xh * vv + xv * vh) / (vh + vv);
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.4);
        }

        // copy CFA values
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int rr = 0; rr < rr1; rr++) {
            for (int cc = 0, row = rr - ba; cc < cc1; cc++) {
                int col = cc - ba;
                int c = fc(cfarray, rr, cc);
                rix[c] = qix[c] + rr * cc1 + cc;

                if ((row >= 0) & (row < height) & (col >= 0) & (col < width)) {
                    rix[c][0] = gamtab[rawData[row][col]];
                } else {
                    rix[c][0] = 0.f;
                }

                if (c != 1) {
                    rix[1] = qix[1] + rr * cc1 + cc;
                    rix[4] = qix[4] + rr * cc1 + cc;
                    rix[1][0] = rix[c][0] + rix[4][0];
                }
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.5);
        }

        // bilinear interpolation for R/B
        // interpolate R/B at G location
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int rr = 1; rr < rr1 - 1; rr++) {
            for (int cc = 1 + (fc(cfarray, rr, 2) & 1), c = fc(cfarray, rr, cc + 1); cc < cc1 - 1; cc += 2) {
                rix[c] = qix[c] + rr * cc1 + cc;
                rix[1] = qix[1] + rr * cc1 + cc;
                rix[c][0] = rix[1][0] + 0.5f * (rix[c][ -1] - rix[1][ -1] + rix[c][ 1] - rix[1][ 1]);
                c = 2 - c;
                rix[c] = qix[c] + rr * cc1 + cc;
                rix[c][0] = rix[1][0] + 0.5f * (rix[c][-w1] - rix[1][-w1] + rix[c][w1] - rix[1][w1]);
                c = 2 - c;
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.6);
        }

        // interpolate R/B at B/R location
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int rr = 1; rr < rr1 - 1; rr++) {
            for (int cc = 1 + (fc(cfarray, rr, 1) & 1), c = 2 - fc(cfarray, rr, cc); cc < cc1 - 1; cc += 2) {
                rix[c] = qix[c] + rr * cc1 + cc;
                rix[1] = qix[1] + rr * cc1 + cc;
                rix[c][0] = rix[1][0] + 0.25f * (rix[c][-w1] - rix[1][-w1] + rix[c][ -1] - rix[1][ -1] + rix[c][  1] - rix[1][  1] + rix[c][ w1] - rix[1][ w1]);
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.7);
        }

    }// End of parallelization 1

    // median filter/
    for (int pass = 0; pass < iter; pass++) {
        // Apply 3x3 median filter
        // Compute median(R-G) and median(B-G)

#ifdef _OPENMP
        #pragma omp parallel for private(rix)
#endif

        for (int rr = 1; rr < rr1 - 1; rr++) {
            for (int c = 0; c < 3; c += 2) {
                int d = c + 3 - (c == 0 ? 0 : 1);
                int cc = 1;
#ifdef __SSE2__

                for (; cc < cc1 - 4; cc += 4) {
                    rix[d] = qix[d] + rr * cc1 + cc;
                    rix[c] = qix[c] + rr * cc1 + cc;
                    rix[1] = qix[1] + rr * cc1 + cc;
                    // Assign 3x3 differential color values
                    const std::array<vfloat, 9> p = {
                        LVFU(rix[c][-w1 - 1]) - LVFU(rix[1][-w1 - 1]),
                        LVFU(rix[c][-w1]) - LVFU(rix[1][-w1]),
                        LVFU(rix[c][-w1 + 1]) - LVFU(rix[1][-w1 + 1]),
                        LVFU(rix[c][   -1]) - LVFU(rix[1][   -1]),
                        LVFU(rix[c][  0]) - LVFU(rix[1][  0]),
                        LVFU(rix[c][    1]) - LVFU(rix[1][    1]),
                        LVFU(rix[c][ w1 - 1]) - LVFU(rix[1][ w1 - 1]),
                        LVFU(rix[c][ w1]) - LVFU(rix[1][ w1]),
                        LVFU(rix[c][ w1 + 1]) - LVFU(rix[1][ w1 + 1])
                    };
                    STVFU(rix[d][0], median(p));
                }

#endif

                for (; cc < cc1 - 1; cc++) {
                    rix[d] = qix[d] + rr * cc1 + cc;
                    rix[c] = qix[c] + rr * cc1 + cc;
                    rix[1] = qix[1] + rr * cc1 + cc;
                    // Assign 3x3 differential color values
                    const std::array<float, 9> p = {
                        rix[c][-w1 - 1] - rix[1][-w1 - 1],
                        rix[c][-w1] - rix[1][-w1],
                        rix[c][-w1 + 1] - rix[1][-w1 + 1],
                        rix[c][   -1] - rix[1][   -1],
                        rix[c][  0] - rix[1][  0],
                        rix[c][    1] - rix[1][    1],
                        rix[c][ w1 - 1] - rix[1][ w1 - 1],
                        rix[c][ w1] - rix[1][ w1],
                        rix[c][ w1 + 1] - rix[1][ w1 + 1]
                    };
                    rix[d][0] = median(p);
                }
            }
        }

        // red/blue at GREEN pixel locations & red/blue and green at BLUE/RED pixel locations
#ifdef _OPENMP
        #pragma omp parallel for private (rix)
#endif

        for (int rr = 0; rr < rr1; rr++) {
            rix[0] = qix[0] + rr * cc1;
            rix[1] = qix[1] + rr * cc1;
            rix[2] = qix[2] + rr * cc1;
            rix[3] = qix[3] + rr * cc1;
            rix[4] = qix[4] + rr * cc1;
            int c0 = fc(cfarray, rr, 0);
            int c1 = fc(cfarray, rr, 1);

            if (c0 == 1) {
                c1 = 2 - c1;
                int d = c1 + 3 - (c1 == 0 ? 0 : 1);
                int cc;

                for (cc = 0; cc < cc1 - 1; cc += 2) {
                    rix[0][0] = rix[1][0] + rix[3][0];
                    rix[2][0] = rix[1][0] + rix[4][0];
                    rix[0]++;
                    rix[1]++;
                    rix[2]++;
                    rix[3]++;
                    rix[4]++;
                    rix[c1][0] = rix[1][0] + rix[d][0];
                    rix[1][0] = 0.5f * (rix[0][0] - rix[3][0] + rix[2][0] - rix[4][0]);
                    rix[0]++;
                    rix[1]++;
                    rix[2]++;
                    rix[3]++;
                    rix[4]++;
                }

                if (cc < cc1) { // remaining pixel, only if width is odd
                    rix[0][0] = rix[1][0] + rix[3][0];
                    rix[2][0] = rix[1][0] + rix[4][0];
                }
            } else {
                c0 = 2 - c0;
                int d = c0 + 3 - (c0 == 0 ? 0 : 1);
                int cc;

                for (cc = 0; cc < cc1 - 1; cc += 2) {
                    rix[c0][0] = rix[1][0] + rix[d][0];
                    rix[1][0] = 0.5f * (rix[0][0] - rix[3][0] + rix[2][0] - rix[4][0]);
                    rix[0]++;
                    rix[1]++;
                    rix[2]++;
                    rix[3]++;
                    rix[4]++;
                    rix[0][0] = rix[1][0] + rix[3][0];
                    rix[2][0] = rix[1][0] + rix[4][0];
                    rix[0]++;
                    rix[1]++;
                    rix[2]++;
                    rix[3]++;
                    rix[4]++;
                }

                if (cc < cc1) { // remaining pixel, only if width is odd
                    rix[c0][0] = rix[1][0] + rix[d][0];
                    rix[1][0] = 0.5f * (rix[0][0] - rix[3][0] + rix[2][0] - rix[4][0]);
                }
            }
        }
    }

    setProgCancel(0.8);

    if (applyGamma) {
        for (int i = 0; i < 65536; i++) {
            float x = i / 65535.f;
            gamtab[i] = 65535.f * (x <= 0.031746f ? x / 17.f : xexpf(xlogf((x + 0.044445f) / 1.044445f) * 2.4f));
        }
        gamtab.setClip(0);
    } else {
        gamtab.makeIdentity();
    }

    float** rgb[3];
    rgb[0] = red;
    rgb[1] = green;
    rgb[2] = blue;

    // copy result back to image matrix
#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int row = 0; row < height; row++) {
        for (int col = 0, rr = row + ba; col < width; col++) {
            int cc = col + ba;
            int c = fc(cfarray, row, col);

            for (int ii = 0; ii < 3; ii++)
                if (ii != c) {
                    float *rix0 = qix[ii] + rr * cc1 + cc;
                    ((rgb[ii]))[row][col] = gamtab[65535.f * rix0[0]];
                } else {
                    ((rgb[ii]))[row][col] = CLIP(rawData[row][col]);
                }
        }
    }

    setProgCancel(1.0);

    if (buffer) {
        free(buffer);
    } else {
        for (int i = 0; i < 5; i++) {
            free(qix[i]);
        }
    }

    if (iterations > 4) {
        refinement(width, height, red, green, blue, cfarray, setProgCancel, passref);
    }

    return RP_NO_ERROR;
}

