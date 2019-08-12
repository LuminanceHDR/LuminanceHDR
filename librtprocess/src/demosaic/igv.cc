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
#include "librtprocess.h"
#include "rt_math.h"
#include "median.h"
#include "StopWatch.h"

using namespace librtprocess;
/***
*
*   Bayer CFA Demosaicing using Integrated Gaussian Vector on Color Differences
*   Revision 1.0 - 2013/02/28
*
*   Copyright (c) 2007-2013 Luis Sanz Rodriguez
*   Using High Order Interpolation technique by Jim S, Jimmy Li, and Sharmil Randhawa
*
*   Contact info: luis.sanz.rodriguez@gmail.com
*
*   This code is distributed under a GNU General Public License, version 3.
*   Visit <http://www.gnu.org/licenses/> for more information.
*
***/
// Adapted to RawTherapee by Jacques Desmis 3/2013
// SSE version by Ingo Weyrich 5/2013
#ifdef __SSE2__
#define CLIPV(a) LIMV(a,zerov,c65535v)
rpError igv_demosaic(int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel)
{
    BENCHFUN
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    rpError rc = RP_NO_ERROR;

    constexpr float eps = 1e-5f, epssq = 1e-5f; //mod epssq -10f =>-5f Jacques 3/2013 to prevent artifact (divide by zero)

    constexpr int h1 = 1, h2 = 2, h3 = 3, h5 = 5;
    const int width = winw, height = winh;
    const int v1 = 1 * width, v2 = 2 * width, v3 = 3 * width, v5 = 5 * width;

    float *rgbarray = (float (*)) malloc((width * height) * sizeof(float));
    float *vdif = (float (*)) calloc(width * height / 2, sizeof * vdif);
    float *hdif = (float (*)) calloc(width * height / 2, sizeof * hdif);
    float *chrarray = (float (*)) calloc(width * height, sizeof(float));

    if(!rgbarray || !vdif || !hdif || !chrarray) {
        if (rgbarray) {
            free(rgbarray);
        }
        if (vdif) {
            free(vdif);
        }
        if (hdif) {
            free(hdif);
        }
        if (chrarray) {
            free(chrarray);
        }
        return RP_MEMORY_ERROR;
    }

    float* rgb[2];
    rgb[0] = rgbarray;
    rgb[1] = rgbarray + (width * height) / 2;

    float* chr[4];

    chr[0] = chrarray;
    chr[1] = chrarray + (width * height) / 2;

    // mapped chr[2] and chr[3] to hdif and hdif, because these are out of use, when chr[2] and chr[3] are used
    chr[2] = hdif;
    chr[3] = vdif;

    setProgCancel(0.0);

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        vfloat ngv, egv, wgv, sgv, nvv, evv, wvv, svv, nwgv, negv, swgv, segv, nwvv, nevv, swvv, sevv, tempv, temp1v, temp2v, temp3v, temp4v, temp5v, temp6v, temp7v, temp8v;
        const vfloat epsv = F2V(eps);
        const vfloat epssqv = F2V(epssq);
        const vfloat c65535v = F2V(65535.f);
        const vfloat c23v = F2V(23.f);
        const vfloat c40v = F2V(40.f);
        const vfloat c51v = F2V(51.f);
        const vfloat c32v = F2V(32.f);
        const vfloat c8v = F2V(8.f);
        const vfloat c7v = F2V(7.f);
        const vfloat c6v = F2V(6.f);
        const vfloat c10v = F2V(10.f);
        const vfloat c21v = F2V(21.f);
        const vfloat c78v = F2V(78.f);
        const vfloat c69v = F2V(69.f);
        const vfloat c3145680v = F2V(3145680.f);
        const vfloat onev = F2V(1.f);
        const vfloat zerov = F2V(0.f);
        const vfloat d725v = F2V(0.725f);
        const vfloat d1375v = F2V(0.1375f);

        float ng, eg, wg, sg, nv, ev, wv, sv, nwg, neg, swg, seg, nwv, nev, swv, sev;
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 0; row < height - 0; row++) {
            float* dest1 = rgb[fc(cfarray, row, 0) & 1];
            float* dest2 = rgb[fc(cfarray, row, 1) & 1];
            int col, indx;

            for (col = 0, indx = row * width + col; col < width - 7; col += 8, indx += 8) {
                temp1v = CLIPV(LVFU(rawData[row][col]));
                temp2v = CLIPV(LVFU(rawData[row][col + 4]));
                STVFU(dest1[indx >> 1], _mm_shuffle_ps(temp1v, temp2v, _MM_SHUFFLE(2, 0, 2, 0)));
                STVFU(dest2[indx >> 1], _mm_shuffle_ps(temp1v, temp2v, _MM_SHUFFLE(3, 1, 3, 1)));
            }

            for (; col < width; col++, indx += 2) {
                dest1[indx >> 1] = CLIP(rawData[row][col]); //rawData = RT datas
                col++;
                if(col < width)
                    dest2[indx >> 1] = CLIP(rawData[row][col]); //rawData = RT datas
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
        setProgCancel(0.13);
        }

#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 5; row < height - 5; row++) {
            int col, indx, indx1;

            for (col = 5 + (fc(cfarray, row, 1) & 1), indx = row * width + col, indx1 = indx >> 1; col < width - 12; col += 8, indx += 8, indx1 += 4) {
                //N,E,W,S Gradients
                ngv = epsv + (vabsf(LVFU(rgb[1][(indx - v1) >> 1]) - LVFU(rgb[1][(indx - v3) >> 1])) + vabsf(LVFU(rgb[0][indx1]) - LVFU(rgb[0][(indx1 - v1)]))) / c65535v;
                egv = epsv + (vabsf(LVFU(rgb[1][(indx + h1) >> 1]) - LVFU(rgb[1][(indx + h3) >> 1])) + vabsf(LVFU(rgb[0][indx1]) - LVFU(rgb[0][(indx1 + h1)]))) / c65535v;
                wgv = epsv + (vabsf(LVFU(rgb[1][(indx - h1) >> 1]) - LVFU(rgb[1][(indx - h3) >> 1])) + vabsf(LVFU(rgb[0][indx1]) - LVFU(rgb[0][(indx1 - h1)]))) / c65535v;
                sgv = epsv + (vabsf(LVFU(rgb[1][(indx + v1) >> 1]) - LVFU(rgb[1][(indx + v3) >> 1])) + vabsf(LVFU(rgb[0][indx1]) - LVFU(rgb[0][(indx1 + v1)]))) / c65535v;
                //N,E,W,S High Order Interpolation (Li & Randhawa)
                //N,E,W,S Hamilton Adams Interpolation
                // (48.f * 65535.f) = 3145680.f
                tempv = c40v * LVFU(rgb[0][indx1]);
                nvv = LIMV(((c23v * LVFU(rgb[1][(indx - v1) >> 1]) + c23v * LVFU(rgb[1][(indx - v3) >> 1]) + LVFU(rgb[1][(indx - v5) >> 1]) + LVFU(rgb[1][(indx + v1) >> 1]) + tempv - c32v * LVFU(rgb[0][(indx1 - v1)]) - c8v * LVFU(rgb[0][(indx1 - v2)]))) / c3145680v, zerov, onev);
                evv = LIMV(((c23v * LVFU(rgb[1][(indx + h1) >> 1]) + c23v * LVFU(rgb[1][(indx + h3) >> 1]) + LVFU(rgb[1][(indx + h5) >> 1]) + LVFU(rgb[1][(indx - h1) >> 1]) + tempv - c32v * LVFU(rgb[0][(indx1 + h1)]) - c8v * LVFU(rgb[0][(indx1 + h2)]))) / c3145680v, zerov, onev);
                wvv = LIMV(((c23v * LVFU(rgb[1][(indx - h1) >> 1]) + c23v * LVFU(rgb[1][(indx - h3) >> 1]) + LVFU(rgb[1][(indx - h5) >> 1]) + LVFU(rgb[1][(indx + h1) >> 1]) + tempv - c32v * LVFU(rgb[0][(indx1 - h1)]) - c8v * LVFU(rgb[0][(indx1 - h2)]))) / c3145680v, zerov, onev);
                svv = LIMV(((c23v * LVFU(rgb[1][(indx + v1) >> 1]) + c23v * LVFU(rgb[1][(indx + v3) >> 1]) + LVFU(rgb[1][(indx + v5) >> 1]) + LVFU(rgb[1][(indx - v1) >> 1]) + tempv - c32v * LVFU(rgb[0][(indx1 + v1)]) - c8v * LVFU(rgb[0][(indx1 + v2)]))) / c3145680v, zerov, onev);
                //Horizontal and vertical color differences
                tempv = LVFU(rgb[0][indx1]) / c65535v;
                STVFU(vdif[indx1], (sgv * nvv + ngv * svv) / (ngv + sgv) - tempv);
                STVFU(hdif[indx1], (wgv * evv + egv * wvv) / (egv + wgv) - tempv);
            }

            // borders without SSE
            for (; col < width - 5; col += 2, indx += 2, indx1++) {
                //N,E,W,S Gradients
                ng = eps + (fabsf(rgb[1][(indx - v1) >> 1] - rgb[1][(indx - v3) >> 1]) + fabsf(rgb[0][indx1] - rgb[0][(indx1 - v1)])) / 65535.f;
                eg = eps + (fabsf(rgb[1][(indx + h1) >> 1] - rgb[1][(indx + h3) >> 1]) + fabsf(rgb[0][indx1] - rgb[0][(indx1 + h1)])) / 65535.f;
                wg = eps + (fabsf(rgb[1][(indx - h1) >> 1] - rgb[1][(indx - h3) >> 1]) + fabsf(rgb[0][indx1] - rgb[0][(indx1 - h1)])) / 65535.f;
                sg = eps + (fabsf(rgb[1][(indx + v1) >> 1] - rgb[1][(indx + v3) >> 1]) + fabsf(rgb[0][indx1] - rgb[0][(indx1 + v1)])) / 65535.f;
                //N,E,W,S High Order Interpolation (Li & Randhawa)
                //N,E,W,S Hamilton Adams Interpolation
                // (48.f * 65535.f) = 3145680.f
                nv = LIM(((23.f * rgb[1][(indx - v1) >> 1] + 23.f * rgb[1][(indx - v3) >> 1] + rgb[1][(indx - v5) >> 1] + rgb[1][(indx + v1) >> 1] + 40.f * rgb[0][indx1] - 32.f * rgb[0][(indx1 - v1)] - 8.f * rgb[0][(indx1 - v2)])) / 3145680.f, 0.f, 1.f);
                ev = LIM(((23.f * rgb[1][(indx + h1) >> 1] + 23.f * rgb[1][(indx + h3) >> 1] + rgb[1][(indx + h5) >> 1] + rgb[1][(indx - h1) >> 1] + 40.f * rgb[0][indx1] - 32.f * rgb[0][(indx1 + h1)] - 8.f * rgb[0][(indx1 + h2)])) / 3145680.f, 0.f, 1.f);
                wv = LIM(((23.f * rgb[1][(indx - h1) >> 1] + 23.f * rgb[1][(indx - h3) >> 1] + rgb[1][(indx - h5) >> 1] + rgb[1][(indx + h1) >> 1] + 40.f * rgb[0][indx1] - 32.f * rgb[0][(indx1 - h1)] - 8.f * rgb[0][(indx1 - h2)])) / 3145680.f, 0.f, 1.f);
                sv = LIM(((23.f * rgb[1][(indx + v1) >> 1] + 23.f * rgb[1][(indx + v3) >> 1] + rgb[1][(indx + v5) >> 1] + rgb[1][(indx - v1) >> 1] + 40.f * rgb[0][indx1] - 32.f * rgb[0][(indx1 + v1)] - 8.f * rgb[0][(indx1 + v2)])) / 3145680.f, 0.f, 1.f);
                //Horizontal and vertical color differences
                vdif[indx1] = (sg * nv + ng * sv) / (ng + sg) - (rgb[0][indx1]) / 65535.f;
                hdif[indx1] = (wg * ev + eg * wv) / (eg + wg) - (rgb[0][indx1]) / 65535.f;
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.26);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++) {
            int col, d, indx1;

            for (col = 7 + (fc(cfarray, row, 1) & 1), indx1 = (row * width + col) >> 1, d = fc(cfarray, row, col) / 2; col < width - 14; col += 8, indx1 += 4) {
                //H&V integrated gaussian vector over variance on color differences
                //Mod Jacques 3/2013
                ngv = LIMV(epssqv + c78v * SQRV(LVFU(vdif[indx1])) + c69v * (SQRV(LVFU(vdif[indx1 - v1])) + SQRV(LVFU(vdif[indx1 + v1]))) + c51v * (SQRV(LVFU(vdif[indx1 - v2])) + SQRV(LVFU(vdif[indx1 + v2]))) + c21v * (SQRV(LVFU(vdif[indx1 - v3])) + SQRV(LVFU(vdif[indx1 + v3]))) - c6v * SQRV(LVFU(vdif[indx1 - v1]) + LVFU(vdif[indx1]) + LVFU(vdif[indx1 + v1]))
                           - c10v * (SQRV(LVFU(vdif[indx1 - v2]) + LVFU(vdif[indx1 - v1]) + LVFU(vdif[indx1])) + SQRV(LVFU(vdif[indx1]) + LVFU(vdif[indx1 + v1]) + LVFU(vdif[indx1 + v2]))) - c7v * (SQRV(LVFU(vdif[indx1 - v3]) + LVFU(vdif[indx1 - v2]) + LVFU(vdif[indx1 - v1])) + SQRV(LVFU(vdif[indx1 + v1]) + LVFU(vdif[indx1 + v2]) + LVFU(vdif[indx1 + v3]))), zerov, onev);
                egv = LIMV(epssqv + c78v * SQRV(LVFU(hdif[indx1])) + c69v * (SQRV(LVFU(hdif[indx1 - h1])) + SQRV(LVFU(hdif[indx1 + h1]))) + c51v * (SQRV(LVFU(hdif[indx1 - h2])) + SQRV(LVFU(hdif[indx1 + h2]))) + c21v * (SQRV(LVFU(hdif[indx1 - h3])) + SQRV(LVFU(hdif[indx1 + h3]))) - c6v * SQRV(LVFU(hdif[indx1 - h1]) + LVFU(hdif[indx1]) + LVFU(hdif[indx1 + h1]))
                           - c10v * (SQRV(LVFU(hdif[indx1 - h2]) + LVFU(hdif[indx1 - h1]) + LVFU(hdif[indx1])) + SQRV(LVFU(hdif[indx1]) + LVFU(hdif[indx1 + h1]) + LVFU(hdif[indx1 + h2]))) - c7v * (SQRV(LVFU(hdif[indx1 - h3]) + LVFU(hdif[indx1 - h2]) + LVFU(hdif[indx1 - h1])) + SQRV(LVFU(hdif[indx1 + h1]) + LVFU(hdif[indx1 + h2]) + LVFU(hdif[indx1 + h3]))), zerov, onev);
                //Limit chrominance using H/V neighbourhood
                nvv = median(d725v * LVFU(vdif[indx1]) + d1375v * LVFU(vdif[indx1 - v1]) + d1375v * LVFU(vdif[indx1 + v1]), LVFU(vdif[indx1 - v1]), LVFU(vdif[indx1 + v1]));
                evv = median(d725v * LVFU(hdif[indx1]) + d1375v * LVFU(hdif[indx1 - h1]) + d1375v * LVFU(hdif[indx1 + h1]), LVFU(hdif[indx1 - h1]), LVFU(hdif[indx1 + h1]));
                //Chrominance estimation
                tempv = (egv * nvv + ngv * evv) / (ngv + egv);
                STVFU(chr[d][indx1], tempv);
                //Green channel population
                temp1v = c65535v * tempv + LVFU(rgb[0][indx1]);
                STVFU(rgb[0][indx1], temp1v);
            }

            for (; col < width - 7; col += 2, indx1++) {
                //H&V integrated gaussian vector over variance on color differences
                //Mod Jacques 3/2013
                ng = LIM(epssq + 78.f * SQR(vdif[indx1]) + 69.f * (SQR(vdif[indx1 - v1]) + SQR(vdif[indx1 + v1])) + 51.f * (SQR(vdif[indx1 - v2]) + SQR(vdif[indx1 + v2])) + 21.f * (SQR(vdif[indx1 - v3]) + SQR(vdif[indx1 + v3])) - 6.f * SQR(vdif[indx1 - v1] + vdif[indx1] + vdif[indx1 + v1])
                         - 10.f * (SQR(vdif[indx1 - v2] + vdif[indx1 - v1] + vdif[indx1]) + SQR(vdif[indx1] + vdif[indx1 + v1] + vdif[indx1 + v2])) - 7.f * (SQR(vdif[indx1 - v3] + vdif[indx1 - v2] + vdif[indx1 - v1]) + SQR(vdif[indx1 + v1] + vdif[indx1 + v2] + vdif[indx1 + v3])), 0.f, 1.f);
                eg = LIM(epssq + 78.f * SQR(hdif[indx1]) + 69.f * (SQR(hdif[indx1 - h1]) + SQR(hdif[indx1 + h1])) + 51.f * (SQR(hdif[indx1 - h2]) + SQR(hdif[indx1 + h2])) + 21.f * (SQR(hdif[indx1 - h3]) + SQR(hdif[indx1 + h3])) - 6.f * SQR(hdif[indx1 - h1] + hdif[indx1] + hdif[indx1 + h1])
                         - 10.f * (SQR(hdif[indx1 - h2] + hdif[indx1 - h1] + hdif[indx1]) + SQR(hdif[indx1] + hdif[indx1 + h1] + hdif[indx1 + h2])) - 7.f * (SQR(hdif[indx1 - h3] + hdif[indx1 - h2] + hdif[indx1 - h1]) + SQR(hdif[indx1 + h1] + hdif[indx1 + h2] + hdif[indx1 + h3])), 0.f, 1.f);
                //Limit chrominance using H/V neighbourhood
                nv = median(0.725f * vdif[indx1] + 0.1375f * vdif[indx1 - v1] + 0.1375f * vdif[indx1 + v1], vdif[indx1 - v1], vdif[indx1 + v1]);
                ev = median(0.725f * hdif[indx1] + 0.1375f * hdif[indx1 - h1] + 0.1375f * hdif[indx1 + h1], hdif[indx1 - h1], hdif[indx1 + h1]);
                //Chrominance estimation
                chr[d][indx1] = (eg * nv + ng * ev) / (ng + eg);
                //Green channel population
                rgb[0][indx1] = rgb[0][indx1] + 65535.f * chr[d][indx1];
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.39);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++) {
            int col, indx, c;

            for (col = 7 + (fc(cfarray, row, 1) & 1), indx = row * width + col, c = 1 - fc(cfarray, row, col) / 2; col < width - 14; col += 8, indx += 8) {
                //NW,NE,SW,SE Gradients
                nwgv = onev / (epsv + vabsf(LVFU(chr[c][(indx - v1 - h1) >> 1]) - LVFU(chr[c][(indx - v3 - h3) >> 1])) + vabsf(LVFU(chr[c][(indx + v1 + h1) >> 1]) - LVFU(chr[c][(indx - v3 - h3) >> 1])));
                negv = onev / (epsv + vabsf(LVFU(chr[c][(indx - v1 + h1) >> 1]) - LVFU(chr[c][(indx - v3 + h3) >> 1])) + vabsf(LVFU(chr[c][(indx + v1 - h1) >> 1]) - LVFU(chr[c][(indx - v3 + h3) >> 1])));
                swgv = onev / (epsv + vabsf(LVFU(chr[c][(indx + v1 - h1) >> 1]) - LVFU(chr[c][(indx + v3 + h3) >> 1])) + vabsf(LVFU(chr[c][(indx - v1 + h1) >> 1]) - LVFU(chr[c][(indx + v3 - h3) >> 1])));
                segv = onev / (epsv + vabsf(LVFU(chr[c][(indx + v1 + h1) >> 1]) - LVFU(chr[c][(indx + v3 - h3) >> 1])) + vabsf(LVFU(chr[c][(indx - v1 - h1) >> 1]) - LVFU(chr[c][(indx + v3 + h3) >> 1])));
                //Limit NW,NE,SW,SE Color differences
                nwvv = median(LVFU(chr[c][(indx - v1 - h1) >> 1]), LVFU(chr[c][(indx - v3 - h1) >> 1]), LVFU(chr[c][(indx - v1 - h3) >> 1]));
                nevv = median(LVFU(chr[c][(indx - v1 + h1) >> 1]), LVFU(chr[c][(indx - v3 + h1) >> 1]), LVFU(chr[c][(indx - v1 + h3) >> 1]));
                swvv = median(LVFU(chr[c][(indx + v1 - h1) >> 1]), LVFU(chr[c][(indx + v3 - h1) >> 1]), LVFU(chr[c][(indx + v1 - h3) >> 1]));
                sevv = median(LVFU(chr[c][(indx + v1 + h1) >> 1]), LVFU(chr[c][(indx + v3 + h1) >> 1]), LVFU(chr[c][(indx + v1 + h3) >> 1]));
                //Interpolate chrominance: R@B and B@R
                tempv = (nwgv * nwvv + negv * nevv + swgv * swvv + segv * sevv) / (nwgv + negv + swgv + segv);
                STVFU(chr[c][indx >> 1], tempv);
            }

            for (; col < width - 7; col += 2, indx += 2) {
                //NW,NE,SW,SE Gradients
                nwg = 1.f / (eps + fabsf(chr[c][(indx - v1 - h1) >> 1] - chr[c][(indx - v3 - h3) >> 1]) + fabsf(chr[c][(indx + v1 + h1) >> 1] - chr[c][(indx - v3 - h3) >> 1]));
                neg = 1.f / (eps + fabsf(chr[c][(indx - v1 + h1) >> 1] - chr[c][(indx - v3 + h3) >> 1]) + fabsf(chr[c][(indx + v1 - h1) >> 1] - chr[c][(indx - v3 + h3) >> 1]));
                swg = 1.f / (eps + fabsf(chr[c][(indx + v1 - h1) >> 1] - chr[c][(indx + v3 + h3) >> 1]) + fabsf(chr[c][(indx - v1 + h1) >> 1] - chr[c][(indx + v3 - h3) >> 1]));
                seg = 1.f / (eps + fabsf(chr[c][(indx + v1 + h1) >> 1] - chr[c][(indx + v3 - h3) >> 1]) + fabsf(chr[c][(indx - v1 - h1) >> 1] - chr[c][(indx + v3 + h3) >> 1]));
                //Limit NW,NE,SW,SE Color differences
                nwv = median(chr[c][(indx - v1 - h1) >> 1], chr[c][(indx - v3 - h1) >> 1], chr[c][(indx - v1 - h3) >> 1]);
                nev = median(chr[c][(indx - v1 + h1) >> 1], chr[c][(indx - v3 + h1) >> 1], chr[c][(indx - v1 + h3) >> 1]);
                swv = median(chr[c][(indx + v1 - h1) >> 1], chr[c][(indx + v3 - h1) >> 1], chr[c][(indx + v1 - h3) >> 1]);
                sev = median(chr[c][(indx + v1 + h1) >> 1], chr[c][(indx + v3 + h1) >> 1], chr[c][(indx + v1 + h3) >> 1]);
                //Interpolate chrominance: R@B and B@R
                chr[c][indx >> 1] = (nwg * nwv + neg * nev + swg * swv + seg * sev) / (nwg + neg + swg + seg);
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
        setProgCancel(0.65);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++) {
            int col, indx;

            for (col = 7 + (fc(cfarray, row, 0) & 1), indx = row * width + col; col < width - 14; col += 8, indx += 8) {
                //N,E,W,S Gradients
                ngv = onev / (epsv + vabsf(LVFU(chr[0][(indx - v1) >> 1]) - LVFU(chr[0][(indx - v3) >> 1])) + vabsf(LVFU(chr[0][(indx + v1) >> 1]) - LVFU(chr[0][(indx - v3) >> 1])));
                egv = onev / (epsv + vabsf(LVFU(chr[0][(indx + h1) >> 1]) - LVFU(chr[0][(indx + h3) >> 1])) + vabsf(LVFU(chr[0][(indx - h1) >> 1]) - LVFU(chr[0][(indx + h3) >> 1])));
                wgv = onev / (epsv + vabsf(LVFU(chr[0][(indx - h1) >> 1]) - LVFU(chr[0][(indx - h3) >> 1])) + vabsf(LVFU(chr[0][(indx + h1) >> 1]) - LVFU(chr[0][(indx - h3) >> 1])));
                sgv = onev / (epsv + vabsf(LVFU(chr[0][(indx + v1) >> 1]) - LVFU(chr[0][(indx + v3) >> 1])) + vabsf(LVFU(chr[0][(indx - v1) >> 1]) - LVFU(chr[0][(indx + v3) >> 1])));
                //Interpolate chrominance: R@G and B@G
                tempv = (ngv * LVFU(chr[0][(indx - v1) >> 1]) + egv * LVFU(chr[0][(indx + h1) >> 1]) + wgv * LVFU(chr[0][(indx - h1) >> 1]) + sgv * LVFU(chr[0][(indx + v1) >> 1])) / (ngv + egv + wgv + sgv);
                STVFU(chr[0 + 2][indx >> 1], tempv);
            }

            for (; col < width - 7; col += 2, indx += 2) {
                //N,E,W,S Gradients
                ng = 1.f / (eps + fabsf(chr[0][(indx - v1) >> 1] - chr[0][(indx - v3) >> 1]) + fabsf(chr[0][(indx + v1) >> 1] - chr[0][(indx - v3) >> 1]));
                eg = 1.f / (eps + fabsf(chr[0][(indx + h1) >> 1] - chr[0][(indx + h3) >> 1]) + fabsf(chr[0][(indx - h1) >> 1] - chr[0][(indx + h3) >> 1]));
                wg = 1.f / (eps + fabsf(chr[0][(indx - h1) >> 1] - chr[0][(indx - h3) >> 1]) + fabsf(chr[0][(indx + h1) >> 1] - chr[0][(indx - h3) >> 1]));
                sg = 1.f / (eps + fabsf(chr[0][(indx + v1) >> 1] - chr[0][(indx + v3) >> 1]) + fabsf(chr[0][(indx - v1) >> 1] - chr[0][(indx + v3) >> 1]));
                //Interpolate chrominance: R@G and B@G
                chr[0 + 2][indx >> 1] = (ng * chr[0][(indx - v1) >> 1] + eg * chr[0][(indx + h1) >> 1] + wg * chr[0][(indx - h1) >> 1] + sg * chr[0][(indx + v1) >> 1]) / (ng + eg + wg + sg);
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
        setProgCancel(0.78);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++) {
            int col, indx;

            for (col = 7 + (fc(cfarray, row, 0) & 1), indx = row * width + col; col < width - 14; col += 8, indx += 8) {
                //N,E,W,S Gradients
                ngv = onev / (epsv + vabsf(LVFU(chr[1][(indx - v1) >> 1]) - LVFU(chr[1][(indx - v3) >> 1])) + vabsf(LVFU(chr[1][(indx + v1) >> 1]) - LVFU(chr[1][(indx - v3) >> 1])));
                egv = onev / (epsv + vabsf(LVFU(chr[1][(indx + h1) >> 1]) - LVFU(chr[1][(indx + h3) >> 1])) + vabsf(LVFU(chr[1][(indx - h1) >> 1]) - LVFU(chr[1][(indx + h3) >> 1])));
                wgv = onev / (epsv + vabsf(LVFU(chr[1][(indx - h1) >> 1]) - LVFU(chr[1][(indx - h3) >> 1])) + vabsf(LVFU(chr[1][(indx + h1) >> 1]) - LVFU(chr[1][(indx - h3) >> 1])));
                sgv = onev / (epsv + vabsf(LVFU(chr[1][(indx + v1) >> 1]) - LVFU(chr[1][(indx + v3) >> 1])) + vabsf(LVFU(chr[1][(indx - v1) >> 1]) - LVFU(chr[1][(indx + v3) >> 1])));
                //Interpolate chrominance: R@G and B@G
                tempv = (ngv * LVFU(chr[1][(indx - v1) >> 1]) + egv * LVFU(chr[1][(indx + h1) >> 1]) + wgv * LVFU(chr[1][(indx - h1) >> 1]) + sgv * LVFU(chr[1][(indx + v1) >> 1])) / (ngv + egv + wgv + sgv);
                STVFU(chr[1 + 2][indx >> 1], tempv);
            }

            for (; col < width - 7; col += 2, indx += 2) {
                //N,E,W,S Gradients
                ng = 1.f / (eps + fabsf(chr[1][(indx - v1) >> 1] - chr[1][(indx - v3) >> 1]) + fabsf(chr[1][(indx + v1) >> 1] - chr[1][(indx - v3) >> 1]));
                eg = 1.f / (eps + fabsf(chr[1][(indx + h1) >> 1] - chr[1][(indx + h3) >> 1]) + fabsf(chr[1][(indx - h1) >> 1] - chr[1][(indx + h3) >> 1]));
                wg = 1.f / (eps + fabsf(chr[1][(indx - h1) >> 1] - chr[1][(indx - h3) >> 1]) + fabsf(chr[1][(indx + h1) >> 1] - chr[1][(indx - h3) >> 1]));
                sg = 1.f / (eps + fabsf(chr[1][(indx + v1) >> 1] - chr[1][(indx + v3) >> 1]) + fabsf(chr[1][(indx - v1) >> 1] - chr[1][(indx + v3) >> 1]));
                //Interpolate chrominance: R@G and B@G
                chr[1 + 2][indx >> 1] = (ng * chr[1][(indx - v1) >> 1] + eg * chr[1][(indx + h1) >> 1] + wg * chr[1][(indx - h1) >> 1] + sg * chr[1][(indx + v1) >> 1]) / (ng + eg + wg + sg);
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.91);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for(int row = 7; row < height - 7; row++) {
            int col, indx, fc0;
            fc0 = fc(cfarray, row, 7) & 1;
            float* src1 = rgb[fc0];
            float* src2 = rgb[fc0 ^ 1];
            float* redsrc0 = chr[fc0 << 1];
            float* redsrc1 = chr[(fc0 ^ 1) << 1];
            float* bluesrc0 = chr[(fc0 << 1) + 1];
            float* bluesrc1 = chr[((fc0 ^ 1) << 1) + 1];

            for(col = 7, indx = row * width + col; col < width - 14; col += 8, indx += 8) {
                temp1v = LVFU(src1[indx >> 1]);
                temp2v = LVFU(src2[(indx + 1) >> 1]);
                tempv = _mm_shuffle_ps(temp1v, temp2v, _MM_SHUFFLE(1, 0, 1, 0));
                tempv = PERMUTEPS(tempv, _MM_SHUFFLE(3, 1, 2, 0));
                STVFU(green[row][col], CLIPV(tempv));
                temp5v = LVFU(redsrc0[indx >> 1]);
                temp6v = LVFU(redsrc1[(indx + 1) >> 1]);
                temp3v = _mm_shuffle_ps(temp5v, temp6v, _MM_SHUFFLE(1, 0, 1, 0));
                temp3v = PERMUTEPS(temp3v, _MM_SHUFFLE(3, 1, 2, 0));
                temp3v = CLIPV(tempv - c65535v * temp3v);
                STVFU(red[row][col], temp3v);
                temp7v = LVFU(bluesrc0[indx >> 1]);
                temp8v = LVFU(bluesrc1[(indx + 1) >> 1]);
                temp4v = _mm_shuffle_ps(temp7v, temp8v, _MM_SHUFFLE(1, 0, 1, 0));
                temp4v = PERMUTEPS(temp4v, _MM_SHUFFLE(3, 1, 2, 0));
                temp4v = CLIPV(tempv - c65535v * temp4v);
                STVFU(blue[row][col], temp4v);

                tempv = _mm_shuffle_ps(temp1v, temp2v, _MM_SHUFFLE(3, 2, 3, 2));
                tempv = PERMUTEPS(tempv, _MM_SHUFFLE(3, 1, 2, 0));
                STVFU(green[row][col + 4], CLIPV(tempv));

                temp3v = _mm_shuffle_ps(temp5v, temp6v, _MM_SHUFFLE(3, 2, 3, 2));
                temp3v = PERMUTEPS(temp3v, _MM_SHUFFLE(3, 1, 2, 0));
                temp3v = CLIPV(tempv - c65535v * temp3v);
                STVFU(red[row][col + 4], temp3v);
                temp4v = _mm_shuffle_ps(temp7v, temp8v, _MM_SHUFFLE(3, 2, 3, 2));
                temp4v = PERMUTEPS(temp4v, _MM_SHUFFLE(3, 1, 2, 0));
                temp4v = CLIPV(tempv - c65535v * temp4v);
                STVFU(blue[row][col + 4], temp4v);
            }

            for(; col < width - 7; col++, indx += 2) {
                red  [row][col] = CLIP(src1[indx >> 1] - 65535.f * redsrc0[indx >> 1]);
                green[row][col] = CLIP(src1[indx >> 1]);
                blue [row][col] = CLIP(src1[indx >> 1] - 65535.f * bluesrc0[indx >> 1]);
                col++;
                red  [row][col] = CLIP(src2[(indx + 1) >> 1] - 65535.f * redsrc1[(indx + 1) >> 1]);
                green[row][col] = CLIP(src2[(indx + 1) >> 1]);
                blue [row][col] = CLIP(src2[(indx + 1) >> 1] - 65535.f * bluesrc1[(indx + 1) >> 1]);
            }
        }
    }// End of parallelization

    rc = bayerborder_demosaic(winw, winh, 8, rawData, red, green, blue, cfarray);

    setProgCancel(1.0);

    free(chrarray);
    free(rgbarray);
    free(vdif);
    free(hdif);

    return rc;
}
#undef CLIPV
#else
rpError igv_demosaic(int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel)
{
    BENCHFUN
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    rpError rc = RP_NO_ERROR;

    constexpr float eps = 1e-5f, epssq = 1e-5f; //mod epssq -10f =>-5f Jacques 3/2013 to prevent artifact (divide by zero)
    constexpr int h1 = 1, h2 = 2, h3 = 3, h4 = 4, h5 = 5, h6 = 6;
    const int width = winw, height = winh;
    const int v1 = 1 * width, v2 = 2 * width, v3 = 3 * width, v4 = 4 * width, v5 = 5 * width, v6 = 6 * width;

    float *rgbarray = (float (*)) malloc((width * height) * sizeof(float));
    float *vdif = (float (*)) calloc(width * height / 2, sizeof * vdif);
    float *hdif = (float (*)) calloc(width * height / 2, sizeof * hdif);
    float *chrarray = (float (*)) calloc(width * height, sizeof(float));

    if(!rgbarray || !vdif || !hdif || !chrarray) {
        if (rgbarray) {
            free(rgbarray);
        }
        if (vdif) {
            free(vdif);
        }
        if (hdif) {
            free(hdif);
        }
        if (chrarray) {
            free(chrarray);
        }
        return RP_MEMORY_ERROR;
    }

    float* rgb[2];
    rgb[0] = rgbarray;
    rgb[1] = rgbarray + (width * height) / 2;

    float* chr[4];

    chr[0] = chrarray;
    chr[1] = chrarray + (width * height) / 2;

    // mapped chr[2] and chr[3] to hdif and hdif, because these are out of use, when chr[2] and chr[3] are used
    chr[2] = hdif;
    chr[3] = vdif;

    setProgCancel(0.0);

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {

        float ng, eg, wg, sg, nv, ev, wv, sv, nwg, neg, swg, seg, nwv, nev, swv, sev;

#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 0; row < height - 0; row++)
            for (int col = 0, indx = row * width + col; col < width - 0; col++, indx++) {
                int c = fc(cfarray, row, col);
                rgb[c][indx] = CLIP(rawData[row][col]); //rawData = RT datas
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.13);
        }

#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 5; row < height - 5; row++)
            for (int col = 5 + (fc(cfarray, row, 1) & 1), indx = row * width + col, c = fc(cfarray, row, col); col < width - 5; col += 2, indx += 2) {
                //N,E,W,S Gradients
                ng = (eps + (fabsf(rgb[1][indx - v1] - rgb[1][indx - v3]) + fabsf(rgb[c][indx] - rgb[c][indx - v2])) / 65535.f);;
                eg = (eps + (fabsf(rgb[1][indx + h1] - rgb[1][indx + h3]) + fabsf(rgb[c][indx] - rgb[c][indx + h2])) / 65535.f);
                wg = (eps + (fabsf(rgb[1][indx - h1] - rgb[1][indx - h3]) + fabsf(rgb[c][indx] - rgb[c][indx - h2])) / 65535.f);
                sg = (eps + (fabsf(rgb[1][indx + v1] - rgb[1][indx + v3]) + fabsf(rgb[c][indx] - rgb[c][indx + v2])) / 65535.f);
                //N,E,W,S High Order Interpolation (Li & Randhawa)
                //N,E,W,S Hamilton Adams Interpolation
                // (48.f * 65535.f) = 3145680.f
                nv = LIM(((23.f * rgb[1][indx - v1] + 23.f * rgb[1][indx - v3] + rgb[1][indx - v5] + rgb[1][indx + v1] + 40.f * rgb[c][indx] - 32.f * rgb[c][indx - v2] - 8.f * rgb[c][indx - v4])) / 3145680.f, 0.f, 1.f);
                ev = LIM(((23.f * rgb[1][indx + h1] + 23.f * rgb[1][indx + h3] + rgb[1][indx + h5] + rgb[1][indx - h1] + 40.f * rgb[c][indx] - 32.f * rgb[c][indx + h2] - 8.f * rgb[c][indx + h4])) / 3145680.f, 0.f, 1.f);
                wv = LIM(((23.f * rgb[1][indx - h1] + 23.f * rgb[1][indx - h3] + rgb[1][indx - h5] + rgb[1][indx + h1] + 40.f * rgb[c][indx] - 32.f * rgb[c][indx - h2] - 8.f * rgb[c][indx - h4])) / 3145680.f, 0.f, 1.f);
                sv = LIM(((23.f * rgb[1][indx + v1] + 23.f * rgb[1][indx + v3] + rgb[1][indx + v5] + rgb[1][indx - v1] + 40.f * rgb[c][indx] - 32.f * rgb[c][indx + v2] - 8.f * rgb[c][indx + v4])) / 3145680.f, 0.f, 1.f);
                //Horizontal and vertical color differences
                vdif[indx >> 1] = (sg * nv + ng * sv) / (ng + sg) - (rgb[c][indx]) / 65535.f;
                hdif[indx >> 1] = (wg * ev + eg * wv) / (eg + wg) - (rgb[c][indx]) / 65535.f;
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.26);
        }

#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++)
            for (int col = 7 + (fc(cfarray, row, 1) & 1), indx = row * width + col, c = fc(cfarray, row, col), d = c / 2; col < width - 7; col += 2, indx += 2) {
                //H&V integrated gaussian vector over variance on color differences
                //Mod Jacques 3/2013
                ng = LIM(epssq + 78.f * SQR(vdif[indx >> 1]) + 69.f * (SQR(vdif[(indx - v2) >> 1]) + SQR(vdif[(indx + v2) >> 1])) + 51.f * (SQR(vdif[(indx - v4) >> 1]) + SQR(vdif[(indx + v4) >> 1])) + 21.f * (SQR(vdif[(indx - v6) >> 1]) + SQR(vdif[(indx + v6) >> 1])) - 6.f * SQR(vdif[(indx - v2) >> 1] + vdif[indx >> 1] + vdif[(indx + v2) >> 1])
                         - 10.f * (SQR(vdif[(indx - v4) >> 1] + vdif[(indx - v2) >> 1] + vdif[indx >> 1]) + SQR(vdif[indx >> 1] + vdif[(indx + v2) >> 1] + vdif[(indx + v4) >> 1])) - 7.f * (SQR(vdif[(indx - v6) >> 1] + vdif[(indx - v4) >> 1] + vdif[(indx - v2) >> 1]) + SQR(vdif[(indx + v2) >> 1] + vdif[(indx + v4) >> 1] + vdif[(indx + v6) >> 1])), 0.f, 1.f);
                eg = LIM(epssq + 78.f * SQR(hdif[indx >> 1]) + 69.f * (SQR(hdif[(indx - h2) >> 1]) + SQR(hdif[(indx + h2) >> 1])) + 51.f * (SQR(hdif[(indx - h4) >> 1]) + SQR(hdif[(indx + h4) >> 1])) + 21.f * (SQR(hdif[(indx - h6) >> 1]) + SQR(hdif[(indx + h6) >> 1])) - 6.f * SQR(hdif[(indx - h2) >> 1] + hdif[indx >> 1] + hdif[(indx + h2) >> 1])
                         - 10.f * (SQR(hdif[(indx - h4) >> 1] + hdif[(indx - h2) >> 1] + hdif[indx >> 1]) + SQR(hdif[indx >> 1] + hdif[(indx + h2) >> 1] + hdif[(indx + h4) >> 1])) - 7.f * (SQR(hdif[(indx - h6) >> 1] + hdif[(indx - h4) >> 1] + hdif[(indx - h2) >> 1]) + SQR(hdif[(indx + h2) >> 1] + hdif[(indx + h4) >> 1] + hdif[(indx + h6) >> 1])), 0.f, 1.f);
                //Limit chrominance using H/V neighbourhood
                nv = median(0.725f * vdif[indx >> 1] + 0.1375f * vdif[(indx - v2) >> 1] + 0.1375f * vdif[(indx + v2) >> 1], vdif[(indx - v2) >> 1], vdif[(indx + v2) >> 1]);
                ev = median(0.725f * hdif[indx >> 1] + 0.1375f * hdif[(indx - h2) >> 1] + 0.1375f * hdif[(indx + h2) >> 1], hdif[(indx - h2) >> 1], hdif[(indx + h2) >> 1]);
                //Chrominance estimation
                chr[d][indx] = (eg * nv + ng * ev) / (ng + eg);
                //Green channel population
                rgb[1][indx] = rgb[c][indx] + 65535.f * chr[d][indx];
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.39);
        }

