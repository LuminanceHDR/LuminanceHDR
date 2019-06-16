////////////////////////////////////////////////////////////////
//
//  Chromatic Aberration correction on raw bayer cfa data
//
//  copyright (c) 2008-2010 Emil Martinec <ejmartin@uchicago.edu>
//  copyright (c) for improvements (speedups, iterated correction and avoid colour shift) 2018 Ingo Weyrich <heckflosse67@gmx.de>
//
//  code dated: September 8, 2018
//
//  CA_correct_RT.cc is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include <memory>

#include "bayerhelper.h"
#include "gauss.h"
#include "librtprocess.h"
#include "jaggedarray.h"
#include "rt_math.h"
#include "median.h"
#include "StopWatch.h"

namespace {

bool LinEqSolve(int nDim, double* pfMatr, double* pfVect, double pfSolution[16])
{
//==============================================================================
// return 1 if system not solving, 0 if system solved
// nDim - system dimension
// pfMatr - matrix with coefficients
// pfVect - vector with free members
// pfSolution - vector with system solution
// pfMatr becomes triangular after function call
// pfVect changes after function call
//
// Developer: Henry Guennadi Levkin
//
//==============================================================================

    double fAcc;

    int i, j, k;

    for(k = 0; k < (nDim - 1); k++) { // base row of matrix
        // search of line with max element
        double fMaxElem = fabs( pfMatr[k * nDim + k] );
        int m = k;

        for (i = k + 1; i < nDim; i++) {
            if(fMaxElem < fabs(pfMatr[i * nDim + k]) ) {
                fMaxElem = pfMatr[i * nDim + k];
                m = i;
            }
        }

        // permutation of base line (index k) and max element line(index m)
        if(m != k) {
            for(i = k; i < nDim; i++) {
                fAcc               = pfMatr[k * nDim + i];
                pfMatr[k * nDim + i] = pfMatr[m * nDim + i];
                pfMatr[m * nDim + i] = fAcc;
            }

            fAcc = pfVect[k];
            pfVect[k] = pfVect[m];
            pfVect[m] = fAcc;
        }

        if( pfMatr[k * nDim + k] == 0.) {
            //linear system has no solution
            return false; // needs improvement !!!
        }

        // triangulation of matrix with coefficients
        for(j = (k + 1); j < nDim; j++) { // current row of matrix
            fAcc = - pfMatr[j * nDim + k] / pfMatr[k * nDim + k];

            for(i = k; i < nDim; i++) {
                pfMatr[j * nDim + i] = pfMatr[j * nDim + i] + fAcc * pfMatr[k * nDim + i];
            }

            pfVect[j] = pfVect[j] + fAcc * pfVect[k]; // free member recalculation
        }
    }

    for(k = (nDim - 1); k >= 0; k--) {
        pfSolution[k] = pfVect[k];

        for(i = (k + 1); i < nDim; i++) {
            pfSolution[k] -= (pfMatr[k * nDim + i] * pfSolution[i]);
        }

        pfSolution[k] = pfSolution[k] / pfMatr[k * nDim + k];
    }

    return true;
}
//end of linear equation solver

}

using namespace std;
using namespace librtprocess;
rpError CA_correct(
    int winx,
    int winy,
    int winw,
    int winh,
    const bool autoCA,
    size_t autoIterations,
    const double cared,
    const double cablue,
    bool avoidColourshift,
    const float * const *rawDataIn,
    float **rawDataOut,
    const unsigned cfarray[2][2],
    const std::function<bool(double)> &setProgCancel,
    double fitParams[2][2][16],
    bool fitParamsIn,
    float inputScale,
    float outputScale,
    size_t chunkSize,
    bool measure
)
{
    BENCHFUN
// multithreaded and vectorized by Ingo Weyrich
    std::unique_ptr<StopWatch> stop;

    if (measure) {
        std::cout << "CA correcting " << winw << "x" << winh << " image with " << chunkSize << " tiles per thread" << std::endl;
        stop.reset(new StopWatch("CA correction"));
    }

    constexpr int ts = 128;
    constexpr int tsh = ts / 2;
    constexpr int cb = 2; // 2 pixels border will be excluded from correction
    //shifts to location of vertical and diagonal neighbors
    constexpr int v1 = ts, v2 = 2 * ts, v3 = 3 * ts, v4 = 4 * ts;

    // Test for RGB cfa
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    // local variables
    const int W = winw - winx;
    const int H = winh - winy;

    std::unique_ptr<JaggedArray<float>> redFactor;
    std::unique_ptr<JaggedArray<float>> blueFactor;
    std::unique_ptr<JaggedArray<float>> oldraw;
    if (avoidColourshift) {
        redFactor.reset(new JaggedArray<float>((W + 1 - 2 * cb) / 2, (H + 1 - 2 * cb) / 2));
        blueFactor.reset(new JaggedArray<float>((W + 1 - 2 * cb) / 2, (H + 1 - 2 * cb) / 2));
        oldraw.reset(new JaggedArray<float>((W + 1- 2 * cb) / 2, H- 2 * cb));
        if(!*redFactor.get() || !*blueFactor.get() || !*oldraw.get()) {
            return RP_MEMORY_ERROR;
        }
        // copy raw values before ca correction
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for (int i = cb; i < H - cb; ++i) {
            for (int j = cb + (fc(cfarray, i + winy, winx) & 1); j < W - cb; j += 2) {
                (*oldraw)[i - cb][(j - cb) / 2] = rawDataIn[i + winy][j + winx];
            }
        }
    }

    if (rawDataOut && rawDataOut != rawDataIn) {
        // copy raw values before ca correction
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                rawDataOut[i + winy][j + winx] = rawDataIn[i + winy][j + winx];
            }
        }
    }

    double progress = 0.0;
    setProgCancel(progress);

    const int width = W + (W & 1), height = H;
    constexpr int border = 8;
    constexpr int border2 = 16;

    const int vz1 = (height + border2) % (ts - border2) == 0 ? 1 : 0;
    const int hz1 = (width + border2) % (ts - border2) == 0 ? 1 : 0;
    const int vblsz = ceil((float)(height + border2) / (ts - border2) + 2 + vz1);
    const int hblsz = ceil((float)(width + border2) / (ts - border2) + 2 + hz1);

    //temporary array to store simple interpolation of G
    std::unique_ptr<float> buffer(new (std::nothrow) float[height * width + vblsz * hblsz * (2 * 2 + 1)]);

    float *Gtmp = buffer.get();
    if (!Gtmp) {
        return RP_MEMORY_ERROR;
    }

    float *RawDataTmp = Gtmp + (winh * (winw + (winw & 1))) / 2;
    //block CA shift values and weight assigned to block
    float *const blockwt = Gtmp + height * width;
    memset(blockwt, 0, vblsz * hblsz * (2 * 2 + 1) * sizeof(float));
    float (*blockshifts)[2][2] = (float (*)[2][2])(blockwt + vblsz * hblsz);

    // Because we can't break parallel processing, we need a switch do handle the errors
    bool processpasstwo = true;

    const size_t iterations =
        autoCA
            ? std::max<size_t>(autoIterations, 1)
            : 1;

    const bool fitParamsSet = fitParamsIn && iterations < 2;

    rpError rc = RP_NO_ERROR;
    for (size_t it = 0; it < iterations && processpasstwo; ++it) {
        float blockave[2][2] = {};
        float blocksqave[2][2] = {};
        float blockdenom[2][2] = {};
        float blockvar[2][2];

        //order of 2d polynomial fit (polyord), and numpar=polyord^2
        int polyord = 4, numpar = 16;

        constexpr float eps = 1e-5f, eps2 = 1e-10f; //tolerance to avoid dividing by zero


#ifdef _OPENMP
        #pragma omp parallel
#endif
        {
            int progresscounter = 0;

            //direction of the CA shift in a tile
            int GRBdir[2][3];

            int shifthfloor[3], shiftvfloor[3], shifthceil[3], shiftvceil[3];

            //polynomial fit coefficients
            //residual CA shift amount within a plaquette
            float   shifthfrac[3], shiftvfrac[3];

            // assign working space
            constexpr int buffersize = ts * ts + 8 * ts * tsh + 8 * 16;
            constexpr int buffersizePassTwo = ts * ts + 4 * ts * tsh + 4 * 16;
            std::unique_ptr<float> bufferThr(new (std::nothrow) float[(autoCA && !fitParamsSet) ? buffersize : buffersizePassTwo]);
            float *data = bufferThr.get();
#ifdef _OPENMP
            #pragma omp critical
#endif
            {
                if (!data) {
                    rc = RP_MEMORY_ERROR;
                }
            }
#ifdef _OPENMP
            #pragma omp barrier
#endif
            if (!rc) {
                // shift the beginning of all arrays but the first by 64 bytes to avoid cache miss conflicts on CPUs which have <= 4-way associative L1-Cache

                //rgb data in a tile
                float* rgb[3];
                rgb[0] = data;
                rgb[1] = data + ts * tsh + 16;
                rgb[2] = data + ts * ts + ts * tsh + 32;

                if (autoCA && !fitParamsSet) {
                    //high pass filter for R/B in vertical direction
                    float *rbhpfh  = data + 2 * ts * ts + 48;
                    //high pass filter for R/B in horizontal direction
                    float *rbhpfv  = data + 2 * ts * ts + ts * tsh + 64;
                    //low pass filter for R/B in horizontal direction
                    float *rblpfh  = data + 3 * ts * ts + 80;
                    //low pass filter for R/B in vertical direction
                    float *rblpfv  = data + 3 * ts * ts + ts * tsh + 96;
                    //low pass filter for colour differences in horizontal direction
                    float *grblpfh = data + 4 * ts * ts + 112;
                    //low pass filter for colour differences in vertical direction
                    float *grblpfv = data + 4 * ts * ts + ts * tsh + 128;
                    // Main algorithm: Tile loop calculating correction parameters per tile

                    //local quadratic fit to shift data within a tile
                    float coeff[2][3][2];
                    //measured CA shift parameters for a tile
                    float CAshift[2][2];

                    //per thread data for evaluation of block CA shift variance
                    float   blockavethr[2][2] = {{0, 0}, {0, 0}}, blocksqavethr[2][2] = {{0, 0}, {0, 0}}, blockdenomthr[2][2] = {{0, 0}, {0, 0}};

#ifdef _OPENMP
                    #pragma omp for collapse(2) schedule(dynamic, chunkSize) nowait
#endif
                    for (int top = -border ; top < height; top += ts - border2)
                        for (int left = -border; left < width - (W & 1); left += ts - border2) {
                            memset(data, 0, buffersize * sizeof(float));
                            const int vblock = ((top + border) / (ts - border2)) + 1;
                            const int hblock = ((left + border) / (ts - border2)) + 1;
                            const int bottom = std::min(top + ts, height + border);
                            const int right  = std::min(left + ts, width - (W & 1) + border);
                            const int rr1 = bottom - top;
                            const int cc1 = right - left;
                            const int rrmin = top < 0 ? border : 0;
                            const int rrmax = bottom > height ? height - top : rr1;
                            const int ccmin = left < 0 ? border : 0;
                            const int ccmax = (right > width - (W & 1)) ? width - (W & 1) - left : cc1;

                            // rgb from input CFA data
                            // rgb values should be floating point numbers between 0 and 1
                            // after white balance multipliers are applied

#ifdef __SSE2__
                            vfloat cinScalev = F2V(inputScale);
#endif

                            for (int rr = rrmin; rr < rrmax; rr++) {
                                int row = rr + top;
                                int cc = ccmin;
                                int col = cc + left;
#ifdef __SSE2__
                                int c0 = fc(cfarray, rr, cc);
                                if(c0 == 1) {
                                    rgb[c0][rr * ts + cc] = rawDataOut[row][col] / inputScale;
                                    cc++;
                                    col++;
                                    c0 = fc(cfarray, rr, cc);
                                }
                                int indx1 = rr * ts + cc;
                                for (; cc < ccmax - 7; cc+=8, col+=8, indx1 += 8) {
                                    vfloat val1 = LVFU(rawDataOut[row][col]) / cinScalev;
                                    vfloat val2 = LVFU(rawDataOut[row][col + 4]) / cinScalev;
                                    vfloat nonGreenv = _mm_shuffle_ps(val1,val2,_MM_SHUFFLE( 2,0,2,0 ));
                                    STVFU(rgb[c0][indx1 >> 1], nonGreenv);
                                    STVFU(rgb[1][indx1], val1);
                                    STVFU(rgb[1][indx1 + 4], val2);
                                }
#endif
                                for (; cc < ccmax; cc++, col++) {
                                    int c = fc(cfarray, rr, cc);
                                    int indx = rr * ts + cc;
                                    rgb[c][indx >> ((c & 1) ^ 1)] = rawDataOut[row][col] / inputScale;
                                }
                            }

                            //fill borders
                            if (rrmin > 0) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = ccmin; cc < ccmax; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + cc) >> ((c & 1) ^ 1)] = rgb[c][((border2 - rr) * ts + cc) >> ((c & 1) ^ 1)];
                                    }
                            }

                            if (rrmax < rr1) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = ccmin; cc < ccmax; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][((rrmax + rr)*ts + cc) >> ((c & 1) ^ 1)] = rawDataOut[(height - rr - 2)][left + cc] / inputScale;
                                    }
                            }

                            if (ccmin > 0) {
                                for (int rr = rrmin; rr < rrmax; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + cc) >> ((c & 1) ^ 1)] = rgb[c][(rr * ts + border2 - cc) >> ((c & 1) ^ 1)];
                                    }
                            }

                            if (ccmax < cc1) {
                                for (int rr = rrmin; rr < rrmax; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + ccmax + cc) >> ((c & 1) ^ 1)] = rawDataOut[(top + rr)][(width - cc - 2)] / inputScale;
                                    }
                            }

                            //also, fill the image corners
                            if (rrmin > 0 && ccmin > 0) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + cc) >> ((c & 1) ^ 1)] = rawDataOut[border2 - rr][border2 - cc] / inputScale;
                                    }
                            }

                            if (rrmax < rr1 && ccmax < cc1) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][((rrmax + rr)*ts + ccmax + cc) >> ((c & 1) ^ 1)] = rawDataOut[(height - rr - 2)][(width - cc - 2)] / inputScale;
                                    }
                            }

                            if (rrmin > 0 && ccmax < cc1) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + ccmax + cc) >> ((c & 1) ^ 1)] = rawDataOut[(border2 - rr)][(width - cc - 2)] / inputScale;
                                    }
                            }

                            if (rrmax < rr1 && ccmin > 0) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][((rrmax + rr)*ts + cc) >> ((c & 1) ^ 1)] = rawDataOut[(height - rr - 2)][(border2 - cc)] / inputScale;
                                    }
                            }

                            //end of border fill
                            //end of initialization


