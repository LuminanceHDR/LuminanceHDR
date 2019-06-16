////////////////////////////////////////////////////////////////
//
//          VNG4 demosaic algorithm
// 
// optimized for speed by Ingo Weyrich
//
//
//  vng4_interpolate_RT.cc is free software: you can redistribute it and/or modify
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
#include <climits>
#include "bayerhelper.h"
#include "librtprocess.h"
#include "opthelper.h"
#include "rt_math.h"
#include "StopWatch.h"

using namespace librtprocess;


rpError vng4_demosaic (int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel)
{
    BENCHFUN
    if (!validateBayerCfa(4, cfarray)) {
        return RP_WRONG_CFA;
    }

    rpError rc = RP_NO_ERROR;

    const signed short int terms[] = {
        -2, -2, +0, -1, 0, 0x01, -2, -2, +0, +0, 1, 0x01, -2, -1, -1, +0, 0, 0x01,
        -2, -1, +0, -1, 0, 0x02, -2, -1, +0, +0, 0, 0x03, -2, -1, +0, +1, 1, 0x01,
        -2, +0, +0, -1, 0, 0x06, -2, +0, +0, +0, 1, 0x02, -2, +0, +0, +1, 0, 0x03,
        -2, +1, -1, +0, 0, 0x04, -2, +1, +0, -1, 1, 0x04, -2, +1, +0, +0, 0, 0x06,
        -2, +1, +0, +1, 0, 0x02, -2, +2, +0, +0, 1, 0x04, -2, +2, +0, +1, 0, 0x04,
        -1, -2, -1, +0, 0, 0x80, -1, -2, +0, -1, 0, 0x01, -1, -2, +1, -1, 0, 0x01,
        -1, -2, +1, +0, 1, 0x01, -1, -1, -1, +1, 0, 0x88, -1, -1, +1, -2, 0, 0x40,
        -1, -1, +1, -1, 0, 0x22, -1, -1, +1, +0, 0, 0x33, -1, -1, +1, +1, 1, 0x11,
        -1, +0, -1, +2, 0, 0x08, -1, +0, +0, -1, 0, 0x44, -1, +0, +0, +1, 0, 0x11,
        -1, +0, +1, -2, 1, 0x40, -1, +0, +1, -1, 0, 0x66, -1, +0, +1, +0, 1, 0x22,
        -1, +0, +1, +1, 0, 0x33, -1, +0, +1, +2, 1, 0x10, -1, +1, +1, -1, 1, 0x44,
        -1, +1, +1, +0, 0, 0x66, -1, +1, +1, +1, 0, 0x22, -1, +1, +1, +2, 0, 0x10,
        -1, +2, +0, +1, 0, 0x04, -1, +2, +1, +0, 1, 0x04, -1, +2, +1, +1, 0, 0x04,
        +0, -2, +0, +0, 1, 0x80, +0, -1, +0, +1, 1, 0x88, +0, -1, +1, -2, 0, 0x40,
        +0, -1, +1, +0, 0, 0x11, +0, -1, +2, -2, 0, 0x40, +0, -1, +2, -1, 0, 0x20,
        +0, -1, +2, +0, 0, 0x30, +0, -1, +2, +1, 1, 0x10, +0, +0, +0, +2, 1, 0x08,
        +0, +0, +2, -2, 1, 0x40, +0, +0, +2, -1, 0, 0x60, +0, +0, +2, +0, 1, 0x20,
        +0, +0, +2, +1, 0, 0x30, +0, +0, +2, +2, 1, 0x10, +0, +1, +1, +0, 0, 0x44,
        +0, +1, +1, +2, 0, 0x10, +0, +1, +2, -1, 1, 0x40, +0, +1, +2, +0, 0, 0x60,
        +0, +1, +2, +1, 0, 0x20, +0, +1, +2, +2, 0, 0x10, +1, -2, +1, +0, 0, 0x80,
        +1, -1, +1, +1, 0, 0x88, +1, +0, +1, +2, 0, 0x08, +1, +0, +2, -1, 0, 0x40,
        +1, +0, +2, +1, 0, 0x10
    },
    chood[] = { -1, -1, -1, 0, -1, +1, 0, +1, +1, +1, +1, 0, +1, -1, 0, -1 };

    double progress = 0.0;
    setProgCancel(progress);

    constexpr unsigned int colors = 4;

    float (*image)[4] = (float (*)[4]) calloc (height * width, sizeof * image);

    if (!image) {
        return RP_MEMORY_ERROR;
    }

    int lcode[16][16][32];
    float mul[16][16][8];
    float csum[16][16][3];

    // first linear interpolation
    for (int row = 0; row < 16; row++)
        for (int col = 0; col < 16; col++) {
            int * ip = lcode[row][col];
            int mulcount = 0;
            float sum[4] = {};

            for (int y = -1; y <= 1; y++)
                for (int x = -1; x <= 1; x++) {
                    int shift = (y == 0) + (x == 0);

                    if (shift == 2) {
                        continue;
                    }

                    int color = fc(cfarray, row + y, col + x);
                    *ip++ = (width * y + x) * 4 + color;

                    mul[row][col][mulcount] = (1 << shift);
                    *ip++ = color;
                    sum[color] += (1 << shift);
                    mulcount++;
                }

            int colcount = 0;

            for (unsigned int c = 0; c < colors; c++)
                if (c != fc(cfarray, row, col)) {
                    *ip++ = c;
                    csum[row][col][colcount] = 1.f / sum[c];
                    colcount ++;
                }
        }

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

        for (int ii = 0; ii < height; ii++) {
            if (firstRow == -1) {
                firstRow = ii;
            }
            lastRow = ii;
            for (int jj = 0; jj < width; jj++) {
                image[ii * width + jj][fc(cfarray, ii, jj)] = rawData[ii][jj];
            }
            if (ii - 1 > firstRow) {
                int row = ii - 1;
                for (int col = 1; col < width - 1; col++) {
                    float * pix = image[row * width + col];
                    int * ip = lcode[row & 15][col & 15];
                    float sum[4] = {};

                    for (int i = 0; i < 8; i++, ip += 2) {
                        sum[ip[1]] += pix[ip[0]] * mul[row & 15][col & 15][i];
                    }

                    for (unsigned int i = 0; i < colors - 1; i++, ip++) {
                        pix[ip[0]] = sum[ip[0]] * csum[row & 15][col & 15][i];
                    }
                }
            }
        }

        // now all rows are processed except the first and last row of each chunk
        // let's process them now but skip row 0 and row H - 1
        if (firstRow > 0 && firstRow < height - 1) {
            const int row = firstRow;
            for (int col = 1; col < width - 1; col++) {
                float * pix = image[row * width + col];
                int * ip = lcode[row & 15][col & 15];
                float sum[4] = {};

                for (int i = 0; i < 8; i++, ip += 2) {
                    sum[ip[1]] += pix[ip[0]] * mul[row & 15][col & 15][i];
                }

                for (unsigned int i = 0; i < colors - 1; i++, ip++) {
                    pix[ip[0]] = sum[ip[0]] * csum[row & 15][col & 15][i];
                }
            }
        }

        if (lastRow > 0 && lastRow < height - 1) {
            const int row = lastRow;
            for (int col = 1; col < width - 1; col++) {
                float * pix = image[row * width + col];
                int * ip = lcode[row & 15][col & 15];
                float sum[4] = {};

                for (int i = 0; i < 8; i++, ip += 2) {
                    sum[ip[1]] += pix[ip[0]] * mul[row & 15][col & 15][i];
                }

                for (unsigned int i = 0; i < colors - 1; i++, ip++) {
                    pix[ip[0]] = sum[ip[0]] * csum[row & 15][col & 15][i];
                }
            }
        }
    }

    constexpr int prow = 7, pcol = 1;
    int32_t *code[8][2];
    int32_t *ipp = (int32_t *) calloc ((prow + 1) * (pcol + 1), 1280);
    if(!ipp) {
        rc = RP_MEMORY_ERROR;
    } else {

        for (int row = 0; row <= prow; row++)   /* Precalculate for VNG */
            for (int col = 0; col <= pcol; col++) {
                code[row][col] = ipp;
                const signed short int* cp = terms;
                for (int t = 0; t < 64; t++) {
                    int y1 = *cp++;
                    int x1 = *cp++;
                    int y2 = *cp++;
                    int x2 = *cp++;
                    int weight = *cp++;
                    int grads = *cp++;
                    unsigned int color = fc(cfarray, row + y1, col + x1);

                    if (fc(cfarray, row + y2, col + x2) != color) {
                        continue;
                    }

                    int diag = (fc(cfarray, row, col + 1) == color && fc(cfarray, row + 1, col) == color) ? 2 : 1;

                    if (abs(y1 - y2) == diag && abs(x1 - x2) == diag) {
                        continue;
                    }

                    *ipp++ = (y1 * width + x1) * 4 + color;
                    *ipp++ = (y2 * width + x2) * 4 + color;
#ifdef __SSE2__
                    // at least on machines with SSE2 feature this cast is save
                    *reinterpret_cast<float*>(ipp++) = 1 << weight;
#else
                    *ipp++ = 1 << weight;
#endif
                    for (int g = 0; g < 8; g++)
                        if (grads & (1 << g)) {
                            *ipp++ = g;
                        }

                    *ipp++ = -1;
                }

                *ipp++ = INT_MAX;

                cp = chood;
                for (int g = 0; g < 8; g++) {
                    int y = *cp++;
                    int x = *cp++;
                    *ipp++ = (y * width + x) * 4;
                    unsigned int color = fc(cfarray, row, col);

                    if (fc(cfarray, row + y, col + x) != color && fc(cfarray, row + y * 2, col + x * 2) == color) {
                        *ipp++ = (y * width + x) * 8 + color;
                    } else {
                        *ipp++ = 0;
                    }
                }
            }

        progress = 0.2;
        setProgCancel(progress);

#ifdef _OPENMP
        #pragma omp parallel
#endif
        {
            constexpr int progressStep = 64;
            const double progressInc = (1.0 - progress) / ((height - 2) / progressStep);
            int firstRow = -1;
            int lastRow = -1;
#ifdef _OPENMP
            // note, static scheduling is important in this implementation
            #pragma omp for schedule(static)
#endif

            for (int row = 2; row < height - 2; row++) {    /* Do VNG interpolation */
                if (firstRow == -1) {
                    firstRow = row;
                }
                lastRow = row;
                for (int col = 2; col < width - 2; col++) {
                    float * pix = image[row * width + col];
                    int color = fc(cfarray, row, col);
                    int32_t * ip = code[row & prow][col & pcol];
                    float gval[8] = {};

                    while (ip[0] != INT_MAX) {        /* Calculate gradients */
#ifdef __SSE2__
                        // at least on machines with SSE2 feature this cast is save and saves a lot of int => float conversions
                        const float diff = std::fabs(pix[ip[0]] - pix[ip[1]]) * reinterpret_cast<float*>(ip)[2];
#else
                        const float diff = std::fabs(pix[ip[0]] - pix[ip[1]]) * ip[2];
#endif
                        gval[ip[3]] += diff;
                        ip += 5;
                        if (UNLIKELY(ip[-1] != -1)) {
                            gval[ip[-1]] += diff;
                            ip++;
                        }
                    }
                    ip++;

                    const float thold = librtprocess::min(gval[0], gval[1], gval[2], gval[3], gval[4], gval[5], gval[6], gval[7])
                                      + librtprocess::max(gval[0], gval[1], gval[2], gval[3], gval[4], gval[5], gval[6], gval[7]) * 0.5f;

                    float sum0 = 0.f;
                    float sum1 = 0.f;
                    const float greenval = pix[color];
                    int num = 0;

                    if(color & 1) {
                        color ^= 2;
                        for (int g = 0; g < 8; g++, ip += 2) {  /* Average the neighbors */
                            if (gval[g] <= thold) {
                                if(ip[1]) {
                                    sum0 += greenval + pix[ip[1]];
                                }

                                sum1 += pix[ip[0] + color];
                                num++;
                            }
                        }
                        sum0 *= 0.5f;
                    } else {
                        for (int g = 0; g < 8; g++, ip += 2) {  /* Average the neighbors */
                            if (gval[g] <= thold) {
                                if(ip[1]) {
                                    sum0 += greenval + pix[ip[1]];
                                }

                                sum1 += pix[ip[0] + 1] + pix[ip[0] + 3];
                                num++;
                            }
                        }
                    }
                    green[row][col] = greenval + (sum1 - sum0) / (2 * num);
                }
                if (row - 1 > firstRow) {
                    interpolate_row_redblue(rawData, cfarray, red[row - 1], blue[row - 1], green[row - 2], green[row - 1], green[row], row - 1, width);
                }

                    if((row % progressStep) == 0)
#ifdef _OPENMP
                        #pragma omp critical (updateprogress)
#endif
                    {
                        progress += progressInc;
                        setProgCancel(progress);
                    }
            }

            if (firstRow > 2 && firstRow < height - 3) {
                interpolate_row_redblue(rawData, cfarray, red[firstRow], blue[firstRow], green[firstRow - 1], green[firstRow], green[firstRow + 1], firstRow, width);
            }

            if (lastRow > 2 && lastRow < height - 3) {
                interpolate_row_redblue(rawData, cfarray, red[lastRow], blue[lastRow], green[lastRow - 1], green[lastRow], green[lastRow + 1], lastRow, width);
            }
#ifdef _OPENMP
            #pragma omp single
#endif
            {
                // let the first thread, which is out of work, do the border interpolation
                // bayerborder_demosaic needs a 3 color cfa.
                unsigned bordercfa[2][2];
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        bordercfa[i][j] = (cfarray[i][j] & 1) ? 1 : cfarray[i][j];
                    }
                }
                rc = bayerborder_demosaic(width, height, 3, rawData, red, green, blue, bordercfa);
            }
        }
        free(code[0][0]);
    }
    free(image);

    setProgCancel(1.0);

    return rc;
}