//  free(vdif); free(hdif);
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row += 2)
            for (int col = 7 + (fc(cfarray, row, 1) & 1), indx = row * width + col, c = 1 - fc(cfarray, row, col) / 2; col < width - 7; col += 2, indx += 2) {
                //NW,NE,SW,SE Gradients
                nwg = 1.f / (eps + fabsf(chr[c][indx - v1 - h1] - chr[c][indx - v3 - h3]) + fabsf(chr[c][indx + v1 + h1] - chr[c][indx - v3 - h3]));
                neg = 1.f / (eps + fabsf(chr[c][indx - v1 + h1] - chr[c][indx - v3 + h3]) + fabsf(chr[c][indx + v1 - h1] - chr[c][indx - v3 + h3]));
                swg = 1.f / (eps + fabsf(chr[c][indx + v1 - h1] - chr[c][indx + v3 + h3]) + fabsf(chr[c][indx - v1 + h1] - chr[c][indx + v3 - h3]));
                seg = 1.f / (eps + fabsf(chr[c][indx + v1 + h1] - chr[c][indx + v3 - h3]) + fabsf(chr[c][indx - v1 - h1] - chr[c][indx + v3 + h3]));
                //Limit NW,NE,SW,SE Color differences
                nwv = median(chr[c][indx - v1 - h1], chr[c][indx - v3 - h1], chr[c][indx - v1 - h3]);
                nev = median(chr[c][indx - v1 + h1], chr[c][indx - v3 + h1], chr[c][indx - v1 + h3]);
                swv = median(chr[c][indx + v1 - h1], chr[c][indx + v3 - h1], chr[c][indx + v1 - h3]);
                sev = median(chr[c][indx + v1 + h1], chr[c][indx + v3 + h1], chr[c][indx + v1 + h3]);
                //Interpolate chrominance: R@B and B@R
                chr[c][indx] = (nwg * nwv + neg * nev + swg * swv + seg * sev) / (nwg + neg + swg + seg);
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.52);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 8; row < height - 7; row += 2)
            for (int col = 7 + (fc(cfarray, row, 1) & 1), indx = row * width + col, c = 1 - fc(cfarray, row, col) / 2; col < width - 7; col += 2, indx += 2) {
                //NW,NE,SW,SE Gradients
                nwg = 1.f / (eps + fabsf(chr[c][indx - v1 - h1] - chr[c][indx - v3 - h3]) + fabsf(chr[c][indx + v1 + h1] - chr[c][indx - v3 - h3]));
                neg = 1.f / (eps + fabsf(chr[c][indx - v1 + h1] - chr[c][indx - v3 + h3]) + fabsf(chr[c][indx + v1 - h1] - chr[c][indx - v3 + h3]));
                swg = 1.f / (eps + fabsf(chr[c][indx + v1 - h1] - chr[c][indx + v3 + h3]) + fabsf(chr[c][indx - v1 + h1] - chr[c][indx + v3 - h3]));
                seg = 1.f / (eps + fabsf(chr[c][indx + v1 + h1] - chr[c][indx + v3 - h3]) + fabsf(chr[c][indx - v1 - h1] - chr[c][indx + v3 + h3]));
                //Limit NW,NE,SW,SE Color differences
                nwv = median(chr[c][indx - v1 - h1], chr[c][indx - v3 - h1], chr[c][indx - v1 - h3]);
                nev = median(chr[c][indx - v1 + h1], chr[c][indx - v3 + h1], chr[c][indx - v1 + h3]);
                swv = median(chr[c][indx + v1 - h1], chr[c][indx + v3 - h1], chr[c][indx + v1 - h3]);
                sev = median(chr[c][indx + v1 + h1], chr[c][indx + v3 + h1], chr[c][indx + v1 + h3]);
                //Interpolate chrominance: R@B and B@R
                chr[c][indx] = (nwg * nwv + neg * nev + swg * swv + seg * sev) / (nwg + neg + swg + seg);
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.65);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++)
            for (int col = 7 + (fc(cfarray, row, 0) & 1), indx = row * width + col; col < width - 7; col += 2, indx += 2) {
                //N,E,W,S Gradients
                ng = 1.f / (eps + fabsf(chr[0][indx - v1] - chr[0][indx - v3]) + fabsf(chr[0][indx + v1] - chr[0][indx - v3]));
                eg = 1.f / (eps + fabsf(chr[0][indx + h1] - chr[0][indx + h3]) + fabsf(chr[0][indx - h1] - chr[0][indx + h3]));
                wg = 1.f / (eps + fabsf(chr[0][indx - h1] - chr[0][indx - h3]) + fabsf(chr[0][indx + h1] - chr[0][indx - h3]));
                sg = 1.f / (eps + fabsf(chr[0][indx + v1] - chr[0][indx + v3]) + fabsf(chr[0][indx - v1] - chr[0][indx + v3]));
                //Interpolate chrominance: R@G and B@G
                chr[0][indx] = ((ng * chr[0][indx - v1] + eg * chr[0][indx + h1] + wg * chr[0][indx - h1] + sg * chr[0][indx + v1]) / (ng + eg + wg + sg));
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.78);
        }