#ifdef __SSE2__
                            vfloat onev = F2V(1.f);
                            vfloat epsv = F2V(eps);
#endif
                            for (int rr = 3; rr < rr1 - 3; rr++) {
                                int row = rr + top;
                                int cc = 3 + (fc(cfarray, rr,3) & 1);
                                int indx = rr * ts + cc;
                                int c = fc(cfarray, rr,cc);
#ifdef __SSE2__
                                for (; cc < cc1 - 9; cc+=8, indx+=8) {
                                    //compute directional weights using image gradients
                                    vfloat rgb1mv1v = LC2VFU(rgb[1][indx - v1]);
                                    vfloat rgb1pv1v = LC2VFU(rgb[1][indx + v1]);
                                    vfloat rgbcv = LVFU(rgb[c][indx >> 1]);
                                    vfloat temp1v = epsv + vabsf(rgb1mv1v - rgb1pv1v);
                                    vfloat wtuv = onev / SQRV(temp1v + vabsf(rgbcv - LVFU(rgb[c][(indx - v2) >> 1])) + vabsf(rgb1mv1v - LC2VFU(rgb[1][indx - v3])));
                                    vfloat wtdv = onev / SQRV(temp1v + vabsf(rgbcv - LVFU(rgb[c][(indx + v2) >> 1])) + vabsf(rgb1pv1v - LC2VFU(rgb[1][indx + v3])));
                                    vfloat rgb1m1v = LC2VFU(rgb[1][indx - 1]);
                                    vfloat rgb1p1v = LC2VFU(rgb[1][indx + 1]);
                                    vfloat temp2v = epsv + vabsf(rgb1m1v - rgb1p1v);
                                    vfloat wtlv = onev / SQRV(temp2v + vabsf(rgbcv - LVFU(rgb[c][(indx - 2) >> 1])) + vabsf(rgb1m1v - LC2VFU(rgb[1][indx - 3])));
                                    vfloat wtrv = onev / SQRV(temp2v + vabsf(rgbcv - LVFU(rgb[c][(indx + 2) >> 1])) + vabsf(rgb1p1v - LC2VFU(rgb[1][indx + 3])));

                                    //store in rgb array the interpolated G value at R/B grid points using directional weighted average
                                    vfloat resultv = (wtuv * rgb1mv1v + wtdv * rgb1pv1v + wtlv * rgb1m1v + wtrv * rgb1p1v) / (wtuv + wtdv + wtlv + wtrv);
                                    STC2VFU(rgb[1][indx], resultv);
                                }

#endif
                                for (; cc < cc1 - 3; cc+=2, indx+=2) {
                                    //compute directional weights using image gradients
                                    float wtu = 1.f / SQR(eps + fabsf(rgb[1][indx + v1] - rgb[1][indx - v1]) + fabsf(rgb[c][indx >> 1] - rgb[c][(indx - v2) >> 1]) + fabsf(rgb[1][indx - v1] - rgb[1][indx - v3]));
                                    float wtd = 1.f / SQR(eps + fabsf(rgb[1][indx - v1] - rgb[1][indx + v1]) + fabsf(rgb[c][indx >> 1] - rgb[c][(indx + v2) >> 1]) + fabsf(rgb[1][indx + v1] - rgb[1][indx + v3]));
                                    float wtl = 1.f / SQR(eps + fabsf(rgb[1][indx + 1] - rgb[1][indx - 1]) + fabsf(rgb[c][indx >> 1] - rgb[c][(indx - 2) >> 1]) + fabsf(rgb[1][indx - 1] - rgb[1][indx - 3]));
                                    float wtr = 1.f / SQR(eps + fabsf(rgb[1][indx - 1] - rgb[1][indx + 1]) + fabsf(rgb[c][indx >> 1] - rgb[c][(indx + 2) >> 1]) + fabsf(rgb[1][indx + 1] - rgb[1][indx + 3]));

                                    //store in rgb array the interpolated G value at R/B grid points using directional weighted average
                                    rgb[1][indx] = (wtu * rgb[1][indx - v1] + wtd * rgb[1][indx + v1] + wtl * rgb[1][indx - 1] + wtr * rgb[1][indx + 1]) / (wtu + wtd + wtl + wtr);
                                }

                                if (row > -1 && row < height) {
                                    int offset = (fc(cfarray, row,std::max(left + 3, 0)) & 1);
                                    int col = std::max(left + 3, 0) + offset;
                                    int indx1 = rr * ts + 3 - (left < 0 ? (left+3) : 0) + offset;
#ifdef __SSE2__
                                    for(; col < std::min(cc1 + left - 3, width) - 7; col+=8, indx1+=8) {
                                        STVFU(Gtmp[(row * width + col) >> 1], LC2VFU(rgb[1][indx1]));
                                    }
#endif
                                    for(; col < std::min(cc1 + left - 3, width); col+=2, indx1+=2) {
                                        Gtmp[(row * width + col) >> 1] = rgb[1][indx1];
                                    }
                                }

                            }

