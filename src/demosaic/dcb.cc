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

/*
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the author nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// If you want to use the code, you need to display name of the original authors in
// your software!

/* DCB demosaicing by Jacek Gozdz (cuniek@kft.umcs.lublin.pl)
 * the code is open source (BSD licence)
*/
#include <cmath>
#include <cassert>
#include <cstring>

#include "bayerhelper.h"
#include "librtprocess.h"
#include "rt_math.h"
#include "StopWatch.h"
#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;

#define FORCC for (unsigned int c=0; c < colors; c++)

#define TILESIZE 192
#define TILEBORDER 10
#define CACHESIZE (TILESIZE+2*TILEBORDER)

using namespace librtprocess;
namespace {

inline void dcb_initTileLimits(int W, int H, int &colMin, int &rowMin, int &colMax, int &rowMax, int x0, int y0, int border)
{
    rowMin = border;
    colMin = border;
    rowMax = CACHESIZE - border;
    colMax = CACHESIZE - border;

    if(!y0 ) {
        rowMin = TILEBORDER + border;
    }

    if(!x0 ) {
        colMin = TILEBORDER + border;
    }

    if( y0 + TILESIZE + TILEBORDER >= H - border) {
        rowMax = TILEBORDER + H - border - y0;
    }

    if( x0 + TILESIZE + TILEBORDER >= W - border) {
        colMax = TILEBORDER + W - border - x0;
    }
}

void fill_raw(int W, int H, float (*cache )[3], int x0, int y0, const float * const *rawData, const unsigned cfarray[2][2])
{
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 0);

    for (int row = rowMin, y = y0 - TILEBORDER + rowMin; row < rowMax; row++, y++)
        for (int col = colMin, x = x0 - TILEBORDER + colMin, indx = row * CACHESIZE + col; col < colMax; col++, x++, indx++) {
            cache[indx][fc(cfarray, y, x)] = rawData[y][x];
        }
}

// saves red and blue

// change buffer[3] -> buffer[2],  possibly to buffer[1] if split
// into two loops, one for R and another for B, could also be smaller because
// there is no need for green pixels pass
// this would decrease the amount of needed memory
// from megapixels*2 records to megapixels*0.5
// also don't know if float is needed as data is 1-65536 integer (I believe!!)
// comment from Ingo: float is needed because rawdata in rt is float
void copy_to_buffer( float (*buffer)[2], float (*image)[3])
{
    for (int indx = 0; indx < CACHESIZE * CACHESIZE; indx++) {
        buffer[indx][0] = image[indx][0]; //R
        buffer[indx][1] = image[indx][2]; //B
    }
}

// restores red and blue

// other comments like in copy_to_buffer
void restore_from_buffer(float (*image)[3], float (*buffer)[2])
{
    for (int indx = 0; indx < CACHESIZE * CACHESIZE; indx++) {
        image[indx][0] = buffer[indx][0]; //R
        image[indx][2] = buffer[indx][1]; //B
    }
}

void fill_border(int W, int H, float (*cache )[3], int border, int x0, int y0, const unsigned cfarray[2][2])
{
    unsigned f;
    float sum[8];
    constexpr unsigned int colors = 3;  // used in FORCC

    for (int row = y0; row < y0 + TILESIZE + TILEBORDER && row < H; row++) {
        for (int col = x0; col < x0 + TILESIZE + TILEBORDER && col < W; col++) {
            if (col >= border && col < W - border && row >= border && row < H - border) {
                col = W - border;

                if(col >= x0 + TILESIZE + TILEBORDER ) {
                    break;
                }
            }

            memset(sum, 0, sizeof sum);

            for (int y = row - 1; y != row + 2; y++)
                for (int x = col - 1; x != col + 2; x++)
                    if (y < H && y < y0 + TILESIZE + TILEBORDER && x < W && x < x0 + TILESIZE + TILEBORDER) {
                        f = fc(cfarray, y, x);
                        sum[f] += cache[(y - y0 + TILEBORDER) * CACHESIZE + TILEBORDER + x - x0][f];
                        sum[f + 4]++;
                    }

            f = fc(cfarray, row, col);
            FORCC

            if (c != f && sum[c + 4] > 0) {
                cache[(row - y0 + TILEBORDER) * CACHESIZE + TILEBORDER + col - x0][c] = sum[c] / sum[c + 4];
            }
        }
    }
}

