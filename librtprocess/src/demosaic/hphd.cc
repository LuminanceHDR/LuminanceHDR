/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2019 Gabor Horvath <hgabor@rawtherapee.com>
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
#include "jaggedarray.h"
#include "librtprocess.h"
#include "rt_math.h"
#include "opthelper.h"
#include "StopWatch.h"
#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;
using namespace librtprocess;

namespace {

rpError hphd_vertical(const float * const *rawData, float** hpmap, int col_from, int col_to, int H)
{

    // process 'numCols' columns for better usage of L1 cpu cache (especially faster for large values of H)
    constexpr int numCols = 8;

    JaggedArray<float> temp(numCols, H, true);
    JaggedArray<float> avg(numCols, H, true);
    JaggedArray<float> dev(numCols, H, true);

    if(!(temp && avg && dev)) {
        return RP_MEMORY_ERROR;
    }

    int k = col_from;
#ifdef __SSE2__
    const vfloat ninev = F2V(9.f);
    const vfloat epsv = F2V(0.001f);
#endif
    for (; k < col_to - 7; k += numCols) {
        for (int i = 5; i < H - 5; i++) {
#ifdef _OPENMP
            #pragma omp simd
#endif
            for(int h = 0; h < numCols; ++h) {
                temp[i][h] = std::fabs((rawData[i - 5][k + h] - rawData[i + 5][k + h])  - 8 * (rawData[i - 4][k + h] - rawData[i + 4][k + h]) + 27 * (rawData[i - 3][k + h] - rawData[i + 3][k + h]) - 48 * (rawData[i - 2][k + h] - rawData[i + 2][k + h]) + 42 * (rawData[i - 1][k + h] - rawData[i + 1][k + h]));
            }
        }

        for (int j = 4; j < H - 4; j++) {
#ifdef __SSE2__
            // faster than #pragma omp simd...
            const vfloat avgL1 = ((LVFU(temp[j - 4][0]) + LVFU(temp[j - 3][0])) + (LVFU(temp[j - 2][0]) + LVFU(temp[j - 1][0])) + (LVFU(temp[j][0]) + LVFU(temp[j + 1][0])) + (LVFU(temp[j + 2][0]) + LVFU(temp[j + 3][0])) + LVFU(temp[j + 4][0])) / ninev;
            STVFU(avg[j][0], avgL1);
            STVFU(dev[j][0], vmaxf(epsv, (SQRV(LVFU(temp[j - 4][0]) - avgL1) + SQRV(LVFU(temp[j - 3][0]) - avgL1)) + (SQRV(LVFU(temp[j - 2][0]) - avgL1) + SQRV(LVFU(temp[j - 1][0]) - avgL1)) + (SQRV(LVFU(temp[j][0]) - avgL1) + SQRV(LVFU(temp[j + 1][0]) - avgL1)) + (SQRV(LVFU(temp[j + 2][0]) - avgL1) + SQRV(LVFU(temp[j + 3][0]) - avgL1)) + SQRV(LVFU(temp[j + 4][0]) - avgL1)));
            const vfloat avgL2 = ((LVFU(temp[j - 4][4]) + LVFU(temp[j - 3][4])) + (LVFU(temp[j - 2][4]) + LVFU(temp[j - 1][4])) + (LVFU(temp[j][4]) + LVFU(temp[j + 1][4])) + (LVFU(temp[j + 2][4]) + LVFU(temp[j + 3][4])) + LVFU(temp[j + 4][4])) / ninev;
            STVFU(avg[j][4], avgL2);
            STVFU(dev[j][4], vmaxf(epsv, (SQRV(LVFU(temp[j - 4][4]) - avgL2) + SQRV(LVFU(temp[j - 3][4]) - avgL2)) + (SQRV(LVFU(temp[j - 2][4]) - avgL2) + SQRV(LVFU(temp[j - 1][4]) - avgL2)) + (SQRV(LVFU(temp[j][4]) - avgL2) + SQRV(LVFU(temp[j + 1][4]) - avgL2)) + (SQRV(LVFU(temp[j + 2][4]) - avgL2) + SQRV(LVFU(temp[j + 3][4]) - avgL2)) + SQRV(LVFU(temp[j + 4][4]) - avgL2)));
#else
#ifdef _OPENMP
            #pragma omp simd
#endif
            for(int h = 0; h < numCols; ++h) {
                const float avgL = ((temp[j - 4][h] + temp[j - 3][h]) + (temp[j - 2][h] + temp[j - 1][h]) + (temp[j][h] + temp[j + 1][h]) + (temp[j + 2][h] + temp[j + 3][h]) + temp[j + 4][h]) / 9.f;
                avg[j][h] = avgL;
                dev[j][h] = std::max(0.001f, (SQR(temp[j - 4][h] - avgL) + SQR(temp[j - 3][h] - avgL)) + (SQR(temp[j - 2][h] - avgL) + SQR(temp[j - 1][h] - avgL)) + (SQR(temp[j][h] - avgL) + SQR(temp[j + 1][h] - avgL)) + (SQR(temp[j + 2][h] - avgL) + SQR(temp[j + 3][h] - avgL)) + SQR(temp[j + 4][h] - avgL));
            }
#endif
        }

        for (int j = 5; j < H - 5; j++) {
#ifdef _OPENMP
            #pragma omp simd
#endif
            for(int h = 0; h < numCols; ++h) {
                const float avgL = avg[j - 1][h];
                const float avgR = avg[j + 1][h];
                const float devL = dev[j - 1][h];
                const float devR = dev[j + 1][h];
                hpmap[j][k + h] = avgL + (avgR - avgL) * devL / (devL + devR);
            }
        }
    }
    for (; k < col_to; k++) {
        for (int i = 5; i < H - 5; i++) {
            temp[i][0] = std::fabs((rawData[i - 5][k] - rawData[i + 5][k]) - 8 * (rawData[i - 4][k] - rawData[i + 4][k]) + 27 * (rawData[i - 3][k] - rawData[i + 3][k]) - 48 * (rawData[i - 2][k] - rawData[i + 2][k]) + 42 * (rawData[i - 1][k] -rawData[i + 1][k]));
        }

        for (int j = 4; j < H - 4; j++) {
            const float avgL = (temp[j - 4][0] + temp[j - 3][0] + temp[j - 2][0] + temp[j - 1][0] + temp[j][0] + temp[j + 1][0] + temp[j + 2][0] + temp[j + 3][0] + temp[j + 4][0]) / 9.f;
            avg[j][0] = avgL;
            dev[j][0] = std::max(0.001f, SQR(temp[j - 4][0] - avgL) + SQR(temp[j - 3][0] - avgL) + SQR(temp[j - 2][0] - avgL) + SQR(temp[j - 1][0] - avgL) + SQR(temp[j][0] - avgL) + SQR(temp[j + 1][0] - avgL) + SQR(temp[j + 2][0] - avgL) + SQR(temp[j + 3][0] - avgL) + SQR(temp[j + 4][0] - avgL));
        }

        for (int j = 5; j < H - 5; j++) {
            const float avgL = avg[j - 1][0];
            const float avgR = avg[j + 1][0];
            const float devL = dev[j - 1][0];
            const float devR = dev[j + 1][0];
            hpmap[j][k] = avgL + (avgR - avgL) * devL / (devL + devR);
        }
    }

    return RP_NO_ERROR;
}

rpError hphd_horizontal(const float * const *rawData, float** hpmap, int row_from, int row_to, int W)
{

    float* temp = new (std::nothrow) float[W] ();
    float* avg = new (std::nothrow) float[W] ();
    float* dev = new (std::nothrow) float[W] ();

    rpError rc = RP_NO_ERROR;
    if(!(temp && avg && dev)) {
        rc = RP_MEMORY_ERROR;
    } else {

#ifdef __SSE2__
        const vfloat onev = F2V(1.f);
        const vfloat twov = F2V(2.f);
        const vfloat zd8v = F2V(0.8f);
#endif
        for (int i = row_from; i < row_to; i++) {
#ifdef _OPENMP
            #pragma omp simd
#endif
            for (int j = 5; j < W - 5; j++) {
                temp[j] = std::fabs((rawData[i][j - 5] - rawData[i][j + 5]) - 8 * (rawData[i][j - 4] - rawData[i][j + 4]) + 27 * (rawData[i][j - 3] - rawData[i][j + 3]) - 48 * (rawData[i][j - 2] - rawData[i][j + 2]) + 42 * (rawData[i][j - 1] - rawData[i][j + 1]));
            }

#ifdef _OPENMP
            #pragma omp simd
#endif
            for (int j = 4; j < W - 4; j++) {
                const float avgL = ((temp[j - 4] + temp[j - 3]) + (temp[j - 2] + temp[j - 1]) + (temp[j] + temp[j + 1]) + (temp[j + 2] + temp[j + 3]) + temp[j + 4]) / 9.f;
                avg[j] = avgL;
                dev[j] = std::max(0.001f, (SQR(temp[j - 4] - avgL) + SQR(temp[j - 3] - avgL)) + (SQR(temp[j - 2] - avgL) + SQR(temp[j - 1] - avgL)) + (SQR(temp[j] - avgL) + SQR(temp[j + 1] - avgL)) + (SQR(temp[j + 2] - avgL) + SQR(temp[j + 3] - avgL)) + SQR(temp[j + 4] - avgL));
            }

            int j = 5;
#ifdef __SSE2__
            // faster than #pragma omp simd
            for (; j < W - 8; j+=4) {
                const vfloat avgL = LVFU(avg[j - 1]);
                const vfloat avgR = LVFU(avg[j + 1]);
                const vfloat devL = LVFU(dev[j - 1]);
                const vfloat devR = LVFU(dev[j + 1]);
                const vfloat hpv = avgL + (avgR - avgL) * devL / (devL + devR);

                const vfloat hpmapoldv = LVFU(hpmap[i][j]);
                const vfloat hpmapv = vselfzero(vmaskf_lt(hpmapoldv, zd8v * hpv), twov);
                STVFU(hpmap[i][j], vself(vmaskf_lt(hpv, zd8v * hpmapoldv), onev, hpmapv));
            }
#endif
            for (; j < W - 5; j++) {
                const float avgL = avg[j - 1];
                const float avgR = avg[j + 1];
                const float devL = dev[j - 1];
                const float devR = dev[j + 1];
                const float hpv = avgL + (avgR - avgL) * devL / (devL + devR);

                if (hpmap[i][j] < 0.8f * hpv) {
                    hpmap[i][j] = 2;
                } else if (hpv < 0.8f * hpmap[i][j]) {
                    hpmap[i][j] = 1;
                } else {
                    hpmap[i][j] = 0;
                }
            }
        }
    }

    delete [] temp;
    delete [] avg;
    delete [] dev;

    return rc;
}

rpError hphd_RedGreenBlue(const float * const *rawData, const unsigned cfarray[2][2], const float * const *hpmap, int W, int H, float **red, float **green, float **blue)
{

    rpError rc = RP_NO_ERROR;
    constexpr float eps = 0.001f;
#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        int firstRow = -1;
        int lastRow = -1;
#ifdef _OPENMP
        // note, static scheduling is important in this implementation
        #pragma omp for schedule(static)
#endif
        for (int i = 3; i < H - 3; i++) {
            if (firstRow == -1) {
                firstRow = i;
            }
            lastRow = i;
            for (int j = 3; j < W - 3; j++) {
                if (fc(cfarray, i, j) & 1) {
                    green[i][j] = rawData[i][j];
                } else {
                    if (hpmap[i][j] == 1) {
                        const float g2 = rawData[i][j + 1] - rawData[i][j + 2] * 0.5f;
                        const float g4 = rawData[i][j - 1] - rawData[i][j - 2] * 0.5f;

                        const float dx = eps + std::fabs(rawData[i][j + 1] - rawData[i][j - 1]);
                        float d1 = rawData[i][j + 3] - rawData[i][j + 1];
                        float d2 = rawData[i][j + 2] - rawData[i][j];
                        float d3 = rawData[i - 1][j + 2] - rawData[i - 1][j];
                        float d4 = rawData[i + 1][j + 2] - rawData[i + 1][j];

                        const float e2 = 1.f / (dx + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        d1 = rawData[i][j - 3] - rawData[i][j - 1];
                        d2 = rawData[i][j - 2] - rawData[i][j];
                        d3 = rawData[i - 1][j - 2] - rawData[i - 1][j];
                        d4 = rawData[i + 1][j - 2] - rawData[i + 1][j];

                        const float e4 = 1.f / (dx + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        green[i][j] = rawData[i][j] * 0.5f + (e2 * g2 + e4 * g4) / (e2 + e4);
                    } else if (hpmap[i][j] == 2) {
                        const float g1 = rawData[i - 1][j] - rawData[i - 2][j] * 0.5f;
                        const float g3 = rawData[i + 1][j] - rawData[i + 2][j] * 0.5f;

                        const float dy = eps + std::fabs(rawData[i + 1][j] - rawData[i - 1][j]);
                        float d1 = rawData[i - 1][j] - rawData[i - 3][j];
                        float d2 = rawData[i][j] - rawData[i - 2][j];
                        float d3 = rawData[i][j - 1] - rawData[i - 2][j - 1];
                        float d4 = rawData[i][j + 1] - rawData[i - 2][j + 1];

                        const float e1 = 1.f / (dy + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        d1 = rawData[i + 1][j] - rawData[i + 3][j];
                        d2 = rawData[i][j] - rawData[i + 2][j];
                        d3 = rawData[i][j - 1] - rawData[i + 2][j - 1];
                        d4 = rawData[i][j + 1] - rawData[i + 2][j + 1];

                        const float e3 = 1.f / (dy + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        green[i][j] = rawData[i][j] * 0.5f + (e1 * g1 + e3 * g3) / (e1 + e3);
                    } else {
                        const float g1 = rawData[i - 1][j] - rawData[i - 2][j] * 0.5f;
                        const float g2 = rawData[i][j + 1] - rawData[i][j + 2] * 0.5f;
                        const float g3 = rawData[i + 1][j] - rawData[i + 2][j] * 0.5f;
                        const float g4 = rawData[i][j - 1] - rawData[i][j - 2] * 0.5f;

                        const float dx = eps + std::fabs(rawData[i][j + 1] - rawData[i][j - 1]);
                        const float dy = eps + std::fabs(rawData[i + 1][j] - rawData[i - 1][j]);

                        float d1 = rawData[i - 1][j] - rawData[i - 3][j];
                        float d2 = rawData[i][j] - rawData[i - 2][j];
                        float d3 = rawData[i][j - 1] - rawData[i - 2][j - 1];
                        float d4 = rawData[i][j + 1] - rawData[i - 2][j + 1];

                        const float e1 = 1.f / (dy + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        d1 = rawData[i][j + 3] - rawData[i][j + 1];
                        d2 = rawData[i][j + 2] - rawData[i][j];
                        d3 = rawData[i - 1][j + 2] - rawData[i - 1][j];
                        d4 = rawData[i + 1][j + 2] - rawData[i + 1][j];

                        const float e2 = 1.f / (dx + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        d1 = rawData[i + 1][j] - rawData[i + 3][j];
                        d2 = rawData[i][j] - rawData[i + 2][j];
                        d3 = rawData[i][j - 1] - rawData[i + 2][j - 1];
                        d4 = rawData[i][j + 1] - rawData[i + 2][j + 1];

                        const float e3 = 1.f / (dy + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        d1 = rawData[i][j - 3] - rawData[i][j - 1];
                        d2 = rawData[i][j - 2] - rawData[i][j];
                        d3 = rawData[i - 1][j - 2] - rawData[i - 1][j];
                        d4 = rawData[i + 1][j - 2] - rawData[i + 1][j];

                        const float e4 = 1.f / (dx + (std::fabs(d1) + std::fabs(d2)) + (std::fabs(d3) + std::fabs(d4)) * 0.5f);

                        green[i][j] = rawData[i][j] * 0.5f + ((e1 * g1 + e2 * g2) + (e3 * g3 + e4 * g4)) / (e1 + e2 + e3 + e4);
                    }
                }
            }
            if (i - 1 > firstRow) {
                interpolate_row_redblue(rawData, cfarray, red[i - 1], blue[i - 1], green[i - 2], green[i - 1], green[i], i - 1, W);
            }

        }
        if (firstRow > 3 && firstRow < H - 4) {
            interpolate_row_redblue(rawData, cfarray, red[firstRow], blue[firstRow], green[firstRow - 1], green[firstRow], green[firstRow + 1], firstRow, W);
        }

        if (lastRow > 3 && lastRow < H - 4) {
            interpolate_row_redblue(rawData, cfarray, red[lastRow], blue[lastRow], green[lastRow - 1], green[lastRow], green[lastRow + 1], lastRow, W);
        }
#ifdef _OPENMP
        #pragma omp single
#endif
        {
            // let the first thread, which is out of work, do the border interpolation
            rc = bayerborder_demosaic(W, H, 4, rawData, red, green, blue, cfarray);
        }
    }
    return rc;
}

}


rpError hphd_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel)
{
    BENCHFUN
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    rpError rc = RP_NO_ERROR;
    const int W = width;
    const int H = height;

    setProgCancel(0.0);

    JaggedArray<float> hpmap(W, H, true);

#ifdef _OPENMP
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
        int blk = W / nthreads;
        rpError errCode;
        if (tid < nthreads - 1) {
            errCode = hphd_vertical(rawData, hpmap, tid * blk, (tid + 1) * blk, H);
        } else {
            errCode = hphd_vertical(rawData, hpmap, tid * blk, W, H);
        }
        #pragma omp critical
        {
            if (errCode) {
                rc = errCode;
            }
        }
    }
#else
    rc = hphd_vertical(rawData, hpmap, 0, W, H);
#endif

    if (!rc) {
        setProgCancel(0.35);

#ifdef _OPENMP
        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            int nthreads = omp_get_num_threads();
            int blk = H / nthreads;
            rpError errCode;

            if (tid < nthreads - 1) {
                errCode = hphd_horizontal(rawData, hpmap, tid * blk, (tid + 1) * blk, W);
            } else {
                errCode = hphd_horizontal(rawData, hpmap, tid * blk, H, W);
            }
            #pragma omp critical
            {
                if (errCode) {
                    rc = errCode;
                }
            }
        }
#else
        rc = hphd_horizontal(rawData, hpmap, 0, H, W);
#endif
        if (!rc) {
            setProgCancel(0.43);
            rc = hphd_RedGreenBlue(rawData, cfarray, hpmap, W, H, red, green, blue);
        }
    }
    setProgCancel(1.0);
    return rc;
}