#ifdef __SSE2__
                            vfloat zd25v = F2V(0.25f);
#endif
                            for (int rr = 4; rr < rr1 - 4; rr++) {
                                int cc = 4 + (fc(cfarray, rr, 2) & 1);
                                int indx = rr * ts + cc;
                                int c = fc(cfarray, rr, cc);
#ifdef __SSE2__
                                for (; cc < cc1 - 10; cc += 8, indx += 8) {
                                    vfloat rgb1v = LC2VFU(rgb[1][indx]);
                                    vfloat rgbcv = LVFU(rgb[c][indx >> 1]);
                                    vfloat rgb1mv4 = LC2VFU(rgb[1][indx - v4]);
                                    vfloat rgb1pv4 = LC2VFU(rgb[1][indx + v4]);
                                    vfloat temp1v = vabsf(vabsf((rgb1v - rgbcv) - (rgb1pv4 - LVFU(rgb[c][(indx + v4) >> 1]))) +
                                                          vabsf(rgb1mv4 - LVFU(rgb[c][(indx - v4) >> 1]) - rgb1v + rgbcv) -
                                                          vabsf(rgb1mv4 - LVFU(rgb[c][(indx - v4) >> 1]) - rgb1pv4 + LVFU(rgb[c][(indx + v4) >> 1])));
                                    STVFU(rbhpfv[indx >> 1], temp1v);
                                    vfloat rgb1m4 = LC2VFU(rgb[1][indx - 4]);
                                    vfloat rgb1p4 = LC2VFU(rgb[1][indx + 4]);
                                    vfloat temp2v = vabsf(vabsf((rgb1v - rgbcv) - (rgb1p4 - LVFU(rgb[c][(indx + 4) >> 1]))) +
                                                          vabsf(rgb1m4 - LVFU(rgb[c][(indx - 4) >> 1]) - rgb1v + rgbcv) -
                                                          vabsf(rgb1m4 - LVFU(rgb[c][(indx - 4) >> 1]) - rgb1p4 + LVFU(rgb[c][(indx + 4) >> 1])));
                                    STVFU(rbhpfh[indx >> 1], temp2v);

                                    //low and high pass 1D filters of G in vertical/horizontal directions
                                    rgb1v = vmul2f(rgb1v);
                                    vfloat glpfvv = (rgb1v + LC2VFU(rgb[1][indx + v2]) + LC2VFU(rgb[1][indx - v2]));
                                    vfloat glpfhv = (rgb1v + LC2VFU(rgb[1][indx + 2]) + LC2VFU(rgb[1][indx - 2]));
                                    rgbcv = vmul2f(rgbcv);
                                    STVFU(rblpfv[indx >> 1], zd25v * vabsf(glpfvv - (rgbcv + LVFU(rgb[c][(indx + v2) >> 1]) + LVFU(rgb[c][(indx - v2) >> 1]))));
                                    STVFU(rblpfh[indx >> 1], zd25v * vabsf(glpfhv - (rgbcv + LVFU(rgb[c][(indx + 2) >> 1]) + LVFU(rgb[c][(indx - 2) >> 1]))));
                                    STVFU(grblpfv[indx >> 1], zd25v * (glpfvv + (rgbcv + LVFU(rgb[c][(indx + v2) >> 1]) + LVFU(rgb[c][(indx - v2) >> 1]))));
                                    STVFU(grblpfh[indx >> 1], zd25v * (glpfhv + (rgbcv + LVFU(rgb[c][(indx + 2) >> 1]) + LVFU(rgb[c][(indx - 2) >> 1]))));
                                }

#endif
                                for (; cc < cc1 - 4; cc += 2, indx += 2) {
                                    rbhpfv[indx >> 1] = fabsf(fabsf((rgb[1][indx] - rgb[c][indx >> 1]) - (rgb[1][indx + v4] - rgb[c][(indx + v4) >> 1])) +
                                                              fabsf((rgb[1][indx - v4] - rgb[c][(indx - v4) >> 1]) - (rgb[1][indx] - rgb[c][indx >> 1])) -
                                                              fabsf((rgb[1][indx - v4] - rgb[c][(indx - v4) >> 1]) - (rgb[1][indx + v4] - rgb[c][(indx + v4) >> 1])));
                                    rbhpfh[indx >> 1] = fabsf(fabsf((rgb[1][indx] - rgb[c][indx >> 1]) - (rgb[1][indx + 4] - rgb[c][(indx + 4) >> 1])) +
                                                              fabsf((rgb[1][indx - 4] - rgb[c][(indx - 4) >> 1]) - (rgb[1][indx] - rgb[c][indx >> 1])) -
                                                              fabsf((rgb[1][indx - 4] - rgb[c][(indx - 4) >> 1]) - (rgb[1][indx + 4] - rgb[c][(indx + 4) >> 1])));

                                    //low and high pass 1D filters of G in vertical/horizontal directions
                                    float glpfv = (2.f * rgb[1][indx] + rgb[1][indx + v2] + rgb[1][indx - v2]);
                                    float glpfh = (2.f * rgb[1][indx] + rgb[1][indx + 2] + rgb[1][indx - 2]);
                                    rblpfv[indx >> 1] = 0.25f * fabsf(glpfv - (2.f * rgb[c][indx >> 1] + rgb[c][(indx + v2) >> 1] + rgb[c][(indx - v2) >> 1]));
                                    rblpfh[indx >> 1] = 0.25f * fabsf(glpfh - (2.f * rgb[c][indx >> 1] + rgb[c][(indx + 2) >> 1] + rgb[c][(indx - 2) >> 1]));
                                    grblpfv[indx >> 1] = 0.25f * (glpfv + (2.f * rgb[c][indx >> 1] + rgb[c][(indx + v2) >> 1] + rgb[c][(indx - v2) >> 1]));
                                    grblpfh[indx >> 1] = 0.25f * (glpfh + (2.f * rgb[c][indx >> 1] + rgb[c][(indx + 2) >> 1] + rgb[c][(indx - 2) >> 1]));
                                }
                            }

                            for (int dir = 0; dir < 2; dir++) {
                                for (int k = 0; k < 3; k++) {
                                    for (int c = 0; c < 2; c++) {
                                        coeff[dir][k][c] = 0;
                                    }
                                }
                            }

#ifdef __SSE2__
                            vfloat zd3v = F2V(0.3f);
                            vfloat zd1v = F2V(0.1f);
                            vfloat zd5v = F2V(0.5f);