// First pass green interpolation

// remove entirely: bufferH and bufferV
void dcb_hid(int W, int H, float (*image)[3], int x0, int y0, const unsigned cfarray[2][2])
{
    const int u = CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 2);

// simple green bilinear in R and B pixels
    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), indx = row * CACHESIZE + col; col < colMax; col += 2, indx += 2) {
            assert(indx - u - 1 >= 0 && indx + u + 1 < u * u);

            image[indx][1] = 0.25*(image[indx-1][1]+image[indx+1][1]+image[indx-u][1]+image[indx+u][1]);
        }
}

// missing colours are interpolated
void dcb_color(int W, int H, float (*image)[3], int x0, int y0, const unsigned cfarray[2][2])
{
    const int u = CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 1);

    // red in blue pixel, blue in red pixel
    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), indx = row * CACHESIZE + col, c = 2 - fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col); col < colMax; col += 2, indx += 2) {
            assert(indx >= 0 && indx < u * u && c >= 0 && c < 4);


//Jacek comment: one multiplication less
            image[indx][c] = image[indx][1] +
                               ( image[indx + u + 1][c] + image[indx + u - 1][c] + image[indx - u + 1][c] + image[indx - u - 1][c]
                              - (image[indx + u + 1][1] + image[indx + u - 1][1] + image[indx - u + 1][1] + image[indx - u - 1][1]) ) * 0.25f;

/* original
            image[indx][c] = ( 4.f * image[indx][1]
                               - image[indx + u + 1][1] - image[indx + u - 1][1] - image[indx - u + 1][1] - image[indx - u - 1][1]
                               + image[indx + u + 1][c] + image[indx + u - 1][c] + image[indx - u + 1][c] + image[indx - u - 1][c] ) * 0.25f;
*/
       }

    // red or blue in green pixels
    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin + 1) & 1), indx = row * CACHESIZE + col, c = fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col + 1), d = 2 - c; col < colMax; col += 2, indx += 2) {
            assert(indx >= 0 && indx < u * u && c >= 0 && c < 4);

//Jacek comment: two multiplications (in total) less
            image[indx][c] = image[indx][1] + (image[indx + 1][c] + image[indx - 1][c] - (image[indx + 1][1] + image[indx - 1][1])) * 0.5f;
            image[indx][d] = image[indx][1] + (image[indx + u][d] + image[indx - u][d] - (image[indx + u][1] + image[indx - u][1])) * 0.5f;


/* original
            image[indx][c] = (2.f * image[indx][1] - image[indx + 1][1] - image[indx - 1][1] + image[indx + 1][c] + image[indx - 1][c]) * 0.5f;
            image[indx][d] = (2.f * image[indx][1] - image[indx + u][1] - image[indx - u][1] + image[indx + u][d] + image[indx - u][d]) * 0.5f;
*/
        }
}

// green correction
void dcb_hid2(int W, int H, float (*image)[3], int x0, int y0, const unsigned cfarray[2][2])
{
    const int v = 2 * CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 2);

    for (int row = rowMin; row < rowMax; row++) {
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), indx = row * CACHESIZE + col, c = fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col); col < colMax; col += 2, indx += 2) {
            assert(indx - v >= 0 && indx + v < CACHESIZE * CACHESIZE);

//Jacek comment: one multiplication less
            image[indx][1] = image[indx][c] +
                             (image[indx + v][1] + image[indx - v][1] + image[indx - 2][1] + image[indx + 2][1]
                           - (image[indx + v][c] + image[indx - v][c] + image[indx - 2][c] + image[indx + 2][c])) * 0.25f;

/* original
            image[indx][1] = (image[indx + v][1] + image[indx - v][1] + image[indx - 2][1] + image[indx + 2][1]) * 0.25f +
                             image[indx][c] - ( image[indx + v][c] + image[indx - v][c] + image[indx - 2][c] + image[indx + 2][c]) * 0.25f;
*/
        }
    }
}

