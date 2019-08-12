////////////////////////////////////////////////////////////////
//
//      Fast demosaicing algorithm
//
//      copyright (c) 2008-2010  Emil Martinec <ejmartin@uchicago.edu>
//
//
// code dated: August 26, 2010
//
//  fast_demo.cc is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include <cmath>
#include "bayerhelper.h"
#include "librtprocess.h"
#include "opthelper.h"
#include "rt_math.h"
#include "StopWatch.h"

using namespace librtprocess;

#define TS 224

#define INVGRAD(i) (16.0f/SQR(4.0f+i))
#ifdef __SSE2__
#define INVGRADV(i) (c16v*_mm_rcp_ps(SQRV(fourv+i)))
#endif


rpError bayerfast_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, double initGain)
{

    BENCHFUN
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    rpError rc = RP_NO_ERROR;

    double progress = 0.0;
    setProgCancel(progress);

    const int bord = 5;
    const int H = height;
    const int W = width;
    const float clip_pt = 4 * 65535 * initGain;

    rc = bayerborder_demosaic(width, height, bord, rawData, red, green, blue, cfarray);

    progress += 0.1;
    setProgCancel(progress);

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {

#define CLF 1
        // assign working space
        char *buffer = (char *) calloc(3 * sizeof(float) * TS * TS + 3 * CLF * 64 + 63, 1);
#ifdef _OPENMP
    #pragma omp critical
#endif
        {
            if (!buffer) {
                rc = RP_MEMORY_ERROR;
            }
        }
#ifdef _OPENMP
        #pragma omp barrier
#endif
        if (!rc) {
            char *data = (char*)((uintptr_t(buffer) + uintptr_t(63)) / 64 * 64);

            float * const greentile = (float (*)) data; //pointers to array
            float * const redtile   = (float (*)) ((char*)greentile + sizeof(float) * TS * TS + CLF * 64);
            float * const bluetile  = (float (*)) ((char*)redtile + sizeof(float) * TS * TS + CLF * 64);

            int progressCounter = 0;
            const double progressInc = 16.0 * (1.0 - progress) / ((H * W) / ((TS - 4) * (TS - 4)));

#ifdef _OPENMP
            #pragma omp for nowait
#endif

            for (int top = bord - 2; top < H - bord + 2; top += TS - 4)
                for (int left = bord - 2; left < W - bord + 2; left += TS - 4) {
                    const int bottom = min(top + TS, H - bord + 2);
                    const int right  = min(left + TS, W - bord + 2);

#ifdef __SSE2__
                    const vfloat c16v = F2V(16.0f);
                    const vfloat fourv = F2V(4.0f);
                    vmask selmask;
                    const vmask andmask = _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

                    if(fc(cfarray, top, left) == 1) {
                        selmask = _mm_set_epi32(0, 0xffffffff, 0, 0xffffffff);
                    } else {
                        selmask = _mm_set_epi32(0xffffffff, 0, 0xffffffff, 0);
                    }

#endif

                    // interpolate G using gradient weights
                    for (int i = top, rr = 0; i < bottom; i++, rr++) {
                        int j = left;
                        int cc = 0;
#ifdef __SSE2__
                        selmask = (vmask)_mm_andnot_ps((vfloat)selmask, (vfloat)andmask);

                        for (; j < right - 3; j += 4, cc += 4) {
                            const vfloat tempv = LVFU(rawData[i][j]);
                            const vfloat absv = vabsf(LVFU(rawData[i - 1][j]) - LVFU(rawData[i + 1][j]));
                            const vfloat wtuv = INVGRADV(absv + vabsf(tempv - LVFU(rawData[i - 2][j])) + vabsf(LVFU(rawData[i - 1][j]) - LVFU(rawData[i - 3][j])));
                            const vfloat wtdv = INVGRADV(absv + vabsf(tempv - LVFU(rawData[i + 2][j])) + vabsf(LVFU(rawData[i + 1][j]) - LVFU(rawData[i + 3][j])));
                            const vfloat abs2v = vabsf(LVFU(rawData[i][j - 1]) - LVFU(rawData[i][j + 1]));
                            const vfloat wtlv = INVGRADV(abs2v + vabsf(tempv - LVFU(rawData[i][j - 2])) + vabsf(LVFU(rawData[i][j - 1]) - LVFU(rawData[i][j - 3])));
                            const vfloat wtrv = INVGRADV(abs2v + vabsf(tempv - LVFU(rawData[i][j + 2])) + vabsf(LVFU(rawData[i][j + 1]) - LVFU(rawData[i][j + 3])));
                            const vfloat greenv = (wtuv * LVFU(rawData[i - 1][j]) + wtdv * LVFU(rawData[i + 1][j]) + wtlv * LVFU(rawData[i][j - 1]) + wtrv * LVFU(rawData[i][j + 1])) / (wtuv + wtdv + wtlv + wtrv);
                            STVF(greentile[rr * TS + cc], vself(selmask, greenv, tempv));
                            STVF(redtile[rr * TS + cc], tempv);
                            STVF(bluetile[rr * TS + cc], tempv);
                        }
#endif
                        for (; j < right; j++, cc++) {

                            if (fc(cfarray, i, j) == 1) {
                                greentile[rr * TS + cc] = rawData[i][j];

                            } else {
                                //compute directional weights using image gradients
                                const float wtu = INVGRAD((abs(rawData[i + 1][j] - rawData[i - 1][j]) + abs(rawData[i][j] - rawData[i - 2][j]) + abs(rawData[i - 1][j] - rawData[i - 3][j])));
                                const float wtd = INVGRAD((abs(rawData[i - 1][j] - rawData[i + 1][j]) + abs(rawData[i][j] - rawData[i + 2][j]) + abs(rawData[i + 1][j] - rawData[i + 3][j])));
                                const float wtl = INVGRAD((abs(rawData[i][j + 1] - rawData[i][j - 1]) + abs(rawData[i][j] - rawData[i][j - 2]) + abs(rawData[i][j - 1] - rawData[i][j - 3])));
                                const float wtr = INVGRAD((abs(rawData[i][j - 1] - rawData[i][j + 1]) + abs(rawData[i][j] - rawData[i][j + 2]) + abs(rawData[i][j + 1] - rawData[i][j + 3])));

                                //store in rgb array the interpolated G value at R/B grid points using directional weighted average
                                greentile[rr * TS + cc] = (wtu * rawData[i - 1][j] + wtd * rawData[i + 1][j] + wtl * rawData[i][j - 1] + wtr * rawData[i][j + 1]) / (wtu + wtd + wtl + wtr);
                            }

                            redtile[rr * TS + cc] = rawData[i][j];
                            bluetile[rr * TS + cc] = rawData[i][j];
                        }
                    }

#ifdef __SSE2__
                    const vfloat zd25v = F2V(0.25f);
                    const vfloat clip_ptv = F2V(clip_pt);
#endif

                    for (int i = top + 1, rr = 1; i < bottom - 1; i++, rr++) {
                        if (fc(cfarray, i, left + (fc(cfarray, i, 2) & 1) + 1) == 0)
#ifdef __SSE2__
                            for (int j = left + 1, cc = 1; j < right - 1; j += 4, cc += 4) {
                                //interpolate B/R colors at R/B sites
                                STVFU(bluetile[rr * TS + cc], LVFU(greentile[rr * TS + cc]) - zd25v * ((LVFU(greentile[(rr - 1)*TS + (cc - 1)]) + LVFU(greentile[(rr - 1)*TS + (cc + 1)]) + LVFU(greentile[(rr + 1)*TS + cc + 1]) + LVFU(greentile[(rr + 1)*TS + cc - 1])) -
                                              vminf(LVFU(rawData[i - 1][j - 1]) + LVFU(rawData[i - 1][j + 1]) + LVFU(rawData[i + 1][j + 1]) + LVFU(rawData[i + 1][j - 1]), clip_ptv)));
                            }

#else

                            for (int cc = (fc(cfarray, i, 2) & 1) + 1, j = left + cc; j < right - 1; j += 2, cc += 2) {
                                //interpolate B/R colors at R/B sites
                                bluetile[rr * TS + cc] = greentile[rr * TS + cc] - 0.25f * ((greentile[(rr - 1) * TS + (cc - 1)] + greentile[(rr - 1) * TS + (cc + 1)] + greentile[(rr + 1) * TS + cc + 1] + greentile[(rr + 1) * TS + cc - 1]) -
                                                         min(clip_pt, rawData[i - 1][j - 1] + rawData[i - 1][j + 1] + rawData[i + 1][j + 1] + rawData[i + 1][j - 1]));
                            }

#endif
                        else
#ifdef __SSE2__
                            for (int j = left + 1, cc = 1; j < right - 1; j += 4, cc += 4) {
                                //interpolate B/R colors at R/B sites
                                STVFU(redtile[rr * TS + cc], LVFU(greentile[rr * TS + cc]) - zd25v * ((LVFU(greentile[(rr - 1)*TS + cc - 1]) + LVFU(greentile[(rr - 1)*TS + cc + 1]) + LVFU(greentile[(rr + 1)*TS + cc + 1]) + LVFU(greentile[(rr + 1)*TS + cc - 1])) -
                                              vminf(LVFU(rawData[i - 1][j - 1]) + LVFU(rawData[i - 1][j + 1]) + LVFU(rawData[i + 1][j + 1]) + LVFU(rawData[i + 1][j - 1]), clip_ptv)));
                            }

#else

                            for (int cc = (fc(cfarray, i, 2) & 1) + 1, j = left + cc; j < right - 1; j += 2, cc += 2) {
                                //interpolate B/R colors at R/B sites
                                redtile[rr * TS + cc] = greentile[rr * TS + cc] - 0.25f * ((greentile[(rr - 1) * TS + cc - 1] + greentile[(rr - 1) * TS + cc + 1] + greentile[(rr + 1) * TS + cc + 1] + greentile[(rr + 1) * TS + cc - 1]) -
                                                        min(clip_pt, rawData[i - 1][j - 1] + rawData[i - 1][j + 1] + rawData[i + 1][j + 1] + rawData[i + 1][j - 1]));
                            }

#endif
                    }


#ifdef __SSE2__
                    selmask = _mm_set_epi32(0xffffffff, 0, 0xffffffff, 0);
#endif

                    // interpolate R/B using color differences
                    for (int i = top + 2, rr = 2; i < bottom - 2; i++, rr++) {
#ifdef __SSE2__

                        for (int cc = 2 + (fc(cfarray, i, 2) & 1), j = left + cc; j < right - 2; j += 4, cc += 4) {
                            // no need to take care about the borders of the tile. There's enough free space.
                            //interpolate R and B colors at G sites
                            const vfloat greenv = LVFU(greentile[rr * TS + cc]);
                            const vfloat greensumv = LVFU(greentile[(rr - 1) * TS + cc]) + LVFU(greentile[(rr + 1) * TS + cc]) + LVFU(greentile[rr * TS + cc - 1]) + LVFU(greentile[rr * TS + cc + 1]);

                            vfloat temp1v = LVFU(redtile[rr * TS + cc]);
                            vfloat temp2v = greenv - zd25v * (greensumv - LVFU(redtile[(rr - 1) * TS + cc]) - LVFU(redtile[(rr + 1) * TS + cc]) - LVFU(redtile[rr * TS + cc - 1]) - LVFU(redtile[rr * TS + cc + 1]));

                            STVFU(redtile[rr * TS + cc], vself(selmask, temp1v, temp2v));

                            temp1v = LVFU(bluetile[rr * TS + cc]);

                            temp2v = greenv - zd25v * (greensumv - LVFU(bluetile[(rr - 1) * TS + cc]) - LVFU(bluetile[(rr + 1) * TS + cc]) - LVFU(bluetile[rr * TS + cc - 1]) - LVFU(bluetile[rr * TS + cc + 1]));

                            STVFU(bluetile[rr * TS + cc], vself(selmask, temp1v, temp2v));
                        }

#else

                        for (int cc = 2 + (fc(cfarray, i, 2) & 1), j = left + cc; j < right - 2; j += 2, cc += 2) {
                            //interpolate R and B colors at G sites
                            redtile[rr * TS + cc] = greentile[rr * TS + cc] - 0.25f * ((greentile[(rr - 1) * TS + cc] - redtile[(rr - 1) * TS + cc]) + (greentile[(rr + 1) * TS + cc] - redtile[(rr + 1) * TS + cc]) +
                                                    (greentile[rr * TS + cc - 1] - redtile[rr * TS + cc - 1]) + (greentile[rr * TS + cc + 1] - redtile[rr * TS + cc + 1]));
                            bluetile[rr * TS + cc] = greentile[rr * TS + cc] - 0.25f * ((greentile[(rr - 1) * TS + cc] - bluetile[(rr - 1) * TS + cc]) + (greentile[(rr + 1) * TS + cc] - bluetile[(rr + 1) * TS + cc]) +
                                                     (greentile[rr * TS + cc - 1] - bluetile[rr * TS + cc - 1]) + (greentile[rr * TS + cc + 1] - bluetile[rr * TS + cc + 1]));
                        }

#endif
                    }


                    for (int i = top + 2, rr = 2; i < bottom - 2; i++, rr++) {
                        int j = left + 2;
                        int cc = 2;
#ifdef __SSE2__
                        for (; j < right - 5; j += 4, cc += 4) {
                            STVFU(red[i][j], LVFU(redtile[rr * TS + cc]));
                            STVFU(green[i][j], LVFU(greentile[rr * TS + cc]));
                            STVFU(blue[i][j], LVFU(bluetile[rr * TS + cc]));
                        }
#endif
                        for (; j < right - 2; j++, cc++) {
                            red[i][j] = redtile[rr * TS + cc];
                            green[i][j] = greentile[rr * TS + cc];
                            blue[i][j] = bluetile[rr * TS + cc];
                        }
                    }

                    if((++progressCounter) % 16 == 0) {
#ifdef _OPENMP
                        #pragma omp critical (updateprogress)
#endif
                        {
                            progress += progressInc;
                            progress = min(1.0, progress);
                            setProgCancel(progress);
                        }
                    }
                }
            }
        free(buffer);
    } // End of parallelization

    setProgCancel(1.0);
    return rc;

}
#undef TS
#undef CLF