#endif

                            // along line segments, find the point along each segment that minimizes the colour variance
                            // averaged over the tile; evaluate for up/down and left/right away from R/B grid point
                            for (int rr = 8; rr < rr1 - 8; rr++) {
                                int cc = 8 + (fc(cfarray, rr, 2) & 1);
                                int indx = rr * ts + cc;
                                int c = fc(cfarray, rr, cc);
#ifdef __SSE2__
                                vfloat coeff00v = ZEROV;
                                vfloat coeff01v = ZEROV;
                                vfloat coeff02v = ZEROV;
                                vfloat coeff10v = ZEROV;
                                vfloat coeff11v = ZEROV;
                                vfloat coeff12v = ZEROV;
                                for (; cc < cc1 - 14; cc += 8, indx += 8) {

                                    //in linear interpolation, colour differences are a quadratic function of interpolation position;
                                    //solve for the interpolation position that minimizes colour difference variance over the tile

                                    //vertical
                                    vfloat temp1 = zd3v * (LC2VFU(rgb[1][indx + ts + 1]) - LC2VFU(rgb[1][indx - ts - 1]));
                                    vfloat temp2 = zd3v * (LC2VFU(rgb[1][indx - ts + 1]) - LC2VFU(rgb[1][indx + ts - 1]));
                                    vfloat gdiffvv = (LC2VFU(rgb[1][indx + ts]) - LC2VFU(rgb[1][indx - ts])) + (temp1 - temp2);
                                    vfloat deltgrbv = LVFU(rgb[c][indx >> 1]) - LC2VFU(rgb[1][indx]);

                                    vfloat gradwtvv = (LVFU(rbhpfv[indx >> 1]) + zd5v * (LVFU(rbhpfv[(indx >> 1) + 1]) + LVFU(rbhpfv[(indx >> 1) - 1]))) * (LVFU(grblpfv[(indx >> 1) - v1]) + LVFU(grblpfv[(indx >> 1) + v1])) / (epsv + zd1v * (LVFU(grblpfv[(indx >> 1) - v1]) + LVFU(grblpfv[(indx >> 1) + v1])) + LVFU(rblpfv[(indx >> 1) - v1]) + LVFU(rblpfv[(indx >> 1) + v1]));

                                    coeff00v += gradwtvv * deltgrbv * deltgrbv;
                                    coeff01v += gradwtvv * gdiffvv * deltgrbv;
                                    coeff02v += gradwtvv * gdiffvv * gdiffvv;

                                    //horizontal
                                    vfloat gdiffhv = (LC2VFU(rgb[1][indx + 1]) - LC2VFU(rgb[1][indx - 1])) + (temp1 + temp2);

                                    vfloat gradwthv = (LVFU(rbhpfh[indx >> 1]) + zd5v * (LVFU(rbhpfh[(indx >> 1) + v1]) + LVFU(rbhpfh[(indx >> 1) - v1]))) * (LVFU(grblpfh[(indx >> 1) - 1]) + LVFU(grblpfh[(indx >> 1) + 1])) / (epsv + zd1v * (LVFU(grblpfh[(indx >> 1) - 1]) + LVFU(grblpfh[(indx >> 1) + 1])) + LVFU(rblpfh[(indx >> 1) - 1]) + LVFU(rblpfh[(indx >> 1) + 1]));

                                    coeff10v += gradwthv * deltgrbv * deltgrbv;
                                    coeff11v += gradwthv * gdiffhv * deltgrbv;
                                    coeff12v += gradwthv * gdiffhv * gdiffhv;
                                }

                                coeff[0][0][c>>1] += vhadd(coeff00v);
                                coeff[0][1][c>>1] += vhadd(coeff01v);
                                coeff[0][2][c>>1] += vhadd(coeff02v);
                                coeff[1][0][c>>1] += vhadd(coeff10v);
                                coeff[1][1][c>>1] += vhadd(coeff11v);
                                coeff[1][2][c>>1] += vhadd(coeff12v);

#endif
                                for (; cc < cc1 - 8; cc += 2, indx += 2) {

                                    //in linear interpolation, colour differences are a quadratic function of interpolation position;
                                    //solve for the interpolation position that minimizes colour difference variance over the tile

                                    //vertical
                                    float gdiff = (rgb[1][indx + ts] - rgb[1][indx - ts]) + 0.3f * (rgb[1][indx + ts + 1] - rgb[1][indx - ts + 1] + rgb[1][indx + ts - 1] - rgb[1][indx - ts - 1]);
                                    float deltgrb = (rgb[c][indx >> 1] - rgb[1][indx]);

                                    float gradwt = (rbhpfv[indx >> 1] + 0.5f * (rbhpfv[(indx >> 1) + 1] + rbhpfv[(indx >> 1) - 1]) ) * (grblpfv[(indx >> 1) - v1] + grblpfv[(indx >> 1) + v1]) / (eps + 0.1f * (grblpfv[(indx >> 1) - v1] + grblpfv[(indx >> 1) + v1]) + rblpfv[(indx >> 1) - v1] + rblpfv[(indx >> 1) + v1]);

                                    coeff[0][0][c>>1] += gradwt * deltgrb * deltgrb;
                                    coeff[0][1][c>>1] += gradwt * gdiff * deltgrb;
                                    coeff[0][2][c>>1] += gradwt * gdiff * gdiff;

                                    //horizontal
                                    gdiff = (rgb[1][indx + 1] - rgb[1][indx - 1]) + 0.3f * (rgb[1][indx + 1 + ts] - rgb[1][indx - 1 + ts] + rgb[1][indx + 1 - ts] - rgb[1][indx - 1 - ts]);

                                    gradwt = (rbhpfh[indx >> 1] + 0.5f * (rbhpfh[(indx >> 1) + v1] + rbhpfh[(indx >> 1) - v1]) ) * (grblpfh[(indx >> 1) - 1] + grblpfh[(indx >> 1) + 1]) / (eps + 0.1f * (grblpfh[(indx >> 1) - 1] + grblpfh[(indx >> 1) + 1]) + rblpfh[(indx >> 1) - 1] + rblpfh[(indx >> 1) + 1]);

                                    coeff[1][0][c>>1] += gradwt * deltgrb * deltgrb;
                                    coeff[1][1][c>>1] += gradwt * gdiff * deltgrb;
                                    coeff[1][2][c>>1] += gradwt * gdiff * gdiff;

                                    //  In Mathematica,
                                    //  f[x_]=Expand[Total[Flatten[
                                    //  ((1-x) RotateLeft[Gint,shift1]+x RotateLeft[Gint,shift2]-cfapad)^2[[dv;;-1;;2,dh;;-1;;2]]]]];
                                    //  extremum = -.5Coefficient[f[x],x]/Coefficient[f[x],x^2]
                                }
                            }

                            for (int dir = 0; dir < 2; dir++) {
                                for (int k = 0; k < 3; k++) {
                                    for (int c = 0; c < 2; c++) {
                                        coeff[dir][k][c] *= 0.25f;
                                        if(k == 1) {
                                            coeff[dir][k][c] *= 0.3125f;
                                        } else if(k == 2) {
                                            coeff[dir][k][c] *= SQR(0.3125f);
                                        }
                                    }
                                }
                            }

                            for (int c = 0; c < 2; c++) {
                                for (int dir = 0; dir < 2; dir++) { // vert/hor

                                    // CAshift[dir][c] are the locations
                                    // that minimize colour difference variances;
                                    // This is the approximate _optical_ location of the R/B pixels
                                    if (coeff[dir][2][c] > eps2) {
                                        CAshift[dir][c] = coeff[dir][1][c] / coeff[dir][2][c];
                                        blockwt[vblock * hblsz + hblock] = coeff[dir][2][c] / (eps + coeff[dir][0][c]) ;
                                    } else {
                                        CAshift[dir][c] = 17.0;
                                        blockwt[vblock * hblsz + hblock] = 0;
                                    }

                                    //data structure = CAshift[vert/hor][colour]
                                    //dir : 0=vert, 1=hor

                                    //offset gives NW corner of square containing the min; dir : 0=vert, 1=hor
                                    if (fabsf(CAshift[dir][c]) < 2.0f) {
                                        blockavethr[dir][c] += CAshift[dir][c];
                                        blocksqavethr[dir][c] += SQR(CAshift[dir][c]);
                                        blockdenomthr[dir][c] += 1;
                                    }
                                    //evaluate the shifts to the location that minimizes CA within the tile
                                    blockshifts[vblock * hblsz + hblock][c][dir] = CAshift[dir][c]; //vert/hor CA shift for R/B

                                }//vert/hor
                            }//colour

                            progresscounter++;

                            if(progresscounter % 8 == 0)
#ifdef _OPENMP
                                #pragma omp critical (cadetectpass1)
#endif
                            {
                                progress += (double)(8.0 * (ts - border2) * (ts - border2)) / (2 * height * width);

                                if (progress > 1.0) {
                                    progress = 1.0;
                                }
                                setProgCancel(progress);
                            }

                        }

                    //end of diagnostic pass
#ifdef _OPENMP
                    #pragma omp critical (cadetectpass2)
#endif
                    {
                        for (int dir = 0; dir < 2; dir++)
                            for (int c = 0; c < 2; c++) {
                                blockdenom[dir][c] += blockdenomthr[dir][c];
                                blocksqave[dir][c] += blocksqavethr[dir][c];
                                blockave[dir][c]   += blockavethr[dir][c];
                            }
                    }
#ifdef _OPENMP
                    #pragma omp barrier

                    #pragma omp single