void dcb_map(int W, int H, float (*image)[3], uint8_t *map, int x0, int y0)
{
    const int u = 3 * CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 2);

    for (int row = rowMin; row < rowMax; row++) {
        for (int col = colMin, indx = row * CACHESIZE + col; col < colMax; col++, indx++) {
            float *pix = &(image[indx][1]);

            assert(indx >= 0 && indx < u * u);

            // comparing 4 * a to (b+c+d+e) instead of a to (b+c+d+e)/4 is faster because divisions are slow
            if ( 4 * (*pix) > ( (pix[-3] + pix[+3]) + (pix[-u] + pix[+u])) ) {
                map[indx] = ((std::min(pix[-3], pix[+3]) + (pix[-3] + pix[+3]) ) < (std::min(pix[-u], pix[+u]) + (pix[-u] + pix[+u])));
            } else {
                map[indx] = ((std::max(pix[-3], pix[+3]) + (pix[-3] + pix[+3]) ) > (std::max(pix[-u], pix[+u]) + (pix[-u] + pix[+u])));
            }
        }
    }
}

// green is used to create
// an interpolation direction map
// 1 = vertical
// 0 = horizontal
// saved in image[][3]

// seems at least 2 persons implemented some code, as this one has different coding style, could be unified
// I don't know if *pix is faster than a loop working on image[] directly

// interpolated green pixels are corrected using the map
void dcb_correction(int W, int H, float (*image)[3], uint8_t *map, int x0, int y0, const unsigned cfarray[2][2])
{
    const int u = CACHESIZE, v = 2 * CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 2);

    for (int row = rowMin; row < rowMax; row++) {
        for (int indx = row * CACHESIZE + colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1); indx < row * CACHESIZE + colMax; indx += 2) {
//        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), indx = row * CACHESIZE + col; col < colMax; col += 2, indx += 2) {
            float current = 4 * map[indx] +
                            2 * (map[indx + u] + map[indx - u] + map[indx + 1] + map[indx - 1]) +
                            map[indx + v] + map[indx - v] + map[indx + 2] + map[indx - 2];

            assert(indx >= 0 && indx < u * u);
            image[indx][1] = ((16.f - current) * (image[indx - 1][1] + image[indx + 1][1]) + current * (image[indx - u][1] + image[indx + u][1]) ) * 0.03125f;
//            image[indx][1] = ((16.f - current) * (image[indx - 1][1] + image[indx + 1][1]) * 0.5f + current * (image[indx - u][1] + image[indx + u][1]) * 0.5f ) * 0.0625f;
        }
    }
}

// R and B smoothing using green contrast, all pixels except 2 pixel wide border

// again code with *pix, is this kind of calculating faster in C, than this what was commented?
void dcb_pp(int W, int H, float (*image)[3], int x0, int y0)
{
    const int u = CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 2);

    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin, indx = row * CACHESIZE + col; col < colMax; col++, indx++) {
//            float r1 = image[indx-1][0] + image[indx+1][0] + image[indx-u][0] + image[indx+u][0] + image[indx-u-1][0] + image[indx+u+1][0] + image[indx-u+1][0] + image[indx+u-1][0];
//            float g1 = image[indx-1][1] + image[indx+1][1] + image[indx-u][1] + image[indx+u][1] + image[indx-u-1][1] + image[indx+u+1][1] + image[indx-u+1][1] + image[indx+u-1][1];
//            float b1 = image[indx-1][2] + image[indx+1][2] + image[indx-u][2] + image[indx+u][2] + image[indx-u-1][2] + image[indx+u+1][2] + image[indx-u+1][2] + image[indx+u-1][2];
            float (*pix)[3] = image + (indx - u - 1);
            float r1 = (*pix)[0];
            float g1 = (*pix)[1];
            float b1 = (*pix)[2];
            pix++;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            pix++;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            pix += CACHESIZE - 2;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            pix += 2;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            pix += CACHESIZE - 2;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            pix++;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            pix++;
            r1 += (*pix)[0];
            g1 += (*pix)[1];
            b1 += (*pix)[2];
            r1 *= 0.125f;
            g1 *= 0.125f;
            b1 *= 0.125f;
            r1 += ( image[indx][1] - g1 );
            b1 += ( image[indx][1] - g1 );

            assert(indx >= 0 && indx < u * u);
            image[indx][0] = r1;
            image[indx][2] = b1;
        }
}