#ifdef _OPENMP
        #pragma omp for
#endif

        for (int row = 7; row < height - 7; row++)
            for (int col = 7 + (fc(cfarray, row, 0) & 1), indx = row * width + col; col < width - 7; col += 2, indx += 2) {

                //N,E,W,S Gradients
                ng = 1.f / (eps + fabsf(chr[1][indx - v1] - chr[1][indx - v3]) + fabsf(chr[1][indx + v1] - chr[1][indx - v3]));
                eg = 1.f / (eps + fabsf(chr[1][indx + h1] - chr[1][indx + h3]) + fabsf(chr[1][indx - h1] - chr[1][indx + h3]));
                wg = 1.f / (eps + fabsf(chr[1][indx - h1] - chr[1][indx - h3]) + fabsf(chr[1][indx + h1] - chr[1][indx - h3]));
                sg = 1.f / (eps + fabsf(chr[1][indx + v1] - chr[1][indx + v3]) + fabsf(chr[1][indx - v1] - chr[1][indx + v3]));
                //Interpolate chrominance: R@G and B@G
                chr[1][indx] = ((ng * chr[1][indx - v1] + eg * chr[1][indx + h1] + wg * chr[1][indx - h1] + sg * chr[1][indx + v1]) / (ng + eg + wg + sg));
            }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            setProgCancel(0.91);
        }

#ifdef _OPENMP
        #pragma omp for
#endif

        for(int row = 7; row < height - 7; row++)
            for(int col = 7, indx = row * width + col; col < width - 7; col++, indx++) {
                red  [row][col] = CLIP(rgb[1][indx] - 65535.f * chr[0][indx]);
                green[row][col] = CLIP(rgb[1][indx]);
                blue [row][col] = CLIP(rgb[1][indx] - 65535.f * chr[1][indx]);
            }
    }// End of parallelization

    rc = bayerborder_demosaic(winw, winh, 8, rawData, red, green, blue, cfarray);

    setProgCancel(1.0);

    free(chrarray);
    free(rgbarray);
    free(vdif);
    free(hdif);

    return rc;
}
#endif