#endif
                    {
                        for (int dir = 0; dir < 2; dir++)
                            for (int c = 0; c < 2; c++) {
                                if (blockdenom[dir][c]) {
                                    blockvar[dir][c] = blocksqave[dir][c] / blockdenom[dir][c] - SQR(blockave[dir][c] / blockdenom[dir][c]);
                                } else {
                                    processpasstwo = false;
                                    std::cout << "blockdenom vanishes" << std::endl;
                                    break;
                                }
                            }

                        //now prepare for CA correction pass
                        //first, fill border blocks of blockshift array
                        if(processpasstwo) {
                            for (int vblock = 1; vblock < vblsz - 1; vblock++) { //left and right sides
                                for (int c = 0; c < 2; c++) {
                                    for (int i = 0; i < 2; i++) {
                                        blockshifts[vblock * hblsz][c][i] = blockshifts[(vblock) * hblsz + 2][c][i];
                                        blockshifts[vblock * hblsz + hblsz - 1][c][i] = blockshifts[(vblock) * hblsz + hblsz - 3][c][i];
                                    }
                                }
                            }

                            for (int hblock = 0; hblock < hblsz; hblock++) { //top and bottom sides
                                for (int c = 0; c < 2; c++) {
                                    for (int i = 0; i < 2; i++) {
                                        blockshifts[hblock][c][i] = blockshifts[2 * hblsz + hblock][c][i];
                                        blockshifts[(vblsz - 1)*hblsz + hblock][c][i] = blockshifts[(vblsz - 3) * hblsz + hblock][c][i];
                                    }
                                }
                            }

                            //end of filling border pixels of blockshift array

                            //initialize fit arrays
                            double polymat[2][2][256], shiftmat[2][2][16];

                            for (int i = 0; i < 256; i++) {
                                polymat[0][0][i] = polymat[0][1][i] = polymat[1][0][i] = polymat[1][1][i] = 0;
                            }

                            for (int i = 0; i < 16; i++) {
                                shiftmat[0][0][i] = shiftmat[0][1][i] = shiftmat[1][0][i] = shiftmat[1][1][i] = 0;
                            }

                            int numblox[2] = {0, 0};

                            for (int vblock = 1; vblock < vblsz - 1; vblock++)
                                for (int hblock = 1; hblock < hblsz - 1; hblock++) {
                                    // block 3x3 median of blockshifts for robustness
                                    for (int c = 0; c < 2; c ++) {
                                        float bstemp[2];
                                        for (int dir = 0; dir < 2; dir++) {
                                            //temporary storage for median filter
                                            const std::array<float, 9> p = {
                                                blockshifts[(vblock - 1) * hblsz + hblock - 1][c][dir],
                                                blockshifts[(vblock - 1) * hblsz + hblock][c][dir],
                                                blockshifts[(vblock - 1) * hblsz + hblock + 1][c][dir],
                                                blockshifts[(vblock) * hblsz + hblock - 1][c][dir],
                                                blockshifts[(vblock) * hblsz + hblock][c][dir],
                                                blockshifts[(vblock) * hblsz + hblock + 1][c][dir],
                                                blockshifts[(vblock + 1) * hblsz + hblock - 1][c][dir],
                                                blockshifts[(vblock + 1) * hblsz + hblock][c][dir],
                                                blockshifts[(vblock + 1) * hblsz + hblock + 1][c][dir]
                                            };
                                            bstemp[dir] = median(p);
                                        }

                                        //now prepare coefficient matrix; use only data points within caautostrength/2 std devs of zero
                                        if (SQR(bstemp[0]) > 8.f * blockvar[0][c] || SQR(bstemp[1]) > 8.f * blockvar[1][c]) {
                                            continue;
                                        }

                                        numblox[c]++;

                                        for (int dir = 0; dir < 2; dir++) {
                                            double powVblockInit = 1.0;
                                            for (int i = 0; i < polyord; i++) {
                                                double powHblockInit = 1.0;
                                                for (int j = 0; j < polyord; j++) {
                                                    double powVblock = powVblockInit;
                                                    for (int m = 0; m < polyord; m++) {
                                                        double powHblock = powHblockInit;
                                                        for (int n = 0; n < polyord; n++) {
                                                            polymat[c][dir][numpar * (polyord * i + j) + (polyord * m + n)] += powVblock * powHblock * blockwt[vblock * hblsz + hblock];
                                                            powHblock *= hblock;
                                                        }
                                                        powVblock *= vblock;
                                                    }
                                                    shiftmat[c][dir][(polyord * i + j)] += powVblockInit * powHblockInit * bstemp[dir] * blockwt[vblock * hblsz + hblock];
                                                    powHblockInit *= hblock;
                                                }
                                                powVblockInit *= vblock;
                                            }//monomials
                                        }//dir
                                    }//c
                                }//blocks

                            numblox[1] = std::min(numblox[0], numblox[1]);

                            //if too few data points, restrict the order of the fit to linear
                            if (numblox[1] < 32) {
                                polyord = 2;
                                numpar = 4;

                                if (numblox[1] < 10) {

                                    std::cout << "numblox = " << numblox[1] << std::endl;
                                    processpasstwo = false;
                                }
                            }

                            if(processpasstwo)

                                //fit parameters to blockshifts
                                for (int c = 0; c < 2; c++)
                                    for (int dir = 0; dir < 2; dir++) {
                                        if (!LinEqSolve(numpar, polymat[c][dir], shiftmat[c][dir], fitParams[c][dir])) {
                                            std::cout << "CA correction pass failed -- can't solve linear equations for colour %d direction " << c << std::endl;
                                            processpasstwo = false;
                                        }
                                    }
                        }

                        //fitparams[polyord*i+j] gives the coefficients of (vblock^i hblock^j) in a polynomial fit for i,j<=4
                    }
                    //end of initialization for CA correction pass
                    //only executed if autoCA is true
                }

                // Main algorithm: Tile loop
                if(processpasstwo) {
                    float *grbdiff = data + 2 * ts * ts + 48; // there is no overlap in buffer usage => share
                    //green interpolated to optical sample points for R/B
                    float *gshift  = data + 2 * ts * ts + ts * tsh + 64; // there is no overlap in buffer usage => share
#ifdef _OPENMP
                    #pragma omp for schedule(dynamic, chunkSize) collapse(2) nowait
#endif
                    for (int top = winy-border; top < winy+winh; top += ts - border2)
                      for (int left = winx-border; left < winx+winw; left += ts - border2) {
                            memset(data, 0, buffersizePassTwo * sizeof(float));
                            float lblockshifts[2][2];
                            const int vblock = ((top + border) / (ts - border2)) + 1;
                            const int hblock = ((left + border) / (ts - border2)) + 1;
                            const int bottom = std::min(top + ts, winy + winh + border);
                            const int right  = std::min(left + ts, winx + winw + border);
                            const int rr1 = bottom - top;
                            const int cc1 = right - left;

                            const int rrmin = top < 0 ? border : 0;
                            const int rrmax = bottom > height ? height - top : rr1;
                            const int ccmin = left < 0 ? border : 0;
                            const int ccmax = (right > width - (W & 1)) ? width - (W & 1) - left : cc1;

                            // rgb from input CFA data
                            // rgb values should be floating point number between 0 and 1
                            // after white balance multipliers are applied

#ifdef __SSE2__
                            vfloat cinscalev = F2V(inputScale);
                            vmask gmask = _mm_set_epi32(0, 0xffffffff, 0, 0xffffffff);
#endif
                            for (int rr = rrmin; rr < rrmax; rr++) {
                                int row = rr + top;
                                int cc = ccmin;
                                int col = cc + left;
                                int indx = row * width + col;
                                int indx1 = rr * ts + cc;
#ifdef __SSE2__
                                int c = fc(cfarray, rr, cc);
                                if(c & 1) {
                                    rgb[1][indx1] = rawDataOut[row][col] / inputScale;
                                    indx++;
                                    indx1++;
                                    cc++;
                                    col++;
                                    c = fc(cfarray, rr, cc);
                                }
                                for (; cc < ccmax - 7; cc += 8, col += 8, indx += 8, indx1 += 8) {
                                    vfloat val1v = LVFU(rawDataOut[row][col]) / cinscalev;
                                    vfloat val2v = LVFU(rawDataOut[row][col + 4]) / cinscalev;
                                    STVFU(rgb[c][indx1 >> 1], _mm_shuffle_ps(val1v, val2v, _MM_SHUFFLE(2, 0, 2, 0)));
                                    vfloat gtmpv = LVFU(Gtmp[indx >> 1]);
                                    STVFU(rgb[1][indx1], vself(gmask, PERMUTEPS(gtmpv, _MM_SHUFFLE(1, 1, 0, 0)), val1v));
                                    STVFU(rgb[1][indx1 + 4], vself(gmask, PERMUTEPS(gtmpv, _MM_SHUFFLE(3, 3, 2, 2)), val2v));
                                }
#endif
                                for (; cc < ccmax; cc++, col++, indx++, indx1++) {
                                    int cl = fc(cfarray, rr, cc);
                                    rgb[cl][indx1 >> ((cl & 1) ^ 1)] = rawDataOut[row][col] / inputScale;

                                    if ((cl & 1) == 0) {
                                        rgb[1][indx1] = Gtmp[indx >> 1];
                                    }
                                }
                            }

                            //fill borders
                            if (rrmin > 0) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = ccmin; cc < ccmax; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + cc) >> ((c & 1) ^ 1)] = rgb[c][((border2 - rr) * ts + cc) >> ((c & 1) ^ 1)];
                                        rgb[1][rr * ts + cc] = rgb[1][(border2 - rr) * ts + cc];
                                    }
                            }

                            if (rrmax < rr1) {
                                for (int rr = 0; rr < std::min(border, rr1 - rrmax); rr++)
                                    for (int cc = ccmin; cc < ccmax; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][((rrmax + rr)*ts + cc) >> ((c & 1) ^ 1)] = (rawDataOut[(height - rr - 2)][left + cc]) / inputScale;
                                        if ((c & 1) == 0) {
                                            rgb[1][(rrmax + rr)*ts + cc] = Gtmp[((height - rr - 2) * width + left + cc) >> 1];
                                        }
                                    }
                            }

                            if (ccmin > 0) {
                                for (int rr = rrmin; rr < rrmax; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + cc) >> ((c & 1) ^ 1)] = rgb[c][(rr * ts + border2 - cc) >> ((c & 1) ^ 1)];
                                        rgb[1][rr * ts + cc] = rgb[1][rr * ts + border2 - cc];
                                    }
                            }

                            if (ccmax < cc1) {
                                for (int rr = rrmin; rr < rrmax; rr++)
                                    for (int cc = 0; cc < std::min(border, cc1 - ccmax); cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + ccmax + cc) >> ((c & 1) ^ 1)] = (rawDataOut[(top + rr)][(width - cc - 2)]) / inputScale;
                                        if ((c & 1) == 0) {
                                            rgb[1][rr * ts + ccmax + cc] = Gtmp[((top + rr) * width + (width - cc - 2)) >> 1];
                                        }
                                    }
                            }

                            //also, fill the image corners
                            if (rrmin > 0 && ccmin > 0) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + cc) >> ((c & 1) ^ 1)] = (rawDataOut[border2 - rr][border2 - cc]) / inputScale;
                                        if ((c & 1) == 0) {
                                            rgb[1][rr * ts + cc] = Gtmp[((border2 - rr) * width + border2 - cc) >> 1];
                                        }
                                    }
                            }

                            if (rrmax < rr1 && ccmax < cc1) {
                                for (int rr = 0; rr < std::min(border, rr1 - rrmax); rr++)
                                    for (int cc = 0; cc < std::min(border, cc1 - ccmax); cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][((rrmax + rr)*ts + ccmax + cc) >> ((c & 1) ^ 1)] = (rawDataOut[(height - rr - 2)][(width - cc - 2)]) / inputScale;
                                        if ((c & 1) == 0) {
                                            rgb[1][(rrmax + rr)*ts + ccmax + cc] = Gtmp[((height - rr - 2) * width + (width - cc - 2)) >> 1];
                                        }
                                    }
                            }

                            if (rrmin > 0 && ccmax < cc1) {
                                for (int rr = 0; rr < border; rr++)
                                    for (int cc = 0; cc < std::min(border, cc1 - ccmax); cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][(rr * ts + ccmax + cc) >> ((c & 1) ^ 1)] = (rawDataOut[(border2 - rr)][(width - cc - 2)]) / inputScale;
                                        if ((c & 1) == 0) {
                                            rgb[1][rr * ts + ccmax + cc] = Gtmp[((border2 - rr) * width + (width - cc - 2)) >> 1];
                                        }
                                    }
                            }

                            if (rrmax < rr1 && ccmin > 0) {
                                for (int rr = 0; rr < std::min(border, rr1 - rrmax); rr++)
                                    for (int cc = 0; cc < border; cc++) {
                                        int c = fc(cfarray, rr, cc);
                                        rgb[c][((rrmax + rr)*ts + cc) >> ((c & 1) ^ 1)] = (rawDataOut[(height - rr - 2)][(border2 - cc)]) / inputScale;
                                        if ((c & 1) == 0) {
                                            rgb[1][(rrmax + rr)*ts + cc] = Gtmp[((height - rr - 2) * width + (border2 - cc)) >> 1];
                                        }
                                    }
                            }

                            //end of border fill

                            if (!autoCA || fitParamsSet) {
#ifdef __SSE2__
                                const vfloat onev = F2V(1.f);
                                const vfloat epsv = F2V(eps);
#endif

                                //manual CA correction; use red/blue slider values to set CA shift parameters
                                for (int rr = 3; rr < rr1 - 3; rr++) {
                                    int cc = 3 + fc(cfarray, rr, 1), c = fc(cfarray, rr,cc), indx = rr * ts + cc;
#ifdef __SSE2__
                                    for (; cc < cc1 - 10; cc += 8, indx += 8) {
                                        //compute directional weights using image gradients
                                        vfloat val1v = epsv + vabsf(LC2VFU(rgb[1][(rr + 1) * ts + cc]) - LC2VFU(rgb[1][(rr - 1) * ts + cc]));
                                        vfloat val2v = epsv + vabsf(LC2VFU(rgb[1][indx + 1]) - LC2VFU(rgb[1][indx - 1]));
                                        vfloat wtuv = onev / SQRV(val1v + vabsf(LVFU(rgb[c][(rr * ts + cc) >> 1]) - LVFU(rgb[c][((rr - 2) * ts + cc) >> 1])) + vabsf(LC2VFU(rgb[1][(rr - 1) * ts + cc]) - LC2VFU(rgb[1][(rr - 3) * ts + cc])));
                                        vfloat wtdv = onev / SQRV(val1v + vabsf(LVFU(rgb[c][(rr * ts + cc) >> 1]) - LVFU(rgb[c][((rr + 2) * ts + cc) >> 1])) + vabsf(LC2VFU(rgb[1][(rr + 1) * ts + cc]) - LC2VFU(rgb[1][(rr + 3) * ts + cc])));
                                        vfloat wtlv = onev / SQRV(val2v + vabsf(LVFU(rgb[c][indx >> 1]) - LVFU(rgb[c][(indx - 2) >> 1])) + vabsf(LC2VFU(rgb[1][indx - 1]) - LC2VFU(rgb[1][indx - 3])));
                                        vfloat wtrv = onev / SQRV(val2v + vabsf(LVFU(rgb[c][indx >> 1]) - LVFU(rgb[c][(indx + 2) >> 1])) + vabsf(LC2VFU(rgb[1][indx + 1]) - LC2VFU(rgb[1][indx + 3])));

                                        //store in rgb array the interpolated G value at R/B grid points using directional weighted average
                                        vfloat resultv = (wtuv * LC2VFU(rgb[1][indx - v1]) + wtdv * LC2VFU(rgb[1][indx + v1]) + wtlv * LC2VFU(rgb[1][indx - 1]) + wtrv * LC2VFU(rgb[1][indx + 1])) / (wtuv + wtdv + wtlv + wtrv);
                                        STC2VFU(rgb[1][indx], resultv);
                                    }
#endif
                                    for (; cc < cc1 - 3; cc += 2, indx += 2) {
                                        //compute directional weights using image gradients
                                        float wtu = 1.f / SQR(eps + fabsf(rgb[1][(rr + 1) * ts + cc] - rgb[1][(rr - 1) * ts + cc]) + fabsf(rgb[c][(rr * ts + cc) >> 1] - rgb[c][((rr - 2) * ts + cc) >> 1]) + fabsf(rgb[1][(rr - 1) * ts + cc] - rgb[1][(rr - 3) * ts + cc]));
                                        float wtd = 1.f / SQR(eps + fabsf(rgb[1][(rr + 1) * ts + cc] - rgb[1][(rr - 1) * ts + cc]) + fabsf(rgb[c][(rr * ts + cc) >> 1] - rgb[c][((rr + 2) * ts + cc) >> 1]) + fabsf(rgb[1][(rr + 1) * ts + cc] - rgb[1][(rr + 3) * ts + cc]));
                                        float wtl = 1.f / SQR(eps + fabsf(rgb[1][rr * ts + cc + 1] - rgb[1][rr * ts + cc - 1]) + fabsf(rgb[c][(rr * ts + cc) >> 1] - rgb[c][(rr * ts + cc - 2) >> 1]) + fabsf(rgb[1][rr * ts + cc - 1] - rgb[1][rr * ts + cc - 3]));
                                        float wtr = 1.f / SQR(eps + fabsf(rgb[1][rr * ts + cc + 1] - rgb[1][rr * ts + cc - 1]) + fabsf(rgb[c][(rr * ts + cc) >> 1] - rgb[c][(rr * ts + cc + 2) >> 1]) + fabsf(rgb[1][rr * ts + cc + 1] - rgb[1][rr * ts + cc + 3]));

                                        //store in rgb array the interpolated G value at R/B grid points using directional weighted average
                                        rgb[1][indx] = (wtu * rgb[1][indx - v1] + wtd * rgb[1][indx + v1] + wtl * rgb[1][indx - 1] + wtr * rgb[1][indx + 1]) / (wtu + wtd + wtl + wtr);
                                    }
                                }
                            }
                            if (!autoCA) {
                                float hfrac = -((float)(hblock - 0.5) / (hblsz - 2) - 0.5);
                                float vfrac = -((float)(vblock - 0.5) / (vblsz - 2) - 0.5) * height / width;
                                lblockshifts[0][0] = 2 * vfrac * cared;
                                lblockshifts[0][1] = 2 * hfrac * cared;
                                lblockshifts[1][0] = 2 * vfrac * cablue;
                                lblockshifts[1][1] = 2 * hfrac * cablue;
                            } else {
                                //CA auto correction; use CA diagnostic pass to set shift parameters
                                lblockshifts[0][0] = lblockshifts[0][1] = 0;
                                lblockshifts[1][0] = lblockshifts[1][1] = 0;
                                double powVblock = 1.0;
                                for (int i = 0; i < polyord; i++) {
                                    double powHblock = powVblock;
                                    for (int j = 0; j < polyord; j++) {
                                        lblockshifts[0][0] += powHblock * fitParams[0][0][polyord * i + j];
                                        lblockshifts[0][1] += powHblock * fitParams[0][1][polyord * i + j];
                                        lblockshifts[1][0] += powHblock * fitParams[1][0][polyord * i + j];
                                        lblockshifts[1][1] += powHblock * fitParams[1][1][polyord * i + j];
                                        powHblock *= hblock;
                                    }
                                    powVblock *= vblock;
                                }
                                constexpr float bslim = 3.99; //max allowed CA shift
                                lblockshifts[0][0] = LIM(lblockshifts[0][0], -bslim, bslim);
                                lblockshifts[0][1] = LIM(lblockshifts[0][1], -bslim, bslim);
                                lblockshifts[1][0] = LIM(lblockshifts[1][0], -bslim, bslim);
                                lblockshifts[1][1] = LIM(lblockshifts[1][1], -bslim, bslim);
                            }//end of setting CA shift parameters


                            for (int c = 0; c < 3; c += 2) {

                                //some parameters for the bilinear interpolation
                                shiftvfloor[c] = floor((float)lblockshifts[c>>1][0]);
                                shiftvceil[c] = ceil((float)lblockshifts[c>>1][0]);
                                if (lblockshifts[c>>1][0] < 0.f) {
                                    std::swap(shiftvfloor[c], shiftvceil[c]);
                                }
                                shiftvfrac[c] = fabs(lblockshifts[c>>1][0] - shiftvfloor[c]);

                                shifthfloor[c] = floor((float)lblockshifts[c>>1][1]);
                                shifthceil[c] = ceil((float)lblockshifts[c>>1][1]);
                                if (lblockshifts[c>>1][1] < 0.f) {
                                    std::swap(shifthfloor[c], shifthceil[c]);
                                }
                                shifthfrac[c] = fabs(lblockshifts[c>>1][1] - shifthfloor[c]);

                                GRBdir[0][c] = lblockshifts[c>>1][0] > 0 ? 2 : -2;
                                GRBdir[1][c] = lblockshifts[c>>1][1] > 0 ? 2 : -2;

                            }


                            for (int rr = 4; rr < rr1 - 4; rr++) {
                                int cc = 4 + (fc(cfarray, rr, 2) & 1);
                                int c = fc(cfarray, rr, cc);
                                int indx = (rr * ts + cc) >> 1;
                                int indxfc = (rr + shiftvfloor[c]) * ts + cc + shifthceil[c];
                                int indxff = (rr + shiftvfloor[c]) * ts + cc + shifthfloor[c];
                                int indxcc = (rr + shiftvceil[c]) * ts + cc + shifthceil[c];
                                int indxcf = (rr + shiftvceil[c]) * ts + cc + shifthfloor[c];
#ifdef __SSE2__
                                vfloat shifthfracv = F2V(shifthfrac[c]);
                                vfloat shiftvfracv = F2V(shiftvfrac[c]);
                                for (; cc < cc1 - 10; cc += 8, indxfc += 8, indxff += 8, indxcc += 8, indxcf += 8, indx += 4) {
                                    //perform CA correction using colour ratios or colour differences
                                    vfloat Ginthfloorv = vintpf(shifthfracv, LC2VFU(rgb[1][indxfc]), LC2VFU(rgb[1][indxff]));
                                    vfloat Ginthceilv = vintpf(shifthfracv, LC2VFU(rgb[1][indxcc]), LC2VFU(rgb[1][indxcf]));
                                    //Gint is bilinear interpolation of G at CA shift point
                                    vfloat Gintv = vintpf(shiftvfracv, Ginthceilv, Ginthfloorv);

                                    //determine R/B at grid points using colour differences at shift point plus interpolated G value at grid point
                                    //but first we need to interpolate G-R/G-B to grid points...
                                    STVFU(grbdiff[indx], Gintv - LVFU(rgb[c][indx]));
                                    STVFU(gshift[indx], Gintv);
                                }

#endif
                                for (; cc < cc1 - 4; cc += 2, indxfc += 2, indxff += 2, indxcc += 2, indxcf += 2, ++indx) {
                                    //perform CA correction using colour ratios or colour differences
                                    float Ginthfloor = intp(shifthfrac[c], rgb[1][indxfc], rgb[1][indxff]);
                                    float Ginthceil = intp(shifthfrac[c], rgb[1][indxcc], rgb[1][indxcf]);
                                    //Gint is bilinear interpolation of G at CA shift point
                                    float Gint = intp(shiftvfrac[c], Ginthceil, Ginthfloor);

                                    //determine R/B at grid points using colour differences at shift point plus interpolated G value at grid point
                                    //but first we need to interpolate G-R/G-B to grid points...
                                    grbdiff[indx] = Gint - rgb[c][indx];
                                    gshift[indx] = Gint;
                                }
                            }

                            shifthfrac[0] /= 2.f;
                            shifthfrac[2] /= 2.f;
                            shiftvfrac[0] /= 2.f;
                            shiftvfrac[2] /= 2.f;