// interpolated green pixels are corrected using the map
// with correction
void dcb_correction2(int W, int H, float (*image)[3], uint8_t *map, int x0, int y0, const unsigned cfarray[2][2])
{
    const int u = CACHESIZE, v = 2 * CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 4);

    for (int row = rowMin; row < rowMax; row++) {
        for (int indx = row * CACHESIZE + colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), c = fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1)); indx < row * CACHESIZE + colMax; indx += 2) {
            // map values are uint8_t either 0 or 1. Adding them using integer instructions is perfectly valid and fast. Final result is converted to float then
            float current = 4 * map[indx] +
                            2 * (map[indx + u] + map[indx - u] + map[indx + 1] + map[indx - 1]) +
                            map[indx + v] + map[indx - v] + map[indx + 2] + map[indx - 2];

            assert(indx >= 0 && indx < u * u);

// Jacek comment: works now, and has 3 float mults and 9 float adds
            image[indx][1] =  image[indx][c] +
                    ((16.f - current) * (image[indx - 1][1] + image[indx + 1][1] - (image[indx + 2][c] + image[indx - 2][c]))
                            + current * (image[indx - u][1] + image[indx + u][1] - (image[indx + v][c] + image[indx - v][c]))) * 0.03125f;


            // 4 float mults and 9 float adds
            // Jacek comment: not mathematically identical to original
/*            image[indx][1] = 16.f * image[indx][c] +
                             ((16.f - current) * ((image[indx - 1][1] + image[indx + 1][1])
                                                  - (image[indx + 2][c] + image[indx - 2][c]))
                              + current * ((image[indx - u][1] + image[indx + u][1]) - (image[indx + v][c] + image[indx - v][c]))) * 0.03125f;
*/
            // 7 float mults and 10 float adds
            // original code
/*
            image[indx][1] = ((16.f - current) * ((image[indx - 1][1] + image[indx + 1][1]) * 0.5f
                                                  + image[indx][c] - (image[indx + 2][c] + image[indx - 2][c]) * 0.5f)
                              + current * ((image[indx - u][1] + image[indx + u][1]) * 0.5f + image[indx][c] - (image[indx + v][c] + image[indx - v][c]) * 0.5f)) * 0.0625f;
*/
        }
    }
}

// image refinement
void dcb_refinement(int W, int H, float (*image)[3], uint8_t *map, int x0, int y0, const unsigned cfarray[2][2])
{
    const int u = CACHESIZE, v = 2 * CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 4);

    float f0, f1, f2, g1, h0, h1, h2, g2;

    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), indx = row * CACHESIZE + col, c = fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col); col < colMax; col += 2, indx += 2) {

            float current = 4 * map[indx] +
                            2 * (map[indx + u] + map[indx - u] + map[indx + 1] + map[indx - 1])
                            + map[indx + v] + map[indx - v] + map[indx - 2] + map[indx + 2];

            float currPix = image[indx][c];

            f0 = (float)(image[indx - u][1] + image[indx + u][1]) / (1.f + 2.f * currPix);
            f1 = 2.f * image[indx - u][1] / (1.f + image[indx - v][c] + currPix);
            f2 = 2.f * image[indx + u][1] / (1.f + image[indx + v][c] + currPix);

            g1 = f0 + f1 + f2;

            h0 = (float)(image[indx - 1][1] + image[indx + 1][1]) / (1.f + 2.f * currPix);
            h1 = 2.f * image[indx - 1][1] / (1.f + image[indx - 2][c] + currPix);
            h2 = 2.f * image[indx + 1][1] / (1.f + image[indx + 2][c] + currPix);

            g2 = h0 + h1 + h2;

            // new green value
            assert(indx >= 0 && indx < u * u);
            currPix *= (current * g1 + (16.f - current) * g2) / 48.f;

            // get rid of the overshot pixels
            float minVal = std::min(image[indx - 1][1], std::min(image[indx + 1][1], std::min(image[indx - u][1], image[indx + u][1])));
            float maxVal = std::max(image[indx - 1][1], std::max(image[indx + 1][1], std::max(image[indx - u][1], image[indx + u][1])));

            image[indx][1] =  LIM(currPix, minVal, maxVal);

        }
}

