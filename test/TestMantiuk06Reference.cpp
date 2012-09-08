/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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
 */

//! \brief Reference implementations for TestMantiuk06
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note This functions are copied from the original source code of PFSTMO
//! Please refer to PFSTMO license for more information

#include "TestMantiuk06Reference.h"

#include <cstddef>

namespace reference
{
int imin(int a, int b)
{
    return (a > b) ? b : a;
}

//! \brief Reference implementation of matrix_upsample_full
void matrix_upsample_full(int outCols, int outRows,
                          const float* in, float* out)
{
    const size_t inRows = outRows/2;
    const size_t inCols = outCols/2;

    // Transpose of experimental downsampling matrix (theoretically the correct
    // thing to do)

    const float dx = (float)inCols / ((float)outCols);
    const float dy = (float)inRows / ((float)outRows);
    // This gives a genuine upsampling matrix, not the transpose of the
    // downsampling matrix
    // Theoretically, this should be the best.
    // const float factor = 1.0f;
    const float factor = 1.0f / (dx*dy);

    for (size_t y = 0; y < outRows; y++)
    {
        const float sy = y * dy;
        const int iy1 =      (  y   * inRows) / outRows;
        const int iy2 = imin(((y+1) * inRows) / outRows, inRows-1);

        for (size_t x = 0; x < outCols; x++)
        {
            const float sx = x * dx;
            const int ix1 =      (  x   * inCols) / outCols;
            const int ix2 = imin(((x+1) * inCols) / outCols, inCols-1);

            out[x + y*outCols] = (((ix1+1) - sx)*((iy1+1 - sy))
                                  * in[ix1 + iy1*inCols]
                                  +
                                  ((ix1+1) - sx)*(sy+dy - (iy1+1))
                                  * in[ix1 + iy2*inCols]
                                  +
                                  (sx+dx - (ix1+1))*((iy1+1 - sy))
                                  * in[ix2 + iy1*inCols]
                                  +
                                  (sx+dx - (ix1+1))*(sy+dx - (iy1+1))
                                  * in[ix2 + iy2*inCols])*factor;
        }
    }
}

void matrix_upsample_simple(const int outCols, const int outRows,
                            const float* const in, float* const out)
{
    for (int y = 0; y < outRows; y++)
    {
        const int iy1 = y / 2;
        float* outp = out + y*outCols;
        const float* inp = in + iy1*(outCols/2);
        for (int x = 0; x < outCols; x+=2)
        {
            const int ix1 = x / 2;
            outp[x] = outp[x+1] = inp[ix1];
        }
    }
}

void calculate_and_add_divergence(int COLS, int ROWS,
                                  const float* Gx,
                                  const float* Gy,
                                  float* divG)
{
    float divGx, divGy;

    // kx = 0 AND ky = 0;
    divG[0] += Gx[0] + Gy[0];                       // OUT

    // ky = 0
    for (size_t kx=1; kx < COLS; kx++)
    {
        divGx = Gx[kx] - Gx[kx - 1];
        divGy = Gy[kx];
        divG[kx] += divGx + divGy;                    // OUT
    }

    for (size_t ky=1; ky < ROWS; ky++)
    {
        // kx = 0
        divGx = Gx[ky*COLS];
        divGy = Gy[ky*COLS] - Gy[ky*COLS - COLS];
        divG[ky*COLS] += divGx + divGy;               // OUT

        // kx > 0
        for (size_t kx=1; kx<COLS; kx++)
        {
            divGx = Gx[kx + ky*COLS] - Gx[kx + ky*COLS-1];
            divGy = Gy[kx + ky*COLS] - Gy[kx + ky*COLS - COLS];
            divG[kx + ky*COLS] += divGx + divGy;        // OUT
        }
    }

}

void calculate_gradient(const int COLS, const int ROWS,
                        const float* lum, float* Gx, float* Gy)
{
    int Y_IDX, IDX;

    for (int ky = 0; ky < ROWS-1; ky++)
    {
        Y_IDX = ky*COLS;
        for (int kx = 0; kx < COLS-1; kx++)
        {
            IDX = Y_IDX + kx;

            Gx[IDX] = lum[IDX + 1]    - lum[IDX];
            Gy[IDX] = lum[IDX + COLS] - lum[IDX];
        }

        Gx[Y_IDX + COLS - 1] = 0.0f; // last columns (kx = COLS - 1)
        Gy[Y_IDX + COLS - 1] =
                lum[Y_IDX + COLS - 1 + COLS] - lum[Y_IDX + COLS - 1];
    }

    // last row (ky = ROWS-1)
    for (int kx = 0; kx < (COLS-1); kx++)
    {
        IDX = (ROWS - 1)*COLS + kx;

        Gx[IDX] = lum[IDX + 1] - lum[IDX];
        Gy[IDX] = 0.0f;
    }

    // last row & last col = last element
    Gx[ROWS*COLS - 1] = 0.0f;
    Gy[ROWS*COLS - 1] = 0.0f;
}

} // namespace reference