#ifdef __SSE2__
                            vfloat zd25v = F2V(0.25f);
                            vfloat onev = F2V(1.f);
                            vfloat zd5v = F2V(0.5f);
                            vfloat epsv = F2V(eps);
#endif
                            for (int rr = 8; rr < rr1 - 8; rr++) {
                                int cc = 8 + (fc(cfarray, rr, 2) & 1);
                                int c = fc(cfarray, rr, cc);
                                int GRBdir0 = GRBdir[0][c];
                                int GRBdir1 = GRBdir[1][c];
#ifdef __SSE2__
                                vfloat shifthfracc = F2V(shifthfrac[c]);
                                vfloat shiftvfracc = F2V(shiftvfrac[c]);
                                for (int indx = rr * ts + cc; cc < cc1 - 14; cc += 8, indx += 8) {
                                    //interpolate colour difference from optical R/B locations to grid locations
                                    vfloat grbdiffinthfloor = vintpf(shifthfracc, LVFU(grbdiff[(indx - GRBdir1) >> 1]), LVFU(grbdiff[indx >> 1]));
                                    vfloat grbdiffinthceil = vintpf(shifthfracc, LVFU(grbdiff[((rr - GRBdir0) * ts + cc - GRBdir1) >> 1]), LVFU(grbdiff[((rr - GRBdir0) * ts + cc) >> 1]));
                                    //grbdiffint is bilinear interpolation of G-R/G-B at grid point
                                    vfloat grbdiffint = vintpf(shiftvfracc, grbdiffinthceil, grbdiffinthfloor);

                                    //now determine R/B at grid points using interpolated colour differences and interpolated G value at grid point
                                    vfloat cinv = LVFU(rgb[c][indx >> 1]);
                                    vfloat rinv = LC2VFU(rgb[1][indx]);
                                    vfloat RBint = rinv - grbdiffint;
                                    vmask cmask = vmaskf_ge(vabsf(RBint - cinv), zd25v * (RBint + cinv));
                                    if(_mm_movemask_ps((vfloat)cmask)) {
                                        // if for any of the 4 pixels the condition is true, do the math for all 4 pixels and mask the unused out at the end
                                        //gradient weights using difference from G at CA shift points and G at grid points
                                        vfloat p0 = onev / (epsv + vabsf(rinv - LVFU(gshift[indx >> 1])));
                                        vfloat p1 = onev / (epsv + vabsf(rinv - LVFU(gshift[(indx - GRBdir1) >> 1])));
                                        vfloat p2 = onev / (epsv + vabsf(rinv - LVFU(gshift[((rr - GRBdir0) * ts + cc) >> 1])));
                                        vfloat p3 = onev / (epsv + vabsf(rinv - LVFU(gshift[((rr - GRBdir0) * ts + cc - GRBdir1) >> 1])));

                                        grbdiffint = vself(cmask, (p0 * LVFU(grbdiff[indx >> 1]) + p1 * LVFU(grbdiff[(indx - GRBdir1) >> 1]) +
                                                      p2 * LVFU(grbdiff[((rr - GRBdir0) * ts + cc) >> 1]) + p3 * LVFU(grbdiff[((rr - GRBdir0) * ts + cc - GRBdir1) >> 1])) / (p0 + p1 + p2 + p3), grbdiffint);

                                    }
                                    vfloat grbdiffold = rinv - cinv;
                                    RBint = rinv - grbdiffint;
                                    RBint = vself(vmaskf_gt(vabsf(grbdiffold), vabsf(grbdiffint)), RBint, cinv);
                                    RBint = vself(vmaskf_lt(grbdiffold * grbdiffint, ZEROV), rinv - zd5v * (grbdiffold + grbdiffint), RBint);
                                    STVFU(rgb[c][indx >> 1], RBint);
                                }
#endif
                                for (int c1 = fc(cfarray, rr, cc), indx = rr * ts + cc; cc < cc1 - 8; cc += 2, indx += 2) {
                                    float grbdiffold = rgb[1][indx] - rgb[c1][indx >> 1];

                                    //interpolate colour difference from optical R/B locations to grid locations
                                    float grbdiffinthfloor = intp(shifthfrac[c1], grbdiff[(indx - GRBdir1) >> 1], grbdiff[indx >> 1]);
                                    float grbdiffinthceil = intp(shifthfrac[c1], grbdiff[((rr - GRBdir0) * ts + cc - GRBdir1) >> 1], grbdiff[((rr - GRBdir0) * ts + cc) >> 1]);
                                    //grbdiffint is bilinear interpolation of G-R/G-B at grid point
                                    float grbdiffint = intp(shiftvfrac[c1], grbdiffinthceil, grbdiffinthfloor);

                                    //now determine R/B at grid points using interpolated colour differences and interpolated G value at grid point
                                    float RBint = rgb[1][indx] - grbdiffint;

                                    if (fabsf(RBint - rgb[c1][indx >> 1]) < 0.25f * (RBint + rgb[c1][indx >> 1])) {
                                        if (fabsf(grbdiffold) > fabsf(grbdiffint) ) {
                                            rgb[c1][indx >> 1] = RBint;
                                        }
                                    } else {

                                        //gradient weights using difference from G at CA shift points and G at grid points
                                        float p0 = 1.f / (eps + fabsf(rgb[1][indx] - gshift[indx >> 1]));
                                        float p1 = 1.f / (eps + fabsf(rgb[1][indx] - gshift[(indx - GRBdir1) >> 1]));
                                        float p2 = 1.f / (eps + fabsf(rgb[1][indx] - gshift[((rr - GRBdir0) * ts + cc) >> 1]));
                                        float p3 = 1.f / (eps + fabsf(rgb[1][indx] - gshift[((rr - GRBdir0) * ts + cc - GRBdir1) >> 1]));

                                        grbdiffint = (p0 * grbdiff[indx >> 1] + p1 * grbdiff[(indx - GRBdir1) >> 1] +
                                                      p2 * grbdiff[((rr - GRBdir0) * ts + cc) >> 1] + p3 * grbdiff[((rr - GRBdir0) * ts + cc - GRBdir1) >> 1]) / (p0 + p1 + p2 + p3) ;

                                        //now determine R/B at grid points using interpolated colour differences and interpolated G value at grid point
                                        if (fabsf(grbdiffold) > fabsf(grbdiffint) ) {
                                            rgb[c1][indx >> 1] = rgb[1][indx] - grbdiffint;
                                        }
                                    }

                                    //if colour difference interpolation overshot the correction, just desaturate
                                    if (grbdiffold * grbdiffint < 0) {
                                        rgb[c1][indx >> 1] = rgb[1][indx] - 0.5f * (grbdiffold + grbdiffint);
                                    }
                                }
                            }