// missing colours are interpolated using high quality algorithm by Luis Sanz Rodriguez
void dcb_color_full(int W, int H, float (*image)[3], int x0, int y0, float (*chroma)[2], const unsigned cfarray[2][2])
{
    const int u = CACHESIZE, w = 3 * CACHESIZE;
    int rowMin, colMin, rowMax, colMax;
    dcb_initTileLimits(W, H, colMin, rowMin, colMax, rowMax, x0, y0, 3);

    float f[4], g[4];

    for (int row = 1; row < CACHESIZE - 1; row++)
        for (int col = 1 + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + 1) & 1), indx = row * CACHESIZE + col, c = fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col), d = c / 2; col < CACHESIZE - 1; col += 2, indx += 2) {
            assert(indx >= 0 && indx < u * u && c >= 0 && c < 4);
            chroma[indx][d] = image[indx][c] - image[indx][1];
        }

    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin) & 1), indx = row * CACHESIZE + col, c = 1 - fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col) / 2; col < colMax; col += 2, indx += 2) {
            f[0] = 1.f / (float)(1.f + fabs(chroma[indx - u - 1][c] - chroma[indx + u + 1][c]) + fabs(chroma[indx - u - 1][c] - chroma[indx - w - 3][c]) + fabs(chroma[indx + u + 1][c] - chroma[indx - w - 3][c]));
            f[1] = 1.f / (float)(1.f + fabs(chroma[indx - u + 1][c] - chroma[indx + u - 1][c]) + fabs(chroma[indx - u + 1][c] - chroma[indx - w + 3][c]) + fabs(chroma[indx + u - 1][c] - chroma[indx - w + 3][c]));
            f[2] = 1.f / (float)(1.f + fabs(chroma[indx + u - 1][c] - chroma[indx - u + 1][c]) + fabs(chroma[indx + u - 1][c] - chroma[indx + w + 3][c]) + fabs(chroma[indx - u + 1][c] - chroma[indx + w - 3][c]));
            f[3] = 1.f / (float)(1.f + fabs(chroma[indx + u + 1][c] - chroma[indx - u - 1][c]) + fabs(chroma[indx + u + 1][c] - chroma[indx + w - 3][c]) + fabs(chroma[indx - u - 1][c] - chroma[indx + w + 3][c]));
            g[0] = 1.325f * chroma[indx - u - 1][c] - 0.175f * chroma[indx - w - 3][c] - 0.075f * (chroma[indx - w - 1][c] + chroma[indx - u - 3][c]);
            g[1] = 1.325f * chroma[indx - u + 1][c] - 0.175f * chroma[indx - w + 3][c] - 0.075f * (chroma[indx - w + 1][c] + chroma[indx - u + 3][c]);
            g[2] = 1.325f * chroma[indx + u - 1][c] - 0.175f * chroma[indx + w - 3][c] - 0.075f * (chroma[indx + w - 1][c] + chroma[indx + u - 3][c]);
            g[3] = 1.325f * chroma[indx + u + 1][c] - 0.175f * chroma[indx + w + 3][c] - 0.075f * (chroma[indx + w + 1][c] + chroma[indx + u + 3][c]);

            assert(indx >= 0 && indx < u * u && c >= 0 && c < 2);
            chroma[indx][c] = (f[0] * g[0] + f[1] * g[1] + f[2] * g[2] + f[3] * g[3]) / (f[0] + f[1] + f[2] + f[3]);
        }

    for (int row = rowMin; row < rowMax; row++)
        for (int col = colMin + (fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + colMin + 1) & 1), indx = row * CACHESIZE + col, c = fc(cfarray, y0 - TILEBORDER + row, x0 - TILEBORDER + col + 1) / 2; col < colMax; col += 2, indx += 2)
            for(int d = 0; d <= 1; c = 1 - c, d++) {
                f[0] = 1.f / (1.f + fabs(chroma[indx - u][c] - chroma[indx + u][c]) + fabs(chroma[indx - u][c] - chroma[indx - w][c]) + fabs(chroma[indx + u][c] - chroma[indx - w][c]));
                f[1] = 1.f / (1.f + fabs(chroma[indx + 1][c] - chroma[indx - 1][c]) + fabs(chroma[indx + 1][c] - chroma[indx + 3][c]) + fabs(chroma[indx - 1][c] - chroma[indx + 3][c]));
                f[2] = 1.f / (1.f + fabs(chroma[indx - 1][c] - chroma[indx + 1][c]) + fabs(chroma[indx - 1][c] - chroma[indx - 3][c]) + fabs(chroma[indx + 1][c] - chroma[indx - 3][c]));
                f[3] = 1.f / (1.f + fabs(chroma[indx + u][c] - chroma[indx - u][c]) + fabs(chroma[indx + u][c] - chroma[indx + w][c]) + fabs(chroma[indx - u][c] - chroma[indx + w][c]));

                g[0] = intp(0.875f, chroma[indx - u][c], chroma[indx - w][c]);
                g[1] = intp(0.875f, chroma[indx + 1][c], chroma[indx + 3][c]);
                g[2] = intp(0.875f, chroma[indx - 1][c], chroma[indx - 3][c]);
                g[3] = intp(0.875f, chroma[indx + u][c], chroma[indx + w][c]);

                assert(indx >= 0 && indx < u * u && c >= 0 && c < 2);
                chroma[indx][c] = (f[0] * g[0] + f[1] * g[1] + f[2] * g[2] + f[3] * g[3]) / (f[0] + f[1] + f[2] + f[3]);
            }

    for(int row = rowMin; row < rowMax; row++)
        for(int col = colMin, indx = row * CACHESIZE + col; col < colMax; col++, indx++) {
            assert(indx >= 0 && indx < u * u);

            image[indx][0] = chroma[indx][0] + image[indx][1];
            image[indx][2] = chroma[indx][1] + image[indx][1];
        }
}

}

// DCB demosaicing main routine
rpError dcb_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int iterations, bool dcb_enhance)
{
BENCHFUN
    if (!validateBayerCfa(3, cfarray)) {
        return RP_WRONG_CFA;
    }

    rpError rc = RP_NO_ERROR;

    double currentProgress = 0.0;
    setProgCancel(currentProgress);

    int wTiles = width / TILESIZE + ((width % TILESIZE) ? 1 : 0);
    int hTiles = height / TILESIZE + ((height % TILESIZE) ? 1 : 0);
    int numTiles = wTiles * hTiles;
    int tilesDone = 0;
    constexpr int cldf = 2; // factor to multiply cache line distance. 1 = 64 bytes, 2 = 128 bytes ...

#ifdef _OPENMP
    #pragma omp parallel
#endif
{
    // assign working space
    char *buffer0 = (char *) malloc(5 * sizeof(float) * CACHESIZE * CACHESIZE + sizeof(uint8_t) * CACHESIZE * CACHESIZE + 3 * cldf * 64 + 63);
#ifdef _OPENMP
    #pragma omp critical
#endif
    {
        if(!buffer0) {
            rc = RP_MEMORY_ERROR;
        }
    }
#ifdef _OPENMP
    #pragma omp barrier
#endif
    if (!rc) {
        // aligned to 64 byte boundary
        char *data = (char*)( ( uintptr_t(buffer0) + uintptr_t(63)) / 64 * 64);

        float (*tile)[3]   = (float(*)[3]) data;
        float (*buffer)[2] = (float(*)[2]) ((char*)tile + sizeof(float) * CACHESIZE * CACHESIZE * 3 + cldf * 64);
        float (*chrm)[2]   = (float(*)[2]) (buffer); // No overlap in usage of buffer and chrm means we can reuse buffer
        uint8_t *map       = (uint8_t*) ((char*)buffer + sizeof(float) * CACHESIZE * CACHESIZE * 2 + cldf * 64);

#ifdef _OPENMP
        #pragma omp for schedule(dynamic) nowait
#endif

        for( int iTile = 0; iTile < numTiles; iTile++) {
            int xTile = iTile % wTiles;
            int yTile = iTile / wTiles;
            int x0 = xTile * TILESIZE;
            int y0 = yTile * TILESIZE;

            memset(tile, 0, CACHESIZE * CACHESIZE * sizeof * tile);
            memset(map, 0, CACHESIZE * CACHESIZE * sizeof * map);

            fill_raw(width, height, tile, x0, y0, rawData, cfarray);

            if( !xTile || !yTile || xTile == wTiles - 1 || yTile == hTiles - 1) {
                fill_border(width, height, tile, 6, x0, y0, cfarray);
            }

            copy_to_buffer(buffer, tile);
            dcb_hid(width, height, tile, x0, y0, cfarray);

            for (int i = iterations; i > 0; i--) {
                dcb_hid2(width, height, tile, x0, y0, cfarray);
                dcb_hid2(width, height, tile, x0, y0, cfarray);
                dcb_hid2(width, height, tile, x0, y0, cfarray);
                dcb_map(width, height, tile, map, x0, y0);
                dcb_correction(width, height, tile, map, x0, y0, cfarray);
            }

            dcb_color(width, height, tile, x0, y0, cfarray);
            dcb_pp(width, height, tile, x0, y0);
            dcb_map(width, height, tile, map, x0, y0);
            dcb_correction2(width, height, tile, map, x0, y0, cfarray);
            dcb_map(width, height, tile, map, x0, y0);
            dcb_correction(width, height, tile, map, x0, y0, cfarray);
            dcb_color(width, height, tile, x0, y0, cfarray);
            dcb_map(width, height, tile, map, x0, y0);
            dcb_correction(width, height, tile, map, x0, y0, cfarray);
            dcb_map(width, height, tile, map, x0, y0);
            dcb_correction(width, height, tile, map, x0, y0, cfarray);
            dcb_map(width, height, tile, map, x0, y0);
            restore_from_buffer(tile, buffer);

            if (!dcb_enhance)
                dcb_color(width, height, tile, x0, y0, cfarray);
            else
            {
                memset(chrm, 0, CACHESIZE * CACHESIZE * sizeof * chrm);
                dcb_refinement(width, height, tile, map, x0, y0, cfarray);
                dcb_color_full(width, height, tile, x0, y0, chrm, cfarray);
            }

            for(int y = 0; y < TILESIZE && y0 + y < height; y++) {
                for (int j = 0; j < TILESIZE && x0 + j < width; j++) {
                    red[y0 + y][x0 + j]   = tile[(y + TILEBORDER) * CACHESIZE + TILEBORDER + j][0];
                    green[y0 + y][x0 + j] = tile[(y + TILEBORDER) * CACHESIZE + TILEBORDER + j][1];
                    blue[y0 + y][x0 + j]  = tile[(y + TILEBORDER) * CACHESIZE + TILEBORDER + j][2];
                }
            }

#ifdef _OPENMP

            if(omp_get_thread_num() == 0)
#endif
            {
                if(double(tilesDone) / numTiles > currentProgress) {
                    currentProgress += 0.1; // Show progress each 10%
                    setProgCancel(currentProgress);
                }
            }

#ifdef _OPENMP
            #pragma omp atomic
#endif
            tilesDone++;
        }
    }
    if (buffer0) {
        free(buffer0);
    }
}
    if (!rc) {
        rc = bayerborder_demosaic(width, height, 1, rawData, red, green, blue, cfarray);
    }

    setProgCancel(1.0);

    return rc;
}

#undef TILEBORDER
#undef TILESIZE
#undef CACHESIZE
#undef FORCC