#ifdef __SSE2__
                            vfloat coutScalev = F2V(outputScale);
#endif
                            // copy CA corrected results to temporary image matrix
                            for (int rr = border; rr < rr1 - border; rr++) {
                                int c = fc(cfarray, rr + top, left + border + (fc(cfarray, rr + top, 2) & 1));
                                int row = rr + top;
                                int cc = border + (fc(cfarray, rr, 2) & 1);
                                int indx = ((row-winy) * (winw + (winw & 1)) + cc + left - winx) >> 1;
                                int indx1 = (rr * ts + cc) >> 1;
#ifdef __SSE2__
                                for (; indx < ((row-winy) * (winw + (winw & 1)) + cc1 - border - 7 + left - winx) >> 1; indx+=4, indx1 += 4) {
                                    STVFU(RawDataTmp[indx], coutScalev * LVFU(rgb[c][indx1]));
                                }
#endif
                                for (; indx < ((row-winy) * (winw + (winw & 1)) + cc1 - border + left - winx) >> 1; indx++, indx1++) {
                                    RawDataTmp[indx] = outputScale * rgb[c][indx1];
                                }
                            }

                            progresscounter++;

                            if(progresscounter % 8 == 0)
#ifdef _OPENMP
                                #pragma omp critical (cacorrect)
#endif
                            {
                                progress += (double)(8.0 * (ts - border2) * (ts - border2)) / (2 * height * width);

                                if (progress > 1.0) {
                                    progress = 1.0;
                                }

                                setProgCancel(progress);
                            }
                        }

#ifdef _OPENMP
                    #pragma omp barrier
                    // copy temporary image matrix back to image matrix
                    #pragma omp for
#endif

                    for(int row = cb; row < winh - cb; row++) {
                        int col = cb + (fc(cfarray, row + winy, winx) & 1);
                        int indx = (row * (winw + (winw & 1)) + col) >> 1;
#ifdef __SSE2__
                        for (; col < (winw + (winw & 1)) - 7 - cb; col += 8, indx += 4) {
                            vfloat val = LVFU(RawDataTmp[indx]);
                            STC2VFU(rawDataOut[row + winy][col + winx], val);
                        }
#endif
                        for (; col < (winw + (winw & 1)) - cb; col += 2, indx++) {
                            rawDataOut[row + winy][col + winx] = RawDataTmp[indx];
                        }
                    }
                }
            }
        }

        if (!rc && avoidColourshift) {
            // to avoid or at least reduce the colour shift caused by raw ca correction we compute the per pixel difference factors
            // of red and blue channel and apply a gaussian blur to them.
            // Then we apply the resulting factors per pixel on the result of raw ca correction

#ifdef _OPENMP
            #pragma omp parallel
#endif
            {
#ifdef __SSE2__
                const vfloat onev = F2V(1.f);
                const vfloat twov = F2V(2.f);
                const vfloat zd5v = F2V(0.5f);
#endif
#ifdef _OPENMP
                #pragma omp for
#endif
                for (int i = winy; i < winh - 2 * cb; ++i) {
                    const int firstCol = winx + (fc(cfarray, i, winx) & 1);
                    const int colour = fc(cfarray, i, firstCol);
                    JaggedArray<float>* nonGreen = colour == 0 ? redFactor.get() : blueFactor.get();
                    int j = firstCol;
#ifdef __SSE2__
                    for (; j < winw - 7 - 2 * cb; j += 8) {
                        const vfloat newvals = LC2VFU(rawDataOut[i + cb][j + cb]);
                        const vfloat oldvals = LVFU((*oldraw)[i - winy][(j - winx) / 2]);
                        vfloat factors = oldvals / newvals;
                        factors = vself(vmaskf_le(newvals, onev), onev, factors);
                        factors = vself(vmaskf_le(oldvals, onev), onev, factors);
                        STVFU((*nonGreen)[(i - winy) / 2][(j - winx) / 2], LIMV(factors, zd5v, twov));
                    }
#endif
                    for (; j < winw - 2 * cb; j += 2) {
                        (*nonGreen)[(i - winy) / 2][(j - winx) / 2] = (rawDataOut[i + cb][j + cb] <= 1.f || (*oldraw)[i - winy][(j - winx) / 2] <= 1.f) ? 1.f : librtprocess::LIM((*oldraw)[i - winy][(j - winx) / 2] / rawDataOut[i + cb][j + cb], 0.5f, 2.f);
                    }
                }

#ifdef _OPENMP
                #pragma omp single
#endif
                {
                    if (H % 2) {
                        // odd height => factors are not set in last row => use values of preceding row
                        // odd height => factors are not set in last row => use values of preceding row
                        for (int j = 0; j < (W + 1 - 2 * cb) / 2; ++j) {
                            (*redFactor)[(H - 2 * cb + 1) / 2 - 1][j] = (*redFactor)[(H - 2 * cb + 1) / 2 - 2][j];
                            (*blueFactor)[(H - 2 * cb + 1) / 2 - 1][j] = (*blueFactor)[(H - 2 * cb + 1) / 2 - 2][j];
                        }
                    }

                    if (W % 2) {
                        // odd width => factors for one channel are not set in last column => use value of preceding column
                        const int ngRow = winy + (1 - (fc(cfarray, winy, winx) & 1));
                        const int ngCol = winx + (fc(cfarray, ngRow, winx) & 1);
                        const int colour = fc(cfarray, ngRow, ngCol);
                        JaggedArray<float>* nonGreen = colour == 0 ? redFactor.get() : blueFactor.get();
                        for (int i = 0; i < (H + 1 - 2 * cb) / 2; ++i) {
                            (*nonGreen)[i][(W - 2 * cb + 1) / 2 - 1] = (*nonGreen)[i][(W - 2* cb + 1) / 2 - 2];
                        }
                    }
                }

                // blur correction factors
                gaussianBlur(*redFactor, *redFactor, (W + 1 - 2 * cb) / 2, (H + 1 - 2 * cb) / 2, 30.0);
                gaussianBlur(*blueFactor, *blueFactor, (W + 1 - 2 * cb) / 2, (H + 1 - 2 * cb) / 2, 30.0);

                // apply correction factors to avoid (reduce) colour shift
#ifdef _OPENMP
                #pragma omp for
#endif
                for (int i = winy; i < winh - 2 * cb; ++i) {
                    const int firstCol = winx + (fc(cfarray, i, winx) & 1);
                    const int colour = fc(cfarray, i, firstCol);
                    JaggedArray<float>* nonGreen = colour == 0 ? redFactor.get() : blueFactor.get();
                    for (int j = firstCol; j < winw - 2 * cb; j += 2) {
                        rawDataOut[i + cb][j + cb] *= (*nonGreen)[i/2][j/2];
                    }
                }
            }
        }
    }

    setProgCancel(1.0);

    return rc ? rc : (processpasstwo ? RP_NO_ERROR : RP_CACORRECT_ERROR);
}
